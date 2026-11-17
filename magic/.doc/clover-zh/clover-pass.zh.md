# Clover Pass

Clover Pass 在 Clover IR 已经完成解析和结构校验之后，对内存中的 `clover::Module` 执行局部或全局变换，再由同一个打印器输出新的 Clover IR。Pass 不属于前端或目标后端：前端负责生成合法 IR，Pass 负责在不改变程序可观察语义的前提下修改 IR，后端消费修改后的 IR。

本文把经过一个或多个 Pass 处理后的 IR 称为 passed IR。passed 只表示 Pass 已经运行，不表示所有可能优化都已完成，也不表示 IR 已经变成目标机器指令。

## clover-opt

`clover-opt` 是当前运行 Clover Pass 的命令行工具。输入和输出都使用 Clover 文本 IR，未指定 `-o` 时输出到标准输出。

```text
clover-opt input.ir --passes=forward-jumps -o output.ir
```

当前支持以下选项：

- `-o output.ir`：把结果写入指定文件。
- `--passes=forward-jumps`：运行转发跳转块删除 Pass。
- `-h` 或 `--help`：显示帮助。

未指定 `--passes` 时，工具仍会解析并重新打印输入，因此输出可能被规范化，但不会主动执行优化。当前每次只能指定一个输入文件，`--passes` 只接受完整的 `forward-jumps` 名称；以后出现第二个实际 Pass 后，再根据真实组合需求设计 Pass pipeline 和多个 Pass 的顺序语义。

## forward-jumps

`forward-jumps` 删除只负责把控制流无条件转发到另一个基本块的非入口基本块，并把所有指向该块的跳转直接改为指向最终目标。

一个基本块只有同时满足以下条件才是候选转发块：

- 它不是函数的第一个基本块；第一个基本块是函数入口。
- 它恰好包含一条指令。
- 该指令的 opcode 是 `jmp`。
- 该 `jmp` 没有条件操作数和假分支目标，因此是无条件跳转。
- 跳转目标存在于同一个函数中。
- 跳转目标不是该基本块自身。

删除候选块之前，Pass 会改写同一函数中所有 `jmp` 指令的目标。对于无条件跳转，改写 `target`；对于条件跳转，分别检查真目标 `target` 和假目标 `target_false`。删除一个块后继续检查当前位置，因此连续的转发块可以在一次 Pass 执行中全部消除。

输入示例：

```clover
declare func @work() -> void
func @test(i1 %0) -> void {
^entry:
    jmp i1 %0, ^if.then.0, ^if.else.1
^if.then.0:
    call void @work()
    jmp ^if.end.2
^if.else.1:
    jmp ^if.end.2
^if.end.2:
    ret void
}
```

输出示例：

```clover
declare func @work() -> void
func @test(i1 %0) -> void {
^entry:
    jmp i1 %0, ^if.then.0, ^if.end.2
^if.then.0:
    call void @work()
    jmp ^if.end.2
^if.end.2:
    ret void
}
```

入口块即使只包含一条无条件跳转也会保留，因为函数调用约定和后续分析都可以把第一个块视为稳定入口。自跳转块也会保留，因为它表示一个无限循环；把它删除后无法为指向它的边选择等价目标。

含有常量、调用、内存访问或其他任何额外指令的块不是转发块。当前 Pass 也不执行相邻块合并、不可达块删除、条件跳转同目标折叠、常量传播或死代码删除。这些变换具有不同的判定规则，应作为独立 Pass 实现和测试。

转发块自身携带的源码位置会随该块删除。前驱跳转原有的调试位置不会改变，目标块的调试位置也不会复制到前驱。当前 Clover IR 没有 `phi` 指令；未来加入 `phi` 或其他显式记录前驱块的结构后，本 Pass 必须同步改写 incoming block，并在不能保持值来源时拒绝删除该转发块。

`forward-jumps` 是幂等的：对已经处理过的输出再次运行该 Pass，不应继续改变 IR。公开 API `clover::ForwardJumpBlocks(Module&)` 返回 `PassResult`，其中 `success` 表示执行是否完成，`changes` 表示本次删除的转发块数量。

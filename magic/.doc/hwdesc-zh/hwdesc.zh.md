# SystemVerilog 前端与 Clover Hard IR

当前 SystemVerilog 前端用于建立从硬件描述源代码到 Clover Hard IR 的最小可综合闭环。它不是完整的 IEEE SystemVerilog 编译器，也不实现四态事件仿真。

## 使用

`.sv` 后缀会自动选择 SystemVerilog 前端，也可以显式使用 `--lango=systemverilog` 或 `--lango=sv`：

```text
mgc --emit-ir add1.sv
mgc --lango=systemverilog --emit-ir add1.sv -o add1.ir
```

当前标准名称为 `systemverilog-subset`。未指定 `--std` 时自动使用该名称；也接受 `--std=systemverilog` 作为别名。

## 支持的源语言子集

当前支持 ANSI 风格模块端口、`input`、`output`、`inout`、`logic`、`[N:0]` 二态位宽、十进制非负整数、标识符、二元加法和 `assign` 连续赋值。位宽范围为 1 至 64 位。

```systemverilog
module add1(
	input logic [31:0] a,
	output logic [31:0] y
);
	assign y = a + 1;
endmodule
```

该模块生成：

```clover
module @add1 {
	port @a b32 input
	port @y b32 output

^entry:
	%0 = read b32 @a
	%1 = const b32 1
	%2 = add b32 %0, %1
	assign @y, %2
}
```

端口使用 `@name`，因为端口是具有稳定身份的硬件对象。`read` 产生端口当前值，计算结果使用 `%N` SSA 值，`assign` 对输出或双向端口建立连续驱动。无尺寸十进制常量按赋值目标宽度生成，数值必须能由该位宽表示。表达式中的端口宽度必须与赋值目标一致，当前不会自动插入截断或扩展。

Clover `{endian}` 描述多字节内存值，默认 little endian；它不定义 SystemVerilog packed 向量或 packed struct 的位布局。Clover `bN` 以 bit 0 表示最低有效位。SystemVerilog packed 向量的显式索引范围必须保留其方向，右界映射到最低有效位；例如 `[7:0]` 和 `[0:7]` 虽然宽度相同，索引到位位置的映射相反。packed struct 则按声明顺序由高位向低位放置成员，这可能不同于 Clover `struct` 类型按文本顺序从 bit 0 向高位连续排列的布局。前端实现这些类型时必须显式保留位映射或降低为明确的切片/拼接，不能用内存 endian 替代位布局。

## 当前限制

当前不支持 `wire`、`reg`、有符号类型、位选择、切片、拼接、packed struct、减法及其他运算符、`always`、`always_comb`、`always_ff`、阻塞或非阻塞过程赋值、时钟和触发沿、parameter、generate、模块实例、interface、package、预处理指令、四态 `X/Z` 值及事件调度。当前支持的 `[N:0]` 端口范围从右界 0 开始，降低为 `b(N+1)` 时保持索引 0 对应 bit 0；其他范围方向尚未实现。一个 output 或 inout 端口只能有一个连续驱动。

SystemVerilog `logic` 在当前子集中降低为二态 `bN`。这只适用于当前可综合子集；以后实现四态仿真时必须增加独立类型和对应操作，不能把未知值或高阻值继续表示成普通 `bN`。

## Hard IR 验证

Hard IR Parser 会验证模块名、端口名和块名不重复，`bN` 位宽合法，SSA 值先定义后使用，`read` 引用已声明且宽度相同的端口，`add` 操作数与结果宽度一致，`assign` 只驱动 output 或 inout，并拒绝多个驱动。Hard IR 可以通过 `clover-opt` 在不指定 Pass 时重新解析和打印；当前 `forward-jumps` 和 RISC-V 后端只接受 Soft IR，会拒绝 Hard IR。

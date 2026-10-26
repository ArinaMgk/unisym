# 【ZH-CN】 mgc (clover compiler, like gcc) 软件编译器参考手册

版本 0.1.0

## IR 中间语言设计

Clover IR 是 C 前端与机器后端之间的中间表示。前端负责把 C AST 转换为
Clover IR，后端负责把 Clover IR 转换为 RISC-V 等目标架构的汇编。

IR 采用强类型、SSA、三地址形式：

```clover
%result = add i32 %left, %right
```

每个 `%` 值只能定义一次。C 语言中可反复赋值的局部变量暂时使用
`local`、`load` 和 `store` 表示，不在第一版引入 `phi`。

### 基本语法

| 形式 | 含义 | 示例 |
| :--- | :--- | :--- |
| `%name` | SSA 值 | `%sum` |
| `@name` | 函数 | `@main` |
| `^name` | 基本块 | `^entry` |
| `; text` | 行注释 | `; comment` |

第一版支持以下类型：

```text
void  i1  i8  i16  i32  i64  ptr
```

整数类型本身不区分有符号和无符号；由 `sdiv`、`udiv` 和 `icmp` 的谓词
决定如何解释整数。`ptr` 的实际位宽由目标后端决定。

函数定义格式：

```clover
func @add(i32 %a, i32 %b) -> i32 {
^entry:
    %result = add i32 %a, %b
    ret i32 %result
}
```

外部函数只声明签名：

```clover
declare @putchar(i32) -> i32
```

### 指令

#### 值与内存

```clover
%value = const i32 42
%copy = mov i32 %value

%address = local i32
%loaded = load i32, %address
store i32 %value, %address
```

- `const`：产生整数常量。
- `local`：创建函数局部存储，结果类型为 `ptr`。
- `mov`：复制一个 SSA 值，不读写内存。
- `load`：从 `ptr` 地址读取值。
- `store`：向 `ptr` 地址写入值，不产生结果。

`mov` 不统一表示 `load` 和 `store`。三者虽然都可能降低为机器的移动指令，
但在 IR 中具有不同语义：`mov` 没有副作用，`load` 读取内存，`store` 修改
内存。分开后，优化器不需要根据操作数形式猜测指令是否访问内存。

#### 整数运算

```text
add  sub  mul
sdiv udiv
srem urem
and  or   xor
shl  lshr ashr
```

统一格式为：

```clover
%result = add i32 %left, %right
```

`sdiv`、`srem` 和 `ashr` 按有符号整数处理；`udiv`、`urem` 和 `lshr`
按无符号整数处理。

#### 比较

```clover
%condition = icmp slt i32 %left, %right
```

`icmp` 的结果类型总是 `i1`。比较谓词包括：

```text
eq ne
slt sle sgt sge
ult ule ugt uge
```

#### 整数转换

```clover
%small = trunc i64 %value to i32
%wide1 = zext i8 %value to i32
%wide2 = sext i8 %value to i32
```

- `trunc`：截断高位。
- `zext`：零扩展。
- `sext`：符号扩展。

#### 控制流

```clover
jmp ^target
br i1 %condition, ^true, ^false
ret void
ret i32 %value
```

每个基本块必须以 `jmp`、`br` 或 `ret` 结束，不允许隐式跳转到下一个
基本块。

#### 函数调用

```clover
%result = call i32 @add(i32 %a, i32 %b)
call void @abort()
```

调用的参数和返回类型必须与函数声明一致。具体的参数寄存器、返回值寄存器
和栈布局由目标后端按照 ABI 处理。

第一版暂不支持浮点、聚合类型、全局变量、指针运算、`phi`、原子操作和
可变参数。

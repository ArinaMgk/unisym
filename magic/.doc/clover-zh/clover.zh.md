# 【ZH-CN】 mgc (clover compiler, like gcc) 软件编译器参考手册

版本 0.1.0

## 工具链使用与命令选项

工具链当前由 `mgc` 和 `clover-riscv` 组成：`mgc` 将源代码转换为 AST 或
Clover IR，`clover-riscv` 将 Clover IR 转换为 RISC-V 汇编。

`mgc` 的命令格式：

```text
mgc [options] input...
```

| 选项 | 含义 |
| :--- | :--- |
| `--emit-ir` | 输出 Clover IR |
| `--emit-ast` | 输出前端 AST |
| `--lango=c` | 选择 C 前端 |
| `--std=c99` | 将 C99 标准选项传递给 C 前端 |
| `-Fname[=value]` | 将一个选项传递给当前选中的语言前端 |
| `-Fdata-model=ilp32/lp64/llp64` | 选择 C 前端使用的数据模型 |
| `-g` | 在 Clover IR 中保留对应的源文件、行号和源代码注释 |
| `-o file` | 将结果写入指定文件 |
| `-h`、`--help` | 显示帮助信息 |
| `--` | 结束命令选项解析 |

前端选择顺序为：`--lango`、输入文件后缀、默认 C 前端。`.c` 文件在未指定
输出形式时默认输出 AST；直接传入命令行源码时默认输出 Clover IR。当前只
支持 `--lango=c` 和 `--std=c99`，未知选项会直接报错。`-F` 后面的选项由
当前前端解释并严格验证；C 前端当前只接受 `data-model`，默认值为 `lp64`。
`ilp32`、`lp64`、`llp64` 分别规定 C 的整数与指针 ABI 宽度，而不是所有语言
共享的类型规则。

C 前端支持 C99 十进制、八进制、十六进制整数常量及 `U`、`L`、`LL` 组合后缀，
其类型由数值、后缀和当前数据模型共同决定；普通字符常量支持 C99 转义序列。
浮点、宽字符和多字符常量暂未实现。

C99 `_Bool` 对象占一个 C 字节，赋值时归一化为 0 或 1，参与整数运算时提升为
`int`。有标签 `enum` 支持文件及块作用域、作用域遮蔽、前向声明、定义后直接
声明对象，以及枚举对象、函数参数和返回值。枚举常量表达式支持整数和字符常量、
此前枚举项、整数 cast、`sizeof(type)`、一元、算术、移位、比较、位、逻辑和
条件运算；不接受赋值、逗号或函数调用等非常量运算。

enum 的兼容整数类型根据完整枚举值域和当前 C 数据模型，从 signed/unsigned 的
`int`、`long`、`long long` 中选择。前向声明是当前 Magice C99 子集提供的扩展；
完成定义前，只能声明指针等不要求完整对象布局的类型，不能定义 enum 对象。
当前枚举常量求值器的数值范围为 `-2^63` 至 `2^63-1`；更大的 unsigned
`long long` 常量表达式留待后续扩展任意精度常量表示。

`clover-riscv` 的命令格式：

```text
clover-riscv input.ir [options]
```

| 选项 | 含义 |
| :--- | :--- |
| `-mbits=64` | 选择 64 位 RISC-V 目标 |
| `-minst=i` | 使用基础整数指令集 |
| `-minst=im` | 启用基础整数及乘除法指令集 |
| `-g` | IR 含源位置时输出 `.file`、`.loc` 等 GCC 汇编调试指令 |
| `--asm=gcc` | 输出 GCC 工具链接受的 GNU 汇编语法 |
| `-o file` | 将汇编写入指定文件 |
| `-h`、`--help` | 显示帮助信息及已支持的目标信息 |

当前默认后端目标为 `-mbits=64 -minst=im --asm=gcc`。目前只实现
`--asm=gcc`，汇编格式选项与目标位数、指令集选项相互独立。

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

启用前端调试信息时，模块使用 `source` 建立源文件编号，并在函数内用
`file`、`line` 两个伪指令选择后续 IR 指令的源位置：

```clover
source 1 "main.c"

func @main() -> i32 {
^entry:
    file 1
    line 3
    %0 = const i32 0
    ret i32 %0
}
```

`file` 和 `line` 的操作数均为 32 位无符号编号，不产生 SSA 值，也不会
直接生成机器指令。后端仅在启用 `-g` 时把它们转换为目标汇编的调试指令。

模块可在开头使用 `target ptrbits N` 记录 `ptr` 所需的指针位数：

```clover
target ptrbits 64
```

当前文本格式接受 32 或 64。旧 IR 未写该指令时，指针位数保持未指定；写出后，
后端必须验证自身目标位数与之相符，不能静默采用不同宽度。

第一版支持以下类型：

```text
void  i1  i8  i16  i32  i64  ptr
```

整数类型本身不区分有符号和无符号；由 `sdiv`、`udiv` 和 `icmp` 的谓词
决定如何解释整数。`ptr` 的实际位宽由 `target ptrbits` 约束，并由目标后端实现。

函数定义格式：

```clover
func @add(i32 %0, i32 %1) -> i32 {
^entry:
    %2 = add i32 %0, %1
    ret i32 %2
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
%array = local i32, 12
%buffer = local i8, 24 align 8
global @counter i32 = 0
constant @.L.clover.string.0 i8, 4 = {97, 98, 99, 0}
%global_address = globaladdr @counter
%loaded = load i32, %address align 4
store i32 %value, %address align 4
%next = ptradd i32 %address, %index
```

- `const`：产生整数常量。
- `local`：创建函数局部存储，结果类型为 `ptr`；可选元素数量用于连续数组存储，
  可选的 `align` 指定以字节计的存储对齐，且必须是二的幂。
- `global`：定义具有静态存储期的模块级对象，未显式初始化时由前端写入零初始化值。
- `constant`：定义模块内部的只读聚合数据；初始化列表的元素数量必须与声明数量一致。
- `globaladdr`：取得模块级对象的地址，结果类型为 `ptr`。
- `mov`：复制一个 SSA 值，不读写内存。
- `load`：从 `ptr` 地址读取值；可选的 `align` 表示地址至少具有多少字节对齐。
- `store`：向 `ptr` 地址写入值，不产生结果；同样可以携带可选的 `align`。
- `ptradd`：将整数索引乘以给定元素类型的存储大小，再加到 `ptr` 地址；结果类型为 `ptr`。

`local`、`load` 和 `store` 的 `align N` 均以字节为单位，且 `N` 必须是正的
二次幂。内存访问未写 `align` 时按照访问类型的自然对齐处理；显式写出的值表示
前端能够保证的最低地址对齐。后端若不能直接完成低于自然对齐的访问，必须拆分
访问或明确拒绝，不能静默假设地址已自然对齐。当前 RISC-V 后端会把未对齐的
`i16`、`i32`、`i64` 访问拆成按小端顺序组合或分解的字节访问。

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

#### 整数与指针转换

```clover
%address = inttoptr i64 %integer to ptr
%integer = ptrtoint ptr %address to i64
```

- `inttoptr`：把整数位模式转换为目标地址；前端应先把整数调整到目标指针位宽。
- `ptrtoint`：把地址位模式转换为指定宽度的整数，目标较窄时保留低位。

这两条指令明确表达地址空间与整数域之间的转换，不能使用 `mov` 代替。

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
%result = call i32 @add(i32 %0, i32 %1)
call void @abort()
```

调用的参数和返回类型必须与函数声明一致。具体的参数寄存器、返回值寄存器
和栈布局由目标后端按照 ABI 处理。

第一版暂不支持浮点、聚合类型、指针差值、`phi`、原子操作和
可变参数。

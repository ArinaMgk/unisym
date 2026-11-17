# 【ZH-CN】 Clover 广义中间表示与工具链参考手册

版本 0.1.0

Clover 是供软件编译、数字硬件描述和机器执行使用的广义中间表示框架。Magice、WEL 和 HDL 工具链尽可能共用相同的 Clover 类型、值、操作、符号、源位置、文本格式和验证器，但各自保留软件、硬件和执行环境特有的语义。对于固有差距不要求完全一样，但是IR不能具有多义性。

Clover IR 分为三个主要类别：

| IR 类别 | 主要用途 | 典型使用者 |
| :--- | :--- | :--- |
| Soft IR | 描述软件函数、数据、控制流和普通内存操作 | Magice、软件语言前端、AOT 后端 |
| Hard IR | 描述硬件模块、端口、连接、组合逻辑和时序状态 | Verilog、SystemVerilog、VHDL 前后端及综合、仿真工具 |
| Exec IR | 描述客户机寄存器、地址空间、指令语义、异常和执行块 | WEL、指令解释器和 JIT 后端 |


## 统一表示形式

Soft IR、Hard IR 和 Exec IR 应共享相同的词法风格和基本对象模型。一个通用操作可按自身语义记录操作码、结果、操作数、类型、属性、后继、嵌套区域和源位置；这些信息没有统一的排列顺序，也不要求每个操作全部具备。

三类 IR 都可以使用 `%name` 表示有类型的临时值、`@name` 表示模块级符号、`^name` 表示控制流块或命名区域，并沿用相同的注释、属性和文本解析规则。数据依赖尽量使用 SSA 值；对内存、客户机状态和硬件时序状态的修改必须由带有明确副作用的操作表示，不能仅根据操作数形式推测。

相同运算应尽量使用共享操作。例如软件计算、客户指令语义和硬件组合逻辑中的加法都可以采用统一的强类型形式：

```clover
%result = add i32 %left, %right
```

领域特有语义使用对应类别的操作，不能为了形式统一而丢失必要信息。例如：

```clover
%value = load i32, %address
%guest_value = exec.load i32, %guest_address
port @data b4 input
```

上述示例分别表示普通软件内存读取、可能经过地址转换、权限检查或 MMIO 的客户机读取，以及四位硬件输入端口。它们可以共享 Operation 容器和类型、属性机制，但具有不同的副作用、验证和 lowering 规则。

### 统一类型原则

当前 Soft IR 实现提供：

```text
void  i1  i8  i16  i32  i64  f32  f64  f128  ptr
```

广义 Clover 还需要允许领域类型扩展。Hard IR 可使用 `bN` 表示 N 位硬件位向量，例如 `b4`；以后支持四态逻辑时，应使用独立类型表示未知值和高阻值。Exec IR 可引入客户地址、客户寄存器或执行上下文相关类型，但不得把客户地址无条件等同于宿主 `ptr`。

### IR 类别与操作归属

共享操作只表达与领域无关的计算，例如常量、算术、位运算、比较、选择和宽度转换。领域相关操作按照语义归入对应 IR：

| 类别 | 操作示例 |
| :--- | :--- |
| 共享计算 | `const`、`add`、`sub`、`and`、`icmp`、`zext`、`trunc` |
| Soft IR | `func`、`global`、`local`、`load`、`store`、`call`、`ret` |
| Hard IR | `module`、`port`、`connect`、`instance`、`register`、`clock` |
| Exec IR | `readreg`、`writereg`、`readcsr`、`setpc`、`exception`、`exit` |

后续实现可以采用类别前缀或操作注册表避免一个不可扩展的全局操作码枚举；文本格式是否省略前缀由语法上下文和歧义情况决定，但内存模型、时序模型和客户机状态等不同语义必须在内存表示中保持可区分。

## Pass 与 passed IR

Pass 是 Clover 各类 IR 共享的分析和变换机制。Pass 不仅包括性能优化，也包括验证、规范化、修改、插桩、lowering 和目标合法化：

```text
IR → Pass pipeline → passed IR
```

因此 `passed soft IR`、`passed hard IR` 和 `passed exec IR` 表示对应 IR 经过指定 Pass 管线后的状态，而不是三种新的语法。passed IR 仍使用与输入 IR 相同的统一表示形式，但可以满足更严格的不变量，供后续 backend、解释器或 JIT 使用。

典型管线包括：

```text
Soft IR → verify → canonicalize → optimize → passed soft IR
Hard IR → verify-connectivity → infer-width → lower-process → passed hard IR
Exec IR → verify-state → form-blocks → optimize → passed exec IR
```

不同后端可以声明自己要求的 passed 状态。AOT 后端消费 passed soft IR；HDL 后端、综合器或硬件仿真器消费 passed hard IR；WEL 解释器和 JIT 消费 passed exec IR。

## Soft IR、Hard IR 与 Exec IR

### Soft IR

Soft IR 描述软件程序的函数、模块级对象、SSA 数据流、控制流、普通地址和函数调用，是源语言前端与目标后端之间的软件中间表示。本文档后续已有的函数、global、local、load/store、算术、控制流和 call/ret 语法属于 Soft IR 设计。

### Hard IR

Hard IR 描述数字硬件结构和行为。模块端口可采用与 global 相近但语义独立的声明风格：

```clover
module @adder {
	port @left b4 input
	port @right b4 input
	port @result b4 output

^entry:
	%0 = read b4 @left
	%1 = read b4 @right
	%2 = add b4 %0, %1
	assign @result, %2
}
```

Port 是模块连接点，不是具有软件链接地址的 global，通常不携带软件式的存储对齐和初始化值。`read` 取得端口当前值，`assign` 表示连续驱动；`@name` 表示具有稳定身份的端口，`%name` 表示操作产生的 SSA 临时值。当前实现的 `bN` 支持 1 至 64 位二态位向量。Hard IR 后续还需要定义模块实例、信号连接、组合运算、寄存器、时钟、复位及仿真事件；可综合 RTL 和完整事件仿真应作为不同的 lowering 阶段，不在初始 Hard IR 中混为一种语义。

### Exec IR

Exec IR 是 WEL 等执行工具使用的机器语义表示。客户 ISA 前端将可执行文件中的指令译码并提升为 Exec IR；通用算术继续复用共享操作，客户机状态通过显式操作访问：

```clover
%left = exec.readreg i32 6
%right = exec.readreg i32 7
%result = add i32 %left, %right
exec.writereg i32 5, %result
```

Exec IR 还需要表达 PC、CSR、地址空间、异常、中断和翻译块退出。客户内存访问不能过早降低为普通 Soft IR `load/store`，否则会丢失地址转换、访问权限、对齐异常、MMIO 和自修改代码等语义。passed exec IR 可以由参考解释器执行，也可以由 WEL JIT 降低为宿主机器码直接执行。

## 工具链使用与命令选项

工具链当前由 `mgc`、`clover-opt` 和 `clover-riscv` 组成：`mgc` 将软件或硬件描述源代码转换为 AST 或 Clover IR，`clover-opt` 对 Clover IR 执行 Pass，`clover-riscv` 将 Clover Soft IR 转换为 RISC-V 汇编。

`mgc` 的命令格式：

```text
mgc [options] input...
```

| 选项 | 含义 |
| :--- | :--- |
| `--emit-ir` | 输出 Clover IR |
| `--emit-ast` | 输出前端 AST |
| `--relocation-model=MODEL` | 在 Clover IR 中记录 `static`、`pic` 或 `pie` 模式 |
| `--lango=c` | 选择生成 Soft IR 的 C 前端 |
| `--lango=systemverilog`、`--lango=sv` | 选择生成 Hard IR 的 SystemVerilog 前端 |
| `--std=c99` | 选择 C99 子集 |
| `--std=systemverilog-subset` | 选择当前 SystemVerilog 可综合子集 |
| `-Fname[=value]` | 将一个选项传递给当前选中的语言前端 |
| `-Fdata-model=ilp32/lp64/llp64` | 选择 C 前端使用的数据模型 |
| `-g` | 在 Clover IR 中保留对应的源文件、行号和源代码注释 |
| `-o file` | 将结果写入指定文件 |
| `-h`、`--help` | 显示帮助信息 |
| `--` | 结束命令选项解析 |

前端选择顺序为：`--lango`、输入文件后缀、默认 C 前端。`.c` 选择 C 前端，`.sv` 选择 SystemVerilog 前端；不同前端的文件后缀不能在一次自动推断中混合。源文件在未指定输出形式时默认输出 AST，直接传入命令行源码时默认输出 Clover IR。`-F` 后面的选项由当前前端解释并严格验证；C 前端当前只接受 `data-model`，默认值为 `lp64`。`ilp32`、`lp64`、`llp64` 分别规定 C 的整数与指针 ABI 宽度，而不是所有语言共享的类型规则。`--relocation-model` 只适用于生成 Soft IR 的前端。

C 前端支持 C99 十进制、八进制、十六进制整数常量及 `U`、`L`、`LL` 组合后缀，
其类型由数值、后缀和当前数据模型共同决定；普通字符常量支持 C99 转义序列。
支持十进制及十六进制 `float`、`double` 常量，`F` 后缀选择 `float`；`long double`、
宽字符和多字符常量暂未实现。

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
| `-minst=imf` | 启用单精度硬浮点指令 |
| `-minst=imfd` | 启用单精度和双精度硬浮点指令 |
| `-mabi=lp64` | 浮点参数和返回值按照整数调用约定传递 |
| `-mabi=lp64f` | `f32` 参数和返回值使用浮点寄存器，`f64` 使用整数调用约定 |
| `-mabi=lp64d` | `f32`、`f64` 参数和返回值使用浮点寄存器 |
| `-g` | IR 含源位置时输出当前汇编格式对应的源文件与行号信息 |
| `--asm=gcc` | 输出 GCC 工具链接受的 GNU 汇编语法 |
| `--asm=aasm` | 输出 Unisym `aasm` 接受的汇编语法 |
| `-o file` | 将汇编写入指定文件 |
| `-h`、`--help` | 显示帮助信息及已支持的目标信息 |

当前默认后端目标为 `-mbits=64 -minst=im -mabi=lp64 --asm=gcc`。
`mgc` 默认记录 `static` relocation model；指定 PIC/PIE 时该设置随 Clover IR
传给后端。RISC-V 后端在 PIC/PIE 下会按符号定义、链接属性、visibility 和 binding
选择 PC-relative 或 GOT 地址形式；静态模式保留原有地址生成行为。汇编输出同时保留
外部定义的 ELF visibility 与 weak binding。

`globaladdr` 与 `funcaddr` 使用相同的地址选择规则：

| 符号情况 | Static | PIC | PIE |
| --- | --- | --- | --- |
| internal 定义 | PC-relative | PC-relative | PC-relative |
| external 强定义、default visibility | PC-relative | GOT | PC-relative |
| external 强定义、hidden/protected visibility | PC-relative | PC-relative | PC-relative |
| external 声明 | PC-relative | GOT | GOT |
| weak 定义或声明 | PC-relative | GOT | GOT |

可写 global 中的符号指针初始化在三种模式下都会输出地址数据 relocation；PIC 共享对象
和 PIE 下由后续链接器处理相应 relocation。当前 PIC/PIE 模式拒绝只读 global 中的符号
指针初始化，包括聚合初始化片段；Static 模式支持只读和可写 global 的这类初始化。
这是当前后端的限制。

直接函数调用根据 relocation model 和符号属性选择调用 relocation。下表中“定义”指当前
模块内有定义，“声明”指当前模块只有外部声明；“强定义”指 external linkage 且 strong
binding 的模块内定义，weak 项优先适用。CALL 和 CALL_PLT 表示 relocation 语义，不要求
汇编输出文本完全相同：

| 符号情况 | Static | PIC | PIE |
| --- | --- | --- | --- |
| internal 定义 | CALL | CALL | CALL |
| external 强定义、default visibility | CALL | CALL_PLT | CALL |
| external 强定义、hidden/protected visibility | CALL | CALL | CALL |
| external 声明 | CALL | CALL_PLT | CALL_PLT |
| weak 定义或声明 | CALL | CALL_PLT | CALL_PLT |
| 运行库 helper | CALL | CALL_PLT | CALL_PLT |

间接调用通过函数指针执行 `jalr`，不经过 PLT。外部声明在汇编模块开头输出；运行库
helper 在调用处按外部函数声明输出。运行库 helper 在 Static 下使用 CALL，在 PIC/PIE
下使用 CALL_PLT；GCC 汇编输出用 `call` 宏表达 PLT 调用，aasm 输出使用 `CALL_PLT`。
`lp64f` 要求 F 扩展，`lp64d` 要求 F、D 扩展。生成的汇编交给 GCC 或汇编器时，
应向后续工具传递相同的 `-mabi`。`--asm=aasm` 会输出 `[ARCH]`、`[BITS]`、
`[INST]`、`[ABI]` 目标指令，以及 aasm 的 section、符号、数据、地址和指令语法；
可使用 `aasm -march=riscv -f elf64` 生成目标文件。启用 `-g` 时，GNU 格式使用
`.file/.loc`，aasm 格式使用源行指令。汇编格式选项与目标位数、指令集和 ABI
选项相互独立。

## 当前 Soft IR 语言设计

Clover Soft IR 是语言前端与目标后端之间的中间表示。前端负责把源语言语法树转换为 Soft IR；后端负责将 Soft IR 转换为相应目标的代码。本节描述 Soft IR 的语法与语义；前文中的 Hard IR 和 Exec IR 设计不改变本节内容。

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

模块还可使用 `target relocation static|pic|pie` 记录 relocation model；旧 IR
未写时按 `static` 处理。Global 和 Function 声明可在类型后附加 `visibility`
与 `binding` 属性，例如：

```clover
declare global @shared_value i32 visibility hidden binding weak
declare func @shared_function() -> i32 visibility protected binding weak
```

visibility 接受 `default`、`hidden`、`protected`，binding 接受 `strong`、`weak`；
默认分别为 `default` 和 `strong`，默认属性可省略。`internal` 表示 Clover 模块内
链接，不等同于 ELF `hidden` visibility。目标后端据此选择符号引用形式，并将支持的
visibility 与 binding 属性映射到目标对象格式。

第一版支持以下类型：

```text
void  i1  i8  i16  i32  i64  f32  f64  f128  ptr
```

整数类型本身不区分有符号和无符号；由 `sdiv`、`udiv` 和 `icmp` 的谓词
决定如何解释整数。`f32`、`f64`、`f128` 分别表示 IEEE 754 binary32、
binary64、binary128。
`ptr` 的实际位宽由 `target ptrbits` 约束，并由目标后端实现。

函数定义格式：

```clover
func @add(i32 %0, i32 %1) -> i32 {
^entry:
    %2 = add i32 %0, %1
    ret i32 %2
}
```

模块内部函数在 `func` 前使用 `internal`：

```clover
internal func @helper(i32 %0) -> i32 {
^entry:
    ret i32 %0
}
```

外部函数只声明签名：

```clover
declare func @putchar(i32) -> i32
```

函数声明只保留参数类型，不写参数 SSA 名，也没有基本块；后端可用它校验调用
签名，但不会为声明输出函数体。

### 指令

#### 值与内存

```clover
%value = const i32 42
%floating = fconst f32 1069547520
%long_double = fconst f128 0, 4611545280939032576
%copy = mov i32 %value

%address = local i32
%array = local i32, 12
%buffer = local i8, 24 align 8
global @counter i32 = 0
internal global @private_counter i32 = 0
global @handler ptr align 8 = @handle
global @third ptr align 8 = @values + 8
internal constant @.L.clover.string.0 i8, 4 = {97, 98, 99, 0}
global @packet i8, 16 align 8 = data {0: i8 1, 8: ptr @values + 4}
declare global @external_counter i32
%global_address = globaladdr @counter
%function_address = funcaddr @handle
%loaded = load i32, %address align 4
store i32 %value, %address align 4
memcopy %destination, %source, 12 align 4, 4
%next = ptradd i32 %address, %index
```

- `const`：产生整数常量。
- `fconst`：使用十进制整数写出的精确 IEEE 位模式产生浮点常量；`f128` 依次写
  低 64 位和高 64 位。
- `local`：创建函数局部存储，结果类型为 `ptr`；可选元素数量用于连续数组存储，
  可选的 `align` 指定以字节计的存储对齐，且必须是二的幂。
- `global`：定义具有静态存储期的模块级对象；可选的 `align` 指定对象对齐。
  `data` 初始化器由“字节偏移、标量类型、常量值”片段组成，未覆盖的对象区域补零；
  片段保留标量类型，由后端按照目标字节序输出，不要求前端提前编码原始字节。
  标量 `ptr` 对象及 `data` 中的 `ptr` 片段可使用 `@symbol`、`@symbol + N` 或
  `@symbol - N` 初始化，由目标文件或链接阶段完成地址重定位。`N` 是十进制字节
  加数，而不是元素下标；非零加数当前只用于全局对象符号，函数符号只接受零加数。
- `constant`：定义模块内部的只读聚合数据；初始化列表的元素数量必须与声明数量一致。
- `declare global` / `declare constant`：声明由其他模块提供存储的模块级对象，不分配
  存储且不携带初始化值；`globaladdr` 可以引用这种声明，最终由链接器解析符号。
- `internal`：可用于 `global`、`constant` 或 `func`，表示符号只在当前 Clover
  模块中可见；它与对象是否只读相互独立。
- `globaladdr`：取得模块级对象的地址，结果类型为 `ptr`。
- `funcaddr`：取得模块级函数的地址，结果类型为 `ptr`。
- `mov`：复制一个 SSA 值，不读写内存。
- `load`：从 `ptr` 地址读取值；可选的 `align` 表示地址至少具有多少字节对齐。
- `store`：向 `ptr` 地址写入值，不产生结果；同样可以携带可选的 `align`。
- `memcopy`：在两个不重叠的 `ptr` 对象之间复制固定字节数；两个 `align` 值依次
  表示目标和源地址的最低已知字节对齐。
- `ptradd`：将整数索引乘以给定元素类型的存储大小，再加到 `ptr` 地址；结果类型为 `ptr`。

`global`、`local`、`load` 和 `store` 的 `align N` 均以字节为单位，且 `N` 必须是正的
二次幂。内存访问未写 `align` 时按照访问类型的自然对齐处理；显式写出的值表示
前端能够保证的最低地址对齐。后端若不能直接完成低于自然对齐的访问，必须拆分
访问或明确拒绝，不能静默假设地址已自然对齐。当前 RISC-V 后端会把未对齐的
`i16`、`i32`、`i64` 访问拆成按小端顺序组合或分解的字节访问。
`f32`、`f64`、`f128` 的未对齐访问也会通过整数位模式拆分为字节访问。

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
`shl`、`ashr`、`lshr` 的结果类型和左操作数类型一致，右操作数可以是另一种
整数类型，以保留 C 等前端分别对两个操作数执行整数提升后的结果。

#### 浮点运算与比较

```clover
%sum = fadd f32 %left, %right
%quotient = fdiv f64 %value, %divisor
%extended = fmul f128 %left, %right
%condition = fcmp lt f64 %left, %right
```

浮点四则运算使用 `fadd`、`fsub`、`fmul`、`fdiv`。`fcmp` 的结果为 `i1`，
支持 `eq`、`ne`、`lt`、`le`、`gt`、`ge`；谓词遵循 C 对 NaN 的比较结果。

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

#### 浮点转换

```clover
%single = fptrunc f64 %wide to f32
%wide = fpext f32 %single to f64
%long_double = fpext f64 %wide to f128
%floating1 = sitofp i32 %signed to f64
%floating2 = uitofp i32 %unsigned to f64
%signed = fptosi f64 %floating to i32
%unsigned = fptoui f64 %floating to i32
```

`fptosi`、`fptoui` 转换为整数时向零舍入。浮点常量和转换在 IR 中不记录
软浮点或硬浮点策略，该选择由目标后端完成。

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
%indirect = call i32 %callee(i32 %0, i32 %1)
call void @abort()
```

直接调用使用 `@函数名`，间接调用使用 `ptr` SSA 值。调用的参数和返回类型必须
与源语言函数类型一致；具体的参数寄存器、返回值寄存器和栈布局由目标后端按照
ABI 处理。当前 C 前端暂不生成可变参数函数的间接调用。

可变参数函数在固定参数末尾写 `...`。展平的小型聚合参数成员可附加
`abiagg N`，同一非零编号表示它们来自同一个源语言聚合参数。RISC-V 后端要求
其中浮点成员的精度不超过所选硬浮点 ABI 的 FLEN。

当前暂不支持通用聚合值、聚合返回值、指针差值、`phi` 和原子操作。RISC-V
后端支持 F/D 硬浮点运算，并通过 libgcc helper 降低 binary128 运算；可按
`lp64`、`lp64f`、`lp64d` 处理标量浮点参数和返回值。超出可用参数寄存器而
需要栈传参的情况尚未实现。

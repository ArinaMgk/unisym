# Clover 编译器开发概览

本文概括已经落地的前端与后端能力，供后续增加其他语言前端和目标后端时参考。

## 前端

- 已建立独立前端入口和严格命令选项解析；当前实现 C99 子集，并可通过
  `--lango=c`、`--std=c99` 选择。
- 支持以 `-Fname[=value]` 向选中的前端传递专用选项；C 前端支持
  `-Fdata-model=ilp32/lp64/llp64`，默认使用 `lp64`。
- 支持 C/C++ 注释、AST 输出、Clover IR 输出，以及 `-g` 源文件、行号和源码注释。
- 支持基础整数类型、C99 `_Bool`、signed/unsigned、const、目标相关数据模型、
  整数转换和 `sizeof`；`_Bool` 对象占一个 C 字节，参与整数运算前提升为 `int`。
- 支持 `float`、`double`、RISC-V binary128 `long double` 类型及其局部/全局对象、数组、指针、record 成员和
  `sizeof`；支持十进制、十六进制浮点常量、四则运算、比较、复合赋值、递增递减
  以及整数与浮点、不同浮点精度之间的显式或通常算术转换。函数实参、
  返回表达式和条件运算也执行对应的算术转换。
- 通常算术转换支持整数提升、不同 rank 的 signed/unsigned 组合以及浮点公共类型；
  移位运算的两个操作数分别执行整数提升，结果类型取提升后的左操作数类型。
- 支持 C99 十进制、八进制、十六进制整数常量及 `U`、`L`、`LL` 组合后缀；
  常量类型按数值、后缀和数据模型选择。字符和窄字符串字面量使用公共解析器的
  C99 转义规则，严格检查八进制、任意长度十六进制、4/8 位通用字符名、未知转义
  和 Unicode 标量；通用字符名在窄字符串中编码为 UTF-8。
- 支持局部及全局变量、词法作用域、指针、取地址、解引用、指针算术、字符串和
  单维/多维数组。数组初始化支持缺省元素补零、嵌套括号划分子数组，以及通过
  初始化列表推导最外层数组长度。窄字符串可以初始化 `char`、`signed char`、
  `unsigned char` 及其 const 数组，显式数组长度恰好容纳字符时允许省略结尾零。
  函数形式参数支持 `T array[]`、`T array[N]` 和 `T array[][N]`，并按照 C99
  规则调整为对应指针类型；无初始化器的普通局部对象仍须具有完整数组类型。
- 支持文件作用域 `static` 变量和函数的内部链接、文件作用域 `extern` 对象声明，
  以及同名兼容声明、tentative definition 和完整定义的合并。仅有 `extern` 声明的
  对象降低为 Clover `declare global`，不在当前模块分配存储。
- 支持块作用域 `static` 标量、
  指针和数组的静态存储期。块静态对象降低为具有唯一内部名称的 Clover global，
  初始化只写入模块数据，不会在每次进入函数或代码块时重复执行。当前静态初始化
  沿用全局常量初始化能力；块静态 record、`long double` 数组初始化和块作用域
  `extern` 尚未实现。
- 支持函数定义与调用、return、if/else、while、for、switch/case/default、空语句和
  逗号运算符。switch 支持整数常量 case、default、贯穿、嵌套及重复标签诊断；
  当前降低为 Clover 的整数比较和基本块跳转链，不要求增加专用 IR 指令。
- 支持可变参数函数定义和调用端默认参数提升：窄整数提升为 `int` 或
  `unsigned int`，`float` 提升为 `double`；`va_list` 系列尚未实现。
- `break` 跳出最内层循环或 switch；`continue` 仍只作用于最内层循环，因此位于
  switch 内且外层存在循环时会继续该循环。
- 支持函数作用域 label 和 `goto`，包括前向/后向跳转、嵌套语句中的标签，
  以及重复标签和未定义标签诊断。
- 支持整数一元 `!`、`~`，前后缀 `++`、`--`，以及算术、余数、移位和位运算的
  复合赋值；指针支持 `+=`、`-=`。复合赋值只求值左值一次。
- 支持具有短路副作用语义的 `&&`、`||`，以及右结合的条件运算符 `?:`；当前通过
  隐藏 local 汇合分支结果，尚未要求 Clover IR 提供 `phi`。
- 支持 C99 显式标量转换：整数宽度/符号转换、指针间转换、整数与指针互转及
  转换为 `void`。整数转指针会先按源整数的符号属性扩展到目标指针位宽。
- 支持 struct/union 的 ABI 布局、嵌套成员、成员数组、记录数组、`.`、`->` 和
  对应的 `sizeof`；RISC-V 硬浮点 ABI 已支持由一至两个整数/浮点成员组成、且至少
  含一个浮点成员的 struct 按值参数。聚合初始化、整体赋值和按值返回尚未实现。
- 支持 record 前向声明、指针成员和自引用结构。
- 支持有标签 `enum` 的文件及块作用域、内外层遮蔽、前向声明、定义后直接声明
  对象，以及枚举对象、函数参数和返回值。枚举常量表达式支持整数和字符常量、
  此前枚举项、整数 cast、`sizeof(type)`、一元/算术/移位/比较/位/逻辑和条件运算。
- enum 兼容整数类型按照完整值域和 C 数据模型，从 signed/unsigned 的 `int`、
  `long`、`long long` 中选择；诊断重复枚举项、tag 重定义、record tag 冲突和
  同作用域普通标识符冲突。enum 前向声明是当前 Magice C99 子集提供的扩展，
  完成定义前只能用于指针等不要求完整对象布局的位置。枚举常量求值当前限制在
  64 位有符号数值范围内。
- 支持 Magice 特有的 `packed`、`padded`、`alignb(bytes)` 和 `aligne(exponent)`
  record 布局修饰符；显式对齐当前支持 1、2、4、8、16 字节。这些不是 C99
  标准关键字。
- C 左值会跟踪已知字节对齐，并写入 Clover `load/store align N`，供目标后端判断
  是否可以直接访问。

## C前端架构规划

- `Frontend-c.cpp`：C 前端入口、词法分析和各分析阶段调度。
- `Frontend-c-operator.cpp`：C 运算符分级、cast 识别和条件运算符整理。
- `Frontend-c-statement.cpp`：语句分类与基础语义分析；以后可继续按声明、record
  和类型处理拆分。
- `Frontend-c-clover.cpp`：Clover lowering 总入口，以及翻译单元、函数和全局符号组织。
- `Frontend-c-clover-expression.cpp`：表达式、调用、数组、成员访问和运算符 lowering。
- `Frontend-c-clover-statement.cpp`：局部声明、作用域和控制流 lowering。
- `Frontend-c-clover-type.cpp`：表达式类型推导、整数提升和类型转换。
- `Frontend-c-clover-literal.cpp`：整数、字符与字符串字面量解析，以及字符串常量生成。
- Clover lowering 的共享内部声明统一放在
  `inc/cpp/Lango/Frontend/C/Frontend-c-clover-internal.hpp`，不放入 `lib/cpp`。

## 后端

- 已建立 `BackendInterface`，当前实现独立的 Clover RISC-V 后端和命令行工具
  `clover-riscv`。
- 当前目标为 RV64，支持 `-mbits=64`、`-minst=i/im/imf/imfd`，覆盖整数运算、比较、转换、
  指针寻址、控制流、函数调用、局部栈对象、全局对象和字符串常量。
- 支持 `-minst=imf/imfd`，可降低 Clover `f32/f64` 常量、内存访问、四则运算、
  比较及整数/浮点精度转换。
- `f128` 采用 RISC-V psABI 的 16 字节 binary128 布局和整数寄存器对传参/返回；
  四则运算、比较及转换通过 libgcc TF helper 降低，不要求目标具有 128 位浮点指令。
- 支持 `-mabi=lp64/lp64f/lp64d`。浮点参数和返回值会按 ABI 选择整数参数寄存器
  或浮点参数寄存器；整数与浮点参数寄存器分别计数，暂不支持参数溢出到栈。
- 可变浮点实参遵循 RISC-V 调用约定走整数参数寄存器；binary128 固定参数占用
  连续两个整数参数寄存器，可变参数还按偶数编号寄存器对齐。硬浮点 ABI 可处理成员精度不超过 ABI FLEN 的
  前端展平小型浮点聚合参数。
- 当前实现 GCC/GNU 汇编输出，并预留 `--asm` 选择其他汇编格式的接口。
- `-g` 可以把 Clover 的文件和行号信息输出为 `.file`、`.loc` 等汇编指令。
- Clover 模块使用 `target ptrbits` 保留前端确定的指针宽度；RISC-V 后端会拒绝
  与自身目标位数不一致的 IR。
- Clover 的 global、constant 和 function 均可携带独立的 internal 链接属性；
  RISC-V 汇编输出不会为内部符号生成 `.globl`。
- Clover 支持 `declare global` 和 `declare constant` 外部对象声明；RISC-V 后端不为
  声明分配存储，但允许 `globaladdr` 生成交由链接器解析的符号引用。
- 局部对象支持显式存储对齐；`load/store` 会读取访问对齐信息。RISC-V 后端会将
  未对齐的 i16/i32/i64 load/store 拆为小端字节访问。

# Clover 编译器开发概览

本文概括已经落地的前端与后端能力，供后续增加其他语言前端和目标后端时参考。

## 前端

- 增加 SystemVerilog 可综合子集前端，支持 ANSI 风格 `logic` 端口、`assign`、标识符、十进制常量和加法，并降低为包含 `module`、`port`、`read`、`const`、`add` 和 `assign` 的 Clover Hard IR。
- 已建立独立前端入口和严格命令选项解析；当前实现 C99 子集，并可通过 `--lango=c`、`--std=c99` 选择。
- 支持以 `-Fname[=value]` 向选中的前端传递专用选项；C 前端支持 `-Fdata-model=ilp32/lp64/llp64`，默认使用 `lp64`。
- 支持 C/C++ 注释、AST 输出、Clover IR 输出，以及 `-g` 源文件、行号和源码注释。
- 支持 C99 `#include`、`#define`、`#undef`、`#if/#ifdef/#ifndef/#elif/#else/#endif`；`#include` 支持当前文件相对路径、`-I` 搜索目录、引号及尖括号 header-name，以及宏展开产生的 header-name。宏按 preprocessing token 展开，支持函数式及可变参数宏、`#` 字符串化、`##` 拼接、递归重扫和条件表达式短路求值。
- 支持基础整数类型、C99 `_Bool`、signed/unsigned、const、目标相关数据模型、整数转换和 `sizeof`；`_Bool` 对象占一个 C 字节，参与整数运算前提升为 `int`。
- 支持 `float`、`double`、RISC-V binary128 `long double` 类型及其局部/全局对象、数组、指针、record 成员和 `sizeof`；支持十进制、十六进制浮点常量、四则运算、比较、复合赋值、递增递减以及整数与浮点、不同浮点精度之间的显式或通常算术转换。函数实参、返回表达式和条件运算也执行对应的算术转换。
- 通常算术转换支持整数提升、不同 rank 的 signed/unsigned 组合以及浮点公共类型；移位运算的两个操作数分别执行整数提升，结果类型取提升后的左操作数类型。
- 支持 C99 十进制、八进制、十六进制整数常量及 `U`、`L`、`LL` 组合后缀；常量类型按数值、后缀和数据模型选择。字符和窄字符串字面量使用公共解析器的 C99 转义规则，严格检查八进制、任意长度十六进制、4/8 位通用字符名、未知转义和 Unicode 标量；通用字符名在窄字符串中编码为 UTF-8。
- 支持局部及全局变量、词法作用域、指针、取地址、解引用、指针算术、字符串和单维/多维数组。数组初始化支持缺省元素补零、嵌套括号划分子数组，以及通过初始化列表推导最外层数组长度。窄字符串可以初始化 `char`、`signed char`、`unsigned char` 及其 const 数组，显式数组长度恰好容纳字符时允许省略结尾零。函数形式参数支持 `T array[]`、`T array[N]` 和多维定长或运行期界限，并按照 C99 规则调整为对应指针类型；无初始化器的普通局部对象仍须具有完整数组类型。
- 支持文件作用域 `static` 变量和函数的内部链接、文件作用域 `extern` 对象声明，以及同名兼容声明、tentative definition 和完整定义的合并。仅有 `extern` 声明的对象降低为 Clover `declare global`，不在当前模块分配存储。
- 支持块作用域 `static` 标量、指针和数组的静态存储期。块静态对象降低为具有唯一内部名称的 Clover global，初始化只写入模块数据，不会在每次进入函数或代码块时重复执行。当前静态初始化沿用全局常量初始化能力，并支持 record 和 `long double` 数组；块作用域 `extern` 对象声明复用模块级对象实体，不分配局部存储；名称绑定只在声明所在的词法作用域内生效，并可与后续文件作用域声明或定义合并。
- 支持函数定义、声明、原型与调用；原型支持无名参数、`(void)` 空参数列表、可变参数、数组参数调整和指针返回类型，并检查后续声明及定义的签名兼容性。块作用域函数声明从声明点起仅在所属词法作用域可见，可与文件作用域声明或定义合并；未在本模块定义的函数会降低为 Clover `declare func`。
- 支持通用括号声明符、函数指针和数组指针，包括函数指针参数、返回函数指针、函数指针数组、显式复杂函数定义及连续复杂声明。函数名可退化为函数指针，支持直接、解引用及表达式结果的间接调用；可变参数间接调用暂未实现。
- 支持文件及块作用域 `typedef`，包括标量、指针、数组和括号声明符，并按照 C 普通标识符命名空间处理内层变量、参数及 typedef 的遮蔽与同作用域冲突。
- 支持 return、if/else、while、do-while、for、switch/case/default、空语句和逗号运算符；`for` 第一子句可声明对象，并具有独立块作用域。switch 支持整数常量 case、default、贯穿、嵌套及重复标签诊断；当前降低为 Clover 的整数比较和基本块跳转链，不要求增加专用 IR 指令。
- 支持可变参数函数定义和调用端默认参数提升：窄整数提升为 `int` 或 `unsigned int`，`float` 提升为 `double`；`va_list` 系列尚未实现。
- `break` 跳出最内层循环或 switch；`continue` 仍只作用于最内层循环，因此位于 switch 内且外层存在循环时会继续该循环。
- 支持函数作用域 label 和 `goto`，包括前向/后向跳转、嵌套语句中的标签，以及重复标签和未定义标签诊断。
- 支持整数一元 `!`、`~`，前后缀 `++`、`--`，以及算术、余数、移位和位运算的复合赋值；指针支持 `+=`、`-=`。复合赋值只求值左值一次。
- 支持具有短路副作用语义的 `&&`、`||`，以及右结合的条件运算符 `?:`；当前通过隐藏 local 汇合分支结果，尚未要求 Clover IR 提供 `phi`。
- 支持 C99 显式标量转换：整数宽度/符号转换、指针间转换、整数与指针互转及转换为 `void`。整数转指针会先按源整数的符号属性扩展到目标指针位宽。
- 支持 struct/union 的 ABI 布局、嵌套成员、成员数组、记录数组、`.`、`->` 和对应的 `sizeof`；RISC-V 硬浮点 ABI 已支持由一至两个整数/浮点成员组成、且至少含一个浮点成员的 struct 按值参数。局部、全局和块静态聚合初始化支持嵌套 struct/union、成员数组、缺省成员补零、内部初始化列表括号省略和最外层数组长度推导；兼容 struct/union 对象可用于局部整体复制初始化。支持 C99 指定初始化器 `.member =`、`[constant-index] =` 及连续 designator，覆盖局部、全局和块静态对象、union 非首成员、指定项后的普通项游标推进、重复指定覆盖和不完整数组长度推导。支持 C99 复合字面量，包括标量、数组、struct/union、指定初始化器、最外层数组长度推导、成员访问、下标和取地址；结果保留 C99 左值属性。文件作用域复合字面量降低为具有唯一内部名称的 Clover global，并可用于静态指针初始化；普通全局对象和函数的地址也可作为静态指针初始化器。静态标量、数组、struct、union 和块静态聚合体中的指针成员支持全局对象地址、数组退化、成员/下标取址、常量指针算术、函数地址及文件作用域复合字面量，并保留有符号字节加数。静态存储期初始化暂不支持不同类型或宽度的重叠覆盖。支持 struct/union 普通整体赋值、链式赋值、赋值结果的成员读取和复制初始化，以及 record 按值返回；packed record 也沿用固定尺寸内存复制。
- 支持 record 前向声明、指针成员和自引用结构。
- record 成员接入通用 C 声明符解析，支持函数指针成员、数组指针成员、函数指针数组、复杂括号声明符和逗号连续成员声明。
- C 数组类型区分不完整数组、整数常量长度数组、变长数组及函数原型中的 `[*]`；前端保留变长界限表达式并检查文件作用域、存储期、初始化器和 record 成员限制。单维及多维 VLA 在声明点依次求值各层界限，计算基础元素总数和每层运行期字节步长，通过 Clover 动态 `local` 生成连续局部存储。多维下标、指向 VLA 的指针、指针算术及 `sizeof(VLA)` 使用前端保存的运行期维度尺寸信息，不重复求值声明界限；VLA type-name 的界限则在执行 `sizeof` 时求值。函数形式参数的最外层数组按 C99 调整为指针，内层运行期界限只可引用此前已经声明的参数，固定数组和 VLA 均可传入。块退出、循环迭代、`break`、`continue` 和 `return` 会恢复动态栈位置；C 前端记录各 label、`goto`、`case/default` 位置的变长修改标识符作用域。`goto` 可在相同状态间跳转，也可在恢复动态栈后离开一层或多层 VLA 作用域，但不能跳入尚未生效的 VLA、指向 VLA 的指针等变长修改标识符作用域；`case/default` 采用相同的进入检查。Clover 与 RISC-V 后端支持 `stacksave`、`stackrestore` 和动态栈分配。当前声明准备阶段尚不能在全局/record 布局前识别引用枚举项的长度表达式。
- C 前端会把完整 struct/union 布局输出为 Clover `type @Name = struct/union`；ABI 产生的成员间隙和尾部间隙以匿名 `bN` 成员表示，成员数组保留完整维度。
- 支持有标签 `enum` 的文件及块作用域、内外层遮蔽、前向声明、定义后直接声明对象，以及枚举对象、函数参数和返回值。枚举常量表达式支持整数和字符常量、此前枚举项、整数 cast、`sizeof(type)`、一元/算术/移位/比较/位/逻辑和条件运算。
- enum 兼容整数类型按照完整值域和 C 数据模型，从 signed/unsigned 的 `int`、`long`、`long long` 中选择；诊断重复枚举项、tag 重定义、record tag 冲突和同作用域普通标识符冲突。enum 前向声明是当前 Magice C99 子集提供的扩展，完成定义前只能用于指针等不要求完整对象布局的位置。枚举常量求值当前限制在 64 位有符号数值范围内。
- 支持 Magice 特有的 `packed`、`padded`、`alignb(bytes)` 和 `aligne(exponent)` record 布局修饰符；显式对齐当前支持 1、2、4、8、16 字节。这些不是 C99 标准关键字。
- C 左值会跟踪已知字节对齐，并写入 Clover `load/store align N`，供目标后端判断是否可以直接访问。

## C前端架构规划

- `Frontend-c.cpp`：C 前端入口、词法分析和各分析阶段调度。
- `Frontend-c-preprocessor-lexer.cpp`：C 续行、注释和 preprocessing token 扫描。
- `Frontend-c-preprocessor.cpp`：宏表、宏替换和条件编译指令处理。
- `Frontend-c-preprocessor-include.cpp`：头文件名解析、搜索及递归预处理。
- `Frontend-c-preprocessor-expression.cpp`：`#if/#elif` 整数常量表达式求值。
- `Frontend-c-operator.cpp`：C 运算符分级、cast 识别和条件运算符整理。
- `Frontend-c-declaration.cpp`：共享声明符解析，以及指针、数组、函数等派生类型层的组合顺序。
- `Frontend-c-initializer.cpp`：在通用运算符整理前识别并隔离 C99 指定初始化器语法。
- `Frontend-c-compound-literal.cpp`：识别 `(type-name){initializer}`，记录对象类型并将初始化列表整理为独立的 C99 复合字面量节点。
- `Frontend-c-statement.cpp`：语句分类、声明准备、作用域与基础语义分析；以后可继续把 typedef、函数签名和 record 分析迁入独立模块。
- `Frontend-c-clover.cpp`：Clover lowering 总入口，以及翻译单元、函数和全局符号组织。
- `Frontend-c-clover-expression.cpp`：表达式、调用、数组、成员访问和运算符 lowering。
- `Frontend-c-clover-statement.cpp`：局部声明、作用域和控制流 lowering。
- `Frontend-c-clover-initializer.cpp`：数组、struct 和 union 的递归初始化规划及 lowering。
- `Frontend-c-clover-type.cpp`：表达式类型推导、整数提升和类型转换。
- `Frontend-c-clover-literal.cpp`：整数、字符与字符串字面量解析，以及字符串常量生成。
- Clover lowering 的共享内部声明统一放在 `inc/cpp/Lango/Frontend/C/Frontend-c-clover-internal.hpp`，不放入 `lib/cpp`。

## Pass

- +8=2026/11/17:19 已建立独立的 Clover Pass API 和命令行工具 `clover-opt`。当前实现 `forward-jumps`，用于改写跳转目标并删除仅包含无条件跳转的非入口转发块。

## 后端

- 已建立 `BackendInterface`，当前实现独立的 Clover RISC-V 后端和命令行工具 `clover-riscv`。
- 当前目标为 RV64，支持 `-mbits=64`、`-minst=i/im/imf/imfd`，覆盖整数运算、比较、转换、指针寻址、控制流、函数调用、局部栈对象、全局对象和字符串常量。
- 支持 `-minst=imf/imfd`，可降低 Clover `f32/f64` 常量、内存访问、四则运算、比较及整数/浮点精度转换。
- `f128` 采用 RISC-V psABI 的 16 字节 binary128 布局和整数寄存器对传参/返回；四则运算、比较及转换通过 libgcc TF helper 降低，不要求目标具有 128 位浮点指令。
- 支持 `-mabi=lp64/lp64f/lp64d`。浮点参数和返回值会按 ABI 选择整数参数寄存器或浮点参数寄存器；整数与浮点参数寄存器分别计数，栈参数按 ABI 对齐。
- Clover IR 以 `byval @类型 %地址` 保留完整聚合按值参数。RISC-V 后端已实现不超过两个 XLEN 字的整数类聚合分块传递，以及更大聚合的间接传递；支持寄存器/栈混合分块，并在被调函数栈帧重建直接传递的聚合对象。硬浮点 ABI 的简单小型聚合仍由前端以 `abiagg` 展平，递归硬浮点聚合分类尚未完成。
- Clover IR 函数签名和 `call`/`ret` 以 `byval @类型` 表示聚合返回；RISC-V 后端对不超过两个 XLEN 字的聚合使用 `a0`/`a1` 返回，对更大的聚合使用隐藏返回缓冲区地址，并调整显式参数位置。平坦的单/双浮点成员结构在硬浮点 ABI 下使用浮点返回寄存器；递归硬浮点聚合分类尚未完成。
- 可变浮点实参遵循 RISC-V 调用约定走整数参数寄存器；binary128 使用两个整数参数块，可变参数依其对齐要求分配偶数编号的寄存器对。超出寄存器范围的多字标量与聚合参数可按 ABI 在寄存器和栈之间拆分。
- 支持 `--asm=gcc` 和 `--asm=aasm`。aasm 输出包含目标位数、指令集、ABI、section、符号和数据指令，并转换地址写法及后端使用的 RISC-V 伪指令；可由 Unisym 内建汇编器生成 RISC-V ELF64 目标文件。
- `-g` 可以把 Clover 的文件和行号信息转换为所选汇编格式对应的源位置指令。
- Clover 模块使用 `target ptrbits` 保留前端确定的指针宽度；RISC-V 后端会拒绝与自身目标位数不一致的 IR。
- RISC-V PIC/PIE 地址生成按符号链接属性、visibility 与 binding 选择 PC-relative 或 GOT relocation；GCC 与 AASM 汇编输出均保留定义符号属性，AASM ELF32/ELF64 输出支持 weak binding。
- RISC-V 的 Static、PIC 与 PIE 地址输出已覆盖数据地址、函数地址、可写符号指针初始化及最终链接；PIC 共享对象和 PIE 不产生 `TEXTREL`，只读符号指针初始化在 PIC/PIE 下会明确拒绝。
- AASM RISC-V 指令 relocation 保留目标符号与独立加数，weak GOT 取址和跨 section 控制流可由链接器按符号值正确处理。
- RISC-V 直接调用与运行库 helper 按 relocation model 及符号属性选择 CALL 或 CALL_PLT；外部声明统一输出，GCC 与 AASM 后端均覆盖对应 relocation。
- Clover 的 global、constant 和 function 均可携带独立的 internal 链接属性；RISC-V 汇编输出不会为内部符号生成 `.globl`。
- Clover global 支持显式对象对齐和带字节偏移的有类型初始化片段；未覆盖区域由后端补零，目标后端负责按照自身字节序输出片段。`memcopy` 支持固定大小的非重叠对象复制，当前 RISC-V 后端使用字节循环降低。
- Clover 的标量 `ptr` global 和聚合 `data` 中的 `ptr` 片段支持符号重定位及有符号字节加数；RISC-V 后端按目标指针宽度输出对应地址数据，并校验函数符号不携带非零加数。
- Clover 支持 `declare global` 和 `declare constant` 外部对象声明；RISC-V 后端不为声明分配存储，但允许 `globaladdr` 生成交由链接器解析的符号引用。
- Clover 支持 `declare func` 外部函数签名；RISC-V 后端使用声明验证直接调用，但不会为其输出函数体，符号由链接阶段解析。
- Clover 使用 `funcaddr` 表示函数地址，并允许 `call` 以 `ptr` SSA 值作为间接目标；RISC-V 后端分别降低为地址装载和 `jalr`。标量全局 `ptr` 可使用函数或全局符号初始化，并由汇编器、链接器完成重定位。
- 局部对象支持显式存储对齐；`load/store` 会读取访问对齐信息。RISC-V 后端会将未对齐的 i16/i32/i64 load/store 拆为小端字节访问。
- RISC-V 后端可按 Clover 命名 struct/union 和多维数组类型计算 global、local 及 `ptradd` 的对象尺寸；非二次幂的固定指针步长会降低为基础整数移位与加法。

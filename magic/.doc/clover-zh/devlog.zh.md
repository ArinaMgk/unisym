# Clover 编译器开发概览

本文概括已经落地的前端与后端能力，供后续增加其他语言前端和目标后端时参考。

## 前端

- 已建立独立前端入口和严格命令选项解析；当前实现 C99 子集，并可通过
  `--lango=c`、`--std=c99` 选择。
- 支持 C/C++ 注释、AST 输出、Clover IR 输出，以及 `-g` 源文件、行号和源码注释。
- 支持基础整数类型、signed/unsigned、const、目标相关数据模型、整数转换和 `sizeof`。
- 支持局部及全局变量、词法作用域、指针、取地址、解引用、指针算术、字符串和
  单维/多维数组。
- 支持函数定义与调用、return、if/else、while、for、空语句和逗号运算符。
- 支持整数一元 `!`、`~`，前后缀 `++`、`--`，以及算术、余数、移位和位运算的
  复合赋值；指针支持 `+=`、`-=`。复合赋值只求值左值一次。
- 支持具有短路副作用语义的 `&&`、`||`，以及右结合的条件运算符 `?:`；当前通过
  隐藏 local 汇合分支结果，尚未要求 Clover IR 提供 `phi`。
- 支持 C99 显式标量转换：整数宽度/符号转换、指针间转换、整数与指针互转及
  转换为 `void`。整数转指针会先按源整数的符号属性扩展到目标指针位宽。
- 支持 struct/union 的 ABI 布局、嵌套成员、成员数组、记录数组、`.`、`->` 和
  对应的 `sizeof`；聚合初始化、整体赋值和按值传参/返回尚未实现。
- 支持 record 前向声明、指针成员和自引用结构。
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
- `Frontend-c-clover-literal.cpp`：整数与字符串字面量解析，以及字符串常量生成。
- Clover lowering 的共享内部声明统一放在
  `inc/cpp/Lango/Frontend/C/Frontend-c-clover-internal.hpp`，不放入 `lib/cpp`。

## 后端

- 已建立 `BackendInterface`，当前实现独立的 Clover RISC-V 后端和命令行工具
  `clover-riscv`。
- 当前目标为 RV64，支持 `-mbits=64`、`-minst=i/im`，覆盖整数运算、比较、转换、
  指针寻址、控制流、函数调用、局部栈对象、全局对象和字符串常量。
- 当前实现 GCC/GNU 汇编输出，并预留 `--asm` 选择其他汇编格式的接口。
- `-g` 可以把 Clover 的文件和行号信息输出为 `.file`、`.loc` 等汇编指令。
- 局部对象支持显式存储对齐；`load/store` 会读取访问对齐信息。RISC-V 后端会将
  未对齐的 i16/i32/i64 load/store 拆为小端字节访问。

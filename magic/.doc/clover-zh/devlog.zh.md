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
- 支持 struct/union 的 ABI 布局、嵌套成员、成员数组、记录数组、`.`、`->` 和
  对应的 `sizeof`；聚合初始化、整体赋值和按值传参/返回尚未实现。
- 支持 record 前向声明、指针成员和自引用结构。
- 支持 Magice 特有的 `packed`、`padded`、`alignb(bytes)` 和 `aligne(exponent)`
  record 布局修饰符；显式对齐当前支持 1、2、4、8、16 字节。这些不是 C99
  标准关键字。
- C 左值会跟踪已知字节对齐，并写入 Clover `load/store align N`，供目标后端判断
  是否可以直接访问。

## 后端

- 已建立 `BackendInterface`，当前实现独立的 Clover RISC-V 后端和命令行工具
  `clover-riscv`。
- 当前目标为 RV64，支持 `-mbits=64`、`-minst=i/im`，覆盖整数运算、比较、转换、
  指针寻址、控制流、函数调用、局部栈对象、全局对象和字符串常量。
- 当前实现 GCC/GNU 汇编输出，并预留 `--asm` 选择其他汇编格式的接口。
- `-g` 可以把 Clover 的文件和行号信息输出为 `.file`、`.loc` 等汇编指令。
- 局部对象支持显式存储对齐；`load/store` 会读取访问对齐信息。RISC-V 后端会将
  未对齐的 i16/i32/i64 load/store 拆为小端字节访问。

# AASM 开发概览

本文概括当前 AASM 已有能力和已经确定的代码职责，供维护汇编器、增加架构支持和
维护输出格式时参考。

## 当前能力

- 提供汇编预处理器，支持宏、include、条件预处理、预定义宏和依赖信息。
- 支持 x86、x86-64 指令解析、有效地址、前缀、指令模板匹配和机器码生成。
- 支持多遍汇编、前向符号、局部标签、表达式、section、global、extern 和 common。
- 支持 BIN、ELF32、ELF64、COFF、Mach-O、AOUT、OMF、IEEE 和 AS86 等输出实现。
- 支持 listing，以及部分输出格式提供的 DWARF、STABS 或格式专用调试信息。
- 提供 `AsmTrace.cpp` 文本跟踪输出，用于检查输出层收到的 section、symbol、data、
  relocation 和 debug 事件。

## 代码职责

- `aasm-prep.cpp` 实现 Preprocessor。
- `aasm.cpp` 负责程序入口、目标初始化、运行模式分派和输入输出生命周期。
- `aasm-assembler.cpp` 实现 Assembler，负责架构无关的多遍调度、directive 处理和
  pass 收敛。
- `aasm-statement.cpp` 负责标签、`EQU`、`TIMES`、数据定义、保留空间和 `INCBIN`
  等架构无关汇编语句。
- `aasm-label.cpp` 和 `aasm-eval.cpp` 负责符号与表达式。
- `inc/cpp/Lango/Archend.hpp` 定义 Archend 的标准契约。
- `lib/cpp/Lango/Archend/Archend-x86.cpp` 是当前 x86 Archend 适配层，管理
  架构指令解析、编码与前向引用。
- `inc/cpp/Lango/Archend/x86` 和 `lib/cpp/Lango/Archend/x86` 分别保存 x86 内部头、
  解析器、编码器、生成输入和生成结果。
- `asm/todata` 和 `asm/data` 保存预处理宏等 AASM 通用生成输入与结果。
- `inc/cpp/Format/Code.hpp` 定义 Codeend 的标准契约和只读格式信息。
- `aasm-format.cpp` 负责 Codeend 注册、选择和现有格式函数表的适配。
- `lib/cpp/Format/Code` 保存现有 Codeend 格式实现，继续使用各格式原有文件名。

## 架构命名

AASM 使用 Preprocessor、Assembler、Archend 和 Codeend 描述内部职责。Archend
表示架构相关的汇编解析和编码；Codeend 表示 BIN、ELF、COFF 等代码格式输出层。
Magice 的 Frontend 和 Backend 保持原有含义，其中 Backend 输出汇编文本，再由
GCC AS 或 AASM 处理。

## 测试

`demo/test/clover/arch-x86_64` 保存 Magice x86-64 汇编输出与 AASM x86-64 汇编
处理相关的基线例程。测试文件用于观察指令编码、标签、分支、section 和目标格式
输出是否保持稳定。

已增加 `asm/check.sh` 自动回归入口，检查机器码、前缀、寻址、数据与结构、
预处理结果、分支优化、ELF64 section、符号与 relocation，以及无效寻址诊断。

已引入 `AssemblerOptions` 和 `AssemblySession`。命令行配置统一写入
`AssemblerOptions`，主调度和多遍汇编通过 `AssemblySession` 记录一次汇编任务的
运行状态；现有解析与编码模块暂时通过兼容状态继续工作。

已建立 `AssemblyTarget`，集中保存 Archend、Codeend、默认位宽和 CPU。命令行、
Assembler、通用语句解析和 Archend 通过目标对象访问架构与输出格式；每个 pass
仍将可变位宽和 CPU 状态恢复到 `AssemblySession`。

Archend 已提供只读信息、默认项、查找和列表接口。命令行可以通过 `-march` 或
`--arch` 选择已注册架构；切换 Archend 时同步恢复默认位宽和 CPU，并在汇编前检查
Codeend 声明的架构兼容性。

已从程序入口提取 `Assembler`，多遍汇编、directive 识别与处理和 pass 收敛现由
`aasm-assembler.cpp` 管理。

已建立 `ArchendInterface`，并将 x86 指令行解析、前向引用、尺寸计算和编码接入
`ArchendX86`；Assembler 仅通过标准契约驱动架构相关处理。

已建立 `CodeendInterface`。格式元信息通过 `DumpCodeend()` 只读返回，命令行、
Assembler 和输出生命周期通过 Codeend 调用；现有格式实现由 `aasm-format.cpp`
适配并保持原有文件和函数表。

x86 编码器已改为通过 `CodeendOutput` 提交数据和重定位事件，`aasm-format.cpp`
负责转换为现有格式输出类型。listing 继续使用原接口。

x86 私有指令类型已集中到
`inc/cpp/Lango/Archend/x86/Instruction-x86.hpp`，公共 AASM 头文件不再隐式包含
opcode、operand、instruction、REX/VEX 和指令模板定义。x86 parser 与 encoder 的内部入口
统一使用 `X86*` 命名；CPU、位宽和默认寻址模式策略由 `ArchendX86` 提供。

x86 parser、encoder、指令与寄存器表、反汇编索引、token hash 及其生成输入已收归
对应的 `inc`、`lib` 架构目录。生成脚本使用固定架构目录输出，不再把 x86 数据
写入 AASM 通用 `asm/data` 目录。

标签系统已统一使用 `Labelsys` 静态接口完成生命周期、查询、作用域、外部符号判断、
全局声明、定义、重定义和 common 符号提交。无调用的旧标签函数已删除；接收
`outffmt` 的 `Define` 重载仅保留给旧格式初始化回调，并标记为随旧接口移除。

通用汇编语句已从 x86 parser 中分离。Assembler 先解析标签、`EQU`、`TIMES`、
数据定义、保留空间和 `INCBIN`，再将架构指令交给 Archend；通用表达式扫描不再
依赖 x86 指令和寄存器 token 表。

x86 parser、encoder 和生成表已移除通用语句伪 opcode 及其扩展 operand 数据结构。
prefix-only 行改用 x86 instruction 的独立状态，并保持原有前缀字节输出。

Codeend 已增加语义化 `CodeFixup` 契约，区分绝对地址、PC 相对地址、分支和调用，
并描述存储宽度、有效位、位偏移、右移量、符号属性、目标 section、加数和 PC 偏移。
x86 编码器与通用数据语句通过 fixup 提交重定位；现有格式实现暂由
`aasm-format.cpp` 转换到原有输出接口，生成的机器码和 relocation 保持不变。

Binary 回归已覆盖 `CodeendOutput` 和本地 `CodeFixup` 的完整输出链，包括 `ORG`、
section 对齐、nobits reserve、绝对地址、PC 相对地址、分支和调用。格式专用 directive
现在会交给所选 Codeend 处理，平坦二进制继续拒绝外部未解析符号。

已建立 `BYTE`、`BYT2` 至 `BYT32` 固定数据宽度限定符，并接入 x86 操作数解析与
回归测试；`BYTE` 已从 x86 私有特殊 token 调整为架构无关的 1 字节宽度定义。


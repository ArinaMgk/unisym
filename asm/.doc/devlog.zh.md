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
- `aasm.cpp` 负责程序入口、运行模式分派和输入输出生命周期。
- `aasm-assembler.cpp` 实现 Assembler，负责架构无关的多遍调度、directive 处理和
  pass 收敛。
- `aasm-label.cpp` 和 `aasm-eval.cpp` 负责符号与表达式。
- `inc/cpp/Lango/Archend.hpp` 定义 Archend 的标准契约。
- `lib/cpp/Lango/Archend/Archend-x86.cpp` 是当前 x86 Archend 适配层，管理
  架构相关的行处理与前向引用。
- `aasm-parse.cpp` 和 `aasm-gena.cpp` 是 x86 Archend 内部使用的解析与编码实现。
- `asm/todata` 和 `asm/data` 保存 x86 指令、寄存器和 token 表的生成输入与结果。
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

已从程序入口提取 `Assembler`，多遍汇编、directive 识别与处理和 pass 收敛现由
`aasm-assembler.cpp` 管理。

已建立 `ArchendInterface`，并将 x86 指令行解析、前向引用、尺寸计算和编码接入
`ArchendX86`；Assembler 仅通过标准契约驱动架构相关处理。


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

`asm/check.sh` 是自动回归入口，公共支持及 x86、RISC-V、Codeend 测试分别位于
`asm/.check`。入口支持 `all`、`x86`、`riscv`、`elf`、`codeend` 和 `common`
测试组；无参数时仍运行全部回归。

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

已接入最小 RISC-V Archend，通过 `riscv` 目标的 BITS 选择 RV32I/RV64I，并提供
基础整数编码、内存方括号地址语法、单指令简化形式和 Binary 回归。直接分支和跳转
使用裸地址表达式，方括号仅表示间接访问。`[CPU i]` 与
`[INST i]` 使用相同的基础整数指令集选择逻辑。新增 `[ARCH]` 用于切换后续区域的
Archend；`[CPU]`、`[INST]` 与 `[BITS]` 由当前架构共同验证。
已建立通用 `AssemblySignedness` 指令属性。RISC-V 的 `SIGNED` 和 `UNSIGNED`
限定词可以在指令行顶层任意位置切换现有有符号、无符号指令族；`MOV` 默认使用
无符号语义。
RISC-V 原生指令的名称、操作数形式、固定编码、XLEN、扩展要求和符号变体已改由
指令数据表生成；简化形式、表达式解析和编码字段装配继续由 Archend 实现。

ELF32 与 ELF64 已按初始 Archend 写入目标机器类型。RISC-V 可以输出 32 位或 64 位
ELF 基础对象；ELF 会校验 RISC-V 位宽并拒绝中途切换机器架构。

RISC-V ELF 已支持外部符号的分支、直接跳转及 32/64 位绝对数据 relocation。
ELF32 的 RISC-V relocation 已改用 RELA，机器指令初值与 relocation 加数分别保存；
分支和直接跳转也可以通过 section symbol 与 addend 引用其他 section 中的本地标签。
已接入 `CALL`、`CALL_PLT`、绝对 `HI20/LO12`、`GOT_HI20` 和成对的
`PCREL_HI20/PCREL_LO12_I/PCREL_LO12_S` relocation；`-mrelax` 输出 `RELAX` 提示，
RISC-V `align` 输出 `ALIGN` relocation 及最大 NOP padding。
预处理器新增 `__ARCH__` 内建宏，架构切换时更新其值，供通用宏选择架构相关语句。

RISC-V Archend 已支持多指令展开。`MOV` 可以加载完整 RV32/RV64 数值和标签地址；
加载与存储指令可以使用标签或绝对表达式地址，表达式地址存储由第三个操作数显式提供
临时寄存器。ELF 已接入 PC 相对高低位 relocation。

RISC-V 的 `[CPU]` 与 `[INST]` 已实际约束启用的 I、M、A、F、D、C、V 和 Zicsr
指令集。Archend 已接入 M 乘除法、CSR 访问、基础特权指令、带 AQ/RL 限定词的 A
原子指令、浮点寄存器与舍入模式、显式压缩指令和基础向量操作数及编码。

RISC-V 指令数据已按普通指令和向量指令拆分生成输入。浮点、向量和 CSR 使用独立的
类型化操作数模型；C 指令按 2 字节尺寸参与 pass、listing 和 relocation。

已增加 RISC-V ABI 选择及目标描述。ELF32/ELF64 根据 ABI 写入浮点调用约定 flags，
根据 C 扩展写入 RVC flag，并生成包含栈对齐、架构扩展和扩展版本的
`.riscv.attributes`。

listing 状态已收归 `AssemblyListing`，并接入 `AssemblySession`。通用语句、x86
和 RISC-V Archend 通过语义化代码与 fixup 接口提交 listing；`ListGen` 仅保留为
Preprocessor 的兼容适配。


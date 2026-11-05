# 【ZH-CN】AASM 汇编器参考手册

版本 0.1.0

## 工具链位置

AASM 是 UNISYM 的汇编器，当前实现以 x86、x86-64 汇编语法和指令编码为主。
Magice 与 AASM 之间通过汇编文本连接：Magice 的目标后端输出
`.asm`，随后可以交给 GCC AS 或 AASM 生成目标文件或平坦二进制。

```text
Source
  → Magice Frontend
  → Clover IR
  → Magice Backend
  → Assembly
  → GCC AS / AASM
```

## 基本使用

命令格式：

```text
aasm [options] input
```

常用选项：

| 选项 | 含义 |
| :--- | :--- |
| `-o file` | 指定输出文件 |
| `-f format` | 选择输出格式 |
| `-march=name`、`--arch name` | 选择指令架构 |
| `-E`、`-e` | 仅进行预处理 |
| `-a` | 跳过预处理，直接汇编 |
| `-I path` | 增加 include 搜索路径 |
| `-D name=value` | 定义预处理宏 |
| `-U name` | 取消预处理宏定义 |
| `-l file` | 生成 listing 文件 |
| `-g` | 生成所选格式支持的调试信息 |
| `-O level` | 设置分支优化级别 |

未指定 `-march` 时使用 x86 架构，默认汇编位宽为 16 位；源文件可以通过
`[BITS]` 调整当前位宽。

当前输出实现位于 `lib/cpp/Format/Code`，包括 BIN、ELF、COFF、Mach-O、AOUT、
OMF、IEEE 和 AS86 等格式。`AsmTrace.cpp` 提供文本跟踪格式，用于观察汇编器传递
给输出层的 section、symbol、data、relocation 和 debug 事件，不是可链接目标文件。

## 当前结构

AASM 当前代码可以按职责理解为以下部分：

- **Preprocessor**：宏、include、条件预处理和依赖信息，主要位于
  `aasm-prep.cpp`。
- **Assembler**：`aasm.cpp` 负责入口和运行模式分派，`aasm-assembler.cpp` 负责
  架构无关的多遍调度、directive 和 pass 收敛；`aasm-statement.cpp` 负责标签、
  `EQU`、`TIMES`、数据定义、保留空间和 `INCBIN` 等通用汇编语句；符号与表达式
  位于 `aasm-label.cpp` 和 `aasm-eval.cpp`。`AssemblyTarget` 保存一次汇编任务选择的
  Archend、Codeend、默认位宽和 CPU；`AssemblySession` 保存 pass、section、当前
  位宽和当前 CPU 等运行状态。
- **Archend**：`inc/cpp/Lango/Archend.hpp` 定义架构汇编契约；当前 x86 适配层位于
  `lib/cpp/Lango/Archend/Archend-x86.cpp`。Archend 只接收架构指令；x86 内部头和
  生成头位于 `inc/cpp/Lango/Archend/x86`，解析器、编码器、生成源和生成输入位于
  对应的 `lib` 目录。Archend 通过只读信息、注册和查找接口参与目标选择。
- **Codeend**：`inc/cpp/Format/Code.hpp` 定义代码格式输出契约，
  `aasm-format.cpp` 将现有格式函数表接入该契约；BIN、ELF、COFF、Mach-O 等实现
  继续保存在 `lib/cpp/Format/Code`。

Codeend 负责格式信息、格式选择、输出生命周期、section、symbol、directive 和
调试格式操作。普通数据与保留空间通过 `CodeendOutput` 提交；需要重定位的值通过
`CodeFixup` 描述目标 section、加数、PC 偏移语义和编码位域。`aasm-format.cpp` 当前
将语义化 fixup 适配为现有格式函数表使用的输出类型。listing 保留原有接口；
`NativeHandle()` 目前仅保留在 Codeend 适配和旧格式兼容代码中。开始汇编前会检查
所选 Codeend 是否声明支持当前 Archend 的架构种类。

标签操作统一通过 `Labelsys::Define`、`Labelsys::Redefine` 和 `Labelsys::Common`
调用。标签状态管理仍位于 `aasm-label.cpp`，符号和调试符号分别通过 Codeend
契约提交。Archend、Assembler 和表达式求值器不再直接依赖旧格式驱动类型。

`Frontend` 一词保留给 Magice 的源语言前端，不用于命名 AASM 的预处理或汇编
调度部分。`Backend` 表示 Magice 从 Clover IR 生成汇编文本的目标后端；
`Archend` 表示 AASM 中与指令集架构相关的解析和编码部分。

x86 operand、instruction、指令模板、寄存器、opcode、REX/VEX 和 CPU 等级定义集中
在 `inc/cpp/Lango/Archend/x86/Instruction-x86.hpp`。x86 parser、encoder、生成表和
生成输入按 `inc`、`lib` 职责归入对应架构目录；反汇编器与表达式兼容实现显式引用 x86 内部头。CPU、
位宽和默认寻址模式策略由 `ArchendX86` 实现，Assembler 不直接解释 x86 CPU 等级。
x86 指令表只保存架构指令；数据定义、保留空间、`EQU` 和 `INCBIN` 不再作为 x86
伪 opcode。没有实际指令的前缀行由 x86 instruction 状态单独表示。

## 开发约束

新增代码使用 TAB 缩进，方法名采用大写开头的 Unisym 命名风格，代码注释使用
英文。修改旧文件时保持原有行尾，禁止产生混合行尾。新代码可以使用
`enum class`、RAII 和明确所有权等 C++ 机制；旧的 C 风格接口和生成数据按照模块
边界逐步迁移，不通过猜测依赖枚举数值。


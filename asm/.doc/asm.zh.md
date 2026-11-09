# 【ZH-CN】AASM 汇编器参考手册

版本 0.1.0

## 工具链位置

AASM 是 UNISYM 的汇编器，当前支持 x86 指令编码，并已接入最小 RISC-V Archend。
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

未指定 `-march` 或 `--arch` 时，默认目标为 `x86`，默认位宽为 16 位。

## 目标架构

命令行使用 `-march=name`、`-march name`、`--arch=name` 或 `--arch name` 选择
源文件的初始架构。源文件中的 `[ARCH name]` 可以切换后续区域使用的架构。

### Arch、CPU 与 BITS

| Arch | 默认 BITS | 支持的 BITS | 默认 CPU/INST | `[CPU]`、`[INST]` 支持的名称 |
| :--- | ---: | :--- | :--- | :--- |
| `x86` | 16 | 16；386 及以上支持 32；`x64`、`x86-64` 支持 64 | 不限制指令等级 | 见下方 x86 CPU 名称 |
| `riscv` | 64 | 32、64 | `i` | `i` |

x86 当前识别以下 CPU 名称：

```text
8086
186
286
386
486
586, pentium
686, ppro, pentiumpro, p2
p3, katmai
p4, willamette
prescott
x64, x86-64
```

这些名称用于限制可选指令等级。16 位模式接受全部已识别的 x86 CPU；32 位模式要求
CPU 等级不低于 386；64 位模式要求 CPU 为 `x64` 或 `x86-64`。

`[INST value]` 是 `[CPU value]` 的等价写法，两者使用完全相同的 Archend 解析和验证
逻辑。x86 可以继续使用 `[CPU]` 表示处理器指令等级；RISC-V 可以使用 `[INST]`
突出指令集扩展，也可以统一使用 `[CPU]`。

`[ARCH]`、`[CPU]`、`[INST]` 和 `[BITS]` 的规则如下：

- `[ARCH name]` 切换 Archend，并把 CPU 与 BITS 恢复为该 Arch 的默认值。
- `[BITS n]` 必须同时满足当前 Arch、CPU 和 Codeend 的约束。
- `[CPU name]` 与 `[INST name]` 必须同时满足当前 Arch 和当前 BITS 的约束。
- 当前 Codeend 不支持目标 Arch 时，AASM 会在切换处报错。

例如：

```asm
[ARCH riscv]
[BITS 32]
[INST i]
```

Binary 输出可以在同一源文件中包含多个架构区域：

```asm
[ARCH x86]
[BITS 64]
RET

[ARCH riscv]
[BITS 32]
NOP
```

### Codeend 兼容性

| Codeend | x86 | RISC-V |
| :--- | :---: | :---: |
| `bin` | 支持 | 支持 |
| `elf32`、`elf` | 支持 | 支持 32 位基础对象输出 |
| `elf64` | 支持 | 支持 64 位基础对象输出 |
| COFF、Mach-O、AOUT、OMF、IEEE、AS86 等现有格式 | 支持 | 暂不支持 |

ELF 的机器架构由命令行 `-march` 或 `--arch` 固定；ELF 源文件中的 `[ARCH]` 只能
再次选择同一架构，不能在 x86 与 RISC-V 之间切换。RISC-V ELF32 只接受 32 位指令
区域，ELF64 只接受 64 位指令区域。RISC-V ELF 当前支持外部符号的
`R_RISCV_BRANCH`、`R_RISCV_JAL`、`R_RISCV_32` 和 `R_RISCV_64` relocation；
ELF32 与 ELF64 均使用带显式加数的 RELA section。分支和直接跳转可以引用外部
符号，也可以引用其他 section 中的本地标签。

x86/RISC-V 混合架构源文件必须使用 Binary 输出。文本跟踪格式只用于观察汇编器提交的
section、symbol、data、relocation 和 debug 事件，不是可链接目标文件。

## 通用汇编语法

标签、表达式、`EQU`、`TIMES`、`INCBIN`、数据定义和保留空间由通用汇编层处理，
不属于某个 Archend。数据定义支持 `DB`、`DW`、`DD`、`DQ`、`DT`、`DO`、`DY`；
保留空间支持 `RESB`、`RESW`、`RESD`、`RESQ`、`REST`、`RESO`、`RESY`。

固定数据宽度限定符不随 Arch、CPU 或 BITS 改变：

| 限定符 | 固定宽度 |
| :--- | ---: |
| `BYTE` | 1 字节 |
| `BYT2` | 2 字节 |
| `BYT4` | 4 字节 |
| `BYT8` | 8 字节 |
| `BYT10` | 10 字节 |
| `BYT16` | 16 字节 |
| `BYT32` | 32 字节 |

## RISC-V 汇编语法

当前 RISC-V Archend 提供 RV32I 和 RV64I 基础整数指令编码。完整的寄存器、操作数、
原生指令及单指令简化形式见 `inst-riscv.zh.md`。

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
- **Listing**：`aasm-list.cpp` 的 `AssemblyListing` 保存一次 listing 输出的文件、
  行、机器码、嵌套层级和抑制状态。通用语句与 Archend 通过 `CodeendOutput`、
  `CodeFixup` 语义接口提交 listing 事件；旧 `ListGen` 只作为 Preprocessor 适配接口。
- **Archend**：`inc/cpp/Lango/Archend.hpp` 定义架构汇编契约；当前 x86 适配层位于
  `lib/cpp/Lango/Archend/Archend-x86.cpp`，RISC-V 适配层位于对应的
  `Archend-riscv.cpp`。Archend 只接收架构指令；x86 内部头和
  生成头位于 `inc/cpp/Lango/Archend/x86`，解析器、编码器、生成源和生成输入位于
  对应的 `lib` 目录。Archend 通过只读信息、注册和查找接口参与目标选择。
- **Codeend**：`inc/cpp/Format/Code.hpp` 定义代码格式输出契约，
  `aasm-format.cpp` 将现有格式函数表接入该契约；BIN、ELF、COFF、Mach-O 等实现
  继续保存在 `lib/cpp/Format/Code`。

Codeend 负责格式信息、格式选择、输出生命周期、section、symbol、directive 和
调试格式操作。普通数据与保留空间通过 `CodeendOutput` 提交；需要重定位的值通过
`CodeFixup` 描述目标 section、加数、PC 偏移语义和编码位域。`aasm-format.cpp` 当前
将语义化 fixup 适配为现有格式函数表使用的输出类型。listing 状态由
`AssemblyListing` 管理；
`NativeHandle()` 目前仅保留在 Codeend 适配和旧格式兼容代码中。开始汇编前会检查
所选 Codeend 是否声明支持当前 Archend 的架构种类。

Binary 输出回归覆盖原始数据、保留空间、`ORG`、section 对齐，以及本地跨 section
的绝对地址、PC 相对地址、分支和调用。外部未解析符号不会生成平坦二进制文件。

标签初始化、查询、作用域、外部符号判断、全局声明和定义操作统一通过 `Labelsys`
调用。标签状态管理仍位于 `aasm-label.cpp`，符号和调试符号分别通过 Codeend
契约提交。接收 `outffmt` 的 `Labelsys::Define` 重载仅用于旧格式驱动兼容。

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


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
| `-E`、`-e` | 仅进行预处理 |
| `-a` | 跳过预处理，直接汇编 |
| `-I path` | 增加 include 搜索路径 |
| `-D name=value` | 定义预处理宏 |
| `-U name` | 取消预处理宏定义 |
| `-l file` | 生成 listing 文件 |
| `-g` | 生成所选格式支持的调试信息 |
| `-O level` | 设置分支优化级别 |

当前输出实现位于 `lib/cpp/Format/Code`，包括 BIN、ELF、COFF、Mach-O、AOUT、
OMF、IEEE 和 AS86 等格式。`AsmTrace.cpp` 提供文本跟踪格式，用于观察汇编器传递
给输出层的 section、symbol、data、relocation 和 debug 事件，不是可链接目标文件。

## 当前结构

AASM 当前代码可以按职责理解为以下部分：

- **Preprocessor**：宏、include、条件预处理和依赖信息，主要位于
  `aasm-prep.cpp`。
- **Assembler**：`aasm.cpp` 负责入口和运行模式分派，`aasm-assembler.cpp` 负责
  架构无关的多遍调度、directive 和 pass 收敛；符号与表达式位于
  `aasm-label.cpp` 和 `aasm-eval.cpp`。
- **Archend**：`inc/cpp/Lango/Archend.hpp` 定义架构汇编契约；当前 x86 适配层位于
  `lib/cpp/Lango/Archend/Archend-x86.cpp`，内部使用 `aasm-parse.cpp`、
  `aasm-gena.cpp` 和 `asm/data` 完成语法解析、前向引用、指令匹配与机器码编码。
- **Codeend**：目标文件和代码格式输出，对应 `lib/cpp/Format/Code` 中的现有实现。

`Frontend` 一词保留给 Magice 的源语言前端，不用于命名 AASM 的预处理或汇编
调度部分。`Backend` 表示 Magice 从 Clover IR 生成汇编文本的目标后端；
`Archend` 表示 AASM 中与指令集架构相关的解析和编码部分。

## 开发约束

新增代码使用 TAB 缩进，方法名采用大写开头的 Unisym 命名风格，代码注释使用
英文。修改旧文件时保持原有行尾，禁止产生混合行尾。新代码可以使用
`enum class`、RAII 和明确所有权等 C++ 机制；旧的 C 风格接口和生成数据按照模块
边界逐步迁移，不通过猜测依赖枚举数值。


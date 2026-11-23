# 【ZH-CN】AASM ARM32 指令语法

本文定义 `arm32` Archend 的指令语法。A32 支持 ARMv7-A 最小整数、控制流和加载存储
子集，可以输出 Binary 或 ELF32 对象。Cortex-M7 的 T32 目前支持一组 16 位整数、访存
和分支编码，只能输出 Binary。

## 目标模式

未选择 Core 或指令集时，`arm32` 默认使用 A32。Cortex-M7 使用 T32，可按以下方式选择：

```asm
[ARCH arm32]
[CORE cortex-m7]
[INST T32]
[BITS 32]
```

`CORE` 与 `INST` 的名称均不区分大小写；例如 `cortex-m7`、`CORTEX-M7`、`T32` 和
`t32` 等价。`CORE cortex-m7` 默认选择 T32；`INST T32` 可明确写出当前指令状态。
`BITS 32` 表示通用寄存器和数据位宽，不代表每条指令为 32 位。T32 指令可采用 16 位
或 32 位编码；当前已接入的 T32 指令形式均为 16 位编码。

T32 当前仅支持 Binary 输出。A32 的默认目标、语法和 ELF 输出不受此限制。

指令和寄存器名称不区分大小写。

## 寄存器

通用寄存器写成 `R0` 至 `R15`，并接受以下专用名称：

| 名称 | 对应寄存器 |
| :--- | :--- |
| `SP` | `R13` |
| `LR` | `R14` |
| `PC` | `R15` |

`SP` 和 `LR` 可以用于当前支持的普通寄存器位置。当前最小子集只允许 `PC` 作为
寄存器名称被识别，不允许它作为数据处理、访存或寄存器跳转操作数。

## 立即数、标签和内存

立即数直接书写，不使用 `#`。控制流的裸标签表示目标地址；方括号只表示内存访问。
当前内存操作只接受寄存器基址和立即数偏移。

```asm
ADD R0, R1, 16
LDR R0, [R1 + 8]
B target
BX R2
```

以下形式不属于 ARM32 语法：

```asm
ADD R0, R1, #16
B [target]
BX [R2]
LDR R0, message
```

基础内存寻址使用以下形式：

```asm
[R1]
[R1 + 16]
[R1 - 16]
```

`LDR`、`STR`、`LDRB` 和 `STRB` 的偏移范围为 -4095 至 4095；半字和符号加载的
偏移范围为 -255 至 255。pre-index、post-index、写回、寄存器偏移和表达式地址当前
不属于本语法范围。

## 条件与标志

条件码使用点限定，不采用与助记符直接拼接的形式：

```asm
ADD.EQ R0, R1, R2
SUB.NE R0, R0, 1
B.LT target
```

需要更新条件标志时增加 `.S`，限定顺序固定为
`MNEMONIC[.condition][.S]`：

```asm
ADD.S R0, R1, R2
ADD.EQ.S R0, R1, R2
```

不接受 `ADDEQ`、`SUBNES` 等拼接形式。

`CMP` 固定更新条件标志，因此不接受额外的 `.S`。分支、访存和 `NOP` 也不接受
`.S`。条件码可以用于当前支持的原生指令及 `JMP`、`CALL`、`RET` 简化形式。

## 移位操作数

寄存器移位作为独立限定部分书写：

```asm
ADD R0, R1, R2, LSL 3
MOV R0, R1, ROR 8
```

可用移位名称为 `LSL`、`LSR`、`ASR` 和 `ROR`，不使用 `R2 << 3` 等表达式语法。

## 语义化 MOV

寄存器复制和完整绝对立即数使用统一的 `MOV`：

```asm
MOV R0, R1
MOV R0, 0x12345678
```

立即数 `MOV` 优先使用单条 `MOV` 或 `MVN`，其次使用 `MOVW`，需要时展开为
`MOVW + MOVT`。裸标签和标签加数表示地址，并使用 `MOVW + MOVT`：

```asm
MOV R0, message
MOV R1, message + 8
```

寄存器 `MOV` 可以使用 `.S`；立即数和标签地址 `MOV` 不接受 `.S`，以避免展开后
产生不一致的标志语义。

内存 `MOV` 必须显式指定固定宽度：

```asm
MOV R0, BYTE [R1]
MOV R0, SIGNED BYTE [R1]
MOV R0, BYT2 [R1 + 2]
MOV BYTE [R1], R0
MOV BYT4 [R1 + 4], R0
```

加载默认采用无符号扩展；`SIGNED` 与 `UNSIGNED` 可以显式选择加载属性。BYT4 已
占满 ARM32 通用寄存器，不需要额外扩展。存储接受限定词，但机器码不因符号属性改变。

## 原生加载与存储

原生加载和存储助记符仍使用方括号表示内存：

```asm
LDR R0, [R1]
LDRB R0, [R1 + 1]
LDRSB R0, [R1]
STR R0, [R1 + 4]
STRH R0, [R1]
```

当前不支持 literal load、表达式地址或寄存器偏移地址。

## 当前原生指令

当前支持：

```text
NOP, B, BL, BX, BLX
ADD, SUB, RSB, ADC, SBC, CMP, TST, TEQ, MUL
AND, ORR, EOR
LDR, STR, LDRB, STRB, LDRH, STRH, LDRSB, LDRSH
```

数据处理指令的 Operand2 支持 ARM modified immediate、普通寄存器和立即数移位
寄存器。当前不支持寄存器指定的移位量。

`RSB`、`ADC`、`SBC` 支持与 `ADD`、`SUB` 相同的 Operand2 形式；`TST` 和 `TEQ`
接受一个寄存器及一个 Operand2，并更新条件标志；`MUL` 接受三个寄存器，顺序为
`MUL Rd, Rm, Rs`，也支持 `.S` 更新条件标志。

## Cortex-M7 T32 指令

T32 通用寄存器为 32 位 `R0`–`R12`，另有 `SP`/`R13`、`LR`/`R14` 和 `PC`/`R15`。
别名与编号不区分大小写。不同 16 位指令对可用寄存器有不同限制；目前支持的窄编码
只接受其编码允许的寄存器，超出范围会报错。浮点寄存器和寄存器列表尚未接入。

当前已接入的 16 位形式：

| 指令 | 支持形式 |
| :--- | :--- |
| `MOVS` | `MOVS Rd, imm8`、`MOVS Rd, Rm` |
| `MOV` | `MOV Rd, Rm`，寄存器复制且不更新标志 |
| `ADDS`、`SUBS` | `Rd, Rn, Rm`；或 `Rd, Rd, imm8` |
| `CMP` | `CMP Rn, Rm`、`CMP Rn, imm8` |
| `ORRS` | `ORRS Rd, Rd, Rm` |
| `LDR`、`STR` | 低寄存器、基址寄存器及 0–124 的 4 字节对齐正偏移 |
| `B`、条件分支 | 16 位短分支；条件分支可写标准助记符 `BEQ`、`BNE`、`BHS` 等 |
| `BX` | `BX Rm` |
| `NOP` | 无操作数 |

条件分支的目标使用裸标签，例如 `BHS ready`。当前 T32 分支超出 16 位短分支范围时
会报错，不会自动扩展为 32 位分支。尚未支持 `.N`/`.W` 宽度限定，也不会为当前窄
编码以外的操作数自动选择 32 位形式。

```asm
[ARCH arm32]
[CORE Cortex-M7]
[INST t32]
[BITS 32]

MOVS R0, 1
ADDS R0, R0, 1
STR R0, [R1 + 4]
BHS ready
ready:
BX LR
```

## 控制流简化形式

| AASM 形式 | 对应 ARM32 形式 |
| :--- | :--- |
| `JMP target` | `B target` |
| `JMP register` | `BX register` |
| `CALL target` | `BL target` |
| `CALL register` | `BLX register` |
| `RET` | `BX LR` |
| `NOP` | `NOP` |

控制流目标始终使用裸标签或裸表达式，不使用方括号。
Binary 输出可以跳转或调用其他本地 section 中的标签；未解析外部符号不能写入平坦
二进制。超出 A32 直接分支范围时会报错，不会自动生成远跳转序列。

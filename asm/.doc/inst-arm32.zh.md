# 【ZH-CN】AASM ARM32 指令语法

本文定义 `arm32` Archend 的指令语法。当前接入 ARMv7-A A32 最小整数、控制流和
加载存储子集，输出格式限于 Binary。

## 目标模式

`arm32` 只表示 A32 指令状态，固定使用 `[BITS 32]`，每条机器指令为 4 字节。不支持
Thumb/T32，也不支持在源文件中切换 A32/T32 状态。当前以支持 `MOVW/MOVT` 的
ARMv7-A A32 为编码基线，不接受 `[CPU]` 或 `[INST]`。

```asm
[ARCH arm32]
[BITS 32]
```

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
ADD, SUB, CMP
AND, ORR, EOR
LDR, STR, LDRB, STRB, LDRH, STRH, LDRSB, LDRSH
```

数据处理指令的 Operand2 支持 ARM modified immediate、普通寄存器和立即数移位
寄存器。当前不支持寄存器指定的移位量。

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

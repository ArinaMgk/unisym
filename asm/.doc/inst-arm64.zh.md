# 【ZH-CN】AASM ARM64 指令语法

本文定义 `arm64` Archend 的指令语法。当前接入 A64 最小整数、控制流和加载存储
子集，可以输出 Binary 或 ELF64 对象。

## 目标模式

`arm64` 表示 A64 指令状态，固定使用 `[BITS 64]`，每条机器指令为 4 字节。`Wn`
和 `Xn` 是 32 位与 64 位寄存器视图，不通过 `[BITS]` 切换。当前不接受 `[CPU]`
或 `[INST]`。

```asm
[ARCH arm64]
[BITS 64]
```

指令和寄存器名称不区分大小写。

## 寄存器

接受以下整数寄存器名称：

```text
X0-X30
W0-W30
SP, WSP
XZR, WZR
FP = X29
LR = X30
```

不接受 `X31`、`W31` 或 `R0-R30`。编码字段 31 的具体含义由指令决定，源代码必须
显式写出 `SP`、`WSP`、`XZR` 或 `WZR`。这些专用寄存器是否可用也由具体指令验证。

寄存器复制不进行隐式宽度转换：

```asm
MOV X0, X1
MOV W0, W1
```

`MOV X0, W1` 和 `MOV W0, X1` 均无效。写入 `Wn` 后，对应 `Xn` 的高 32 位为零。

## 立即数、标签和内存

立即数直接书写，不使用 `#`。控制流的裸标签表示目标地址；方括号只表示内存访问。
当前内存操作接受 64 位寄存器基址、立即数偏移、寄存器偏移及写回寻址。

```asm
ADD X0, X1, 16
LDR X0, [X1 + 8]
LDR X0, [X1 - 8]
LDR X0, [X1 + X2, LSL 3]
STR X29, [SP - 16]!
LDR X0, [SP], 8
STP X29, X30, [SP - 16]!
LDP X29, X30, [SP], 16
B target
BR X2
```

以下形式不属于 ARM64 语法：

```asm
ADD X0, X1, #16
B [target]
BR [X2]
LDR X0, message
```

偏移和写回寻址使用以下形式：

```asm
[X1]
[X1 + 16]
[X1 - 8]
[X1 + 8]!
[X1], 8
[X1 + X2]
[X1 + X2, LSL 3]
[X1 + W2, UXTW 3]
[X1 + W2, SXTW 3]
```

内存基址必须是 `Xn` 或 `SP`，不能是 `Wn` 或 `WSP`。普通 `LDR/STR` 对非负、
按访问宽度对齐的偏移使用 scaled 12 位编码；其他 `-256..255` 的偏移使用 unscaled
编码。`LDUR`、`STUR` 及其字节、半字和符号加载形式显式选择 unscaled 编码，偏移
范围为 `-256..255`。pre-index 和 post-index 的偏移为有符号 9 位字节偏移，范围同为
`-256..255`。

寄存器偏移写成 `[base + index, modifier shift]`。`X` 索引允许省略修饰符或使用
`LSL`、`SXTX`；`W` 索引必须使用 `UXTW` 或 `SXTW`。shift 只能为 0 或当前访存宽度
对应的移位量。寄存器偏移不支持减法、写回或额外地址表达式。

`LDP` 和 `STP` 支持两个同宽的 `W` 或 `X` 寄存器，以及 offset、pre-index、
post-index 三种立即数寻址。偏移按寄存器宽度缩放：W 寄存器范围为 `-256..252`，
X 寄存器范围为 `-512..504`；必须按对应宽度对齐。成对访存不支持寄存器偏移。
写回基址不能与数据寄存器重叠，`LDP` 的两个目标寄存器也必须不同。

literal load、标签或绝对地址内存、成对访存的寄存器偏移当前不支持。

## 条件分支

条件分支使用点限定：

```asm
B.EQ target
B.NE target
B.LT target
B.GE target
```

控制流目标使用裸标签或裸表达式，不使用方括号。其他使用条件码的指令将条件作为该
指令定义的独立操作数处理，不把条件码拼接到助记符中。

## 语义化 MOV

寄存器复制和完整绝对立即数使用统一的 `MOV`：

```asm
MOV X0, X1
MOV W0, W1
MOV X0, 0x123456789ABCDEF0
```

立即数 `MOV` 使用 `MOVZ`、`MOVN` 和 `MOVK` 的较短组合，W 寄存器最多展开为两条，
X 寄存器最多展开为四条。裸标签和标签加数表示地址，并使用 `ADRP + ADD`：

```asm
MOV X0, message
MOV X1, message + 8
```

标签地址必须写入 X 通用寄存器，不能写入 W 寄存器、SP 或零寄存器。

内存 `MOV` 必须显式指定固定宽度：

```asm
MOV X0, BYTE [X1]
MOV X0, SIGNED BYTE [X1]
MOV X0, BYT2 [X1 + 2]
MOV X0, BYT8 [X1 + 8]
MOV BYTE [X1], W0
MOV BYT4 [X1 + 4], W0
MOV BYT8 [X1 + 8], X0
```

加载默认采用无符号扩展。`SIGNED BYTE`、`SIGNED BYT2` 和 `SIGNED BYT4` 分别
请求符号扩展；BYT8 已占满 64 位寄存器。目标为 `Wn` 时不能加载 BYT8。存储接受
限定词，但机器码不因符号属性改变。

## 原生加载与存储

原生加载和存储助记符仍使用方括号表示内存：

```asm
LDR X0, [X1]
LDR W0, [X1 + 4]
LDUR X0, [X1 - 8]
LDRB W0, [X1]
LDRSB X0, [X1]
LDRSW X0, [X1]
STR X0, [X1 + 8]
STRH W0, [X1]
STP X29, X30, [SP - 16]!
LDP X29, X30, [SP], 16
```

当前不支持 literal load、表达式地址、`ADR` 或 `ADRP`。普通 `LDR/STR` 根据偏移范围、
对齐和寻址模式选择 scaled 或 unscaled 编码。

## 当前原生指令

当前支持：

```text
NOP, RET, BR, BLR, B, BL
ADD, ADDS, SUB, SUBS, ADC, SBC, CMP, TST
AND, ORR, EOR, MUL, SDIV, UDIV, LSL, LSR, ASR, ROR
LDR, STR, LDUR, STUR, LDRB, LDRH, LDRSB, LDRSH, LDRSW,
LDURB, LDURH, LDURSB, LDURSH, LDURSW, STRB, STRH, STURB, STURH, LDP, STP
```

`ADD`、`ADDS`、`SUB`、`SUBS` 和 `CMP` 支持寄存器形式，以及 12 位立即数和左移
12 位的立即数形式。`ADC`、`SBC`、`TST`、`MUL`、`SDIV`、`UDIV` 以及 `LSL`、`LSR`、
`ASR`、`ROR` 的寄存器移位形式均接受三个同宽寄存器操作数。立即数移位形式和逻辑
立即数尚未接入。

## 控制流简化形式

| AASM 形式 | 对应 ARM64 形式 |
| :--- | :--- |
| `JMP target` | `B target` |
| `JMP register` | `BR register` |
| `CALL target` | `BL target` |
| `CALL register` | `BLR register` |
| `RET` | `RET` |
| `NOP` | `NOP` |

`BR`、`BLR`、`RET` 及相应简化形式使用寄存器中的目标地址，不访问该地址处的内存，
因此不接受方括号。
Binary 输出可以跳转或调用其他本地 section 中的标签；未解析外部符号不能写入平坦
二进制。超出 A64 对应分支形式的范围时会报错，不会自动生成远跳转序列。

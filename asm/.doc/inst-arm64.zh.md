# 【ZH-CN】AASM ARM64 指令语法

本文定义 `arm64` Archend 的指令语法。当前接入 A64 最小整数、控制流和加载存储
子集，输出格式限于 Binary。

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
当前内存操作只接受 64 位寄存器基址和非负的 scaled 12 位偏移。

```asm
ADD X0, X1, 16
LDR X0, [X1 + 8]
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

基础内存寻址使用以下形式：

```asm
[X1]
[X1 + 16]
```

内存地址的寄存器基址必须是 `Xn` 或 `SP`，不能是 `Wn` 或 `WSP`。pre-index、
post-index、写回、scaled register offset 和 extend register offset 当前不属于本语法
范围。负偏移、标签地址和绝对地址内存也尚未接入。

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
LDRB W0, [X1]
LDRSB X0, [X1]
LDRSW X0, [X1]
STR X0, [X1 + 8]
STRH W0, [X1]
```

当前不支持 literal load、表达式地址、`ADR` 或 `ADRP`。

## 当前原生指令

当前支持：

```text
NOP, RET, BR, BLR, B, BL
ADD, SUB, CMP
AND, ORR, EOR
LDR, STR, LDRB, LDRH, LDRSB, LDRSH, LDRSW, STRB, STRH
```

`ADD`、`SUB` 和 `CMP` 支持寄存器形式，以及 12 位立即数和左移 12 位的立即数形式。
寄存器移位、扩展寄存器和逻辑立即数尚未接入。

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

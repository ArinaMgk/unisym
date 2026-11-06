# 【ZH-CN】AASM RISC-V 指令参考

本文记录 AASM 当前 RISC-V Archend 接受的指令语法，不包含 `[ARCH]`、`[BITS]`、
`[CPU]`、`[INST]`、`[SECTION]` 等 directive。

## 操作数

指令和寄存器名称不区分大小写。通用寄存器可以写成 `x0` 至 `x31`，也可以使用
以下 ABI 名称：

| 寄存器 | ABI 名称 | 寄存器 | ABI 名称 |
| :--- | :--- | :--- | :--- |
| `x0` | `zero` | `x16` | `a6` |
| `x1` | `ra` | `x17` | `a7` |
| `x2` | `sp` | `x18` | `s2` |
| `x3` | `gp` | `x19` | `s3` |
| `x4` | `tp` | `x20` | `s4` |
| `x5` | `t0` | `x21` | `s5` |
| `x6` | `t1` | `x22` | `s6` |
| `x7` | `t2` | `x23` | `s7` |
| `x8` | `s0`、`fp` | `x24` | `s8` |
| `x9` | `s1` | `x25` | `s9` |
| `x10` | `a0` | `x26` | `s10` |
| `x11` | `a1` | `x27` | `s11` |
| `x12` | `a2` | `x28` | `t3` |
| `x13` | `a3` | `x29` | `t4` |
| `x14` | `a4` | `x30` | `t5` |
| `x15` | `a5` | `x31` | `t6` |

`imm` 表示立即数表达式。需要 I 型立即数的指令接受 12 位有符号值。移位立即数
在 RV32 中为 `0..31`，在 RV64 中为 `0..63`；字运算移位量为 `0..31`。

`SIGNED` 和 `UNSIGNED` 是指令语义限定词。未指定时保持助记符的标准语义；指定后，
Archend 将指令切换到同一指令族的有符号或无符号形式。限定词可以写在指令行顶层
任意 token 位置，但一条指令只能出现一次，且不能写在内存方括号内部。例如：

```asm
UNSIGNED SLT x5, x6, x7
SLT x5, UNSIGNED x6, x7
SLTU SIGNED x5, x6, x7
```

以上三条分别生成 `SLTU`、`SLTU` 和 `SLT`。当前支持切换的原生指令族为
`SLT/SLTU`、`SLTI/SLTIU`、`BLT/BLTU`、`BGE/BGEU`、`LB/LBU`、`LH/LHU` 和
`LW/LWU`。RV32 的 `LW` 结果已经占满寄存器宽度，因此 `LW UNSIGNED` 仍生成
`LW`。不具有符号变体的指令使用限定词会报错。

内存操作数使用方括号：

```asm
[base]
[base + offset]
[base - offset]
```

`base` 必须是通用寄存器，`offset` 必须符合 12 位有符号立即数范围。裸标签或裸
表达式表示其数值；方括号表示访问该地址指向的内存。例如：

```asm
J  target          ; jump to target
LW x5, [x6 + 12]   ; read memory at x6 + 12
```

## 上位立即数

| 指令 | 语法 |
| :--- | :--- |
| `LUI` | `LUI rd, imm20` |
| `AUIPC` | `AUIPC rd, imm20` |

`imm20` 是写入指令高 20 位的立即数字段。

## 跳转与分支

| 指令 | 语法 |
| :--- | :--- |
| `JAL` | `JAL target` 或 `JAL rd, target` |
| `JALR` | `JALR rd, base` 或 `JALR rd, base + offset` |
| `BEQ` | `BEQ rs1, rs2, target` |
| `BNE` | `BNE rs1, rs2, target` |
| `BLT` | `BLT rs1, rs2, target` |
| `BGE` | `BGE rs1, rs2, target` |
| `BLTU` | `BLTU rs1, rs2, target` |
| `BGEU` | `BGEU rs1, rs2, target` |

`target` 是裸地址表达式，不使用方括号。当前 Binary 输出要求目标能够在当前 section
内解析；分支和跳转目标还必须满足相应编码范围及 2 字节对齐要求。

单操作数 `JAL target` 将返回地址写入 `ra`。`JALR` 计算 `base + offset` 后跳转，
不会访问该地址处的内存，因此不接受 `JALR rd, [base + offset]`。

## 加载与存储

| 指令 | BITS | 语法 |
| :--- | :---: | :--- |
| `LB` | 32、64 | `LB rd, [base + offset]` |
| `LH` | 32、64 | `LH rd, [base + offset]` |
| `LW` | 32、64 | `LW rd, [base + offset]` |
| `LBU` | 32、64 | `LBU rd, [base + offset]` |
| `LHU` | 32、64 | `LHU rd, [base + offset]` |
| `LWU` | 64 | `LWU rd, [base + offset]` |
| `LD` | 64 | `LD rd, [base + offset]` |
| `SB` | 32、64 | `SB rs, [base + offset]` |
| `SH` | 32、64 | `SH rs, [base + offset]` |
| `SW` | 32、64 | `SW rs, [base + offset]` |
| `SD` | 64 | `SD rs, [base + offset]` |

偏移为零时可以省略 `+ offset`。

## 立即数运算

以下指令均支持完整形式 `OP rd, rs1, imm` 和原地简化形式 `OP rd, imm`。简化形式
等价于 `OP rd, rd, imm`。

| 指令 | BITS | 完整语法 |
| :--- | :---: | :--- |
| `ADDI` | 32、64 | `ADDI rd, rs1, imm` |
| `SLTI` | 32、64 | `SLTI rd, rs1, imm` |
| `SLTIU` | 32、64 | `SLTIU rd, rs1, imm` |
| `XORI` | 32、64 | `XORI rd, rs1, imm` |
| `ORI` | 32、64 | `ORI rd, rs1, imm` |
| `ANDI` | 32、64 | `ANDI rd, rs1, imm` |
| `SLLI` | 32、64 | `SLLI rd, rs1, shamt` |
| `SRLI` | 32、64 | `SRLI rd, rs1, shamt` |
| `SRAI` | 32、64 | `SRAI rd, rs1, shamt` |
| `ADDIW` | 64 | `ADDIW rd, rs1, imm` |
| `SLLIW` | 64 | `SLLIW rd, rs1, shamt` |
| `SRLIW` | 64 | `SRLIW rd, rs1, shamt` |
| `SRAIW` | 64 | `SRAIW rd, rs1, shamt` |

例如：

```asm
ADDI x5, x6, 4
ADDI x5, 4       ; ADDI x5, x5, 4
```

## 寄存器运算

以下指令均支持完整形式 `OP rd, rs1, rs2` 和原地简化形式 `OP rd, rs2`。简化形式
等价于 `OP rd, rd, rs2`。

| 指令 | BITS | 完整语法 |
| :--- | :---: | :--- |
| `ADD` | 32、64 | `ADD rd, rs1, rs2` |
| `SUB` | 32、64 | `SUB rd, rs1, rs2` |
| `SLL` | 32、64 | `SLL rd, rs1, rs2` |
| `SLT` | 32、64 | `SLT rd, rs1, rs2` |
| `SLTU` | 32、64 | `SLTU rd, rs1, rs2` |
| `XOR` | 32、64 | `XOR rd, rs1, rs2` |
| `SRL` | 32、64 | `SRL rd, rs1, rs2` |
| `SRA` | 32、64 | `SRA rd, rs1, rs2` |
| `OR` | 32、64 | `OR rd, rs1, rs2` |
| `AND` | 32、64 | `AND rd, rs1, rs2` |
| `ADDW` | 64 | `ADDW rd, rs1, rs2` |
| `SUBW` | 64 | `SUBW rd, rs1, rs2` |
| `SLLW` | 64 | `SLLW rd, rs1, rs2` |
| `SRLW` | 64 | `SRLW rd, rs1, rs2` |
| `SRAW` | 64 | `SRAW rd, rs1, rs2` |

## 系统指令

| 指令 | 语法 |
| :--- | :--- |
| `FENCE` | `FENCE` |
| `ECALL` | `ECALL` |
| `EBREAK` | `EBREAK` |

当前 `FENCE` 不接受 predecessor、successor 或 `fm` 操作数，固定生成完整内存屏障。

## 单指令简化形式

这些形式只展开为一条机器指令。

| 指令 | 语法 | 等价形式 |
| :--- | :--- | :--- |
| `NOP` | `NOP` | `ADDI zero, zero, 0` |
| `RET` | `RET` | `JALR zero, ra` |
| `J` | `J target` | `JAL zero, target` |
| `JR` | `JR base` 或 `JR base + offset` | `JALR zero, base + offset` |
| `MOV` | `MOV rd, rs` | `ADDI rd, rs, 0` |
| `MOV` | `MOV rd, width [base + offset]` | 对应的无符号加载指令 |
| `MOV` | `MOV rd, SIGNED width [base + offset]` | 对应的有符号加载指令 |
| `MOV` | `MOV rd, UNSIGNED width [base + offset]` | 显式无符号加载 |
| `MOV` | `MOV width [base + offset], rs` | 对应的存储指令 |

内存 `MOV` 必须显式写固定宽度：

| 宽度 | 默认/`UNSIGNED` | `SIGNED` | 存储 | 可用模式 |
| :--- | :--- | :--- | :--- | :---: |
| `BYTE` | `LBU` | `LB` | `SB` | RV32、RV64 |
| `BYT2` | `LHU` | `LH` | `SH` | RV32、RV64 |
| `BYT4` | RV32 使用 `LW`；RV64 使用 `LWU` | `LW` | `SW` | RV32、RV64 |
| `BYT8` | `LD` | `LD` | `SD` | RV64 |

`MOV` 默认采用无符号语义。内存加载根据限定词选择扩展方式；寄存器复制和内存存储
也接受限定词，但其机器码不因符号属性改变。限定词位置不受上表写法限制。

## 当前范围

当前实现覆盖 RV32I 和 RV64I 中上述指令。尚未实现 M、A、F、D、C、CSR 等扩展，
也未实现需要展开为多条机器指令的取址、加载常量和远距离控制流简化形式。

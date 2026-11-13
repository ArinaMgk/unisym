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

浮点寄存器可以写成 `f0` 至 `f31`，也可以使用 ABI 名称 `ft0..ft11`、`fs0..fs11`
和 `fa0..fa7`。向量寄存器写成 `v0` 至 `v31`；掩码操作数固定写成 `v0.t`。

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
`LW/LWU`、`DIV/DIVU`、`REM/REMU`、`DIVW/DIVUW`、`REMW/REMUW`、
`AMOMIN/AMOMINU`、`AMOMAX/AMOMAXU`、`VDIV.VV/VDIVU.VV` 和
`VREM.VV/VREMU.VV`。RV32 的 `LW` 结果已经占满寄存器宽度，
因此 `LW UNSIGNED` 仍生成 `LW`。不具有符号变体的指令使用限定词会报错。

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
| `JAL` | `JAL target` 或 `JAL rd, target [, temporary]` |
| `JALR` | `JALR rd, base` 或 `JALR rd, base + offset` |
| `BEQ` | `BEQ rs1, rs2, target [, temporary]` |
| `BNE` | `BNE rs1, rs2, target [, temporary]` |
| `BLT` | `BLT rs1, rs2, target [, temporary]` |
| `BGE` | `BGE rs1, rs2, target [, temporary]` |
| `BLTU` | `BLTU rs1, rs2, target [, temporary]` |
| `BGEU` | `BGEU rs1, rs2, target [, temporary]` |

`target` 是裸地址表达式，不使用方括号，并且必须满足 2 字节对齐要求。Binary 输出
要求目标能够在当前 section 内解析；ELF 可以通过 relocation 引用外部符号或其他
section。

单操作数 `JAL target` 将返回地址写入 `ra`。`JALR` 计算 `base + offset` 后跳转，
不会访问该地址处的内存，因此不接受 `JALR rd, [base + offset]`。

当目标超出直接编码范围时，Archend 按距离扩大指令序列：

- `JAL` 从 4 字节扩大为 `AUIPC + JALR` 的 8 字节序列。`rd` 非 `zero` 时可以同时
  保存中间地址；`J` 或 `JAL zero, target` 必须显式提供 `temporary`。
- 条件分支先扩大为“反向条件分支 + JAL”的 8 字节序列；目标继续超出 `JAL` 范围时，
  扩大为“反向条件分支 + AUIPC + JALR”的 12 字节序列，并要求 `temporary`。
- PC 相对长序列覆盖有符号 32 位距离；超出该范围仍会报错。

显式临时寄存器也可以让 ELF 的外部或跨 section 目标直接生成成对的
`PCREL_HI20/PCREL_LO12_I` relocation：

```asm
J   external_target, t0
JAL ra, external_target, t0
BEQ a0, a1, other_section_target, t0
```

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

## M 扩展

使用 M 指令前必须选择含 `m` 的指令集，例如 `[INST im]` 或 `[INST ima_zicsr]`。
以下指令均支持完整三寄存器形式和省略 `rs1` 的原地形式。

| 指令 | BITS | 语法 |
| :--- | :---: | :--- |
| `MUL`、`MULH`、`MULHSU`、`MULHU` | 32、64 | `OP rd, rs1, rs2` |
| `DIV`、`DIVU`、`REM`、`REMU` | 32、64 | `OP rd, rs1, rs2` |
| `MULW`、`DIVW`、`DIVUW`、`REMW`、`REMUW` | 64 | `OP rd, rs1, rs2` |

`DIV`、`REM` 及其 W 形式可以通过 `SIGNED`、`UNSIGNED` 切换有符号属性。
`MULH`、`MULHSU` 和 `MULHU` 分别保留各自明确的操作数符号组合，不接受符号限定词。

## Zicsr 与 CSR

CSR 访问指令要求指令集包含 `_zicsr`：

| 指令 | 语法 |
| :--- | :--- |
| `CSRRW`、`CSRRS`、`CSRRC` | `OP rd, csr, rs` |
| `CSRRWI`、`CSRRSI`、`CSRRCI` | `OP rd, csr, uimm5` |

`csr` 可以是 `0..0xFFF` 的绝对编号，也可以使用已知名称，例如 `fflags`、`fcsr`、
`sstatus`、`stvec`、`satp`、`mstatus`、`misa`、`mtvec`、`mepc`、`mcause`、
`cycle`、`time` 和 `instret`。计数器、事件和物理内存保护系列名称支持
`hpmcounter3..31`、`mhpmcounter3..31`、`mhpmevent3..31`、`pmpcfg0..15` 和
`pmpaddr0..63`；RV32 高半计数器使用末尾 `h`。立即数形式的 `uimm5` 范围是
`0..31`。

```asm
[INST i_zicsr]
CSRRW  a0, mstatus, a1
CSRRSI a2, mtvec, 31
CSRRW  a0, 0x300, a1
```

## A 扩展

A 指令要求指令集包含 `a`。地址必须是纯寄存器形式 `[base]`；原子指令不接受
`[base + offset]`、标签地址或表达式地址。

| 指令族 | BITS | 语法 |
| :--- | :---: | :--- |
| `LR.W` | 32、64 | `LR.W rd, [base]` |
| `SC.W` | 32、64 | `SC.W rd, rs, [base]` |
| `AMOSWAP.W`、`AMOADD.W`、`AMOXOR.W`、`AMOAND.W`、`AMOOR.W` | 32、64 | `OP rd, rs, [base]` |
| `AMOMIN.W`、`AMOMAX.W`、`AMOMINU.W`、`AMOMAXU.W` | 32、64 | `OP rd, rs, [base]` |
| 上述 `.D` 形式 | 64 | 与 `.W` 形式相同 |

`AQ` 和 `RL` 是原子顺序限定词，可以在指令行顶层任意位置分别或同时使用：

```asm
LR.W AQ a0, [a1]
RL SC.D a0, a1, [a2]
AQ RL AMOADD.W a0, a1, [a2]
```

重复限定词或在非原子指令上使用 `AQ`、`RL` 会报错。

## F 扩展

F 指令使用浮点寄存器，内存地址继续使用方括号。支持的舍入模式为 `RNE`、`RTZ`、
`RDN`、`RUP`、`RMM` 和 `DYN`；可以写在助记符之前，或作为支持舍入模式的指令的
最后一个操作数。未写时使用 `DYN`。

| 指令族 | 语法 |
| :--- | :--- |
| `FLW` | `FLW fd, [base + offset]` |
| `FSW` | `FSW fs, [base + offset]` |
| `FADD.S`、`FSUB.S`、`FMUL.S`、`FDIV.S` | `OP fd, fs1, fs2 [, rm]` |
| `FSQRT.S` | `FSQRT.S fd, fs [, rm]` |
| `FSGNJ.S`、`FSGNJN.S`、`FSGNJX.S` | `OP fd, fs1, fs2` |
| `FMIN.S`、`FMAX.S` | `OP fd, fs1, fs2` |
| `FMADD.S`、`FMSUB.S`、`FNMSUB.S`、`FNMADD.S` | `OP fd, fs1, fs2, fs3 [, rm]` |
| `FEQ.S`、`FLT.S`、`FLE.S` | `OP rd, fs1, fs2` |
| `FCLASS.S`、`FMV.X.W` | `OP rd, fs` |
| `FMV.W.X` | `FMV.W.X fd, rs` |
| `FCVT.W.S`、`FCVT.WU.S` | `OP rd, fs [, rm]` |
| `FCVT.L.S`、`FCVT.LU.S` | `OP rd, fs [, rm]`，仅 RV64 |
| `FCVT.S.W`、`FCVT.S.WU` | `OP fd, rs [, rm]` |
| `FCVT.S.L`、`FCVT.S.LU` | `OP fd, rs [, rm]`，仅 RV64 |

浮点表达式地址加载和存储需要显式提供通用临时寄存器，因为浮点目标寄存器不能保存
中间地址：

```asm
FLW f0, [message], t0
FSW f1, [message + 4], t0
```

## D 扩展

D 自动启用 F。D 指令的操作数规则与 F 相同：

| 指令族 | 语法 |
| :--- | :--- |
| `FLD`、`FSD` | `FLD fd, [base + offset]`；`FSD fs, [base + offset]` |
| `FADD.D`、`FSUB.D`、`FMUL.D`、`FDIV.D` | `OP fd, fs1, fs2 [, rm]` |
| `FSQRT.D` | `FSQRT.D fd, fs [, rm]` |
| `FSGNJ.D`、`FSGNJN.D`、`FSGNJX.D` | `OP fd, fs1, fs2` |
| `FMIN.D`、`FMAX.D` | `OP fd, fs1, fs2` |
| `FMADD.D`、`FMSUB.D`、`FNMSUB.D`、`FNMADD.D` | `OP fd, fs1, fs2, fs3 [, rm]` |
| `FEQ.D`、`FLT.D`、`FLE.D` | `OP rd, fs1, fs2` |
| `FCLASS.D` | `FCLASS.D rd, fs` |
| `FCVT.W.D`、`FCVT.WU.D` | `OP rd, fs [, rm]` |
| `FCVT.L.D`、`FCVT.LU.D` | `OP rd, fs [, rm]`，仅 RV64 |
| `FCVT.D.W`、`FCVT.D.WU` | `OP fd, rs [, rm]` |
| `FCVT.D.L`、`FCVT.D.LU` | `OP fd, rs [, rm]`，仅 RV64 |
| `FCVT.S.D` | `FCVT.S.D fd, fs [, rm]` |
| `FCVT.D.S` | `FCVT.D.S fd, fs` |
| `FMV.X.D`、`FMV.D.X` | `FMV.X.D rd, fs`；`FMV.D.X fd, rs`，仅 RV64 |

`FCVT.D.S` 是精确拓宽，不接受舍入模式。

## C 扩展

当前只在显式写出 `C.*` 时生成 16 位指令，不会自动把普通 32 位指令压缩。

| 指令 | 语法 |
| :--- | :--- |
| `C.ADDI4SPN` | `C.ADDI4SPN rd, sp, offset`，`rd` 为 `x8..x15` |
| `C.NOP` | `C.NOP` |
| `C.ADDI`、`C.LI`、`C.LUI`、`C.SLLI` | `OP rd, imm` |
| `C.ADDIW` | `C.ADDIW rd, imm`，仅 RV64 |
| `C.ADDI16SP` | `C.ADDI16SP sp, imm` |
| `C.SRLI`、`C.SRAI`、`C.ANDI` | `OP rd, imm`，`rd` 为 `x8..x15` |
| `C.SUB`、`C.XOR`、`C.OR`、`C.AND` | `OP rd, rs`，寄存器为 `x8..x15` |
| `C.SUBW`、`C.ADDW` | 与上项相同，仅 RV64 |
| `C.MV`、`C.ADD` | `OP rd, rs` |
| `C.JR`、`C.JALR` | `OP rs` |
| `C.EBREAK` | `C.EBREAK` |
| `C.J` | `C.J target [, temporary]` |
| `C.JAL` | `C.JAL target [, temporary]`，仅 RV32 |
| `C.BEQZ`、`C.BNEZ` | `OP rs, target [, temporary]`，`rs` 必须为 `x8..x15` |
| `C.LW`、`C.SW` | `OP rd/rs, [base + offset]`，寄存器必须为 `x8..x15` |
| `C.LD`、`C.SD` | 与上项相同，仅 RV64 |
| `C.LWSP`、`C.SWSP` | `OP rd/rs, [sp + offset]` |
| `C.LDSP`、`C.SDSP` | 与上项相同，仅 RV64 |
| `C.FLD`、`C.FSD` | `OP fd/fs, [base + offset]`，要求 C 和 D，寄存器为 `f8..f15` |
| `C.FLDSP`、`C.FSDSP` | `OP fd/fs, [sp + offset]`，要求 C 和 D |
| `C.FLW`、`C.FSW` | `OP fd/fs, [base + offset]`，仅 RV32，要求 C 和 F，寄存器为 `f8..f15` |
| `C.FLWSP`、`C.FSWSP` | `OP fd/fs, [sp + offset]`，仅 RV32，要求 C 和 F |

压缩访存偏移必须为非负数，并满足对应数据宽度的对齐及编码范围。`C.ADDI4SPN`、
`C.ADDI16SP` 使用非零对齐立即数；栈形式的基址必须是 `sp`。

压缩分支和跳转仍使用裸目标表达式。ELF 跨 section 或外部目标分别使用
`R_RISCV_RVC_BRANCH`、`R_RISCV_RVC_JUMP` relocation。已解析目标超出压缩编码范围
时，压缩跳转按 2、4、8 字节逐级扩大，压缩分支按 2、4、8、12 字节逐级扩大；需要
`AUIPC + JALR` 时遵循上一节的临时寄存器规则。跨 section 的短压缩 relocation
由最终链接器按布局检查范围，超范围时链接失败。

## V 扩展

V 自动启用 F、D 和 Zicsr。当前提供基础向量配置、整数运算和 unit-stride
加载/存储：

| 指令族 | 语法 |
| :--- | :--- |
| `VSETVLI` | `VSETVLI rd, rs, e8/e16/e32/e64, mf8/mf4/mf2/m1/m2/m4/m8 [, ta/tu] [, ma/mu]` |
| `VSETIVLI` | `VSETIVLI rd, uimm5, vtype...` |
| `VSETVL` | `VSETVL rd, rs1, rs2` |
| `VADD.VV`、`VSUB.VV`、`VMUL.VV` | `OP vd, vs2, vs1 [, v0.t]` |
| `VDIV.VV`、`VDIVU.VV`、`VREM.VV`、`VREMU.VV` | `OP vd, vs2, vs1 [, v0.t]` |
| `VADD.VX`、`VSUB.VX`、`VMUL.VX` | `OP vd, vs2, rs1 [, v0.t]` |
| `VADD.VI` | `VADD.VI vd, vs2, imm5 [, v0.t]` |
| `VLE8.V`、`VLE16.V`、`VLE32.V`、`VLE64.V` | `OP vd, [base]` |
| `VSE8.V`、`VSE16.V`、`VSE32.V`、`VSE64.V` | `OP vs3, [base]` |

向量 unit-stride 内存指令只接受无偏移寄存器地址。V 只支持上表列出的指令，
也不自动推导或插入 `VSETVL*`。

## 系统指令

| 指令 | 语法 |
| :--- | :--- |
| `FENCE` | `FENCE`、`FENCE predecessor, successor` 或 `FENCE 0, predecessor, successor` |
| `FENCE.I` | `FENCE.I`，要求 Zifencei |
| `ECALL` | `ECALL` |
| `EBREAK` | `EBREAK` |
| `MRET` | `MRET` |
| `SRET` | `SRET` |
| `WFI` | `WFI` |
| `SFENCE.VMA` | `SFENCE.VMA` 或 `SFENCE.VMA rs1, rs2` |

`FENCE` 的 predecessor 和 successor 是由 `i`、`o`、`r`、`w` 组成的集合，`0`
表示空集合；集合内不能出现重复字符。无操作数形式等价于 `FENCE iorw, iorw`。
三操作数形式当前只接受 `fm=0`。
特权指令与 CSR 访问是不同类别；`MRET`、`SRET`、`WFI` 和 `SFENCE.VMA` 不要求
Zicsr。AASM 不检查指令执行时所处的权限级别。

## 单指令简化形式

这些形式只展开为一条机器指令。

| 指令 | 语法 | 等价形式 |
| :--- | :--- | :--- |
| `NOP` | `NOP` | `ADDI zero, zero, 0` |
| `RET` | `RET` | `JALR zero, ra` |
| `J` | `J target [, temporary]` | `JAL zero, target` |
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

## 多指令 MOV 与表达式地址

`MOV rd, expression` 可以加载完整的 RV32/RV64 数值或标签地址：

```asm
MOV a0, 0x123456789ABCDEF
MOV a1, message
MOV a2, message + 8
```

绝对表达式按照数值范围展开为 `ADDI`、`LUI`、`ADDIW`、`SLLI` 等指令。裸标签
表示标签地址；同 section 标签直接计算 PC 相对高低位，跨 section 或外部符号由
ELF relocation 处理包括 `CALL`、`CALL_PLT`、绝对 `HI20/LO12_I/LO12_S`、
`GOT_HI20`、成对的 `PCREL_HI20/PCREL_LO12_I/PCREL_LO12_S`，以及 `ALIGN` 和
可选 `RELAX` 提示。PC-relative 低位 relocation 的符号必须指向对应高位指令位置。
AASM 输出静态可重定位对象；最终链接器负责根据 `CALL_PLT` 和 GOT 引用建立需要的
PLT、GOT 及动态 relocation。

```asm
CALL external_function
CALL_PLT external_function
LUI x5, HIGH external_data
ADDI x5, x5, LOW external_data

got_address:
AUIPC x6, GOT external_data
LW x7, [x6 + PCREL_LOW got_address]

data_address:
AUIPC x8, PCREL_HIGH external_data + 8
SW x7, [x8 + PCREL_LOW data_address]
```

加载指令和内存 `MOV` 可以直接使用标签或绝对地址表达式：

```asm
LW  a0, [message]
MOV a0, BYTE [message + 1]
MOV a0, BYT4 [0x12345678]
```

表达式地址加载使用目标寄存器保存中间地址。表达式地址存储必须由第三个操作数显式
提供临时寄存器：

```asm
SW  a0, [message], a1
MOV BYT4 [message + 4], a0, a1
```

临时寄存器不能是 `zero`，也不能与待存储的数据寄存器相同。寄存器基址存储继续使用
两个操作数，不接受多余的临时寄存器：

```asm
SW a0, [a1 + 8]
MOV BYT4 [a1 + 8], a0
```

## 支持范围

支持 RV32I、RV64I、M、A、F、D、C、V、Zicsr、Zifencei 以及上述基础特权指令。
C 只支持显式压缩形式，V 只覆盖本节列出的基础子集。不支持完整特权体系、普通指令
自动压缩和完整向量指令集；AASM 可以输出 `RELAX` 提示，实际松弛由链接器完成。

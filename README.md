# UNISYM

![icon](./.picture/unisym.20240306.png) **[Universal Symbol System](http://unisym.org)**,
also known as ゆにッシンボル, or as 統象庫,
a lossless abstract system of programming and engineering,
including not only implementation, library, tool-kits, chroming engine and demonstrations.

![example](https://img.shields.io/badge/release-U2024AUTUMN-aliceblue.svg) ![example](https://img.shields.io/badge/supervisor-dosconio-violet.svg)


The repository is open-sourced under of *Apache License 2.0*. If you copy the code and used for the public or commercial, please stipulate your <u>copying from *UNISYM* led by *ArinaMgk*</u>. 

**Hsyster** `HERSYS`

- `HERNOT` and public `HEREPC`
    - Herepic Style Color: Herpink(R255, G97, B157, #double-0.618) 
- `HERPRO` and static `HERDAT`
	- Dinah Processor and Controller Chip
    - opensrc **UNISYM** (this, Apache)
	- Mecocoa
	- Host Environment
		- Witch and Nova
		- COTLAB
	- Webnet Service

<font color="magenta">⚠︎ All the code are by hands, so these big changes and mistakes in codes may be forgiven.</font>

For example, there are harmonious identifications for linkage-identifiers and calling-parameters:

```C++
ConPrint("Hello") # Some script-interpret

ConPrint("Hello");/* CPL */

Con::Print("Hello");// C++ Static 
Con.Print("Hello");// C++ Dynamic / Global

ConPrint Hello_String; Assembly Macro

...
```

| Component        | For                                                     | As                                                         |
| ------------------ | ------------------------------------------------------------ | ------------------ |
| ASM `asm/` | sources(asm) <=> object | assembler and dis-assembler |
| DEM `demo/` | / | demonstrations and utilities |
| DOC `doc/` | / | documents, aka `HEREPC` |
| INC `inc/` | / | including files |
| LIB `lib/` | / | implementation source code (including Witch **rendering** engine) |
| MGK `magic/` | sources => sources + list + depend + target | constructor(wizard), compiler, analyzer, translator, linker, synthesizer and modifier |
| WEL `wel/` |  | Si/E-mulator |

**Environment Variable**

Would better to set these environment variables, if using or building UNISYM:

- `uincpath`, points to INC, a must for developer
- `ulibpath`, points to LIB, optional
- `ubinpath`, points to BIN, a must for user
- `uobjpath`, points to OBJ, temporary folder
- `yanopath`, points to GCC cross-compilers and others
- `msvcpath`, points to Microsoft Visual C++ SDK
- `hostarget`, like `_OPT_RISCV64`, define when no target is specified

**Consideration** these if going to modify or using the code:

- Catalog and Manual
- Dependence
- Attribute (especially Allocation)
- Assert, strict-check and erroproc
- Aflag
- Inline and side-effect

To reduce the size of basic header, you can remove some support like STM32Fx.

---

![example](https://img.shields.io/badge/generation-2:Public-pink.svg)

<!-- Contributors -->

[🍨 Buy us a coffee for UNISYM project](https://www.buymeacoffee.com/arinamgk) 

- ![Contributor ArinaMgk (Phina)](./.picture/phina.head.bmp) <del> ***Phina Ren***</del>

- ![Contributor Doshou Haruno](./.picture/haruno.head.jpg) ***Doshou Haruno*** 

    - **Boudoir**: http://doscon.io and dosconio series
    - **Nickname**: Ayano
    - **GitHub**: `dosconio` 
    - **Mail**: dosconyo@gmail.com 
    - **Fork**: [Dosconio-derived USYM](http://github.com/dosconio/unisym) 

- 皆さん～


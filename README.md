# UNISYM

![icon](./.picture/unisym.20240306.png) **[Universal Symbol](http://unisym.org)**, also known as ゆにッシンボル, or as 統象庫, a system of programming and engineering, including not only implementation, library, tool-kits, chroming engine and demonstrations.

The repository is open-sourced under of *Apache License 2.0*. If you copy the code and used for the public or commercial, please stipulate your <u>copying from *UNISYM* led by *ArinaMgk*</u>. 

<font color="magenta">⚠︎ All the code are by hands, so these big changes and mistakes in codes may be forgiven.</font>

For example, there are harmonious identifications for linkage-identifiers and calling-parameters:

```C++
ConPrint("Hello") # Some script-interpret

ConPrint("Hello");/* CPL */

Con::Print("Hello");// C++
Con.Print("Hello");// C++

ConPrint Hello_String; Assembly Macro

...
```

| Component        | For                                                     | As                                                         |
| ------------------ | ------------------------------------------------------------ | ------------------ |
| [ASM](./doc/assembly/AASM.md)  `asm/` | sources(asm) <=> object | assembler and dis-assembler |
| DEM `demo/` | / | demonstrations |
| DOC `doc/` | / | documents |
| INC `inc/` | / | including files |
| LIB `lib/` | / | implementation source code |
| LNK `linker/` | objects => output | linker, synthesizer and modifier |
| MGK `magic/` | sources => sources + list + depend | compiler, analyzer and translator |
| WIT `witch/` | to outputs | **rendering** library and engine |
| WZD `wizard/` | / | constructor |

---

<!-- Contributors -->

[🍨 Buy us a coffee for UNISYM project](https://www.buymeacoffee.com/arinamgk) 

- ![Contributor ArinaMgk (Phina)](./.picture/phina.head.bmp) <del> ***[Phina Ren](./doc/author/arina.md)***</del>

- ![Contributor Doshou Haruno](./.picture/haruno.head.jpg) ***Doshou Haruno*** 

    - **Boudoir**: http://doscon.io and dosconio series
    - **Nickname**: Ayano
    - **GitHub**: `dosconio` 
    - **Mail**: dosconyo@gmail.com 
    - **Fork**: [Dosconio-derived USYM](http://github.com/dosconio/unisym) 

- 皆さん～


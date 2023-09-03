# UNISYM

Uniformed Symbols, also as "Uniform Symbolic Library".

&emsp;For inline and external symbols; for variables, macros and functions.

&emsp;Based on the official library or conventional styles.

This is still a β version. The symbols will be re-formed in the next main version.

If you copy the code and used for the public, please **stipulate your copying from UNISYM led by ArinaMgk**.

### Build - Make into a library

For example, for Windows 64-bit environment, by GCC 8.1.0, and in the parent directory, you can text your Makefile script:

```makefile
u64:
	gcc -c ./unisym/c/source/*.c -m64 -D_dbg -O3 -s -lm -D_WinNT
	ar rcs usym64.lib *.o
	del *.o
```

### Component

| Folder Name                | For                                                          |
| -------------------------- | ------------------------------------------------------------ |
| `old back and locale .git` |                                                              |
| c                          | C Programming Language and C Plus Plus. (All Open-src)       |
| Kasha                      | x86-Series Platform Assembly Kit for OSDev.                  |
| MCU                        | for integrated micro-controller units. (temporarily close-src) |

`ustring`: the operations for string and big-integer-number「大數運算」 (flexible-size). Besides, this contains: SystemConversion「進制轉換」; The "u" is for distinction from that of the standard library, which means "UNISYM", **but "Unicode"**. This is going to divide into "hstring" and "bstring", and "usting" will include them.

`dnode`: double-direction node.

| String Arithmetic  | Description                                                  | State    |
| ------------------ | ------------------------------------------------------------ | -------- |
| ustring::**ChrAr** | [big endian] optional signed ASCII integer arithmetic  and operations, for buffer or heap, which can be printed directly. Now exists the dependence of `arna_eflag`, even input!!! | ripen    |
| **CoeAr**          | [big endian] Arinae classic structure, Coff-expo-divr, based on **ChrAr**. the mechanism can express and operate any big-rational-real-number. | ripen    |
| **NumAr**(4)       | [big endian] the mechanism can express and operate the big-rational-real-numbers (not endless) in 4 dimensions of time and room, or real and imaginary, based on **CoeAr**. | ripen    |
| **RsgAr**          | [little endian] Haruno no base 256 expo-unit 16 extension. Use the register-size unit for faster arithmetic speed. 「多字節數字」 | ripen    |
| **RegAr**          | [big endian] C-E-D structure based on **RsgAr**. the mechanism can express and operate any big-rational-real-number. | building |
| **HrnAr**(4)       | [little endian] 4D based on **RegAr**                        | todo     |
| **BytAr**          | [little endian] base 256 expo-unit 256 for signed integer, whose the first byte is the flag byte. | todo     |
| **ArnAr**          | [little endian] Arinae classic structure based on **BytAr**. the mechanism can express and operate any big-rational-real-number. The expo and divr are optional. | todo     |
| **TenAr**          | Haruno no. Operations for tensor(different sizes and types in an array) and array. | todo     |

`node`: node.

`Treenode`: Old version, to adapt...

| Kasha component | Description | State |
| --------------- | ----------- | ----- |
| `Kasha::arith`|Arithmetic  |       |
| `Kasha::cpuins`|CPU Information |       |
| `Kasha::hdisk`| Hard-disk |       |
| `Kasha::osdev`| OSDEV Special |       |
| `Kasha::pseudo` | Pseudo structure for NASM and others assemblers |       |
| `Kasha::timer`| Date and time|       |
| `Kasha::video`| Video display |       |



*others*: Console Table「終端字符繪表格」;



- `.a`-suffixed file is for Arina-made assembler (private so far) , you may also use NASM0207.



### TODO

- Simply-used version of each potential function.



### Source

Except stipulation, all codes are produced by the contributors but the others. **If the codes rise any mistake while you use the codes in your important service, the contributors should not help you and your loss!** 



---

![Contributor ArinaMgk](./.picture/phina.head.bmp)

[🍨 Buy me a coffee](https://www.buymeacoffee.com/arinamgk)

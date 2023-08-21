# UNISYM

Uniformed Symbols, also as "Uniform Symbolic Library".

&emsp;For inline and external symbols; for variables, macros and functions.

&emsp;Based on the official library or conventional styles.

This is still a β version. The symbols will be re-formed in the next main version.

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
| c                          | C Programming Language and C Plus Plus. (All Opensrc)        |
| Kasha                      | x86 Platform Assembly Kit for OSDev. (かしゃ カーネル)(進修需要、暫不開源) |
| MCU                        | for integrated micro-controller units                        |

`ustring`: the operations for string and big-integer-number「大數運算」 (flexible-size). Besides, this contains: SystemConversion「進制轉換」; The "u" is for distinction from that of the standard library, which means "UNISYM", **but "Unicode"**.

`coear`: the mechanism can express and operate the big-real-number「有理實數運算」, e.g. `-1.23e45`

`numar`: the mechanism can express and operate the big-real-numbers (not endless) in 4 dimensions「時空矢量運算」.

`regar`:  the mechanism can express and operate the numbers in multi-byte way. 「多字節數字」

`dnode`: double-direction node.

`node`: node.

*others*: Console Table「終端字符繪表格」;

### Source

Except stipulation, all codes are produced by the contributors but the others. **If the codes rise any mistake while you use the codes in your important service, the contributors should not help you and your loss!** 



---

![Contributor ArinaMgk](./.picture/phina.head.bmp)

[🍨 Buy me a coffee](https://www.buymeacoffee.com/arinamgk)

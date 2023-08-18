# UNISYM

Uniformed Symbols, also as "Uniform Symbolic Library".

&emsp;For inline and external symbols; for variables, macros and functions.

&emsp;Based on the official library or conventional styles.

### Build

```makefile
c64:
	gcc -c .././Unisym/c/source/*.c -m64 -D_dbg -O3 -s -lm -D_WinNT
	ar rcs unisym64.libr *.o
	del *.o
```

### Component

| Folder Name                | For                                                          |
| -------------------------- | ------------------------------------------------------------ |
| `old back and locale .git` |                                                              |
| c                          | C Programming Language and C Plus Plus. (All Opensrc)        |
| Kasha                      | x86 Platform Assembly Kit for OSDev. (かしゃ カーネル)(進修需要、暫不開源) |
| MCU                        | for integrated micro-controller units                        |

ustring: the operations for string and big-integer-number「大數運算」 (flexible-size). Besides, this contains: SystemConversion「進制轉換」;

coear: the mechanism can express and operate the big-real-number「不限長實數運算」, e.g. `-1.23e45`

numar: the mechanism can express and operate the big-real-numbers (not endless) in 4 dimensions「時空矢量運算」.

regar:  the mechanism can express and operate the numbers in multi-byte way. 「多字節數字」

dnode: double-direction node.

node: node.

*others*: Console Table「終端字符繪表格」;

### Source

Except stipulation, all codes are produced by the contributors but the others. **If the codes rise any mistake while you use the codes in your important service, the contributors should not help you and your loss!** 


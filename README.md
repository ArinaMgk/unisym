# unisym
Uniformed Symbols.
<br/>&emsp;For inline and external symbols; for variables, macros and functions.
<br/>&emsp;Based on the official library or conventional styles.
<br/>
**: How to build by MAKEFILE**
```
c64:
	gcc -c .././Unisym/c/source/*.c -m64 -D_dbg -O3 -s -lm -D_WinNT
	ar rcs unisym64.libr *.o
	del *.o
```

**: Folder**
c &emsp C Programming Language
cpp &emsp C++
MCU &emsp MCU components and others
x86 &emsp x86 Platform Assembly


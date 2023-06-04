# unisym
Uniformed Symbols.
<br/>&emsp;For inline and external symbols; for variables, macros and functions.
<br/>&emsp;Based on the official library or conventional styles.
<br/>
**: How to build by MAKEFILE**
<br/>
```
c64:
	gcc -c .././Unisym/c/source/*.c -m64 -D_dbg -O3 -s -lm -D_WinNT
	ar rcs unisym64.libr *.o
	del *.o
```

**: Folder**
<br/>
<br/>c &emsp C Programming Language
<br/>cpp &emsp C++
<br/>MCU &emsp MCU components and others
<br/>x86 &emsp x86 Platform Assembly
<br/>

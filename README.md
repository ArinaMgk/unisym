# UNISYM

Uniformed Symbols, also as "Uniform Symbolic Library".

&emsp;For inline and external symbols; for variables, macros and functions.

&emsp;Based on the official library or conventional styles.

This is still a β version. The symbols will be re-formed in the next main version.

If you copy the code and used for the public, please **stipulate your copying from UNISYM led by<font color="purple"> ArinaMgk </font> **.

Because only few people have forked the UNISYM, the big changes will be done without log in the specific documents.

If any function changed, the test program in `./*/test/`should be updated or confirm the program is still adaptive.

The private document project: **Herinote**

Usual update strategy:

- over 12 files or over 21 lines changed
- <del>about 30 days from last commit</del> each Sunday 
- Others' request

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

`ustring`: the operations for string and big-number, e.g. `2013e65535`. Besides, this contains: System-Conversion, e.g. `0x2712` to `10002`; The "u" is for distinction from that of the standard library, which means "UNISYM", **but "Unicode"**. This is going to divide into "hstring" and "bstring", and "usting" will include them.

`dnode`: double-direction node.

| String Arithmetic | Description                                                  | State    |
| ----------------- | ------------------------------------------------------------ | -------- |
| **ChrAr**(base)   | [big endian] [variable-length] optional signed ASCII integer arithmetic  and operations, for buffer or heap, which can be printed directly. the used names are `BytAr` and `ASCAr` and `StrAr`. Now exists the optional dependence of `arna_eflag`. These are **2 version** in `hstring` and `bstring`. | ripen    |
| **FltAr**(1)      | [big endian] `{{Sign, ChrAr}, ExpoInDec }` the extension of **ChrAr**. But any fltar-number is fixed-size. If the result is too big, the magic number "999… . …999" will be expressed. For the fixed size of floating part, the result may be cut to adapt to the structure. Natural and only for **buffer** version. | todo     |
| **CdeAr**(1)      | [big endian] Arinae classic structure, Coff-expo-divr, based on **ChrAr**. the mechanism can express and operate any big-rational-real-number. The name follows what ArnMgk called this. | ripen    |
| **NumAr**(4)      | [big endian] the mechanism can express and operate the big-rational-real-numbers (not endless) in 4 dimensions of time and room, or real and imaginary, based on **CdeAr**. | ripen    |
| **RsgAr**(base)   | [little endian] Based on 256. Use the register-size unit for faster arithmetic speed. E.g. `0x12, 0x34` for 0x3412 | ripen    |
| **RedAr**(1)      | [big endian] Dosconio's base 256 expo-unit 16 extension. C-E-D structure based on **RsgAr**. the mechanism can express and operate any big-rational-real-number. | building |
| **HrnAr**(4)      | [little endian] 4D based on **RegAr**                        | todo     |
| **BytAr**(base)   | [optional endian] base 256  for signed integer, whose the first byte is the flag byte. E.g. `0x12, 0x34` for 0x3412 or 0x1234. | todo     |
| **DecAr**(base)   | [optional endian] base 100/256 for signed integer, whose the first byte is the flag byte. The first binary of the first is the sign bit and of the others are state flags. E.g. `0x12, 0x34` for 0d3412 or 0d1234. | todo     |
| **ArnAr**(1)      | [optional endian] Arinae classic structure based on **BytAr**, base 256 expo-unit 256. the mechanism can express and operate any big-rational-real-number. The expo and divr are optional. | todo     |
| **TenAr**(*)      | Haruno no. Operations for tensor(different sizes and types in an array) and array. | todo     |

`node`: node.

`Treenode`: Old version, to adapt...

| Kasha component | Description | State |
| --------------- | ----------- | ----- |
| `Kasha::arith`|Arithmetic  | ripen |
| `Kasha::cpuins`|CPU Information | ripen |
| `Kasha::hdisk`| Hard-disk | ripen |
| `Kasha::osdev`| OSDEV Special | ripen |
| `Kasha::pseudo` | Pseudo structure for NASM and others assemblers | ripen |
| `Kasha::timer`| Date and time| ripen |
| `Kasha::video`| Video display | ripen |

| Chrome   | Description   | State |
| -------- | ------------- | ----- |
| contable | Console Table | ripen |
| consio   | Console IO    | ripen |
|          |               |       |

- `Chr-` means ASCII string, and `Byt-` for the basic memory unit.
- `.a`-suffixed file is for Arina-made assembler (private so far) , you may also use NASM0207 as a replacement.
- `cpp::dstring`: double directions string, RFW06 created, for example, "123" is as `0xFF, 0x00, 0x31, 0x32, 0x33, 0x00 `. The structure is convenient for reverse printing, 0xFF means the head of the memory, the pointer should point to "`0x31`".
- UNISYM-special pragma `#pragma source`

### Adapted Host Environment

- Windows x86 and x64

### Macro

#### > Common Control

- `_ARN_INSIDE_LIBRARY_INCLUDE`
- `_ARN_FLAG_DISABLE`
- 

### Source

Except stipulation, all codes are produced by the contributors but the others. **If the codes rise any mistake while you use the codes in your important service, the contributors should not help you and your loss!** 



---

![Contributor ArinaMgk](./.picture/phina.head.bmp)

[🍨 Buy me a coffee](https://www.buymeacoffee.com/arinamgk)

---
her-note: false
dg-publish: true
---

> It is flaws that make UNISYM perfect.

### Use which basic headfile

Generation 2 style (until `U2025xxx`):

`stdinc.h` for C, `unisym` for C++

Generation 3 style:

If you develop something on host-environment, include `stdinc.h` for C, `unisym` for C++;
if you develop MCU-program, use the special file like `STM32F103VE` for C++...

### Environment Variable

- `uincpath`, points to INC, a must for developer
- `ulibpath`, points to LIB, optional
- `ubinpath`, points to BIN, a must for user
- `hostarget`, like `_OPT_RISCV64`, define when no target is specified

### Development Environment

- GNU
- Microsoft
- {TODO} Magice

### TEMP

- Win64 MSVC&GCC
- Win32 GCC
- Linux ELF64 ELF32
- RISCV-64 GCC on Ubuntu(22.x+)-64
- STM32F 1/4


### About Generation-3

- Guiding head file, like `inc/cpp/uint48`;
- Use `.lib` for normal with weak linkage. use `.c` and other source files if you need to debug them;
- Include host head file like `STM32F103VEx` but `stdinc.h` normally;
- Mainly for `4X\4` aka C++, C#, Rust languages and begin developing Magice;
- C++ based on C to cut duplication;
- `_INCPP_...` rather than `_INC_..._X`, also `_INM_...` for Magice;

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

### Development Environment

- GNU
- Microsoft
- {TODO} Magice

### TEMP

- Win64 MSVC&GCC
- Win32 GCC
- Linux ELF64 ELF32
- RISCV-64 GCC on Ubuntu(22.x+)-64
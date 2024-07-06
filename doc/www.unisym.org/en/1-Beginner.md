---
her-note: false
dg-publish: true
---

> The meaning of UNISYM is not the perfect, but herself.

### About Generation-3

Before `U2025SPR`, although some new characters will be added, the old style will be kept, but it is not recommended to use them.

- Guiding head file, like `inc/cpp/uint48`;
- Use `.lib` for normal with weak linkage. use `.c` and other source files if you need to debug them;
- Include host head file like `STM32F103VEx` but `stdinc.h` normally;
- Mainly for `4X\4` aka C++, C#, Rust languages and begin developing Magice;
- C++ based on C to cut duplication;
- `_INCPP_...` rather than `_INC_..._X`, also `_INM_...` for Magice;
- new format of target names


#### Use which basic headfile

Generation 2 style (until `U2025SPR`):

`stdinc.h` for C, `unisym` for C++

Generation 3 style:

If you develop something on host-environment, include `stdinc.h` for C, `unisym` for C++;
if you develop MCU-program, use the special file like `STM32F103VE` for C++...

#### Gene-3 Style Modules

- Node aka Chain
- Dnode
- Nnode


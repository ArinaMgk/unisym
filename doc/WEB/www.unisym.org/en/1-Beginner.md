---
her-note: false
dg-publish: true
---

> The meaning of UNISYM is not the perfect, but herself.

### Setup UNISYM

1. set the environment variables: `ulibpath` and others. detail to refer [Covenant](./4-Covenant.md).

### Mind them when changing modules

- **Target**: every file should consider each target (instruction/register architecture + board platform). They should be registered in mdFiles, too;
- **Manual**: every function / object should own its.
- **Gene-3**;

Subproject: mgc aas wzd wel

TODO: UNISYM daughter compile herself.

The Update Change Log will be enabled after AD-2025, which may be in `ustest` but not `herlog` depot.

### GENE-2 Release

epic00min cgl32/64 cgw16/32/64 cvw32/64

Until 2024WTR.



### About Generation-3

Before `U2025SPR`, although some new characters will be added, the old style will be kept, but it is not recommended to use them.

- Guiding head file, like `inc/cpp/uint48`;
- Use `.lib` for normal with weak linkage. use `.c` and other source files if you need to debug them;
- Include host head file like `STM32F103VEx` but `stdinc.h` normally;
- Mainly for `4X\4` aka C++, C#, Rust languages and begin developing Magice;
- C++ based on C to cut duplication;
- `_INCPP_...` rather than `_INC_..._X`, also `_INM_...` for Magice;
- new format of target names




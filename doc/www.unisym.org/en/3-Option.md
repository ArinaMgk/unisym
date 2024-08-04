---
her-note: false
dg-publish: true
---

### Option Switch

**option**: `arch`+`host`+`bitmode`
- Architecture Class :chart: like "`_ARC_x86`" (for example, this macro may not exist, so do below)
- Chip Class :chart: "`_CPU_AMDRyzen7_5800H_Radeon`"
- Host Class :chart: "`Win32`"
- Devkit Option :chart: "`_BUILD_MSVC`"

#### > Soft Option Control

- `_DEBUG`

- `_AUTO_INCLUDE` will include ASM or C codes

- `_ASSERT_STRICT`, `_LIB_DEBUG_CHECK_MORE`

- `_ASSERT_NONE`

#### > Kit

- `_BUILD_MSVC`

- `_BUILD_Qt` 

#### > Architecture

- `_ARC_x86` 
    - this equals `3` : `_ARC_i80386`

style gene-3
- `_OPT_RISCV64`

##### > Instruction Set (going to be abandoned)

- p_i386
- p_i8051
- p_CortexM0

#### > Processor and Controller Unit

- `_MCU_STM32F103VE` => `_MCU_STM32F10x`
- `_CPU_AMDRyzen7_5800H_Radeon`

#### > Platform and Environment

Sub-option of CPU or MCU

> `ENV(FREE)` are compatible with `ENV(HOST)` 
>
> The respective documents are `stdinc.h` and `stdinc.h` 

runtime host environment

Trend: The macro name to the left of the colon is for internal use, which is in format "_Architecture_Host_Bitmode"; the user should use the one to the right of the colon

- `_Intelx86_Windows_32`
- `_Intelx86o64_Windows_64`
- `_Intelx86_Windows_16`
- `_Intelx86_Linux_32`
- `_Intelx86o64_Linux_64`
- `_Intelx86_Mcca_32`
- `_Intelx86_Mcca_16`

"o" is "or".

*pref*: means the architecture. If this has not set (not recommended), `Intelx86` or `Intelx86o64` will be applied. Below are potential prefixes.
- `Intelx86`
- `Riscv64`

environment

- `__BITS__` 8 or 16 or 32 or 64
- Endian ... 
- Platform
    - {ISSUE} Whether to make a integer `_LIMIT_INTEL_SERIES_` to simplify the expression: 
    - `_LIMIT_8086` for CPU 8086 or simpler, also contain _LIMIT_80286 and higher. So this contain below
    - `_LIMIT_80286`  contain below


strategy

- `_INLINE` suffix, e.g. `_INC_USTRING_INLINE`  





#### > Pragma

- UNISYM-special pragma `#pragma source`, this may raise `MSVC:C4068`

#### > String

- `_LIB_STRING_HEAP`
- `_LIB_STRING_BUFFER`



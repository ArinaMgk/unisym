---
her-note: false
dg-publish: true
---

### Option Switch

**option**: `arch`+`host`+`bitmode`
- Architecture Class :chart: like "`_ARC_x86`" (for example, this macro may not exist, so do below)
- Chip Class :chart: "`_CPU_AMDRyzen7_5800H_Radeon`"
- Host Class :chart: "`Win32`"
- Devkit Option :chart: "`_DEV_MSVC`"

#### > Soft Option Control

- `_AUTO_INCLUDE` will include ASM or C codes

- `_ASSERT_STRICT`, `_LIB_DEBUG_CHECK_MORE`

- `_ASSERT_NONE`

#### > Platform and Environment

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



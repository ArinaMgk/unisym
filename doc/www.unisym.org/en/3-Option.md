---
her-note: false
dg-publish: true
---

### Option Switch

#### > Soft Option Control

- `_dbg`(old style) <=> `_DEBUG`

- `_AUTO_INCLUDE` will include ASM or C codes

- `_ASSERT_STRICT`

- `_ASSERT_NONE`

- `_ARN_FLAG_DISABLE` Control status variables and `aflag`.
- `_ARN_FLAG_ENABLE` 
- `_LIB_DEBUG_CHECK_MORE`

#### > Kit

- `_BUILD_MSVC`

- `_BUILD_Qt` 

#### > Processor and Controller

- `_INC_MCU_STM32F103VE` 

#### > Platform and Environment

Sub-option of CPU or MCU

> `ENV(FREE)` are compatible with `ENV(HOST)` 
>
> The respective documents are `alice.h` and `host.h` 

runtime host environment

- `_WinNT`
- `_Linux`

environment

- `_BINARY`
    - 8
    - 16
    - 32
    - 64
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



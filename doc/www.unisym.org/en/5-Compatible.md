---
her-note: false
dg-publish: true
---

## Recommendations from future versions for us

- use `STM32F103VEx` without macro but only `stdinc.h` with the macro, more to see in `6-Depend.md`;

Below are target chip reference in `_MCU_STM32F10x`:
- `_MCU_STM32F103VE`
- `_MCU_STM32F105xC`
- `_MCU_STM32F107xC`

## Requirements from future versions for contributors

- rigorous headers;
- rigorous {*ATTRIBUTIVE*} comments and solving them: {TODO} {TEMP};


## For ISO/IEC CPL Standard Library

- CPL: ISO-C99
- C++: ISO-C++11/C++17

### `uassert.h` ≥ `assert.h`



### `uctype.h` ≥ `ctype.h`



### `ustdbool.h`≥ `stdbool.h`

since C99.

### `ustring.h` ≥ `string.h`

| U.S.Identifier | I.I.Identifier |                                   |
| -------------- | -------------- | --------------------------------- |
| StrCopy        | strcpy         | Copy the string terminated by nil |
| ...            |                |                                   |
| MemRelative    | *None*         |                                   |
| ...            |                |                                   |

### `archit.h` ≥ `limits.h`, `stddef.h`

### `integer.h` ≥ `inttypes.h`, `stdint.h`

Users can also provide their own`stdint.h`.

### `arith.h` ≥ `math.h`

### `floating.h` ≥ `float.h`

### `number.h` ≥ `complex.h`(C99+) , `fenv.h`(C99+) , `tgmath.h`(C99+)

### `error.h` ≥ `errno.h`

### `loc.h` ≥ `locale.h`

### `supple.h` ≥ `setjmp.h`  , `stdarg.h.h`

### `datime.h` ≥ `time.h` 

### `consio.h` ≥ `stdio.h` 

### `widechar.h` ≥ `wchar.h`(C95+)  , `wctype.h.h`(C95+) 

## Below : C11+





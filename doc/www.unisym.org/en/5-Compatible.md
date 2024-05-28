---
her-note: false
dg-publish: true
---

## Recommendations from future versions for us

- use `STM32F103VEx` without macro but only `stdinc.h` with the macro, more to see in `6-Depend.md`;

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

## `arith.h` ≥ `math.h`


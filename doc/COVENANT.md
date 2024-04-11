# Symbol Covenant



## Format

- `.a` `.m` assembly file
- `.o` object file
- `.mgk` Magice source file

**Affix (prefix, infix or suffix(postfix))** 

- mk: Makefile
- g: GCC or DJGPP
- v: MSVC
- m: Magice



---



- w16: MSDOS (16bit)
- w32: Win32
- w64: Win64
- e64: ELF 64
- mx: *Mecocoa* 



## Covenant

- For C
    - `{memmec}` All address are logic address and the relative addresses are continuous.
    - `{memmec}` Default decreasing stack(SS:SP) direction.
    - `{digit }` Default binary and bitwise operations.
    - `{endian}` Default little endian.
- `NULL` = `nullptr` = 0
- `sizeof` `std(/u/s)int` == address of pointer == `size_t` == `ptrdiff_t` 



## Naming Rules

- C
    - header style `number.h` 
    - `number_t`  style for struct name
    - `AOpqr` style for  function identifier
- C++ 
    - header style `number` 
    - `Number_T` style for template name
    - namespace `uni` 
    - `AbcdOpqr` style for class name and function identifier
    - `abcdOpqr` style for object of the type AbcdOpqr or other types
    - unisym formal style:
        - If a reference return may point to `nullptr`, make a `ERR` object in the class to occupy the condition but use Exception System. 
        - Usually the content can be cast into boolean object, which is compared with ERR, or we can use the parent/manager item acting as it.
    - the prefix `set`, `get` are all in lower-style
- C#
    - namespace `Unisym` 

- `Number`: Java/C# style
- `NUMBER`: Classic style



- Real-16: The symbols are without '_' prefix.
- Line-64: The symbols are with double '_' prefix.
- Avoid using reference& but pointer* for inside function for more simply transplant.
- Use `uint8`, `uint16` and so on, as integer types.

## Tendency

- use `nullptr` 

---
her-note: false
dg-publish: true
---

Her-style, Code-reusing and Conceptual-razor.

## Target

Target in format `Arch-Format(Devk)-Host`*`(D)`*
- i8086-DJGPP-MSDOS {16}
- i686-COFF(GCC)-Win32 {32}
- i686-COFF(MVC)-Win64 {32}
- ix64-COFF(GCC)-Win32 {64}
- ix64-COFF(MVC)-Win64 {64}
- i686-ELF(GCC)-Lin32 {32}
- ix64-ELF(GCC)-Lin64 {64}
- i686-ELF(GCC)-MCCAx86 {32}
- RISCV64-ELF(GCC) {64}
- RISCV64-ELF(GCC)-MCCAr64 {64}

### ARCH

### Format


### Devk


GCC = GNU Compiler Collection

MVC = Microsoft Visual C/C++

HER = her/pro/Magice 


### Host



## Trait

to see `./inc/c/trait.h`


## Environment Variable

- `uincpath`, points to INC, a must for developer
- `ulibpath`, points to LIB, optional
- `ubinpath`, points to BIN, a must for user
- `hostarget`, like `_OPT_RISCV64`, define when no target is specified

## TEMP

- Win64 MSVC&GCC
- Win32 GCC
- Linux ELF64 ELF32
- RISCV-64 GCC on Ubuntu(22.x+)-64
- STM32F 1/4

## Covenant

- For C
    - `{memmec}` All address are logic address and the relative addresses are continuous.
    - `{memmec}` Default decreasing stack(SS:SP) direction.
    - `{digit }` Default binary and bitwise operations.
    - `{endian}` Default little endian.
- `NULL` = `nullptr` = 0x0 ≠ 0.0
- `sizeof` `std(/u/s)int` == address of pointer == `size_t` == `ptrdiff_t` 



## Naming Rules

- ANY
	- old style `NUMBER`
- C
    - header style `number.h` 
    - `number_t`  style for struct name
    - `AOpqr` style for  function identifier
	- old style `number`
- C++ 
    - header style `number` 
    - `Number_T` style for template name
	- `Number` 
    - namespace `uni` 
    - `AbcdOpqr` style for class name and function identifier
    - `abcdOpqr` style for object of the type AbcdOpqr or other types
    - unisym formal style:
        - If a reference return may point to `nullptr`, make a `ERR` object in the class to occupy the condition but use Exception System. 
        - Usually the content can be cast into boolean object, which is compared with ERR, or we can use the parent/manager item acting as it.
    - the prefix `set`, `get` are all in lower-style
- C#
    - namespace `Unisym` 

- Real-16: The symbols are without '_' prefix.
- Line-64: The symbols are with double '_' prefix.
- Avoid using reference& but pointer* for inside function for more simply transplant.
- Use `uint8`, `uint16` and so on, as integer types.

- prefix`get`, suffix`Get` and midfix`_get`

---

- Pointers
	- `offs` a $void*$ pointer, it cannot be added to integers in some compilers for its non-size;
	- `addr` a $char*$ or $byte*$ pointer, usually unioned with `offs` (an alias);


### delay

instructional delay: in file whose name including a frequency, e.g. `delay11M.asm`;

timer delay: in file whose name does not, e.g. `delay.c`;


## Contributing Rules 

Tendency

- use `nullptr` 

Consideration

- Catalog
- Dependence
- Attribute (especially Allocation)
- Assert, strict-check and erroproc
- Debug
- Aflag
- Inline version


# Universal Logical Linear Language : Magice

- domain: <del>http://magicoll.org</del> (abandoned) or http://magice.org 
- origin: `Magic Collection of Origin and Love Legend`, from `L.A.O.E.C. (Love & Origin Epic Collection, used name of UNISYM and Her-Note)` 
- file extension: `.m` `.mg` `.mgc` `.src` 
    - header extension: `.h` `.mgh` 

- compiler: `mgc` 

## Dependence

For each `.c` input, a `.dep` will be generated, whose each line is the full path of the dependent library file or its source, which differ in the suffix. The content is decide by the macro block `#ifdef _AUTO_INCLUDE`. If the binary library is detected, it is prior to source file. For example, `datime.h` >>> `U:/lib/c/datime.c`.

If you include some file but do not use a function, the function will not be added into your object file, for we cutting the content as the temporary version of the *unisym* library. 


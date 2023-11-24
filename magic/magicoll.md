# Universal Logical Linear Language : Magicoll

- domain: magicoll.org 
- alias: `Magical Epic of Origin and Love `
- file extension: `.mgk`
- compiler: `mgc`

## Dependence

For each `.c` input, a `.dep` will be generated, whose each line is the full path of the dependent library file or its source, which differ in the suffix. The content is decide by the macro block `#ifdef _AUTO_INCLUDE`. If the binary library is detected, it is prior to source file. For example, `datime.h` >>> `U:/lib/c/datime.c`.

If you include some file but do not use a function, the function will not be added into your object file, for we cutting the content as the temporary version of the *unisym* library. 


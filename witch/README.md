## UNI-WITCH

A kit to generate the dependence and provide the builders necessary parameters. This is going to become the MAKEr tool.

For each `.c` input, a `.dep` will be generated, whose each line is the full path of the dependent library file or its source, which differ in the suffix. The content is decide by the macro block `#ifdef _AUTO_INCLUDE`. If the binary library is detected, it is prior to source file. For example, `datime.h` >>> `U:/lib/c/datime.c`.


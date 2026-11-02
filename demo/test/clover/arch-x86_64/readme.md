# Clover x86-64 architecture tests

This directory stores focused x86-64 assembly regression inputs. Each source
states its expected result or observable behavior in an `Expected` comment.

Run the complete baseline from the repository root:

```text
AASM=/path/to/aasm ./asm/check.sh
```

The checks cover instruction and address encoding, prefixes, symbols, data
directives, structures, branch optimization, preprocessing, ELF64 sections and
relocations, and rejection of an invalid effective address. The runner compares
byte-oriented cases directly and normalizes behavior-oriented output before
comparison.


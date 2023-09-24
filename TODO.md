### General

- [ ] Create WIKI for each object in the UNISYM Library.
- [ ] Printable document version: *UNISYM Structures and Algorithms*.
- [ ] Standard C++ support
- [ ] Linux-Distributions Host environments support
- [ ] Covenant and naming-rule
- [ ] Functions depended filter tool (combinate a new source file)

### Among all XxxAr

- [ ] partial `ptrdiff_t` ---> `ssize_t`.
- [ ] Explicit support for negative zero `-0`.
- [ ] Modify `-Heap` for functions of ustring-family.
- [ ] Split RegAr.c.
- [ ] Split partial **ustring** into **hstring** and **bstring**, by the macro switch `_LIB_STRING_HEAP` and `_LIB_STRING_BUFFER` .
  - [x] hstring
  - [ ] bstring
  - [x] debug for each, especially for Chr{+-*/} yo `TestCharArith.c`
- [x] Change the **HeapYo**-bit of Arn-flag from for I&O into just Output.
- [ ] Minimum using mode macro  `_LIB_LIMIT_CHECK_DISABLE`
- [ ] CoeAr update for ustring
- [ ] `/c/test/` and `/c/demo/`
- [ ] Adapt NumAr for CdeAr
- [ ] Make use of `ADC`, `AAS`, `XADD` ... instructions
- [ ] CoeXar Dependence, especially for CoePow and CoeLog
- [ ] set error message but not call `erro`
- [ ] Clarify the limit of `ChrPow` and others use single size_t
- [ ] The reason for the compensation for `1000^(1/3)` not equal completely `10`

### Others

- [ ] `cpp::dstring`
- [x] XNode: insert left or right or sub-end or sub-head or block(sub-range), C++ no class will match these perfectly.
- [ ] a macro to let ustring replacing relative symbols into standard library form. For example, "StrLength" is to "strlen".
- [ ] tnode functions and rewrite `Classic`, `Once`, `All`, where `-All` may call `-Once` and previous `-Once` into `-Classic`.

### Potential Issue

- ustring about heap



### Controversial

- [ ] The type of `addr` of node should be `void*` or `char*`;


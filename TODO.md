### General

- [ ] Create WIKI for each object in the UNISYM Library.
- [ ] Printable document version: *UNISYM Structures and Algorithms*.
- [ ] Standard C++ support
- [ ] Linux-Distributions Host environments support
- [ ] Covenant and naming-rule

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

### Others

- [ ] `cpp::dstring`
- [ ] Node: insert left or right or sub-end or sub-head

### Potential Issue

- ustring about heap



### Controversial

- [ ] The type of `addr` of node should be `void*` or `char*`;


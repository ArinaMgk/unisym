# magice compiler

```mermaid
flowchart TB
    SRC["[source code]"]
    IRs["[Soft IR]"]
    IRh["[Hard IR]"]
    IRd["[Docu IR]"]
    OBJ["[Object]"]
    EXE["[Executable]"]

    SRC -->|frontend| H1["AST"]
    H1 -->|optimizer| IRs
    H1 -->|optimizer| IRh
    H1 -->|optimizer| IRd
    H1 -->|translator| SRC
    IRs -->|asm.backend| OBJ
    OBJ -->|linker with optimize| EXE
    EXE -->|wel| H2[" "]

	IRh --> SysVerilog["SysVerilog Backend"]
	IRh --> Verilog["Verilog Backend"]

    style H1 fill:none,stroke:none,color:none
    style H2 fill:none,stroke:none,color:none
```

Chinese documents
- [MAgRK](./.doc/magrk-zh/magrk.zh.md)


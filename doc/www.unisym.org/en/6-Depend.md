---
her-note: false
dg-publish: true
---

## Depend Map

`[A] [Aloc]`Means containing some functions that need to **allocate** the memory space. 


to renew :
```
graph LR
	%% UNISYM
	%% @ArinaMgk
	root[alice & aldbg]-->node[node:A]
	root-->cpuid[cpuid]
	root-->binary[binary]--bitref-->hash_crc64["crc64 [File]"]
	root-->mcore
	root--systemlib-->consio
	root-->datime
	root-->uctype-->ustring
	root-->dnode[dnode:A]--_Replace-->ustring
	ustring--_StrHeap-->inode:A
	ustring--_toktype-->tnode[tnode:A]-->ustring
	ustring-->coear
	root-->nnode:A
	root-->UPEC
	root-->strpool:A
	root-->stack:A
	root-->error
	root-->uassert
	root-->coear:A-->numar:A
	root-->regar
	root-->tenar:todo
	
	setjmp-->error
	node-->error
	stdio-->error
```


### core

```mermaid
graph LR
	alice[アリス]
	arch[archit.h]
	arch-->alice
	arch-->calling[call.h]
	arch-->flag.h
	proctrl[proctrl.h]
	proctrl-->floating.h-->integer.h-->arch
	proctrl-->uoption.h
	stdinc[stdinc.h]
	stdinc--"_BEBUG? +debug.h"-->proctrl
	unisym-->stdinc
```

### Bootstrapping Circuit

```mermaid
graph LR
	Mcca["Mecocoa(or other ENV)"]--MccaBuiltUnisym&MccaBuiltinMagiceChain-->MagiceChain--ENV=MCCA-->Mcca
```

### Arithmetic

---
her-note: false
dg-publish: true
charset: UTF-8
---

Syst = Host + Arch

Free-stand Environment:
- `Arch` (including the common `Core`)
- `Arith`
> You have to choose the architecture yourself, without the help of hosted characters.

Hosted Environment:
- `Host`
- ... (...)
- `OSDev`

Mixed Environment:
- ... (below are what to do)
- `Node` Node-Data-System
- `Grap` Graphics and Witch
- `Device`
- `Format`

## Depend Map

(Omit the common reference `Core`)

```mermaid
graph LR
	Host-->Arith

```
>Different from other graph: point to reference but dependence

---

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


### Host

Host = Arch(destination system), Core(common system) and Devk(developing system)

> 20240502: With the development of UNISYM, the including files of Core is bound to be complex and huge, it is necessary to ask users to use specific file but the only `stdinc`, more to see in `5-Compatible.md`. 

- [ ] make these true since next generation (Release-U2025XXX):
	- [ ] Arch + Devk --include--> Core

>Why there will be Archi and Devk?
>: For leak of determinism in language and compiler-system, e.g. the length of `int`, while Magice make these built-in rules so we need not provide `Syst` in UNISYM library but Magice-Compiler-Chain.

>Is Core dependent?
>: She is a abstract and independent completely, you can also take it `Orig` or `Love`❤.

>Why UNISYM update slowly and a bit complex?
>: There were few contributors and they were busy with living and feeding their families.

>"The code of UNISYM is so useless or ugly!" ?
>: It is your free to choose how to use. If YOU take anything for granted, there is nothing to say.

```mermaid
graph LR
	alice[アリス]
	arch[archit.h]
	arch-->alice
	arch-->calling[call.h]
	arch-->flag.h
	proctrl[proctrl.h]
	proctrl--+(x86.h,i8051.h...)-->floating.h-->integer.h-->arch
	proctrl-->uoption.h
	stdinc[stdinc.h]
	stdinc--"_BEBUG? +debug.h"-->proctrl
	unisym-->stdinc
```

Next generation

```mermaid
graph LR
	alice[CORE]
	ARCH[archit.h]
	ARCH-->alice
	ARCH-->flag.h
	ARCH-->InstInterface
	proctrl[proctrl.h]-->port/memory.h
	proctrl--+(x86.h,i8051.h...)-->ARCH-->dattype.h
	stdinc["RUNE stdinc(.h)"]
	stdinc--"_BEBUG? +debug.h"-->proctrl
	SPCF[specific]-->stdinc
	SPCF-->DEVK[devkit]-->DCOV[devcove.h]
```

Arch+Board+HostOS+Devkit

### Bootstrapping Building Circuit

```mermaid
graph LR
	Mcca["Mecocoa(or other ENV)"]--MccaBuiltUnisym&MccaBuiltinMagiceChain-->MagiceChain--ENV=MCCA-->Mcca
```

### Arithmetic

- CoeAr
- Builtin-floating functions



#!/bin/bash
# ASCII ShellScript TAB4 LF
# Attribute: 
# LastCheck: 
# AllAuthor: @dosconio
# ModuTitle: Makefile for Magice
# Copyright: ArinaMgk UniSym, Apache License Version 2.0

check()
{
	arguments="$1"
	expectation="$2"
	/mnt/hgfs/_bin/ELF64/mgc "$arguments" > ~/_obj/check.s || exit
	riscv64-unknown-linux-gnu-gcc -static -o ~/_obj/check ~/_obj/check.s
	qemu-riscv64 -L $RISCV/sysroot ~/_obj/check
	feekback="$?"

	if [ "$feekback" = "$expectation" ]; then
		echo "passed by $arguments !"
	else
		echo "$arguments failed for $feekback, Expecting $expectation"
		exit 1
	fi	
}

check 0 0
check 12 12
echo pass!

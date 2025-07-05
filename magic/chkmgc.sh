#!/bin/bash
# ASCII ShellScript TAB4 LF
# Attribute: 
# LastCheck: 
# AllAuthor: @dosconio
# ModuTitle: Makefile for Magice
# Copyright: ArinaMgk UniSym, Apache License Version 2.0
# Dependens: gcc-riscv64-linux-gnu-gcc (used riscv64-unknown-linux-gnu-gcc)

check()
{
	arguments="$1"
	expectation="$2"
	mgc "$arguments" > ~/_obj/check.s || exit
	riscv64-linux-gnu-gcc -static -o ~/_obj/check ~/_obj/check.s
	qemu-riscv64 -L $RISCV/sysroot ~/_obj/check
	feekback="$?"

	if [ "$feekback" = "$expectation" ]; then
		echo "passed by $arguments !"
	else
		echo "$arguments failed for $feekback, Expecting $expectation"
		exit 1
	fi	
}

cd ../
make magice
cd magic

# + and -
check '0;' 0
check '27;' 27
check '20 - 13 + 12;' 19

# pref+- and * and /
check '-1+1;' 0
check '1-8/(2*2)+3*6;' 17
check '- - 12* + - - + + +4;' 48

# == and !=
check '-5==-2-3;' 1
check '2*3==2+3;' 0
check '1!=0;'     1
check '2+2!=2*2;' 0

# signed < <= > >= by inst "SLT (I) (U)", where I for immediate, U for unsigned
check '2<3; ' 1
check '2<=3;' 1
check '3<=3;' 1
check '4<=3;' 0
#
check '-2>-3; ' 1
check '-2>=-3;' 1
check '-3>=-3;' 1
check '-4>=-3;' 0
#
check '-4>=-3==-0*-1;' 1
# Comma
check '1+2;;6;' 6
check '12+23;12+99/3;78-66;' 12
# Variabls
check 'foo=3; foo+1;' 4
check 'foo2=6; bar4=foo2+4; foo2=2; foo2+foo2*bar4;' 22
echo pass!

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
	mgc -emit-ir "$arguments" -o "$uobjpath/check.ir" || exit 1
	clover-riscv "$uobjpath/check.ir" -mbits=64 -minst=im -o "$uobjpath/check.s" || exit 1
	riscv64-linux-gnu-gcc -static -o "$uobjpath/check" "$uobjpath/check.s" || exit 1
	qemu-riscv64 -L "$RISCV/sysroot" "$uobjpath/check"
	feedback="$?"

	if [ "$feedback" = "$expectation" ]; then
		echo "passed by $arguments !"
	else
		echo "$arguments failed for $feedback, Expecting $expectation"
		exit 1
	fi	
}

cd ../
make magice
cd magic

# + and -
check 'main(){return 0;}' 0
check 'main(){return 27;}' 27
check 'main(){return 20 - 13 + 12;}' 19

# pref+- and * and /
check 'main(){return -1+1;}' 0
check 'main(){return 1-8/(2*2)+3*6;}' 17
check 'main(){return - - 12* + - - + + +4;}' 48

# == and !=
check 'main(){return -5==-2-3;}' 1
check 'main(){return 2*3==2+3;}' 0
check 'main(){return 1!=0;}'     1
check 'main(){return 2+2!=2*2;}' 0

# signed < <= > >= by inst "SLT (I) (U)", where I for immediate, U for unsigned
check 'main(){return 2<3; }' 1
check 'main(){return 2<=3;}' 1
check 'main(){return 3<=3;}' 1
check 'main(){return 4<=3;}' 0
#
check 'main(){return -2>-3; }' 1
check 'main(){return -2>=-3;}' 1
check 'main(){return -3>=-3;}' 1
check 'main(){return -4>=-3;}' 0
#
check 'main(){return -4>=-3==-0*-1;}' 1
# Comma
check 'main(){1+2;;return 6;}' 6
check 'main(){12+23;12+99/3; return 78-66;}' 12
# Variabls
check 'main(){foo=3; return foo+1;}' 4
check 'main(){foo2=6; bar4=foo2+4; foo2=2; return foo2+foo2*bar4;}' 22

# Return
check 'main(){return 1;2;4;}' 1
check 'main(){a=1;return 2; 4;}' 2
check 'main(){1; 2; return 4;}' 4

# Nested
check 'main(){{1;};;{}{b=2;} return 3;}' 3

# Statement
check 'main() { if (2-1) return 2; return(3); }' 2
check 'main() { if(1) {1;2;return 3;} else if (0) {4;return 5;6;}} else;' 3
check 'main() { for(;;) {return 1;} return 2;}' 1
check 'main() { i=0; j=0; for(i=0;i<10;i++) {j=j+1;} return j;} // comment-test' 10
check 'main() { i=1; while(i<10) i=i+i+1; } return i; /*comment-test*/ ' 15 # 1 -> 3 -> 7 -> 15

echo pass!

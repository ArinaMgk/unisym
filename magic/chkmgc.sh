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
	mgc --emit-ir "$arguments" -o "$uobjpath/check.ir" || exit 1
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
check 'int main(){return 0;}' 0
check 'int main(){return 27;}' 27
check 'int main(){return 20 - 13 + 12;}' 19

# pref+- and * and /
check 'int main(){return -1+1;}' 0
check 'int main(){return 1-8/(2*2)+3*6;}' 17
check 'int main(){return - - 12* + - - + + +4;}' 48

# == and !=
check 'int main(){return -5==-2-3;}' 1
check 'int main(){return 2*3==2+3;}' 0
check 'int main(){return 1!=0;}'     1
check 'int main(){return 2+2!=2*2;}' 0

# signed < <= > >= by inst "SLT (I) (U)", where I for immediate, U for unsigned
check 'int main(){return 2<3; }' 1
check 'int main(){return 2<=3;}' 1
check 'int main(){return 3<=3;}' 1
check 'int main(){return 4<=3;}' 0
#
check 'int main(){return -2>-3; }' 1
check 'int main(){return -2>=-3;}' 1
check 'int main(){return -3>=-3;}' 1
check 'int main(){return -4>=-3;}' 0
#
check 'int main(){return -4>=-3==-0*-1;}' 1
# Comma
check 'int main(){1+2;;return 6;}' 6
check 'int main(){12+23;12+99/3; return 78-66;}' 12
# Variabls
check 'int main(){int foo=3; return foo+1;}' 4
check 'int main(){int foo2=6; int bar4=foo2+4; foo2=2; return foo2+foo2*bar4;}' 22

# Return
check 'int main(){return 1;2;4;}' 1
check 'int main(){int a=1;return 2; 4;}' 2
check 'int main(){1; 2; return 4;}' 4

# Nested
check 'int main(){{1;};;{}{int b=2;} return 3;}' 3

# Statement
check 'int main() { if (2-1) return 2; return(3); }' 2
check 'int main() { if(1) {1;2;return 3;} else if (0) {4;return 5;6;} else;}' 3
check 'int main() { for(;;) {return 1;} return 2;}' 1
check 'int main() { int i=0; int j=0; for(i=0;i<10;i++) {j=j+1;} return j;} // comment-test' 10
check 'int main() { int i=1; while(i<10) i=i+i+1; return i; } /*comment-test*/ ' 15 # 1 -> 3 -> 7 -> 15

# Prefix * and &, and arithmetic for pointer
check 'int main() {int x=3; return *&x;}' 3
check 'int main() {int x=3; int *y=&x; int** z=&y; return **z;}' 3
check 'int main() {int x=3, y=5, z=3; return *(&y-1);}' 3 # not standard C

# Calling
check 'int f(int x) {return x+1;} int main() {int x = f(1); x++; return x;} ' 3
check 'int add(int x, int y) {return x+y;} int main() { return add(-1, 4);} ' 3

# Array
check 'int main() {int a[3]; a[0]=2; a[1]=3; a[2]=4; return a[0]+a[1]+a[2];}' 9
check 'int main() {int a[4]={1,2,3,4}; return a[2];}' 3
check 'int main() {int a[2][3]; a[0][1]=5; a[1][2]=7; return a[0][1]+a[1][2];}' 12

echo pass!

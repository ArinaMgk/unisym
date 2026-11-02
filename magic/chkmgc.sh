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
	frontend_option="$3"
	backend_inst="${4:-im}"
	backend_abi="${5:-lp64}"
	if [ -n "$frontend_option" ]; then
		mgc --emit-ir "$arguments" "$frontend_option" -o "$uobjpath/check.ir" || exit 1
	else
		mgc --emit-ir "$arguments" -o "$uobjpath/check.ir" || exit 1
	fi
	clover-riscv "$uobjpath/check.ir" -mbits=64 -minst="$backend_inst" \
		-mabi="$backend_abi" -o "$uobjpath/check.s" || exit 1
	riscv64-linux-gnu-gcc -static -o "$uobjpath/check" "$uobjpath/check.s" || exit 1
	qemu-riscv64 -L "$RISCV/sysroot" "$uobjpath/check"
	feedback="$?"

	if [ "$feedback" = "$expectation" ]; then
		printf '\033[1;32mpassed\033[0m: %s !\n' "$arguments"
	else
		echo "$arguments failed for $feedback, Expecting $expectation"
		exit 1
	fi	
}

check_fail()
{
	arguments="$1"
	expectation="$2"
	feedback="$(mgc --emit-ir "$arguments" 2>&1)"
	result="$?"
	if [ "$result" != 0 ] && printf '%s' "$feedback" | grep -Fq "$expectation"; then
		printf '\033[1;32mpassed\033[0m: rejected %s !\n' "$arguments"
	else
		printf '%s\n' "$feedback"
		echo "$arguments was not rejected with: $expectation"
		exit 1
	fi
}

cd ../
make magice
cd magic

# Operators
check 'int main(){return 0;}' 0
check '../demo/test/clover/c/operators.c' 75
# Expression statements
check 'int main(){1+2;;return 6;}' 6
check 'int main(){12+23;12+99/3; return 78-66;}' 12

# Comma operator
check 'int main(){return (1,2,3);}' 3
check 'int main(){return 1,2;}' 2
check 'int main(){int x=1; return (x=4,x+2);}' 6
check 'int main(){int x=(1,2),y=4; return x+y;}' 6
check 'int add(int x,int y){return x+y;} int main(){return add((1,2),3);}' 5
check 'int main(){int a[2]={(1,2),3}; return a[0]+a[1];}' 5

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
check '../demo/test/clover/c/loop.c' 28
check '../demo/test/clover/c/switch.c' 51
check_fail 'int main(){break;}' 'break statement not within a loop or switch'
check_fail 'int main(){continue;}' 'continue statement not within a loop'
check_fail 'int main(){switch(1){case 1:return 1;case 1:return 2;}}' 'duplicate case value'
check_fail 'int main(){switch(1){default:return 1;default:return 2;}}' 'multiple default labels in one switch'
check_fail 'int main(){case 1:return 2;}' 'case label not within a switch'

# Prefix * and &, and arithmetic for pointer
check 'int main() {int x=3; return *&x;}' 3
check 'int main() {int x=3; int *y=&x; int** z=&y; return **z;}' 3
check 'int main() {int x=3, y=5, z=3; return *(&y-1);}' 3 # not standard C

# Calling
check 'int f(int x) {return x+1;} int main() {int x = f(1); x++; return x;} ' 3
check 'int add(int x, int y) {return x+y;} int main() { return add(-1, 4);} ' 3

# Array
check '../demo/test/clover/c/array.c' 38
check_fail 'int main(){int values[];return 0;}' 'object has incomplete array type'

# Label and goto
check '../demo/test/clover/c/goto.c' 10
check_fail 'int main(){label:return 1;label:return 2;}' 'redefinition of label'
check_fail 'int main(){goto missing;return 0;}' 'use of undefined label'

# Basic Types
check 'int main() {char a=1; short b=2; long c=3; long long d=4; return a+b+c+d;}' 10
check 'int main(){_Bool a=0,b=2,c=-3; return a+b+c+sizeof(_Bool);}' 3
check '_Bool flag=7; _Bool pass(_Bool value){return value;} int main(){_Bool x=pass(3); return flag+x;}' 2
check 'int main() {return sizeof(long)+sizeof(void*);}' 16 '-Fdata-model=lp64'
check 'int main() {return sizeof(long)+sizeof(void*);}' 12 '-Fdata-model=llp64'
check 'int main(){return -1L < 1U;}' 1 '-Fdata-model=lp64'
check 'int main(){return -1L < 1U;}' 0 '-Fdata-model=llp64'
check 'int main(){float x=1.5f; double y=2.25; return (int)(x+y);}' 3 '' imfd
check 'int main(){float a[2]={0x1.4p0f,2.75f}; float *p=a; return (int)(p[0]+p[1]+sizeof(float)+sizeof(double));}' 16 '' imfd
check 'float add(float x,float y){return x+y;} double mix(float x,double y,int z){return add(x,1.5f)+y+z;} int main(){return (int)mix(1.25f,2.5,3);}' 8 '' imfd lp64d
check 'int main(){unsigned char a=255; short b=-1; unsigned int c=1; long d=-2; float f=1.5f; double g=2.5; unsigned long n=3; return (a+b==254)+(d+c==-1)+(int)(f+g)+(1<<n);}' 14 '' imfd lp64d
check 'int main() {return sizeof(char)+sizeof(short)+sizeof(int)+sizeof(long)+sizeof(long long);}' 23
check 'int main() {signed char x=-1; unsigned char y=255; y=y+2; return (x<0)+(y==1);}' 2
check 'int main() {unsigned int x=0; x=x-1; return x>1;}' 1
check 'int main() {unsigned int x=0; x=x-1; return x/16777216;}' 255
check 'int main() {int x=1; int n=sizeof(x++); return x*10+n;}' 14
check 'int main() {char x=1; return sizeof(x++);}' 1
check 'int main() {int a[2][3]; return sizeof(a)+sizeof(a[0])+sizeof(a[0][0]);}' 40
check 'int main() {return sizeof(int*)+sizeof(char[3]);}' 11
check 'int main() { return "abc"[0]; }' 97
check 'int main() { return ""[0]; }' 0
check 'int main() { return "\101"[0]; }' 65

# Integer and character literals
check "int main(){return 'A';}" 65
check "int main(){return '\\n'+'\\x41'+'\\101';}" 140
check 'int main(){return sizeof(1U)+sizeof(1L)+sizeof(1LL);}' 20 '-Fdata-model=lp64'
check 'int main(){return sizeof(1U)+sizeof(1L)+sizeof(1LL);}' 16 '-Fdata-model=llp64'
check 'int main(){return (0xFFFFFFFF>0)+(18446744073709551615ULL+2);}' 2
check 'int main(){int values[0x3U]; return sizeof(values);}' 12

# Enumeration
check '../demo/test/clover/c/enum.c' 66
check_fail 'enum E {A}; enum E {B}; int main(){return 0;}' 'redefinition of enum tag'
check_fail 'enum E {A,A}; int main(){return 0;}' 'redefinition of enumerator'
check_fail 'struct E {int value;}; enum E {A}; int main(){return 0;}' 'enum tag conflicts with record tag'
check_fail 'enum E {A}; int A; int main(){return 0;}' 'global variable conflicts with enumerator'

# Structure and union layout
check '../demo/test/clover/c/struct-layout.c' 254

# Storage Area and Lifetime
check 'char c; void func(int* j){*j=*j+1; c=0xFF;} int main() {int i=1; func(&i); return c + i;}' 1


echo pass!

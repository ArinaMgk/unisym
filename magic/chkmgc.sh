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

check_ir()
{
	arguments="$1"
	expectation="$2"
	feedback="$(mgc --emit-ir "$arguments")" || exit 1
	if printf '%s\n' "$feedback" | grep -Fq "$expectation"; then
		printf '\033[1;32mpassed\033[0m: IR contains %s !\n' "$expectation"
	else
		printf '%s\n' "$feedback"
		echo "IR did not contain: $expectation"
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
check '../demo/test/clover/c/array.c' 67
check 'int values[]; int main(){return sizeof(values);}' 4
check_fail 'int main(){int values[];return 0;}' 'object has incomplete array type'
check_fail 'int values[2]={1,2,3}; int main(){return 0;}' 'invalid or excessive global array initializer'

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
check '../demo/test/clover/c/floating.c' 77 '' imfd lp64d
check_ir 'int head(int x,...){return x;} int main(){return head(1,(char)2,3.0f);}' 'fpext f32'
check_ir 'int head(int x,...){return x;} int main(){return head(1,(char)2,3.0f);}' 'sext i8'
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
check "int main(){return \"\\x41\"[0]+\"\\101\"[0]+'\u0041';}" 195
check 'int main(){return "\u4E2D"[0]&255;}' 228
check_fail 'int main(){return "\x"[0];}' 'invalid string literal escape'
check_fail 'int main(){return "\u41"[0];}' 'invalid string literal escape'
check_fail 'int main(){return "\q"[0];}' 'invalid string literal escape'
check_fail 'int main(){return "\e"[0];}' 'invalid string literal escape'
check_fail 'int main(){return "\uD800"[0];}' 'invalid string literal escape'
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
check '../demo/test/clover/c/static.c' 167 '' imfd lp64d
check_ir 'static int value; static int helper(int x){return x;} int main(){return helper(value);}' 'internal global @value'
check_ir 'static int value; static int helper(int x){return x;} int main(){return helper(value);}' 'internal func @helper'
check_ir 'extern int value; int main(){return value;}' 'declare global @value i32'
check_fail 'int source(){return 1;} int main(){static int value=source();return value;}' 'block static initializer must be a constant expression'
check_fail 'static static int value; int main(){return value;}' 'duplicate storage-class specifier'
check_fail 'int value=1; int value=2; int main(){return value;}' 'redefinition of global variable'
check_fail 'extern int value; static int value; int main(){return value;}' 'global variable linkage conflicts'


echo pass!

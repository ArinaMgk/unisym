
#include "../../inc/cpp/cinc"
#include "../../inc/c/aldbg.h"
#include "../../inc/cpp/cinc"

#ifdef _DEBUG
//{TOIN} memory.cpp
void* operator new(size_t size) {
	return malc(size);
	// if (offss == nullptr) {
	// 	throw "Out of memory";
	// }
	// alice("Queue(limit)");
}
void* operator new[](size_t size) {
	return malc(size);
}
//{TOIN} memory.cpp
void operator delete(void* p) {
	memf(p);
}
void operator delete[](void* p) {
	memf(p);
}
// problem:
// [Qt can not use the management of ULIB]
// - 202404071700: if you use `delete new int;` in Qt-8-MinGW32, a malc_count will be left, but GCC and MSVC will not! You can use this to help: `delete (void*) new int;`
// - 202404071738 find: Qt call `delete[]` sometimes.
/*
#include<cpp/basic>
int main() {
	int* i = new int;
	//delete i;
	return malc_count;
}

...>echo %ERRORLEVEL%
0
...>cxc c.c
...>g++ c.c -IE:\PROJ\SVGN\unisym\inc\c\ -IE:\PROJ\SVGN\unisym\inc\ -D_DEBUG -LE:\PROJ\SVGN\_bin -lw32d
...>a
...>echo %ERRORLEVEL%
1
*/

#endif

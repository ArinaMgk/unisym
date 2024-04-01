
#include "../../inc/cpp/cinc"
#include "../../inc/c/aldbg.h"
#include "../../inc/cpp/cinc"

#ifdef _DEBUG

void* operator new(size_t size) {
	void* ret = malc(size);
	return ret;
}

void operator delete(void* p) {
	memf(p);
}

#endif

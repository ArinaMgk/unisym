
#if defined(_MCCA) || defined(_ACCM)
#include "aaaaa.h"
#include <c/ustring.h>
// sleep [int]
// : sleep [int] seconds
int main(int argc, char** argv) {
	auto sec = atoins(argv[1]);
	if (sec && argc > 1) sysrest(0 _Comment(s), sec);// REST
}
#endif

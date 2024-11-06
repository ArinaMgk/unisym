
#ifndef _INC_INTEGER_PREF_ABBR
#define _INC_INTEGER_PREF_ABBR
	#include "../stdinc.h"
	#if !defined(uint)&&!defined(_Linux)&&!defined(_NO_uint)// avoid GCC duplicate 'unsigned'
		#define uint unsigned int
	#endif
	#define llong long long int 
	#define sll signed long long int
	#define ullong unsigned long long int
	#define ull unsigned long long int
	

#endif

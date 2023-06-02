#ifndef ArnHabitDbg
#define ArnHabitDbg


#ifdef _dbg
	#define memalloc(dest,size)\
		(*(char**)&dest=(char*)malloc(size))?((void)malc_count++):(erro("MEMORY RUN OUT!"),(void)0)
	#define memfree(x) if(x){free((char*)(x));malc_count--;}// RFW21 version
	#define srs(x,y) {void*ebx =(y);if(x)free((void*)x);malc_count--;(x)=ebx;}
#else
	#define memalloc(dest,size)\
		(*(char**)&dest=(char*)malloc(size))?((void)0):(erro("MEMORY RUN OUT!"),(void)0)
	#define memfree(x) {if(x)free((char*)(x));}
	#define srs(x,y) {char*ebx =(y);if(x)free((char*)x);(x)=ebx;}
#endif


#define init_total_errmech() \
	if (setjmp(errjb))\
	{\
		if (SGAErroMsg && *SGAErroMsg)\
			fprintf(stderr, "!Err %s\n", SGAErroMsg);\
		if(malc_count) fprintf(stderr, "malc_count = %"PRIXPTR"\n", malc_count);\
		if(mode=='c')system("pause");\
		return 1;\
	}

extern void erro(char*);
extern void warn(char*);
extern size_t malc_count;

#endif

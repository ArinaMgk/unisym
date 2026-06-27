	int i1 = N;        // definition, external linkage
	static int i2 = N; // definition, internal linkage
	extern int i3 = N; // definition, external linkage
	int i4;            // tentative , external linkage
	static int i5;     // tentative definition, internal linkage
	int i1;            // refers to line-1 (valid tentative definition)
	int i2;            // BAD (error in GCC, but pass for MSVC)
	int i3;            // refers to line-3
	int i4;            // refers to line-4
	int i5;            // BAD (like line-7)
	extern int i1;     // refers to previous external
	extern int i2;     // refers to previous internal
	extern int i3;     // refers to previous external
	extern int i4;     // refers to previous external
	extern int i5;     // refers to previous internal
	// line-7 error: non-static declaration of 'i2' follows static declaration
	// line-8 warning: 'i3' initialized and declared 'extern'

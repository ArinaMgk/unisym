	// SUBEXAMPLE 1 - MIXED ORDER OF PREFIX @ARINA
	int unsigned const static p = 0;
	static long const signed long int q = 1;
	// SUBEXAMPLE 2 - @ARINA YO REC20
	int x, const y = 6, z = 2;// not error in MSVC
	x = 1; y = 2; z = 3;// 'const' is omitted

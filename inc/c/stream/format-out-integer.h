	asrtret(base >= 2 || base == -16);
	// ---- COMMON ----
	char buf[bitsof(val) + 1] = { 0 };// asci_zero 
	char* bufp = buf + sizeof(buf) - 1;
	auto neg = sign_have && (*(stdsint*)&val) < 0;
	char pad = zero_padding ? '0' : ' ';
	char* alnum_tab = base == -16 ? _tab_hexa : _tab_HEXA;
	if (neg) {
		val = ~val + 1;
		sign_show = 1;
	}
	if (_IMM1S(bytexpo) < byteof(val)) // use if to avoid MSVC ROL discovered at 20250227
		val &= _IMM1S(_IMM1S(bytexpo + 3)) - 1;// <=> case 0: val = (uint8)val; break ...
	// #if defined(_MCCA) && _MCCA == 0x8632
	// uint64 valquo = 0, valrem = 0;
	// do {
	// 	valquo = udivmoddi4(val, base, &valrem);
	// 	*--bufp = _tab_HEXA[valrem];
	// } while (val = valquo);
	// #else
	do *--bufp = _tab_HEXA[val % base]; while (val /= base);
	// #endif
	// ---- PRINT ----
	if (sign_show) out(neg ? "-" : "+", 1);
	stdsint num_to_print = StrLength(bufp);
	if (least_digits > num_to_print) for0(i, least_digits - num_to_print) out(&pad, 1);
	out(bufp, num_to_print);
	return true;

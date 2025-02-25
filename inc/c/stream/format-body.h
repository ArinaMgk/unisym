#if !defined(_STREAM_FORMAT_C) && !defined(_STREAM_FORMAT_CPP)
#include "../consio.h"
int outsfmtlst(outbyte_t local_out, const char* fmt, para_list paras) {
#endif

	int i;
	byte c;
	char* s;

	//     % +- ent . end (h/l) sym
	unsigned tmp_base = 16;
	char tmp_signed = 0;// +-
	char tmp_percent_feed = 0;
	// 0 float, 1 double, 2 long double
	// -2 byte, -1 short, 0 int, 1 long, 2 long long
	char sizlevel = 0;
	stduint ent = 0;
	stduint end = 0;
	char dotted = 0;

	if (fmt == 0) return 0;

	for (i = 0; tmp_percent_feed || (c = fmt[i]); ) {
		if (c != '%') {
			stduint len = 1;
			const char* q = fmt + i;
			while (q[len] && (q[len] != '%')) len++;
			localout(&fmt[i], len);
			i += len;
			continue;
		}
		if (!tmp_percent_feed) i++; else tmp_percent_feed = 0;
		c = fmt[i];
		if (c == 0)
			break;
		switch (c) {
		case 'c':
			outc(para_next_char(paras));
			break;

			// ---- SIGN & SIZE SWITCH ----
		case '+':// '-' for alignment
			tmp_signed = 1;
			c = '%'; tmp_percent_feed = 1;
			break;
		// '-'
		case 'l':// no mixed with 'h'
			if (sizlevel >= 0) sizlevel++;
			c = '%'; tmp_percent_feed = 1;
			break;
		case 'h':
			if (sizlevel <= 0) sizlevel--;
			c = '%'; tmp_percent_feed = 1;
			break;


			// ---- INTEGER ---- [signed] [base] ...

		case 'b':// alicee extend
			tmp_base = 2;
			goto case_integer;
		case 'o':
			tmp_base = 8;
			goto case_integer;
		case 'd':
			tmp_base = 10;
		case_integer:
			#if !defined(_STREAM_FORMAT_CPP)
			outi(pnext(int), tmp_base, tmp_signed);
			tmp_signed = 0;
			#endif
			break;
		case 'x':
			tmp_base = 16;
		case_unteger:
			#if !defined(_STREAM_FORMAT_CPP)
			outu(pnext(int), tmp_base);
			#endif
			break;



		case 'f':
			if (sizlevel == 1)
				outfloat(pnext(double));
			else if (sizlevel == 0)
				outfloat(pnext(float));
			sizlevel = 0;
			break;
		case 'p':
			localout("0x", 2);
			// typeid
		#if __BITS__ == 64
			outi64hex(pnext(stduint));
		#elif __BITS__ == 32
			outi32hex(pnext(stduint));
		#elif __BITS__ == 16
			outi16hex(pnext(stduint));
		#elif __BITS__ == 8
			outi8hex(pnext(stduint));
		#else
		#error "Unsupport bits"
		#endif
			break;
		case 's':
			s = pnext(char*);
			if (!s) s = (char*)"(null)";
			outs(s);
			break;
		case '%':
			c = '%';
			localout(&fmt[i], 1);
			break;
		case '[': // alicee extend
			if (!StrCompareN(fmt + i, "[2c]", 4)) // Print 2-byte Character
			{
				uint16 tmp = pnext(uint16);
				const char* ptmp = (char*)&tmp;
				outc(ptmp[0]);
				outc(ptmp[1]);
				i += 4 - 1;
			}
			else if (!StrCompareN(fmt + i, "[u]", 3)) // Print Decimal STDUINT
			{
				outu(pnext(stduint), 10);
				i += 3 - 1;
			}
			else if (!StrCompareN(fmt + i, "[i]", 3)) // Print Decimal STDSINT
			{
				stdsint tmp = pnext(stdsint);
				outi(tmp, 10, tmp < 0);
				i += 3 - 1;
			}
			else if (!StrCompareN(fmt + i, "[8H]", 4)) // Print Hex STDUINT 8 bit
			{
				outi8hex(pnext(uint8));
				i += 4 - 1;
			}
			else if (!StrCompareN(fmt + i, "[16H]", 5)) // Print Hex STDSINT 16 bit
			{
				outi16hex(pnext(uint16));
				i += 5 - 1;
			}
			else if (!StrCompareN(fmt + i, "[32H]", 5)) // Print Hex STDSINT 32 bit
			{
				outi32hex(pnext(uint32));
				i += 5 - 1;
			}
			// ...
		#if defined(_BIT_SUPPORT_64)
			// Print Decimal 64 bit
			else if (!StrCompareN(fmt + i, "[64I]", 5)) {
				outi64(pnext(int64), 10, 0);
				i += 5 - 1;
			}
		#endif
			break;
		default:
			localout(&fmt[i], 1);
			break;
		}
		i++;
	}

#if !defined(_STREAM_FORMAT_C) && !defined(_STREAM_FORMAT_CPP)
}

#endif

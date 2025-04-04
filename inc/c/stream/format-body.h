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
		//{TODO} use sizlevel
		case 'o':
			out_integer(pnext(unsigned), 8, tmp_signed, true, _TODO 0, _TODO false, intlog2_iexpo(byteof(unsigned)));
			break;
		case 'd':
			out_integer(pnext(signed), 10, tmp_signed, true, _TODO 0, _TODO false, intlog2_iexpo(byteof(signed)));
			break;

		// ---- UNSIGNED ----
		case 'b':// alicee extend
			out_integer(pnext(unsigned), 2, false, false, _TODO 0, _TODO false, intlog2_iexpo(byteof(unsigned)));
			break;
		case 'x':
			out_integer(pnext(unsigned), -16, false, false, _TODO 0, _TODO false, intlog2_iexpo(byteof(unsigned)));
			break;
		case 'u':
			out_integer(pnext(unsigned), 10, false, false, _TODO 0, _TODO false, intlog2_iexpo(byteof(unsigned)));
			break;



		case 'f':
			if (sizlevel == 1)
				out_floating(pnext(double));
			else if (sizlevel == 0)
				out_floating(pnext(float));
			sizlevel = 0;
			break;
		case 'p':
			localout("0x", 2);
			out_integer(pnext(stduint), -16, false, false, __BITS__ / 4, true, intlog2_iexpo(byteof(stduint)));
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
				stduint uuu = pnext(stduint);
				out_integer(uuu, 10, false, false, 0, false, intlog2_iexpo(byteof(stduint)));
				i += 3 - 1;
			}
			else if (!StrCompareN(fmt + i, "[i]", 3)) // Print Decimal STDSINT
			{
				out_integer(pnext(stdsint), 10, false, true, 0, false, intlog2_iexpo(byteof(stdsint)));
				i += 3 - 1;
			}
			else if (!StrCompareN(fmt + i, "[8H]", 4)) // Print Hex STDUINT 8 bit
			{
				out_integer(pnext(uint8), 16, false, false, byteof(uint8) << 1, true, intlog2_iexpo(byteof(uint8)));
				i += 4 - 1;
			}
			else if (!StrCompareN(fmt + i, "[16H]", 5)) // Print Hex STDSINT 16 bit
			{
				out_integer(pnext(uint16), 16, false, false, byteof(uint16) << 1, true, intlog2_iexpo(byteof(uint16)));
				i += 5 - 1;
			}
			else if (!StrCompareN(fmt + i, "[32H]", 5)) // Print Hex STDSINT 32 bit
			{
				out_integer(pnext(uint32), 16, false, false, byteof(uint32) << 1, true, intlog2_iexpo(byteof(uint32)));
				i += 5 - 1;
			}
		#if defined(_BIT_SUPPORT_64)
			else if (!StrCompareN(fmt + i, "[64H]", 5)) // Print Hex STDSINT 64 bit
			{
				out_integer(pnext(uint64), 16, false, false, byteof(uint64) << 1, true, intlog2_iexpo(byteof(uint64)));
				i += 5 - 1;
			}
			else if (!StrCompareN(fmt + i, "[64I]", 5)) {
				out_integer(pnext(sint64), 10, false, true, 0, true, intlog2_iexpo(byteof(sint64)));
				i += 5 - 1;
			}
		#endif
			break;
		default:
			localout(&fmt[i], 1);
			break;
		}
		if (tmp_percent_feed != 1) {
			tmp_signed = 0;
		}
		i++;
	}

#if !defined(_STREAM_FORMAT_C) && !defined(_STREAM_FORMAT_CPP)
}

#endif

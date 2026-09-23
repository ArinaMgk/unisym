// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant CPU(80586+)
// AllAuthor: @ArinaMgk
// ModuTitle: Generator for trigonometric lookup tables
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRIGTAB_PI 3.14159265358979323846264338327950288419716939937510582097494459

static long double make_sin(long double x) {
	long double term = x;
	long double sum = x;
	for (int n = 1; n < 24; n++) {
		long double a = (long double)(2 * n);
		long double b = (long double)(2 * n + 1);
		term *= -x * x / (a * b);
		sum += term;
	}
	return sum;
}

static void emit_float_table(FILE* dest_file, int n) {
	fprintf(dest_file, "#if defined(_MCU_STM32)\n");
	fprintf(dest_file, "const Trigtab _tab_sin_quarter[TRIGTAB_QUARTER_N + 1] = {\n");
	for (int i = 0; i <= n; i++) {
		double v = (double)make_sin(((long double)TRIGTAB_PI * (long double)i) / (2.0L * (long double)n));
		if ((i & 3) == 0) fprintf(dest_file, "\t");
		fprintf(dest_file, "%.9eF", v);
		if (i != n) fprintf(dest_file, ",");
		if ((i & 3) == 3 || i == n) fprintf(dest_file, "\n");
		else fprintf(dest_file, " ");
	}
	fprintf(dest_file, "};\n");
}

static void emit_double_table(FILE* dest_file, int n) {
	fprintf(dest_file, "#else\n");
	fprintf(dest_file, "const Trigtab _tab_sin_quarter[TRIGTAB_QUARTER_N + 1] = {\n");
	for (int i = 0; i <= n; i++) {
		double v = (double)make_sin(((long double)TRIGTAB_PI * (long double)i) / (2.0L * (long double)n));
		if ((i & 3) == 0) fprintf(dest_file, "\t");
		fprintf(dest_file, "%.17e", v);
		if (i != n) fprintf(dest_file, ",");
		if ((i & 3) == 3 || i == n) fprintf(dest_file, "\n");
		else fprintf(dest_file, " ");
	}
	fprintf(dest_file, "};\n");
	fprintf(dest_file, "#endif\n");
}

int main()
{
	const char* ulibpath = getenv("ulibpath");
	char dest_filename[128];
	const char copyright_info[] = "http://unisym.org/license.html";

	if (!ulibpath) {
		printf("Error: ulibpath is not set\n");
		return 1;
	}
	strcpy(dest_filename, ulibpath);
	strcat(dest_filename, "/c/auxiliary/trigtab.c");
	FILE* dest_file = fopen(dest_filename, "wb");
	if (dest_file == NULL) {
		printf("Error: Cannot open file %s\n", dest_filename);
		return 1;
	}
	fprintf(dest_file, "// ASCII C99 TAB4 LF\n");
	fprintf(dest_file, "// Docutitle: Quarter-wave sine table for trigonometric reduction\n");
	fprintf(dest_file, "// Codifiers: @ArinaMgk\n");
	fprintf(dest_file, "// Attribute: Origin(trigtab.make.c)\n");
	fprintf(dest_file, "// OpLicense: %s\n", copyright_info);
	fprintf(dest_file, "\n");
	fprintf(dest_file, "#include \"../../../inc/c/auxiliary/trigtab.h\"\n\n");
	emit_float_table(dest_file, 128);
	fprintf(dest_file, "\n");
	emit_double_table(dest_file, 256);
	fprintf(dest_file, "\n");
	fclose(dest_file);
	return 0;
}

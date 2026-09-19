// ASCII C99 TAB4 CRLF
// Docutitle: (Algorithm) Discrete Cosine Transform (DCT / IDCT)
// Codifiers: @dosconio, @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
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

#include "../../../inc/c/algorithm/dct.h"
#include "../../../inc/c/arith.h"
#include <stdlib.h>

// 8x8 Fixed-point Cosine Transform Table (IEEE-1180 precision, scaled by 2048)
static const int s_cos_tab8[8][8] = {
	{  724,  1004,   946,   851,   724,   569,   392,   200 },
	{  724,   851,   392,  -200,  -724, -1004,  -946,  -569 },
	{  724,   569,  -392, -1004,  -724,   200,   946,   851 },
	{  724,   200,  -946,  -569,   724,   851,  -392, -1004 },
	{  724,  -200,  -946,   569,   724,  -851,  -392,  1004 },
	{  724,  -569,  -392,  1004,  -724,  -200,   946,  -851 },
	{  724,  -851,   392,   200,  -724,  1004,  -946,   569 },
	{  724, -1004,   946,  -851,   724,  -569,   392,  -200 }
};

// 4x4 Fixed-point Cosine Transform Table (scaled by 2048)
static const int s_cos_tab4[4][4] = {
	{ 1024,  1338,  1024,   554 },
	{ 1024,   554, -1024, -1338 },
	{ 1024,  -554, -1024,  1338 },
	{ 1024, -1338,  1024,  -554 }
};

// Fast 8x8 2D-IDCT
static void IDCT8x8(stdsint block[]) {
	stdsint temp[64];
	// Row 1D-IDCT
	for (int i = 0; i < 8; ++i) {
		const stdsint* in = block + i * 8;
		stdsint* out = temp + i * 8;
		for (int j = 0; j < 8; ++j) {
			stdsint sum = 0;
			for (int k = 0; k < 8; ++k) {
				sum += (stdsint)(in[k] * (stdsint)s_cos_tab8[j][k]);
			}
			out[j] = (stdsint)((sum + 1024) >> 11);
		}
	}
	// Column 1D-IDCT
	for (int j = 0; j < 8; ++j) {
		for (int i = 0; i < 8; ++i) {
			stdsint sum = 0;
			for (int k = 0; k < 8; ++k) {
				sum += (stdsint)(temp[k * 8 + j] * (stdsint)s_cos_tab8[i][k]);
			}
			block[i * 8 + j] = (stdsint)((sum + 1024) >> 11);
		}
	}
}

// Fast 8x8 2D-DCT
static void DCT8x8(stdsint block[]) {
	stdsint temp[64];
	// Row 1D-DCT
	for (int i = 0; i < 8; ++i) {
		const stdsint* in = block + i * 8;
		stdsint* out = temp + i * 8;
		for (int j = 0; j < 8; ++j) {
			stdsint sum = 0;
			for (int k = 0; k < 8; ++k) {
				sum += (stdsint)(in[k] * (stdsint)s_cos_tab8[k][j]);
			}
			out[j] = (stdsint)((sum + 1024) >> 11);
		}
	}
	// Column 1D-DCT
	for (int j = 0; j < 8; ++j) {
		for (int i = 0; i < 8; ++i) {
			stdsint sum = 0;
			for (int k = 0; k < 8; ++k) {
				sum += (stdsint)(temp[k * 8 + j] * (stdsint)s_cos_tab8[k][i]);
			}
			block[i * 8 + j] = (stdsint)((sum + 1024) >> 11);
		}
	}
}

// Fast 4x4 2D-IDCT
static void IDCT4x4(stdsint block[]) {
	stdsint temp[16];
	// Row 1D-IDCT
	for (int i = 0; i < 4; ++i) {
		const stdsint* in = block + i * 4;
		stdsint* out = temp + i * 4;
		for (int j = 0; j < 4; ++j) {
			stdsint sum = 0;
			for (int k = 0; k < 4; ++k) {
				sum += (stdsint)(in[k] * (stdsint)s_cos_tab4[j][k]);
			}
			out[j] = (stdsint)((sum + 1024) >> 11);
		}
	}
	// Column 1D-IDCT
	for (int j = 0; j < 4; ++j) {
		for (int i = 0; i < 4; ++i) {
			stdsint sum = 0;
			for (int k = 0; k < 4; ++k) {
				sum += (stdsint)(temp[k * 4 + j] * (stdsint)s_cos_tab4[i][k]);
			}
			block[i * 4 + j] = (stdsint)((sum + 1024) >> 11);
		}
	}
}

// Fast 4x4 2D-DCT
static void DCT4x4(stdsint block[]) {
	stdsint temp[16];
	// Row 1D-DCT
	for (int i = 0; i < 4; ++i) {
		const stdsint* in = block + i * 4;
		stdsint* out = temp + i * 4;
		for (int j = 0; j < 4; ++j) {
			stdsint sum = 0;
			for (int k = 0; k < 4; ++k) {
				sum += (stdsint)(in[k] * (stdsint)s_cos_tab4[k][j]);
			}
			out[j] = (stdsint)((sum + 1024) >> 11);
		}
	}
	// Column 1D-DCT
	for (int j = 0; j < 4; ++j) {
		for (int i = 0; i < 4; ++i) {
			stdsint sum = 0;
			for (int k = 0; k < 4; ++k) {
				sum += (stdsint)(temp[k * 4 + j] * (stdsint)s_cos_tab4[k][i]);
			}
			block[i * 4 + j] = (stdsint)((sum + 1024) >> 11);
		}
	}
}

void IDCT_Transform(stdsint block[], unsigned n) {
	if (!block || n == 0) return;
	if (n == 8) {
		IDCT8x8(block);
		return;
	}
	if (n == 4) {
		IDCT4x4(block);
		return;
	}

	size_t total_elements = (size_t)n * n;
	stdsint* temp = (stdsint*)malloc(total_elements * sizeof(stdsint));
	int* cos_mat = (int*)malloc(total_elements * sizeof(int));
	if (!temp || !cos_mat) {
		if (temp) free(temp);
		if (cos_mat) free(cos_mat);
		return;
	}

	double sqrt_1_n = dblsqrt(1.0 / (double)n);
	double sqrt_2_n = dblsqrt(2.0 / (double)n);

	for (unsigned x = 0; x < n; ++x) {
		for (unsigned u = 0; u < n; ++u) {
			double c_u = (u == 0) ? sqrt_1_n : sqrt_2_n;
			double angle = ((2.0 * x + 1.0) * (double)u * _VAL_PI) / (2.0 * (double)n);
			double val = c_u * dblcos(angle) * 2048.0;
			cos_mat[x * n + u] = (int)(val >= 0 ? (val + 0.5) : (val - 0.5));
		}
	}

	for (unsigned i = 0; i < n; ++i) {
		const stdsint* in = block + i * n;
		stdsint* out = temp + i * n;
		for (unsigned j = 0; j < n; ++j) {
			stdsint sum = 0;
			const int* mat_row = cos_mat + j * n;
			for (unsigned k = 0; k < n; ++k) {
				sum += (stdsint)(in[k] * (stdsint)mat_row[k]);
			}
			out[j] = (stdsint)((sum + 1024) >> 11);
		}
	}

	for (unsigned j = 0; j < n; ++j) {
		for (unsigned i = 0; i < n; ++i) {
			stdsint sum = 0;
			const int* mat_row = cos_mat + i * n;
			for (unsigned k = 0; k < n; ++k) {
				sum += (stdsint)(temp[k * n + j] * (stdsint)mat_row[k]);
			}
			block[i * n + j] = (stdsint)((sum + 1024) >> 11);
		}
	}

	free(cos_mat);
	free(temp);
}

void DCT_Transform(stdsint block[], unsigned n) {
	if (!block || n == 0) return;
	if (n == 8) {
		DCT8x8(block);
		return;
	}
	if (n == 4) {
		DCT4x4(block);
		return;
	}

	size_t total_elements = (size_t)n * n;
	stdsint* temp = (stdsint*)malloc(total_elements * sizeof(stdsint));
	int* cos_mat = (int*)malloc(total_elements * sizeof(int));
	if (!temp || !cos_mat) {
		if (temp) free(temp);
		if (cos_mat) free(cos_mat);
		return;
	}

	double sqrt_1_n = dblsqrt(1.0 / (double)n);
	double sqrt_2_n = dblsqrt(2.0 / (double)n);

	for (unsigned x = 0; x < n; ++x) {
		for (unsigned u = 0; u < n; ++u) {
			double c_u = (u == 0) ? sqrt_1_n : sqrt_2_n;
			double angle = ((2.0 * x + 1.0) * (double)u * _VAL_PI) / (2.0 * (double)n);
			double val = c_u * dblcos(angle) * 2048.0;
			cos_mat[x * n + u] = (int)(val >= 0 ? (val + 0.5) : (val - 0.5));
		}
	}

	for (unsigned i = 0; i < n; ++i) {
		const stdsint* in = block + i * n;
		stdsint* out = temp + i * n;
		for (unsigned j = 0; j < n; ++j) {
			stdsint sum = 0;
			for (unsigned k = 0; k < n; ++k) {
				sum += (stdsint)(in[k] * (stdsint)cos_mat[k * n + j]);
			}
			out[j] = (stdsint)((sum + 1024) >> 11);
		}
	}

	for (unsigned j = 0; j < n; ++j) {
		for (unsigned i = 0; i < n; ++i) {
			stdsint sum = 0;
			for (unsigned k = 0; k < n; ++k) {
				sum += (stdsint)(temp[k * n + j] * (stdsint)cos_mat[k * n + i]);
			}
			block[i * n + j] = (stdsint)((sum + 1024) >> 11);
		}
	}

	free(cos_mat);
	free(temp);
}

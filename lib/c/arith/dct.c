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

// --- H.264 / AVC Integer & Hadamard Transforms ---

// The three inverse-scaling helpers below fold the "1 << (qP/6)" factor of the H.264
// dequantiser into a single right shift, so that their output feeds IDCT_H264_4x4()
// (which applies the inverse transform's "(x + 32) >> 6") in the right domain.
// These are the *net* right shifts, i.e. the value handed to the inverse transform is
//   coeff * normAdjust[qP%6][class] * 2^(qP/6) / 2^SHIFT.
// Measured on wind.mp4 (a flat saturated-yellow IDR at slice_qp 10): with SHIFT 6/6/5 the
// reconstructed frame came out a desaturated grey (RGB 139,138,129); the correct scale is
// 2^4 larger, which restores the source colour (RGB 252,223,89). That pins the two DC paths
// to 2 / 1.
//
// The AC path is NOT pinned by a flat frame (it had no luma AC coefficients at all), and the
// relative spacing between the paths is fixed by the reference encoder: x264's quant.c uses
//   dequant_4x4    : i_qbits = qp/6 - 4
//   dequant_4x4_dc : i_qbits = qp/6 - 6
//   dequant_8x8    : i_qbits = qp/6 - 6
// over the same dequant_mf and the same "(x + 32) >> 6" inverse transform, so the 4x4 AC
// hand-off must be 2^2 LARGER than the luminance DC one, i.e. SHIFT_AC = SHIFT_DC - 2 = 0.
// That also matches H264_Dequant8x8() below, which hands its coefficients straight to
// H264_IDCT8x8() with no shift at all while using the same normAdjust magnitudes.
// With the old value of 2 every 4x4 AC residual was 4x too small, which visibly flattened
// the textured regions (a reference-comparison showed the I4x4 band of the IDR smoothed out).
#define H264_DQ_SHIFT_AC  0   // 4x4 luma / chroma AC coefficients
#define H264_DQ_SHIFT_DC  2   // Intra16x16 luminance DC (4x4 Hadamard output)
#define H264_DQ_SHIFT_CDC 1   // chroma DC (2x2 Hadamard output)

static const int32 s_h264_dequant_v[6][3] = {
	{ 10, 16, 13 },
	{ 11, 18, 14 },
	{ 13, 20, 16 },
	{ 14, 23, 18 },
	{ 16, 25, 20 },
	{ 18, 29, 23 }
};

void IDCT_H264_4x4(const stdsint in_scaled[16], stdsint out_residual[16]) {
	if (!in_scaled || !out_residual) return;

	stdsint e[4][4];
	for (int i = 0; i < 4; ++i) {
		stdsint d0 = in_scaled[i * 4 + 0];
		stdsint d1 = in_scaled[i * 4 + 1];
		stdsint d2 = in_scaled[i * 4 + 2];
		stdsint d3 = in_scaled[i * 4 + 3];

		stdsint e0 = d0 + d2;
		stdsint e1 = d0 - d2;
		stdsint e2 = (d1 >> 1) - d3;
		stdsint e3 = d1 + (d3 >> 1);

		e[i][0] = e0 + e3;
		e[i][1] = e1 + e2;
		e[i][2] = e1 - e2;
		e[i][3] = e0 - e3;
	}

	for (int j = 0; j < 4; ++j) {
		stdsint f0 = e[0][j];
		stdsint f1 = e[1][j];
		stdsint f2 = e[2][j];
		stdsint f3 = e[3][j];

		stdsint g0 = f0 + f2;
		stdsint g1 = f0 - f2;
		stdsint g2 = (f1 >> 1) - f3;
		stdsint g3 = f1 + (f3 >> 1);

		out_residual[0 * 4 + j] = (g0 + g3 + 32) >> 6;
		out_residual[1 * 4 + j] = (g1 + g2 + 32) >> 6;
		out_residual[2 * 4 + j] = (g1 - g2 + 32) >> 6;
		out_residual[3 * 4 + j] = (g0 - g3 + 32) >> 6;
	}
}

void Hadamard_H264_4x4(const stdsint in_dc[16], int qp, stdsint out_scaled_dc[16]) {
	if (!in_dc || !out_scaled_dc) return;

	stdsint h[4][4];
	for (int i = 0; i < 4; ++i) {
		stdsint c0 = in_dc[i * 4 + 0];
		stdsint c1 = in_dc[i * 4 + 1];
		stdsint c2 = in_dc[i * 4 + 2];
		stdsint c3 = in_dc[i * 4 + 3];

		stdsint h0 = c0 + c2;
		stdsint h1 = c0 - c2;
		stdsint h2 = c1 - c3;
		stdsint h3 = c1 + c3;

		h[i][0] = h0 + h3;
		h[i][1] = h1 + h2;
		h[i][2] = h1 - h2;
		h[i][3] = h0 - h3;
	}

	stdsint v00 = (stdsint)s_h264_dequant_v[qp % 6][0];
	int qp_div = qp / 6;

	for (int j = 0; j < 4; ++j) {
		stdsint f0 = h[0][j];
		stdsint f1 = h[1][j];
		stdsint f2 = h[2][j];
		stdsint f3 = h[3][j];

		stdsint g0 = f0 + f2;
		stdsint g1 = f0 - f2;
		stdsint g2 = f1 - f3;
		stdsint g3 = f1 + f3;

		stdsint d0 = g0 + g3;
		stdsint d1 = g1 + g2;
		stdsint d2 = g1 - g2;
		stdsint d3 = g0 - g3;

		if (qp_div >= H264_DQ_SHIFT_DC) {
			out_scaled_dc[0 * 4 + j] = (d0 * v00) << (qp_div - H264_DQ_SHIFT_DC);
			out_scaled_dc[1 * 4 + j] = (d1 * v00) << (qp_div - H264_DQ_SHIFT_DC);
			out_scaled_dc[2 * 4 + j] = (d2 * v00) << (qp_div - H264_DQ_SHIFT_DC);
			out_scaled_dc[3 * 4 + j] = (d3 * v00) << (qp_div - H264_DQ_SHIFT_DC);
		} else {
			int shift = H264_DQ_SHIFT_DC - qp_div;
			stdsint add = (stdsint)1 << (shift - 1);
			out_scaled_dc[0 * 4 + j] = (d0 * v00 + add) >> shift;
			out_scaled_dc[1 * 4 + j] = (d1 * v00 + add) >> shift;
			out_scaled_dc[2 * 4 + j] = (d2 * v00 + add) >> shift;
			out_scaled_dc[3 * 4 + j] = (d3 * v00 + add) >> shift;
		}
	}
}

void Hadamard_H264_2x2(const stdsint in_dc[4], int qp, stdsint out_scaled_dc[4]) {
	if (!in_dc || !out_scaled_dc) return;

	stdsint c00 = in_dc[0];
	stdsint c01 = in_dc[1];
	stdsint c10 = in_dc[2];
	stdsint c11 = in_dc[3];

	stdsint f00 = c00 + c01;
	stdsint f01 = c00 - c01;
	stdsint f10 = c10 + c11;
	stdsint f11 = c10 - c11;

	stdsint g00 = f00 + f10;
	stdsint g10 = f00 - f10;
	stdsint g01 = f01 + f11;
	stdsint g11 = f01 - f11;

	stdsint v00 = (stdsint)s_h264_dequant_v[qp % 6][0];
	int qp_div = qp / 6;

	if (qp_div >= H264_DQ_SHIFT_CDC) {
		out_scaled_dc[0] = (g00 * v00) << (qp_div - H264_DQ_SHIFT_CDC);
		out_scaled_dc[1] = (g01 * v00) << (qp_div - H264_DQ_SHIFT_CDC);
		out_scaled_dc[2] = (g10 * v00) << (qp_div - H264_DQ_SHIFT_CDC);
		out_scaled_dc[3] = (g11 * v00) << (qp_div - H264_DQ_SHIFT_CDC);
	} else {
		// H.264 (8-258)(8-259): for chroma the fallback is a bare shift, with no rounding term
		// (the luminance DC fallback (8-256) has one, the chroma DC one does not).
		int shift = H264_DQ_SHIFT_CDC - qp_div;
		out_scaled_dc[0] = (g00 * v00) >> shift;
		out_scaled_dc[1] = (g01 * v00) >> shift;
		out_scaled_dc[2] = (g10 * v00) >> shift;
		out_scaled_dc[3] = (g11 * v00) >> shift;
	}
}

void Dequant_H264_4x4(const int16 in_coeffs[16], int qp, bool is_dc_present, stdsint out_scaled[16]) {
	if (!in_coeffs || !out_scaled) return;

	int qp_rem = qp % 6;
	int qp_div = qp / 6;

	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			int idx = i * 4 + j;
			if (idx == 0 && is_dc_present) {
				out_scaled[0] = (stdsint)in_coeffs[0];
				continue;
			}
			stdsint v;
			if ((i % 2 == 0) && (j % 2 == 0)) {
				v = (stdsint)s_h264_dequant_v[qp_rem][0];
			} else if ((i % 2 != 0) && (j % 2 != 0)) {
				v = (stdsint)s_h264_dequant_v[qp_rem][1];
			} else {
				v = (stdsint)s_h264_dequant_v[qp_rem][2];
			}
			stdsint scaled = (stdsint)in_coeffs[idx] * v;
			// The dequantised coefficient handed to IDCT_H264_4x4() must be in the same
			// domain as the DC values produced by Hadamard_H264_4x4(): see the
			// H264_DQ_SHIFT_* comment above.
			if (qp_div >= H264_DQ_SHIFT_AC) {
				out_scaled[idx] = scaled << (qp_div - H264_DQ_SHIFT_AC);
			} else {
				int shift = H264_DQ_SHIFT_AC - qp_div;
				stdsint add = (stdsint)1 << (shift - 1);
				if (scaled >= 0) {
					out_scaled[idx] = (scaled + add) >> shift;
				} else {
					out_scaled[idx] = -(((-scaled) + add) >> shift);
				}
			}
		}
	}
}


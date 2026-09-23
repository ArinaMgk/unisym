// ASCII C/C++ TAB4 CRLF
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

#ifndef _INC_ALGORITHM_DCT
#define _INC_ALGORITHM_DCT

#include "../stdinc.h"
#include "../ustdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

// 2D Inverse Discrete Cosine Transform for NxN block
// - block: 1D array of size n*n with signed integer coefficients (in-place transform)
// - n: dimension of block (e.g. 8 for 8x8, 4 for 4x4, 16 for 16x16, etc.)
void IDCT_Transform(stdsint block[], unsigned n);

// 2D Forward Discrete Cosine Transform for NxN block
void DCT_Transform(stdsint block[], unsigned n);

// --- H.264 / AVC Integer & Hadamard Transforms ---

// H.264 4x4 Core Inverse Integer Transform (ITU-T H.264 8.5.12)
void IDCT_H264_4x4(const stdsint in_scaled[16], stdsint out_residual[16]);

// H.264 4x4 Luma DC Inverse Hadamard Transform & Scaling (ITU-T H.264 8.5.6)
void Hadamard_H264_4x4(const stdsint in_dc[16], int qp, stdsint out_scaled_dc[16]);

// H.264 2x2 Chroma DC Inverse Hadamard Transform & Scaling (ITU-T H.264 8.5.7)
void Hadamard_H264_2x2(const stdsint in_dc[4], int qp, stdsint out_scaled_dc[4]);

// H.264 4x4 Dequantization (ITU-T H.264 8.5.12)
void Dequant_H264_4x4(const int16 in_coeffs[16], int qp, bool is_dc_present, stdsint out_scaled[16]);

#ifdef __cplusplus
}

namespace uni {
	namespace IDCT {
		inline void Transform(stdsint block[], unsigned n) {
			IDCT_Transform(block, n);
		}
		inline void H264_4x4(const stdsint in_scaled[16], stdsint out_residual[16]) {
			IDCT_H264_4x4(in_scaled, out_residual);
		}
	}
	namespace DCT {
		inline void Transform(stdsint block[], unsigned n) {
			DCT_Transform(block, n);
		}
	}
	namespace Hadamard {
		inline void H264_4x4(const stdsint in_dc[16], int qp, stdsint out_scaled_dc[16]) {
			Hadamard_H264_4x4(in_dc, qp, out_scaled_dc);
		}
		inline void H264_2x2(const stdsint in_dc[4], int qp, stdsint out_scaled_dc[4]) {
			Hadamard_H264_2x2(in_dc, qp, out_scaled_dc);
		}
	}
}

namespace IDCT {
	inline void Transform(stdsint block[], unsigned n) {
		IDCT_Transform(block, n);
	}
	inline void H264_4x4(const stdsint in_scaled[16], stdsint out_residual[16]) {
		IDCT_H264_4x4(in_scaled, out_residual);
	}
}

namespace DCT {
	inline void Transform(stdsint block[], unsigned n) {
		DCT_Transform(block, n);
	}
}
#endif

#endif

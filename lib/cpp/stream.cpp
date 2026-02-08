// ASCII C++-STD11 TAB4 CRLF
// LastCheck: 20240229
// AllAuthor: @dosconio
// ModuTitle: Stream
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

#include "../../inc/cpp/unisym"

#if defined(_MCCA)
#else
#include "new"
#endif

#include "../../inc/c/consio.h"
#include "../../inc/c/arith.h"

// useless
// #ifdef _DEV_GCC
// #pragma GCC diagnostic push
// //#pragma GCC diagnostic ignored "-Wint-conversion"// C
// #pragma GCC diagnostic ignored "-Wfloat-conversion"// C++
// #pragma GCC diagnostic ignored "-Wconversion"// C++
// #endif

#define pnext(t) para_next(paras, t)
#define outc     OutChar
#define outtxt   out

#define out_integer  OutInteger
#define out_floating OutFloating


// Console: consio.cpp
//    File: ...
//    XART: ...

// -- C++ MSVC64 class static method -- paralist span a stduint, 20250329
// called from static  OstreamTrait::CountFormat
static stduint _CountFormat(const char* fmt, para_list pl) {
	using namespace uni;
	char buf[byteof(OstreamInstance_t)];
	new (buf) OstreamInstance_t;
	OstreamInstance_t& oi = *(OstreamInstance_t*)buf;
	stduint ret = oi.OutFormat(fmt, pl);
	return ret;
}

#define localout out
namespace uni {
	int OstreamTrait::OutFormatPack(const char* fmt, para_list paras) {
		out_count = 0;
		#define _STREAM_FORMAT_CPP
		#include "../../inc/c/stream/format-body.h"
		return out_count;
	}
	int OstreamTrait::OutFormat(const char* fmt, ...) {
		Letpara(paras, fmt);
		auto ret = OutFormatPack(fmt, paras);
		para_endo(paras);
		return ret;
	}
	stduint OstreamTrait::CountFormat(const char* fmt, ...) {
		Letpara(paras, fmt);
		count_mode = true;
		stduint ret = OutFormatPack(fmt, paras);
		count_mode = false;
		para_endo(paras);
		return ret;
	}

	bool OstreamTrait::OutInteger(uint64 val, int base, bool sign_show, bool sign_have, byte least_digits, bool zero_padding, byte bytexpo)
	{
		#include "../../inc/c/stream/format-out-integer.h"
	}

	__attribute__((target("sse2")))
	void OstreamTrait::OutFloating(double val) {
		#include "../../inc/c/stream/format-out-floating.h"
	}
	__attribute__((target("sse2")))
	void OstreamTrait::out_floating_0(stduint sizlevel, para_list paras) {
		if (sizlevel == 1)
			out_floating(pnext(double));
		else if (sizlevel == 0)
			out_floating(para_next_float(paras));
	}



}

// #ifdef _DEV_GCC
// #pragma GCC diagnostic pop
// #endif

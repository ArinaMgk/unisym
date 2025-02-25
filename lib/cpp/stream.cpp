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
#include "../../inc/c/consio.h"


#define pnext(t) para_next(paras, t)
#define outc     OutChar

static void _stream_out();
static void _stream_inn();

// Console: consio.cpp
//    File: ...
//    XART: ...

#define localout out
namespace uni {
	int OstreamTrait::FormatOut(const char* fmt, ...) {
		Letpara(paras, fmt);
		_crt_out_cnt = 0;
		#define _STREAM_FORMAT_CPP
		#include "../../inc/c/stream/format-body.h"
		return _crt_out_cnt;
	}
}

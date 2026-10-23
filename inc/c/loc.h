// ASCII C TAB4 CRLF
// Docutitle: Locale
// Codifiers: @dosconio: 20240603
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
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

#ifndef _INC_LOCALE
#define _INC_LOCALE

#include "stdinc.h"
#include "ustring.h"

enum Lango_t {
	Lango_English = 0,
	Lango_Japanese,
	Lango_Chinese,
	Lango_Chinese_Traditional,
	Lango_Korean,
	Lango_French,
	Lango_German,
	Lango_Spanish,
	Lango_Russian,
	Lango_Latin,
	Lango_Esperanto,
	Lango_Count
};

static inline enum Lango_t ParseLango(rostr code)
{
	if (!code || !*code) return Lango_English;
	while (*code == ' ' || *code == '\t') code++;
	// english, eigo
	if (StrCompareInsensitive(code, "en") == 0 ||
		StrCompareInsensitive(code, "en-us") == 0 ||
		StrCompareInsensitive(code, "en-gb") == 0 ||
		StrCompareInsensitive(code, "english") == 0) {
		return Lango_English;
	}
	// japanese, nihongo
	if (StrCompareInsensitive(code, "ja") == 0 ||
		StrCompareInsensitive(code, "nh") == 0 ||
		StrCompareInsensitive(code, "jp") == 0 ||
		StrCompareInsensitive(code, "ja-jp") == 0 ||
		StrCompareInsensitive(code, "japanese") == 0) {
		return Lango_Japanese;
	}
	// chinese(Madarin), zhongwen(jianti)
	if (StrCompareInsensitive(code, "zh-cn") == 0 ||
		StrCompareInsensitive(code, "cn") == 0 ||
		StrCompareInsensitive(code, "zh") == 0 ||
		StrCompareInsensitive(code, "chinese") == 0) {
		return Lango_Chinese;
	}

	if (StrCompareInsensitive(code, "zh-tw") == 0 ||
		StrCompareInsensitive(code, "zh-hk") == 0 ||
		StrCompareInsensitive(code, "cht") == 0) {
		return Lango_Chinese_Traditional;
	}
	if (StrCompareInsensitive(code, "ko") == 0 ||
		StrCompareInsensitive(code, "kr") == 0 ||
		StrCompareInsensitive(code, "korean") == 0) {
		return Lango_Korean;
	}
	if (StrCompareInsensitive(code, "fr") == 0 ||
		StrCompareInsensitive(code, "french") == 0) {
		return Lango_French;
	}
	if (StrCompareInsensitive(code, "de") == 0 ||
		StrCompareInsensitive(code, "german") == 0) {
		return Lango_German;
	}
	if (StrCompareInsensitive(code, "es") == 0 ||
		StrCompareInsensitive(code, "spanish") == 0) {
		return Lango_Spanish;
	}
	if (StrCompareInsensitive(code, "ru") == 0 ||
		StrCompareInsensitive(code, "russian") == 0) {
		return Lango_Russian;
	}
	if (StrCompareInsensitive(code, "la") == 0 ||
		StrCompareInsensitive(code, "latin") == 0) {
		return Lango_Latin;
	}
	if (StrCompareInsensitive(code, "eo") == 0 ||
		StrCompareInsensitive(code, "esperanto") == 0) {
		return Lango_Esperanto;
	}
	return Lango_English;
}

#ifdef _INC_CPP
namespace uni {
	using Lango = Lango_t;
	using ::ParseLango;
}
#endif

#endif

// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Data) Token Node Parse Instructions and Directives
// Codifiers: @ArinaMgk 
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

#include "../../../inc/cpp/parse.hpp"

using namespace uni;

// CRLF LFCR CR LF, c is the first char
inline static void EatLinefeed(LinearParser& lp, char c) {
	int cc = lp.getChar();
	if (cc == '\n' || cc == '\r')
		if (c == cc) lp.backChar(cc); else;
	else if (cc != EOF) lp.backChar(cc);
}

bool uni::LineParse_Comment_Sharp(LinearParser& lp, bool just_chk) {
	// no consider line-cont
	int c = lp.getChar();
	bool ret = c == '#';
	if (just_chk) {
		return ret;
	}
	while (c != EOF && c != '\n' && c != '\r')
	{
		lp.setChar(c);
		c = lp.getChar();
	}
	if (c == EOF) return ret;
	lp.backChar(c);
	return ret;
}

bool uni::LineParse_Comment_C(LinearParser& lp, bool just_chk) {
	int c1 = lp.getChar();
	if (just_chk) {
		if (c1 != '/') return false;
		int c2 = lp.getChar();
		lp.backChar(c2);
		return c2 == '*';
	}
	int c = c1;
	int last = 0;
	while (c != EOF && last != '*' && c != '/')
	{
		if (last) lp.setChar(last);
		last = c;
		c = lp.getChar();
	}
	if (c == EOF) return true;
	return true;
}

bool uni::LineParse_Comment_Cpp(LinearParser& lp, bool just_chk) {
	int c1 = lp.getChar();
	if (just_chk) {
		if (c1 != '/') return false;
		int c2 = lp.getChar();
		lp.backChar(c2);
		return c2 == '/';
	}
	int c = c1;
	int last;
	while (c != EOF)
	{
		if (c == '\n' || c == '\r') {
			if (lp.method_line_continuation && last == '\\') {
				lp.unsetChar();
				EatLinefeed(lp, c);
				++lp.pos.y;
			}
			else break;
		}
		lp.setChar(c);
		AssignParallel(last, c, lp.getChar());
	}

	if (c != EOF) lp.backChar(c);
	return true;
}

bool uni::LineParse_Comment_C_Cpp(LinearParser& lp, bool just_chk)
{
	int c1 = lp.getChar();
	if (just_chk) {
		if (c1 != '/') return false;
		int c2 = lp.getChar();
		lp.backChar(c2);
		return c2 == '/' || c2 == '*';
	}
	switch (c1) {
	case '*':LineParse_Comment_C(lp, false); break;
	case '/':LineParse_Comment_Cpp(lp, false); break;
	default:break;
	}
	return true;
}



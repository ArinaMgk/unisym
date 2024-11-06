// ASCII C99 TAB4 CRLF
// Docutitle: (Module) 
// Codifiers: @dosconio: 20240906 ~ <Last-check> 
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

#include "../../inc/c/strpage.h"
#include "../../inc/c/ustring.h"

inline static stduint StrpageGetShiftBits(const Strpage* sp) {
	return sp->layer_level * _STRPAGE_SHIFT;
}

inline static stduint StrpageGetCoverage(const Strpage* sp) {
	return _IMM(1) << (_STRPAGE_SHIFT + StrpageGetShiftBits(sp));
}

inline static stduint StrpageGetMask(const Strpage* sp) {
	return (_IMM(1) << StrpageGetShiftBits(sp)) - 1;
}

static Strpage* StrpageSetup(stduint level)
{
	Strpage* sp;
	if (sp = zalcof(Strpage)); else 
		return NULL;
	if (sp->layer_level = level)
		;// MetSetof(sp->branches, NULL);
	return sp;
}

Strpage* StrpageNew(void)
{
	return StrpageSetup(0);
}

void StrpageFree(Strpage* sp)
{
	if (!sp) return;
	if (sp->layer_level) {
		for0a(i, sp->branches)
			StrpageFree(sp->branches[i]);
	}
	memfree(sp);
}

stduint StrpageGet(const Strpage* sp, stduint index)
{
	if (index >= StrpageGetCoverage(sp))
		return 0;// undefined entries
	while (sp->layer_level) {
		stduint l = index >> StrpageGetShiftBits(sp);
		index &= StrpageGetMask(sp);
		if (sp = sp->branches[l]); else
			return 0;// undefined entries
	}
	return sp->leaves[index];
}

Strpage* StrpageSet(Strpage* sp, stduint index, stduint val)
{
	Strpage* res;
	//: whether it needs to add a layer
	while (StrpageGetCoverage(sp) <= _IMM(index))
	{
		Strpage* tmp = StrpageSetup(sp->layer_level + 1);
		AssignParallel(tmp->branches[0], sp, tmp);
	}
	res = sp;
	while (sp->layer_level) {
		Strpage** tmp;
		stduint l = index >> StrpageGetShiftBits(sp);
		index &= StrpageGetMask(sp);
		tmp = &sp->branches[l];
		if (!*tmp)
			*tmp = StrpageSetup(sp->layer_level - 1);
		sp = *tmp;
	}
	sp->leaves[index] = val;
	return res;
}

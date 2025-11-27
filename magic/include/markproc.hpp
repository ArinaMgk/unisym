// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Compile) Mark Layer
// Codifiers: @dosconio
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

#ifndef _INCX_MARKPROC
#define _INCX_MARKPROC

#include "../../inc/c/mnode.h"
#include "../../inc/cpp/parse.hpp"
#include "../../inc/c/graphic/color.h"

template<class datatype_T>// normally `enum`
struct TagObject {
	datatype_T type;
	pureptr_t offs;
	bool mutabl;
};

template<class datatype_T>
struct TagChain {
	typedef TagObject<datatype_T> IObject;
	//
	uni::Mchain chn;
	TagChain() { chn.func_comp = (_tocomp_ft)StrCompare; }
	~TagChain() { }
	uni::Dchain& refChain() { return chn.refChain(); }

	IObject* operator[] (const char* iden) {
		uni::Dnode* res = nullptr;
		(void)chn.isExist((pureptr_t)iden, &res);
		return res ? (IObject*)res->type : nullptr;
	}

	void Modify(const char* iden, pureptr_t offs, datatype_T typ, bool mutabl) {
		IObject* io = zalcof(IObject), * tmp;
		io->type = typ;
		io->offs = offs;
		io->mutabl = mutabl;
		if (tmp = self[iden]) {
			if (!tmp->mutabl) return;
		}
		chn.Map((pureptr_t)StrHeap(iden), (pureptr_t)io);
	}

	void Remove(const char* iden, bool consider_mutabl = true) {
		uni::Dnode* dn = refChain().LocateNode((pureptr_t)iden, chn.func_comp);
		if (!dn) return;
		Letvar(io, IObject*, dn->type);
		if (consider_mutabl && !io->mutabl) return;
		refChain().Remove(dn);
	}

};
/* Referenced
*** magice-mark
*** dosconio/COTLAB
*/

//// ---- ---- . ---- ---- ////

class MarkProcessor
	: public uni::IstreamTrait, public uni::OstreamTrait
{
protected:
	virtual int inn() override;
	virtual int out(const char* str, stduint len) override;
public:
	MarkProcessor() {
		
	}
	bool B = false;// bold
	bool I = false;// italic
	bool U = false;// underline
	bool DeleteLine = false;// delete line
	uni::Color Forecolor = uni::Color::Black;
	uni::Color Backcolor = uni::Color::White;
	bool _up = false;// <sup>
	bool _dn = false;// <sub>

	// through Ostream

	//
	bool OutputMarkdown();
	//
	bool OutputHTML();
	//
	bool OutputLaTeX();
	//
	bool OutputStdout();


};// line-unit

#endif

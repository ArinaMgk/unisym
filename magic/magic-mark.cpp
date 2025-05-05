// ASCII C++ TAB4 LF
// Docutitle: Magice Mark-Like Language Compiler
// Input    : Intermediate mark-language
// Output   : HTML, LaTeX, Markdown, ...
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

// .mgc ---> .html
//       |-> .tex
//       |-> .md

#include "magice.hpp"
#include "../inc/c/file.h"

using namespace uni;

//{TODO} move into file-format modules
bool OutputMarkdown(OstreamTrait& out) {

	return _TODO false;
}
bool OutputHTML(OstreamTrait& out) {
	return _TODO false;
}
bool OutputLaTeX(OstreamTrait& out) {
	return _TODO false;
}

void mark(int argc, char** argv) {
	ploginfo("process %s into .tex, .md, .html", argv[1]);
	String filename = argv[1];
	HostFile file(filename.reference());
	byte ch;
	while (file >> ch) {
		Console.OutFormat("%c", ch);
	}

	//{} UTF-8 Support in String
	// Process by LINE

	char* _Heap texname = StrReplace(filename.reference(), ".mgc", ".tex", NULL);
	ploginfo(".tex \t=> %s", texname);
	char* _Heap mdname = StrReplace(filename.reference(), ".mgc", ".md", NULL);
	ploginfo(".md  \t=> %s", mdname);
	char* _Heap htmlname = StrReplace(filename.reference(), ".mgc", ".html", NULL);
	ploginfo(".html\t=> %s", htmlname);
	mfree(texname);
	mfree(mdname);
	mfree(htmlname);



}


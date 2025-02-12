// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: File
// Codifiers: @dosconio: 20240422 ~ <Last-check> 
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

#ifndef _INC_File
#define _INC_File

#include "stdinc.h"

#if defined(_INC_CPP)
extern "C" {
#endif



#if defined(_INC_CPP)
} //: C++ Area
#include "../cpp/string"
#include "../cpp/trait/XstreamTrait.hpp"
namespace uni {

	// depend { String }
	enum class FileOpenType {
		Read, Write, Append //{MORE}
	};
	class HostFile : public OstreamTrait, public IstreamTrait
	// not inherited from File(FileSystem(BlockTraitObject))
	{
		void* fptr;
	public:
		~HostFile();
		HostFile() : fptr(nullptr) {}
		HostFile(rostr filepath, FileOpenType fopen_type = FileOpenType::Read);
		explicit operator bool() const { return nullptr != fptr; }
		//
		virtual int inn() { return _TODO - 1; }
		virtual int out(const char* str, dword len) { return _TODO - 1; }
		//
		bool operator>> (byte& B);
		// will check available zone of String.
		bool operator>> (String& str);
		//
		bool operator<< (byte B);
		//
		bool operator<< (const String& str);
	};

	
} //END C++ Area
#else//: C Area

#endif
#endif

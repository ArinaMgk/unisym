// ASCII C++11 TAB4 CRLF
// Attribute: ArnCovenant CPU(80586+)
// LastCheck: RFZ03
// AllAuthor: @dosconio
// ModuTitle: Test for Instruction CPUID
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

#include "../../../inc/cpp/cpuid"
#include <iostream>// include stdio.h
#include <stdlib.h>

char buf[1 + _CPU_BRAND_SIZE];

int main()
{
	puts("C++ Version");
	// Show CPU Brand
	CpuBrand(buf);
	printf("CPU Brand:  %s\n", buf);
	system("pause");
}

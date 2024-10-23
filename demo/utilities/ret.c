// ASCII C/C++ TAB4 CRLF
// Docutitle: (Format: File) Return Codes
// Codifiers: @dosconio: 20241021 
// Attribute: Arn-Covenant Any-Architect <Environment> <Reference/Dependence>
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

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	int ret = 0;
	if (argc > 1)
		ret = atoi(argv[1]);
	if (argc > 2)
		puts(argv[2]);
	return ret;
}

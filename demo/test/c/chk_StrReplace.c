// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant
// LastCheck: RFZ26
// AllAuthor: @dosconio
// ModuTitle: Test for StrReplace()
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

#include "../../../inc/c/ustring.h"
#include <stdio.h>

int main()
{
	char a[] = "110101110101";
	char* str = StrReplace(a, "010", "2", 0);
	printf("%s\n", str);// 11211121
	memf(str);
	if (malc_count)
		printf("Memory leak: %d\n", malc_count);
	return 0;
}

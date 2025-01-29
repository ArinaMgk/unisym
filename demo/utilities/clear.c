// ASCII C99 TAB4 CRLF
// Attribute: 
// LastCheck: RFZ10
// AllAuthor: @dosconio
// ModuTitle: Clear Screen 
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

// Compensate for console if it does not support the "clear" command.

#include <stdlib.h>
#include <stdio.h>

int main()
{	
#ifdef _WinNT
	system("cls");
#else
	printf("\033c");// or "\x1b[H\x1b[2J"
#endif
}

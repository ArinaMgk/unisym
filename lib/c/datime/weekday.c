// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant yo MIX bin^16+
// LastCheck: RFZ18
// AllAuthor: @dosconio
// ModuTitle: Date and Time
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

#include "../../../inc/c/datime.h"

unsigned weekday(word year, word month, word day)
{
	if (year > 2013) goto a;
	if (year == 2013) if (month == 12) if (day == 28) return 6; else if (day > 28) goto a;
	return (6 - (-herspan(year, month, day)) % 7);
a:
	return (herspan(year, month, day) - 1) % 7;
}


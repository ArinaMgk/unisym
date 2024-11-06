// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Module) Pseudo-random
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

#include "../../inc/c/stdinc.h"
#include "../../inc/c/random.h"

#ifdef _BIT_SUPPORT_64 //{TEMP}

static int _hash_generate(int n)
{
	// big prime number
	uint64 r =
		6364136223846793005ULL * n + 1;//{TEMP} Mersenne Twister 19937 generator ?
	return r >> 33;
}
static uint32 last_seed;

int rand(void) {
	return last_seed = _hash_generate(last_seed);
}

void srand(unsigned int seed) {
	last_seed = seed;
}

#endif

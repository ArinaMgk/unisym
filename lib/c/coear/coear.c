// ASCII Powerful Number of Arinae
// The first generation is outdated, this 2-gen is the first open-sourced version.
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#include "../../../inc/c/coear.h"

#define coe_const(iden,co,ex) const coe iden = {.coff=(co), .divr="+1", .expo=(ex)};

size_t show_precise = _CDE_PRECISE_SHOW_DEFAULT;
size_t lup_times = _CDE_PRECISE_LOOPTIMES_LEAST_DEFAULT;
size_t lup_limit = _CDE_PRECISE_LOOPTIMES_LIMIT_DEFAULT;
size_t lup_last;// [OUT]
size_t _DIG_CUT = 4;

//TODO. temporary use
//pi=3.1415926535 8979323846 2643383279 5028841971 6939937510
//     5820974944 5923078164 0628620899 8628034825 3421170679
const coe coeinf = { .coff = "+1", .divr = "+0", .expo = "+0" };
const coe coenan = { .coff = "+0", .divr = "+0", .expo = "+0" };
coe_const(coepi,         "+3141592653589793238462643383279502884197169399375105820974944592", "-63")
coe_const(coe2pi,        "+6283185307179586476925286766559005768394338798750211641949889185", "-63")
coe_const(coepi_half,    "+1570796326794896619231321691639751442098584699687552910487472296", "-63")
coe_const(coepi_quarter, "+785398163397448309615660845819875721049292349843776455243736148", "-63")
coe_const(coeln2,        "+69314718055994530941723212145817656807550013436025525412068001", "-62")// {HARD} (-1)^(i+1)/(i)
coe_const(coeone,        "+1", "+0")
coe_const(coenegone,     "-1", "+0")
coe_const(coezero,       "+0", "+0")
coe_const(coetwo,        "+2", "+0")
coe_const(coefour,       "+4", "+0")
const coe coehalf = { .coff = "+1", .divr = "+2", .expo = "+0" };

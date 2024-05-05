// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Module) Real Time Clock
// Codifiers: @dosconio: 20240220 ~ 20240429
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
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

#include "../../../../inc/c/driver/i8259A.h"
#include "../../../../inc/c/driver/RealtimeClock.h"

#ifdef _MCCA // 0x8632

void RTC_Init()
{
	outpb(PORT_RTC, 0x8B);// mgk: RTC Register B and NMI
	outpb(PORT_RTC | 1, 0x12);// mgk: Set Reg-B {Ban Periodic, Open Update-Int, BCD, 24h}
	i8259Slaver_Enable(0);// Slave 0 is linked with RTC
	outpb(PORT_RTC, 0x0C);// mgk: ?
	innpb(PORT_RTC | 1);// Read Reg-C, reset pending interrupt
	//{TODO} Check PIC Device?
}

#endif

// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Digital-Analog Converter
// Codifiers: @dosconio: 20240731~;
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Dependens: GPIO
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

#ifndef _INC_Device_MatrixKeyboard
#define _INC_Device_MatrixKeyboard
#include "../stdinc.h"
#ifdef _INC_CPP

namespace uni {
	class Mtrkbd_4x4 {
	protected:
		GPIO_Pin& R0, & R1, & R2, & R3;
		GPIO_Pin& C0, & C1, & C2, & C3;
	public:
		Handler_t func_delay;
		Mtrkbd_4x4(GPIO_Pin& r0, GPIO_Pin& r1, GPIO_Pin& r2, GPIO_Pin& r3, GPIO_Pin& c0, GPIO_Pin& c1, GPIO_Pin& c2, GPIO_Pin& c3) :
			R0(r0), R1(r1), R2(r2), R3(r3), C0(c0), C1(c1), C2(c2), C3(c3) {}
		void setMode() {
			R0.setMode(GPIOMode::OUT_PushPull);
			R1.setMode(GPIOMode::OUT_PushPull);
			R2.setMode(GPIOMode::OUT_PushPull);
			R3.setMode(GPIOMode::OUT_PushPull);
			C0.setMode(GPIOMode::IN_Floating);
			C0.setPull(false);
			C1.setMode(GPIOMode::IN_Floating);
			C1.setPull(false);
			C2.setMode(GPIOMode::IN_Floating);
			C2.setPull(false);
			C3.setMode(GPIOMode::IN_Floating);
			C3.setPull(false);
		}
		word Scan() {
			word res = 0;
			R0 = 1; R1 = 0; R2 = 0; R3 = 0;
			asserv(func_delay)();
			if (C0) res |= 0x0001;
			if (C1) res |= 0x0002;
			if (C2) res |= 0x0004;
			if (C3) res |= 0x0008;
			// R0 = 0; R1 = 1; R2 = 0; R3 = 0;
			R0.Toggle(); R1.Toggle();
			asserv(func_delay)();
			if (C0) res |= 0x0010;
			if (C1) res |= 0x0020;
			if (C2) res |= 0x0040;
			if (C3) res |= 0x0080;
			// R0 = 0; R1 = 0; R2 = 1; R3 = 0;
			R1.Toggle(); R2.Toggle();
			asserv(func_delay)();
			if (C0) res |= 0x0100;
			if (C1) res |= 0x0200;
			if (C2) res |= 0x0400;
			if (C3) res |= 0x0800;
			// R0 = 0; R1 = 0; R2 = 0; R3 = 1;
			R2.Toggle(); R3.Toggle();
			asserv(func_delay)();
			if (C0) res |= 0x1000;
			if (C1) res |= 0x2000;
			if (C2) res |= 0x4000;
			if (C3) res |= 0x8000;
			return res;
		}
	};
}


#endif
#endif

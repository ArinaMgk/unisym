// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Floating Point Unit
// Codifiers: @dosconio: 20241209 ~ <Last-check> 
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

#include "../../../inc/cpp/Device/FPU"

#ifdef _MPU_STM32MP13
#include "../../../inc/c/proctrl/ARM/cortex_a7.h"

namespace uni {
	FPU_t FPU;

	void FPU_t::enAble(bool ena) const {
		if (!ena) _TEMP return;
		//: Below Enable FPU
		_ASM volatile(
	//Permit access to VFP/NEON, registers by modifying CPACR
			"        MRC     p15,0,R1,c1,c0,2  \n"
			"        ORR     R1,R1,#0x00F00000 \n"
			"        MCR     p15,0,R1,c1,c0,2  \n"
			//Ensure that subsequent instructions occur in the context of VFP/NEON access permitted
			"        ISB                       \n"
			//Enable VFP/NEON
			"        VMRS    R1,FPEXC          \n"
			"        ORR     R1,R1,#0x40000000 \n"
			"        VMSR    FPEXC,R1          \n"
			//Initialise VFP/NEON registers to 0
			"        MOV     R2,#0             \n"
			//Initialise D16 registers to 0
			"        VMOV    D0, R2,R2         \n"
			"        VMOV    D1, R2,R2         \n"
			"        VMOV    D2, R2,R2         \n"
			"        VMOV    D3, R2,R2         \n"
			"        VMOV    D4, R2,R2         \n"
			"        VMOV    D5, R2,R2         \n"
			"        VMOV    D6, R2,R2         \n"
			"        VMOV    D7, R2,R2         \n"
			"        VMOV    D8, R2,R2         \n"
			"        VMOV    D9, R2,R2         \n"
			"        VMOV    D10,R2,R2         \n"
			"        VMOV    D11,R2,R2         \n"
			"        VMOV    D12,R2,R2         \n"
			"        VMOV    D13,R2,R2         \n"
			"        VMOV    D14,R2,R2         \n"
			"        VMOV    D15,R2,R2         \n"

		#if (defined(__ARM_NEON) && (__ARM_NEON == 1))
			//Initialise D32 registers to 0
			"        VMOV    D16,R2,R2         \n"
			"        VMOV    D17,R2,R2         \n"
			"        VMOV    D18,R2,R2         \n"
			"        VMOV    D19,R2,R2         \n"
			"        VMOV    D20,R2,R2         \n"
			"        VMOV    D21,R2,R2         \n"
			"        VMOV    D22,R2,R2         \n"
			"        VMOV    D23,R2,R2         \n"
			"        VMOV    D24,R2,R2         \n"
			"        VMOV    D25,R2,R2         \n"
			"        VMOV    D26,R2,R2         \n"
			"        VMOV    D27,R2,R2         \n"
			"        VMOV    D28,R2,R2         \n"
			"        VMOV    D29,R2,R2         \n"
			"        VMOV    D30,R2,R2         \n"
			"        VMOV    D31,R2,R2         \n"
		#endif

			//Initialise FPSCR to a known state
			"        VMRS    R1,FPSCR          \n"
			"        LDR     R2,=0x00086060    \n" //Mask off all bits that do not have to be preserved. Non-preserved bits can/should be zero.
			"        AND     R1,R1,R2          \n"
			"        VMSR    FPSCR,R1            "
			: : : "cc", "r1", "r2"
			);
	}
	
}

#endif

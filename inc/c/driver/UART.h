// ASCII CPL TAB4 CRLF
// Docutitle: (Device) UART and USART
// Datecheck: 20240502 ~ <Last-check>
// Developer: @dosconio
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Reference: <Reference>
// Dependens: <Dependence>
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

#if !defined(_INC_STANDARD_UART)
#define _INC_STANDARD_UART
#include "../stdinc.h"
#if defined(_MCU_Intel8051)

// We need to know the frequency like 9600bps@11.0592MHz

extern void (*RoutineINT4)(void);// Keil will consider this as a define but a declaration? Haruno RFR19.
void UART_Init(void);
void UART_Init11W(void);
void UART_Out(unsigned char Byte);

/* Example
	void RINT4(void)
	{
		//
	} void (*RoutineINT4)(void) = RINT0;
*/

#elif defined(_MCU_STM32F1x)

_TODO

#elif defined(_MCU_STM32F4x) && defined(_INC_CPP)
#include "../../cpp/Device/GPIO"
#include "../../cpp/Device/RCC/RCC"
#include "../../cpp/interrupt"

#define _USART_CR2_POSI_CLKEN 11
#define _USART_CR2_POSI_LINEN 14
#define _USART_CR3_POSI_IREN  1
#define _USART_CR3_POSI_HDSEL 3
#define _USART_CR3_POSI_SCEN  5


namespace uni {
	namespace XARTReg {
		typedef enum {
			SR = 0, DR, BRR, CR1,
			CR2, CR3, GTPR
		} USARTReg;
	}

	
	// Now only accept USART1, //{TODO} more
	
	class UART_t {
	protected:
		stduint baseaddr;
		byte XART_ID;
	public:
		UART_t(stduint _baseaddr, byte _XART_ID) : baseaddr(_baseaddr), XART_ID(_XART_ID) {}
		_TODO
	};

	class USART_t {
	protected:
		stduint baseaddr;
		byte XART_ID;
	public:
		Reference operator[](XARTReg::USARTReg idx) {
			return Reference(baseaddr + (((stduint)idx) << 2));
		}
		USART_t(stduint _baseaddr, byte _XART_ID) : baseaddr(_baseaddr), XART_ID(_XART_ID) {}

		void setMode(_TEMP void);

		//aka HAL_UART_Receive
		//{TODO} conflict
		inline int operator>> (int& res) {
			using namespace XARTReg;
			bool len9b = false;// or 8b
			bool parity = false;
			stduint mask = len9b ?
				(parity ? 0x1FF : 0xFF):
				(parity ? 0x0FF : 0x7F);
			int d = int(self[DR]);
			return res = d & mask;
		}

		//aka HAL_UART_Transmit
		//{TODO} conflict
		inline USART_t& operator<< (stduint dat) {
			using namespace XARTReg;
			bool len9b = false;// or 8b
			self[DR] = dat & (len9b ? 0x1FF : 0x0FF);
			return self;
		}
		inline USART_t& operator<< (const char* p) {
			while (*p) {
				self << stduint(*p++);
				for0(i, _TEMP 1000);
			}
			return self;
		}

		bool enAble(bool ena = true) {
			using namespace XARTReg;
			self[CR1].setof(13, ena);// 13 is USART_CR1_UE_Pos
			return true;
		}

		bool enClock(bool ena = true) {
			//{!} only for usart1
			using namespace RCCReg;
			RCC[APB2ENR].setof(_RCC_APB2ENR_POSI_ENCLK_USART1, ena);
			return RCC[APB2ENR].bitof(_RCC_APB2ENR_POSI_ENCLK_USART1) == ena;
		}

		void setInterrupt(Handler_t fn);
		void setInterruptPriority(byte preempt, byte sub_priority);
		void enInterrupt(bool enable = true);


	};

	extern USART_t XART1, XART2, XART3, XART6;
	extern UART_t  XART4, XART5, XART7, XART8;
	static struct {
		pureptr_t operator[](byte id) {
			switch (id) {
			case 1: return (pureptr_t)&XART1;
			case 2: return (pureptr_t)&XART2;
			case 3: return (pureptr_t)&XART3;
			case 4: return (pureptr_t)&XART4;
			case 5: return (pureptr_t)&XART5;
			case 6: return (pureptr_t)&XART6;
			case 7: return (pureptr_t)&XART7;
			case 8: return (pureptr_t)&XART8;
			default: return 0;
			}
		}
		static bool isSync(byte id) {
			return id == 1 || id == 2 || id == 3 || id == 6;
		}
	} XART;

}

#endif
#endif

// ASCII CPL TAB4 CRLF
// Docutitle: (Device) Universal (Synchronous) Asynchronous Receiver Transmitter
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

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
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

	class UART_t {
	protected:
		stduint baseaddr;
		byte XART_ID;
	public:
		UART_t(stduint _baseaddr, byte _XART_ID) : baseaddr(_baseaddr), XART_ID(_XART_ID) {}
		_TODO
	};

	class USART_t : public RuptTrait {
	protected:
		stduint baseaddr;
		byte XART_ID;
		stduint last_baudrate;
		void Delay_unit();
	public:
		Reference operator[](XARTReg::USARTReg idx) const {
			return baseaddr + _IMMx4(idx);
		}
		USART_t(stduint _baseaddr, byte _XART_ID) : baseaddr(_baseaddr), XART_ID(_XART_ID), last_baudrate(1){}
		void setMode(stduint band_rate = 115200);
		int operator>> (int& res);
		USART_t& operator<< (stduint dat);
		USART_t& operator<< (const char* p);
		bool enAble(bool ena = true);
		bool enClock(bool ena = true);
		_COM_DEF_Interrupt_Interface();
	};

	class XART_SOFT {
		_TODO byte todo;
	};

}

#elif defined(_MCU_STM32H7x)
#include "../../cpp/Device/GPIO"
#include "../../cpp/Device/RCC/RCC"
#include "../../cpp/string"
#include "../../cpp/interrupt"
#include "../../cpp/trait/XstreamTrait.hpp"

#ifdef _MCU_XART_TEMP
#include "../../cpp/Device/XART/XART-STM32H7.hpp"
#endif

#define ERR_UART_NONE    0x00
#define ERR_UART_Parity  0x01
#define ERR_UART_Noise   0x02
#define ERR_UART_Frame   0x04
#define ERR_UART_Overrun 0x08
#define ERR_UART_DMA     0x10

namespace uni {
	namespace XARTReg {
		typedef enum {
			CR1 = 0x00,
			CR2 = 0x04,
			CR3 = 0x08,
			BRR = 0x0C,
				// If oversampling is 16 or in LIN mode, = PCLKx / Baudrate
				// If oversampling is 8                , [15:4]=((2*PCLKx) / Baudrate)[15:4], [3]=0, [2:0]=(((2*PCLKx) / Baudrate)[3:0])>>1
			RTOR = 0x14,// Receiver Time Out
			ISR = 0x1C,// Interrupt and status
			ICR = 0x20,// Interrupt flag Clear
			PRESC = 0x2C,// Clock Prescaler
		} USARTReg;
		typedef enum {
			GTPR = 0x10,// Guard time and prescaler
			RQR = 0x18,//  Request
			RDR = 0x24,//  Receive Data
			TDR = 0x28,//  Transmit Data
		} USARTReg16;
	}
	enum class UART_CLKSRC //  D2PCLK1 D2PCLK2 D3PCLK1 PLL2 HSI CSI LSE PLL3 ...
	{
		DxPCLKx = 0,// D2PCLK2 for 1 and 6, D2PCLK1 for 2~5, 7~8
		PLL2, PLL3, HSI, CSI, LSE,
		UNDEFINED = 0x80U
	};// UART_ClockSourceTypeDef

	class UART_t final: public RuptTrait, public IstreamTrait, public OstreamTrait {
	protected:
		byte XART_ID;
	public:
		_COM_DEF_Interrupt_Interface();

		_Comment("[lock] [depend(temp_inn)]")
			virtual int inn() override;

		_Comment("[lock] [depend(temp_out)]")
			virtual int out(const char* str, stduint len) override;
	public:
		UART_t(byte _XART_ID) : XART_ID(_XART_ID) {}
		_TODO
	};

	class USART_t final: public RuptTrait, public IstreamTrait, public OstreamTrait {
	protected:
		byte XART_ID;// 1..n+1
		void Delay_unit();
		Slice rx_buffer = { 0, 0 };
		bool lock = false;
		stduint status = ERR_UART_NONE;
	public:
		_COM_DEF_Interrupt_Interface();

		_Comment("[lock]")
			virtual int inn() override;

		_Comment("[lock]")// "str" maybe point to uint32 and len may be ignored
			virtual int out(const char* str, stduint len) override;
	public:
		int operator>> (int& res) { return res = inn(); }
		USART_t& operator<< (stduint dat);
		USART_t& operator<< (const char* p) { out(p, StrLength(p)); return self; }

	public:
		Reference operator[](XARTReg::USARTReg idx) const;// 32b
		Reference_T<uint16> operator[](XARTReg::USARTReg16 idx) const;// 16b
		//
		USART_t(byte _XART_ID) : XART_ID(_XART_ID) {}
		//
		bool setMode(stduint band_rate = 115200);
		bool enAble(bool ena = true);
		bool enClock(bool ena = true);
		//
		UART_CLKSRC getClockSource();
		//

	};
}

// 0:1b, 1:0.5b, 2:2b, 3:1.5b
#define USART_CR2_STOP(x) Stdfield(x[XARTReg::CR2], 12, 2)

#endif


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

#elif defined(_MCU_STM32F1x) && defined(_INC_CPP)
//{TODO} apply and check
//{TODO} Now only accept USART1
namespace uni {
	extern USART_t XART1, XART2, XART3;
	extern UART_t  XART4, XART5;
}

#elif defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)
// F4: Now only accept USART1

namespace uni {
	extern USART_t XART1, XART2, XART3, XART6;
	extern UART_t  XART4, XART5, XART7, XART8;
}

#elif defined(_WinNT) || defined(_Linux) // Any Hosted

#ifdef _WinNT
#define UNICODE
#include <WinSock2.h>
#include <windows.h>
#define _INC_USTDBOOL

#elif defined(_Linux)
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/select.h>

#endif
#include "../../cpp/string"

namespace uni {
	enum class UARTCheck {
		None, Odd, Even, Mark
	};
	enum class UARTStopBit {
		One, OneHalf, Two
	};
	class UART_t {
		stduint baudrate;
		bool state;
		String portname;// "\\\\.\\COM10" or "ttyUSB0"
		stduint databits;
		UARTCheck parity;
		UARTStopBit stopbits;
	#ifdef _WinNT
		HANDLE pHandle;
	#elif defined(_Linux)
		int pHandle;
	#endif
	public:
		bool sync;
		UART_t(String portname) :
			baudrate(baudrate), state(false), portname(portname), databits(8), parity(UARTCheck::None), stopbits(UARTStopBit::One),
			sync(true) {}
		~UART_t();
		bool setMode(stduint _baudrate = 115200);
		bool operator>> (int& res);// return if new data received
		bool operator<< (stduint dat);
		inline UART_t& operator<< (const char* p) {
			while (*p) self << stduint(*p++);
			return self;
		}
	};
}

#endif
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)
namespace uni {
	static struct {
		pureptr_t operator[](byte id) {
			switch (id) {
			case 1: return (pureptr_t)&XART1;
			case 2: return (pureptr_t)&XART2;
			case 3: return (pureptr_t)&XART3;
			case 4: return (pureptr_t)&XART4;
			case 5: return (pureptr_t)&XART5;
			#if defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)
			case 6: return (pureptr_t)&XART6;
			case 7: return (pureptr_t)&XART7;
			case 8: return (pureptr_t)&XART8;
			#endif
			default: return 0;
			}
		}
		static bool isSync(byte id) {
			return id == 1 || id == 2 || id == 3
			#if defined(_MCU_STM32F4x)
				|| id == 6
			#endif
				;
		}
	} XART;
}
#endif

#endif

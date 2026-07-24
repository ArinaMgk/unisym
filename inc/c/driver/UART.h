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

namespace uni {
	enum class UARTCheck {
		None, Odd, Even, Mark
	};
	enum class UARTStopBit {
		One, OneHalf, Two
	};
}

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
#include "../../cpp/MCU/_ADDRESS/ADDR-STM32.h"
#include "../../cpp/Device/RCC/RCC"
#include "../../cpp/string"
#include "../../cpp/interrupt"
#include "../../cpp/trait/XstreamTrait.hpp"

#if !defined(_MPU_STM32MP13)
#include "../../cpp/Device/GPIO"
#endif

#if defined(_MCU_STM32H7x) && defined(_MCU_XART_TEMP)
#include "../../cpp/Device/XART/XART-STM32H7.hpp"
#endif

#define ERR_UART_NONE    0x00
#define ERR_UART_Parity  0x01
#define ERR_UART_Noise   0x02
#define ERR_UART_Frame   0x04
#define ERR_UART_Overrun 0x08
#define ERR_UART_DMA     0x10

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
#define _USART_CR2_POSI_CLKEN 11
#define _USART_CR2_POSI_LINEN 14
#define _USART_CR3_POSI_IREN  1
#define _USART_CR3_POSI_HDSEL 3
#define _USART_CR3_POSI_SCEN  5
#endif

namespace uni {
	namespace XARTReg {
		typedef enum {
		#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
			SR = 0, DR, BRR, CR1, CR2, CR3, GTPR
		#elif defined(_MCU_STM32H7x)
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
		#elif defined(_MPU_STM32MP13)
			CR1 = 0x00,
			CR2 = 0x04,
			CR3 = 0x08,
			BRR = 0x0C,
			ISR = 0x1C,
			ICR = 0x20,
			RDR = 0x24,
			TDR = 0x28,
			PRESC = 0x2C,
		#endif
		} USARTReg;
		#if defined(_MCU_STM32H7x)
		typedef enum {
			GTPR = 0x10,// Guard time and prescaler
			RQR = 0x18,//  Request
			RDR = 0x24,//  Receive Data
			TDR = 0x28,//  Transmit Data
		} USARTReg16;
		#endif
	}

	enum class UART_CLKSRC //  D2PCLK1 D2PCLK2 D3PCLK1 PLL2 HSI CSI LSE PLL3 ...
	{
		DxPCLKx = 0,// D2PCLK2 for 1 and 6, D2PCLK1 for 2~5, 7~8
		PLL2, PLL3, HSI, CSI, LSE,
		UNDEFINED = 0x80U
	};// UART_ClockSourceTypeDef
	enum class WordLength_E {
		Bits7 = 0x10000000,
		Bits8 = 0x00000000,
		Bits9 = 0x00001000,
	};
	enum class XARTParity_E {
		None = 0x00000000,
		Even = 0x00000400,
		Odd = 0x00000600,
	};
	enum class XARTStopBits_E {
		Half = 0x00001000,
		One = 0x00000000,
		OneHalf = 0x00003000,
		Two = 0x00002000,
	};
	enum class XARTHwFlowCtl_E {
		None = 0,
		RTS = 1,
		CTS = 2,
		RTS_CTS = 3,
	};
	enum class XARTFIFOThreshold_E {
		_1_8 = 0,
		_1_4 = 1,
		_1_2 = 2,
		_3_4 = 3,
		_7_8 = 4,
		_8_8 = 5,
	};

	class UART_t : public RuptTrait, public IstreamTrait, public OstreamTrait {
	#if defined(_MCU_STM32H7x)
	public:
		Slice rx_buffer = { 0, 0 };
		Slice tx_buffer = { 0, 0 };
		rostr error = NULL;
	#endif
	protected:
	#if !defined(_MCU_STM32H7x)
		stduint baseaddr = nil;
	#endif
	#if defined(_MCU_STM32H7x)
		stduint rx_pointer = 0;// should < rx_buffer.length
		stduint tx_pointer = 0;// should < tx_buffer.length
	#endif
		byte XART_ID;
	#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
		stduint last_baudrate = 1;
	#endif
	#if defined(_MCU_STM32H7x)
		bool lock_r = false;
		bool lock_t = false;
	#endif
	#if !defined(_MCU_STM32H7x)
		stduint databits = 8;
		UARTCheck check = UARTCheck::None;
		UARTStopBit stopbit = UARTStopBit::One;
	#endif
	#if defined(_MCU_STM32H7x)
		WordLength_E wordlen = WordLength_E::Bits8;
		XARTParity_E parity = XARTParity_E::None;
		XARTStopBits_E stopbits = XARTStopBits_E::One;
		XARTHwFlowCtl_E hwflow = XARTHwFlowCtl_E::None;
		XARTFIFOThreshold_E tx_fifo_threshold = XARTFIFOThreshold_E::_1_8;
		XARTFIFOThreshold_E rx_fifo_threshold = XARTFIFOThreshold_E::_1_8;
		bool oversampling8 = false;
		bool onebit_sampling = false;
		bool fifo_mode = false;
		stduint mask = 0xFF;
	#endif
	#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
		void Delay_unit();
	#endif
	public:
		_COM_DEF_Interrupt_Interface();

		_Comment("[lock] [depend(temp_inn)]")
			virtual int inn() override;

		_Comment("[lock] [depend(temp_out)]")
			virtual int out(const char* str, stduint len) override;
	public:
		UART_t(byte _XART_ID) : XART_ID(_XART_ID) {}
	#if !defined(_MCU_STM32H7x)
		UART_t(stduint _baseaddr, byte _XART_ID) : baseaddr(_baseaddr), XART_ID(_XART_ID) {}
	#endif
	#if defined(_MCU_STM32H7x)
		Reference operator[](XARTReg::USARTReg idx) const;// 32b
		Reference_T<uint16> operator[](XARTReg::USARTReg16 idx) const;// 16b
	#else
		Reference operator[](XARTReg::USARTReg idx) const {
		#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
			return baseaddr + _IMMx4(idx);
		#else
			return getAddress() + _IMM(idx);
		#endif
		}
	#endif
	#if defined(_MCU_STM32H7x)
		stduint getAddress() const;
	#else
		stduint getAddress() const { return baseaddr; }
	#endif
		bool setMode(stduint band_rate = 115200);
	#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
		int operator>> (int& res);
		UART_t& operator<< (const char* p);
	#else
		bool operator>> (int& res) { return (res = inn()) >= 0; }
		UART_t& operator<< (const char* p) { out(p, StrLength(p)); return self; }
	#endif
		UART_t& operator<< (stduint dat);
		bool enAble(bool ena = true);
		virtual bool enClock(bool ena = true);
	#if defined(_MCU_STM32H7x)
		UART_t& setDataBits(stduint bits);
		UART_t& setCheck(UARTCheck val);
		UART_t& setStopBit(UARTStopBit val);
	#else
		UART_t& setDataBits(stduint bits) { databits = bits; return self; }
		UART_t& setCheck(UARTCheck val) { check = val; return self; }
		UART_t& setStopBit(UARTStopBit val) { stopbit = val; return self; }
	#endif
	#if defined(_MCU_STM32H7x)
		UART_t& setWordLength(WordLength_E val) { wordlen = val; return self; }
		UART_t& setParity(XARTParity_E val) { parity = val; return self; }
		UART_t& setStopBits(XARTStopBits_E val) { stopbits = val; return self; }
		UART_t& setHardwareFlowControl(XARTHwFlowCtl_E val) { hwflow = val; return self; }
		UART_t& setOverSampling8(bool ena = true) { oversampling8 = ena; return self; }
		UART_t& setOneBitSampling(bool ena = true) { onebit_sampling = ena; return self; }
		UART_t& setFIFOMode(bool ena = true) { fifo_mode = ena; return self; }
		UART_t& setTXFIFOThreshold(XARTFIFOThreshold_E val) { tx_fifo_threshold = val; return self; }
		UART_t& setRXFIFOThreshold(XARTFIFOThreshold_E val) { rx_fifo_threshold = val; return self; }
		void innByInterrupt();
		void abortReceive();
		bool outByInterrupt(const char* str, stduint len);
		void abortTransmit();
		bool isReady() {
			return !lock_r && !lock_t;
		}
		void ClearBuffer() { rx_pointer = nil; }
		stduint getBufferPointer() { return rx_pointer; }
		void innHandlerByInterrupt();
		void outHandlerByInterrupt();
		void outDoneHandlerByInterrupt();
	#endif
		_TODO
	};

	class USART_t : public UART_t {
	#if defined(_MCU_STM32H7x)
	protected:
		stduint status = ERR_UART_NONE;
	#endif
	public:
		USART_t(byte _XART_ID) : UART_t(_XART_ID) {}
	#if !defined(_MCU_STM32H7x)
		USART_t(stduint _baseaddr, byte _XART_ID) : UART_t(_baseaddr, _XART_ID) {}
	#endif
		int operator>> (int& res) { return (res = inn()) >= 0; }
		USART_t& operator<< (stduint dat) { UART_t::operator<<(dat); return self; }
		USART_t& operator<< (const char* p) { out(p, StrLength(p)); return self; }
	#if defined(_MCU_STM32H7x)
		bool setModeSync(stduint band_rate = 115200, bool clock_polarity = false, bool clock_phase = false, bool lastbit_ena = false, bool slave_ena = false);
	#endif
		virtual bool enClock(bool ena = true) override;
	#if defined(_MCU_STM32H7x)
		UART_CLKSRC getClockSource();
	#endif
	};

	class XART_SOFT {
		_TODO byte todo;
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

#elif defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
// F4: Now only accept USART1

namespace uni {
	extern USART_t XART1, XART2, XART3, XART6;
	extern UART_t  XART4, XART5, XART7, XART8;
}

#elif defined(_WinNT) || defined(_Linux)

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
#include "../../cpp/trait/XstreamTrait.hpp"

#if defined(_WinNT) || defined(_Linux)
#define PORTNAME_TYPE String
#else
#define PORTNAME_TYPE stduint
#endif

namespace uni {
	class UART_t : public IstreamTrait, public OstreamTrait
	{
		stduint baudrate;
		bool state;
		stduint databits;
		UARTCheck parity;
		UARTStopBit stopbits;

		PORTNAME_TYPE portname;// "\\\\.\\COM10" or "ttyUSB0"

		#ifdef _WinNT
		HANDLE pHandle;
		#elif defined(_Linux)
		int pHandle;
		#endif

	protected:
		virtual int inn() override;
		virtual int out(const char* str, stduint len) override;
	public:
		bool sync;

		UART_t(PORTNAME_TYPE portname, stduint baudrate = 115200) :
			baudrate(baudrate), state(false), databits(8), parity(UARTCheck::None), stopbits(UARTStopBit::One)
			, portname(portname)
			, sync(true)
		{
		}
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
#elif defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
#include "../../c/consio.h"

namespace uni {
	class x86_COM : public Console_t
	{
	public:
		virtual int out(const char* str, stduint len) override {
			for0(i, len) {
				while (!is_transmit_empty());
				outpb(PORT_COM1_DATA, str[i]);
			}
			return len;
		}
		virtual int inn() override {
			return innpb(PORT_COM1_DATA) & 0xFFu;
		}
	public:
		bool is_transmit_empty() { return innpb(0x3F8 + 5) & 0x20; }
	};

}

#elif defined(_MCCA) && ((_MCCA & 0xFF00) == 0x1000)
#include "../../cpp/string"
#include "../../cpp/interrupt"
#include "../../cpp/trait/XstreamTrait.hpp"

#include "../../c/board/QemuVirt-Riscv.h"

#define PORTNAME_TYPE stduint

namespace uni {
	// RISCV Qemuvirt
	enum XARTReg {
		RHR = 0, // Receive Holding Register (read mode)
		THR = 0, // Transmit Holding Register (write mode)
		DLL = 0, // LSB of Divisor Latch (write mode)
		IER = 1, // Interrupt Enable Register (write mode)
		DLM = 1, // MSB of Divisor Latch (write mode)
		FCR = 2, // FIFO Control Register (write mode)
		ISR = 2, // Interrupt Status Register (read mode)
		LCR = 3, // Line Control Register
		MCR = 4, // Modem Control Register

		// LSR BIT 0:
		// 0 = no data in receive holding register or FIFO.
		// 1 = data has been receive and saved in the receive holding register or FIFO.
		//...
		// LSR BIT 5:
		// 0 = transmit holding register is full. 16550 will not accept any data for transmission.
		// 1 = transmitter hold register (or FIFO) is empty. CPU can load the next character.
		LSR = 5, // Line Status Register
		#define _BITPOS_LSR_RX_READY 0
		#define _BITPOS_LSR_TX_IDLE  5

		MSR = 6, // Modem Status Register
		SPR = 7, // ScratchPad Register
	};// refer: TECHNICAL DATA ON 16550, http://byterunner.com/16550.html
	/*** POWER UP DEFAULTS
	* IER   = 0: TX/RX holding register interrupts are both disabled
	* ISR   = 1: no interrupt penting
	* LCR   = 0
	* MCR   = 0
	* LSR   = 60 HEX
	* MSR   = BITS 0-3 = 0, BITS 4-7 = inputs
	* FCR   = 0
	* TX    = High
	* OP1   = High
	* OP2   = High
	* RTS   = High
	* DTR   = High
	* RXRDY = High
	* TXRDY = Low
	* INT   = Low
	*/


	class UART_t : public IstreamTrait, public OstreamTrait, public RuptTrait
	{
		stduint baudrate;
		bool state;
		stduint databits;
		UARTCheck parity;
		UARTStopBit stopbits;

		PORTNAME_TYPE portname;// "\\\\.\\COM10" or "ttyUSB0"

	public:
		virtual int inn() override;
		virtual int out(const char* str, stduint len) override;
		virtual void setInterrupt(Handler_t _func) const override;
		virtual void setInterruptPriority(byte preempt, byte sub_priority) const override;
		virtual void enInterrupt(bool enable = true) const override;
	public:
		bool sync;

		UART_t(PORTNAME_TYPE portname, stduint baudrate = 115200) :
			baudrate(baudrate), state(false), databits(8), parity(UARTCheck::None), stopbits(UARTStopBit::One)
			, portname(portname)
			, sync(true)
		{
		}
		~UART_t();
		bool setMode(stduint _baudrate = 115200);
		bool operator>> (int& res);// return if new data received
		bool operator<< (stduint dat);
		inline UART_t& operator<< (const char* p) {
			while (*p) self << stduint(*p++);
			return self;
		}


		constexpr // constexpr & consteval(C++20?)
			byte& operator[](XARTReg reg) const {
			return *((byte*)ADDR_VIRT_UART0 + _IMM(reg));
		}

	};

	// for QEMUVIRT-RV UART0
	extern UART_t UART0;

}

#endif
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
namespace uni {
	static struct {
		pureptr_t operator[](byte id) {
			switch (id) {
			case 1: return (pureptr_t)&XART1;
			case 2: return (pureptr_t)&XART2;
			case 3: return (pureptr_t)&XART3;
			case 4: return (pureptr_t)&XART4;
			case 5: return (pureptr_t)&XART5;
			#if defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
			case 6: return (pureptr_t)&XART6;
			case 7: return (pureptr_t)&XART7;
			case 8: return (pureptr_t)&XART8;
			#endif
			default: return 0;
			}
		}
		static bool isSync(byte id) {
			return id == 1 || id == 2 || id == 3
			#if defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
				|| id == 6
			#endif
				;
		}
	} XART;
}
#endif

#endif

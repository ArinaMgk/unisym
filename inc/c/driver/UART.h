// ASCII CPL TAB4 CRLF
// Docutitle: [Device] Universal (Synchronous) Asynchronous Receiver Transmitter
// Datecheck: 20260731
// Developer: @ArinaMgk, @dosconio
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
#ifdef _WinNT
#define UNICODE
#define _INC_USTDBOOL
#endif

#include "../../c/consio.h"
#include "../../cpp/string"
#include "../../cpp/trait/XstreamTrait.hpp"
#include "_predefine/predef.xart.hpp"

namespace uni {
	enum class UARTCheck {
		None, Odd, Even, Mark
	};
	enum class UARTStopBit {
		One, OneHalf, Two
	};
#ifndef _INC_UNI_IOMethod
#define _INC_UNI_IOMethod
	enum class IOMethod : byte {
		Loop,   // polling
		Rupt,   // interrupt
		DMA     // direct memory access
	};
#endif
	class DMAStream; // forward declaration for UART DMA integration
}

/// UART
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
	} void (*RoutineINT4)(void) = RINT4;
*/

#elif defined(_MCU_STM32) || \
	(defined(_WinNT) || defined(_Linux)) || \
	(defined(_MCCA) && \
		((_MCCA & 0xFF00) == 0x1000 || (_MCCA & 0xFF00) == 0x8600) \
	)
namespace uni {
	class UART_t :
		#if !defined(_WinNT) && !defined(_Linux) && !defined(_ACCM)// hosted
		public RuptTrait,
		#endif
		public Console_t
	{
	public:
		#if defined(_MCU_STM32H7x)
		Slice rx_buffer = { 0, 0 };
		Slice tx_buffer = { 0, 0 };
		rostr error = NULL;
		const DMAStream* hdmatx = nullptr;
		const DMAStream* hdmarx = nullptr;
		#endif
	protected:
		#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)\
		|| (defined(_WinNT) || defined(_Linux)) \
		|| (defined(_MCCA) && ((_MCCA & 0xFF00) == 0x1000))
		stduint baudrate = 1;
		#endif

		#if (defined(_MCCA) && ((_MCCA & 0xFF00) == 0x1000)) \
		|| (defined(_WinNT) || defined(_Linux)) \
		|| (!defined(_MCU_STM32H7x) && defined(_MCU_STM32)) // ???
		stduint databits = 8;
		#endif

		#if defined(_MCU_STM32H7x)
		XARTParity_E parity = XARTParity_E::None;
		#elif (defined(_MCCA) && ((_MCCA & 0xFF00) == 0x1000)) \
		|| (defined(_WinNT) || defined(_Linux))
		UARTCheck parity;
		#endif

		#if defined(_MCU_STM32H7x)
		XARTStopBits_E stopbits = XARTStopBits_E::One;
		#elif (defined(_MCCA) && ((_MCCA & 0xFF00) == 0x1000)) \
		|| (defined(_WinNT) || defined(_Linux))
		UARTStopBit stopbits;
		#endif

		#if defined(_MCU_STM32)
		byte XART_ID;
		#else
		PORTNAME_TYPE portname;// "\\\\.\\COM10" or "ttyUSB0"
		#endif





		#if defined(_MCU_STM32)
		#if !defined(_MCU_STM32H7x) && defined(_MCU_STM32) // ???
		stduint baseaddr = nil;
		#endif
		#if defined(_MCU_STM32H7x)
		stduint rx_pointer = 0;// should < rx_buffer.length
		stduint tx_pointer = 0;// should < tx_buffer.length
		#endif




		#if defined(_MCU_STM32H7x)
		bool lock_r = false;
		bool lock_t = false;
		#endif
		#if !defined(_MCU_STM32H7x) && defined(_MCU_STM32) // ???
		UARTCheck check = UARTCheck::None;
		UARTStopBit stopbit = UARTStopBit::One;
		#endif
		#if defined(_MCU_STM32H7x)
		WordLength_E wordlen = WordLength_E::Bits8;
		
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
		#endif

		#if (defined(_MCCA) && ((_MCCA & 0xFF00) == 0x1000)) \
		|| (defined(_WinNT) || defined(_Linux))
		bool state;
	public:
		bool sync;
		#endif
		#ifdef _WinNT
		HANDLE pHandle;
		#elif defined(_Linux)
		int pHandle;
		#endif

	public:// interface
		// Rupt
		#if defined(_MCU_STM32) || defined(_MCCA)
		_COM_DEF_Interrupt_Interface();
		#endif
		// Console
		virtual int inn() override;
		virtual int out(const char* str, stduint len) override;
	//



	public:

		bool operator>> (int& res) { return (res = inn()) >= 0; }
		bool operator<< (stduint dat);
		UART_t& operator<< (const char* p) {
			while (*p) {
				if (!operator<<(*p++)) break;
				// F1/F4: Delay_unit();
			}
			return self;
		}

	public:

		// Init
		#if defined(_MCU_STM32)
		UART_t(byte _XART_ID) : XART_ID(_XART_ID) {}
		#if !defined(_MCU_STM32H7x) && defined(_MCU_STM32) // ???
		UART_t(stduint _baseaddr, byte _XART_ID) : baseaddr(_baseaddr), XART_ID(_XART_ID) {}
		#endif
		#elif (defined(_MCCA) && ((_MCCA & 0xFF00) == 0x1000)) \
		|| (defined(_WinNT) || defined(_Linux))
		UART_t(PORTNAME_TYPE portname, stduint baudrate = 115200) :
			baudrate(baudrate), state(false), databits(8), parity(UARTCheck::None), stopbits(UARTStopBit::One)
			, portname(portname)
			, sync(true)
		{
		}
		~UART_t();
		#endif

		// setMode Family
		bool setMode(stduint band_rate = 115200);

		// REG
		#if defined(_MCU_STM32)
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
		#elif (defined(_MCCA) && ((_MCCA & 0xFF00) == 0x1000))
		constexpr // constexpr & consteval(C++11)
			byte& operator[](XARTReg reg) const {
			return *((byte*)ADDR_VIRT_UART0 + _IMM(reg));
		}
		#endif

		// is_transmit_empty
		#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
		bool is_transmit_empty() { return innpb(0x3F8 + 5) & 0x20; }
		#endif

		#if defined(_MCU_STM32)
		#if defined(_MCU_STM32H7x)
		stduint getAddress() const;
		#else
		stduint getAddress() const { return baseaddr; }
		#endif


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
		void abortReceive();
		void abortTransmit();
		bool isReady() {
			return !lock_r && !lock_t;
		}
		void ClearBuffer() { rx_pointer = nil; }
		stduint getBufferPointer() { return rx_pointer; }
		void innHandlerByInterrupt();
		void outHandlerByInterrupt();
		void outDoneHandlerByInterrupt();
		bool canMode();
		int out(const char* str, stduint len, IOMethod method);
		stduint Receive(char* rx_data, stduint size, IOMethod method = IOMethod::Loop);
		void PauseDMA();
		void ResumeDMA();
		void StopDMA();
		stduint getDMARequestID(bool is_tx) const;
		#endif
		#endif
	};
}
#endif


/// USART
#ifdef _MCU_STM32
namespace uni {

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
		bool canMode();
		stduint Transmit(const char* tx_data, stduint size, IOMethod method = IOMethod::Loop);
		stduint Receive(char* rx_data, stduint size, IOMethod method = IOMethod::Loop);
		stduint Transceive(const char* tx_data, char* rx_data, stduint size, IOMethod method = IOMethod::Loop);
	#endif
	};

	class XART_SOFT {
		_TODO byte todo;
	};
}
#endif



#if defined(_MCU_STM32F1x)
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

#endif

#if defined(_MCU_STM32)
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
#elif (defined(_MCCA) && ((_MCCA & 0xFF00) == 0x1000))
// for QEMUVIRT-RV UART0
namespace uni {
    extern UART_t UART0;
}
#endif

#endif

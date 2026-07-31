
#ifndef _INC_DEV_PREDEF_UART
#define _INC_DEV_PREDEF_UART

#if defined(_MCU_STM32)
#include "../../../cpp/MCU/_ADDRESS/ADDR-STM32.h"
#include "../../../cpp/Device/RCC/RCC"
#include "../../../cpp/interrupt"

#if !defined(_MPU_STM32MP13)
#include "../../../cpp/Device/GPIO"
#endif

#if defined(_MCU_STM32H7x) && defined(_MCU_XART_TEMP)
#include "../../../cpp/Device/XART/XART-STM32H7.hpp"
#endif

#elif defined(_WinNT)

#include <WinSock2.h>
#include <windows.h>


#elif defined(_Linux)
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/select.h>

#elif defined(_MCCA)

#include "../../../cpp/interrupt"

#if ((_MCCA & 0xFF00) == 0x1000)
#include "../../../c/board/QemuVirt-Riscv.h"
#endif

#endif

///

#if defined(_WinNT) || defined(_Linux)
#define PORTNAME_TYPE String
#else
#define PORTNAME_TYPE stduint
#endif
#if defined(_MCU_STM32)


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

#endif

/// XART REG
#if defined(_MCU_STM32)
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

}

// 0:1b, 1:0.5b, 2:2b, 3:1.5b
#define USART_CR2_STOP(x) Stdfield(x[XARTReg::CR2], 12, 2)

#elif defined(_MCCA) && ((_MCCA & 0xFF00) == 0x1000)
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
}

#endif // XART REG





#endif

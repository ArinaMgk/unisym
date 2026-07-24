#define _MCU_XART_TEMP
#define _MCU_RCC_TEMP
#include "../../../inc/c/driver/UART.h"
#include "../../../inc/cpp/Device/RCC/RCC"

#ifdef _WinNT
namespace uni {
	UART_t::~UART_t() {
		HANDLE hCom = *(HANDLE*)pHandle;
		CloseHandle(hCom);
	}

	bool UART_t::setMode(stduint _baudrate) {
		baudrate = _baudrate;
		DCB dcb = { 0 };
		pHandle = CreateFileA(portname.reference(),
			GENERIC_READ | GENERIC_WRITE,
			nil,// not-shared
			NULL,// default security attributes
			OPEN_EXISTING,// open existing file
			sync ? 0 : FILE_FLAG_OVERLAPPED,
			NULL);// default template file
		state = pHandle != (HANDLE)None;
		asserv(state) = SetupComm(pHandle, 1024, 1024);
		dcb.DCBlength = sizeof(dcb);
		dcb.BaudRate = baudrate;
		dcb.ByteSize = databits;
		dcb.Parity = stduint(parity);
		dcb.StopBits = stduint(stopbits);
		asserv(state) = SetCommState(pHandle, &dcb);
		if (!state) return state;
		// AllTotal = XTTMultiplier * numsof_rw + XTTConstant (ms)
		COMMTIMEOUTS TimeOuts;
		TimeOuts.ReadIntervalTimeout = 1000;
		TimeOuts.ReadTotalTimeoutMultiplier = 500;
		TimeOuts.ReadTotalTimeoutConstant = 5000;
		TimeOuts.WriteTotalTimeoutMultiplier = 500;
		TimeOuts.WriteTotalTimeoutConstant = 2000;
		SetCommTimeouts(pHandle, &TimeOuts);
		PurgeComm(pHandle, PURGE_TXCLEAR | PURGE_RXCLEAR);// Clear Buffer
		return state;
	}

	bool UART_t::operator>> (int& res) {
		HANDLE hCom = pHandle;
		byte buf[4];
		dword dwBytesWritten;
		if (sync) {
			BOOL bReadStat = ReadFile(hCom, buf, 1, (LPDWORD)&dwBytesWritten, NULL/*sync*/);
			res = buf[0];// -52 is the EOF
		}
		else {
			DWORD dwErrorFlags;
			COMSTAT comStat;
			OVERLAPPED m_osRead = { 0 };
			m_osRead.hEvent = CreateEvent(NULL, TRUE, FALSE,
			#ifndef _DEV_GCC
				(LPCWSTR)L"ReadEvent"
			#else
				(LPCWSTR)"ReadEvent"
			#endif
				);
			ClearCommError(hCom, &dwErrorFlags, &comStat);
			if (!comStat.cbInQue)return 0;
			BOOL bReadStat = ReadFile(hCom, buf, 1, (LPDWORD)&dwBytesWritten, &m_osRead);
			if (!bReadStat) {
				if (GetLastError() == ERROR_IO_PENDING)
					GetOverlappedResult(hCom, &m_osRead, (LPDWORD)&dwBytesWritten, TRUE);// wait until read is done
				else {
					ClearCommError(hCom, &dwErrorFlags, &comStat);
					CloseHandle(m_osRead.hEvent);
					return false;
				}
				res = buf[0];// -52 is the EOF
			}
		}
		return dwBytesWritten;
	}
	
	bool UART_t::operator<< (stduint dat) {
		HANDLE hCom = pHandle;
		byte buf = dat;
		dword dwBytesWritten;
		if (sync) {
			BOOL bWriteStat = WriteFile(hCom, &buf, 1, (LPDWORD)&dwBytesWritten, NULL/*sync*/);
			if (!bWriteStat) return false;
		}
		else {
			DWORD dwErrorFlags;
			COMSTAT comStat;
			OVERLAPPED m_osWrite = { 0 };
			m_osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE,
			#ifndef _DEV_GCC
				(LPCWSTR)L"WriteEvent"
			#else
				(LPCWSTR)"WriteEvent"
			#endif
				);
			ClearCommError(hCom, &dwErrorFlags, &comStat);
			BOOL bWriteStat = WriteFile(hCom, &buf, 1, (LPDWORD)&dwBytesWritten, &m_osWrite);
			if (!bWriteStat) {
				if (GetLastError() == ERROR_IO_PENDING)
					WaitForSingleObject(m_osWrite.hEvent, 1000);// wait 1000ms
				else {
					ClearCommError(hCom, &dwErrorFlags, &comStat);
					CloseHandle(m_osWrite.hEvent);
					return false;
				}
			}
		}
		return dwBytesWritten;
	}
	
}

#elif defined(_Linux)
namespace uni {
	UART_t::~UART_t() {
		if (state) close(pHandle);
		state = false;
	}

	_WEAK bool UART_t::setMode(stduint _baudrate) {
		baudrate = _baudrate;
		int fd = open(portname.reference(), O_RDWR | O_NOCTTY);
		state = fd != -1;
		struct termios tty = { 0 };
		if (!state) return state;
		else tcgetattr(fd, &tty) == 0;
		if (!state) {
			close(fd);
			return state;
		}
		cfsetispeed(&tty, baudrate);
		cfsetospeed(&tty, baudrate);
		tty.c_cflag &= ~PARENB; // _TEMP no-parity
		tty.c_cflag &= ~CSTOPB; // _TEMP 1 stop bit
		tty.c_cflag &= ~CSIZE; // _TEMP clear data size
		tty.c_cflag |= CS8; // _TEMP 8 data bits
		tty.c_cflag &= ~CRTSCTS; // _TEMP no flow control
		tty.c_cflag |= CREAD | CLOCAL; // enable receiver and set local mode
		tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // raw input
		tty.c_iflag &= ~(IXON | IXOFF | IXANY); // disable software flow control
		tty.c_oflag &= ~OPOST; // raw output
		tty.c_cc[VMIN] = 1; // read a char
		tty.c_cc[VTIME] = 0; // no timeout
		state = tcsetattr(fd, TCSANOW, &tty) == 0;
		if (!state) {
			close(fd);
		}
		else tcflush(fd, TCIFLUSH);
		pHandle = fd;
		return state;
	}

	bool UART_t::operator>> (int& res) {
		char buf;
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(pHandle, &readfds);
		int loc_state = select(pHandle + 1, &readfds, NULL, NULL, NULL);
		if (loc_state == -1) {// wait for data, may block caller
			return false;
		}
		else if (loc_state > 0 && FD_ISSET(pHandle, &readfds)) {
			int bytes_read = read(pHandle, &buf, 1);
			res = buf;
			return bytes_read > 0;
		}
		return false;
	}

	bool UART_t::operator<< (stduint dat) {
		int written = write(pHandle, &dat, 1);
		return written > 0;
	}

}

#elif defined(_MCCA) && ((_MCCA & 0xFF00) == 0x1000) // for QEMUVIRT-RV UART0
namespace uni {
	UART_t UART0(0, _TEMP 115200);
	
	UART_t::~UART_t() {}

	int UART_t::inn() { int ch; return operator>>(ch) ? ch : -1; }
	int UART_t::out(const char* str, stduint len) {
		for0(i, len) operator<<(str[i]);
		return len;
	}
	void UART_t::setInterrupt(Handler_t _func) const { _TODO }
	void UART_t::setInterruptPriority(byte preempt, byte sub_priority) const {
		InterruptControl::setPriority(IRQ_UART0, preempt);
	}
	void UART_t::enInterrupt(bool enable) const {
		// receive interrupts
		Reference_T<uint8> ier(&(self[XARTReg::IER]));
		ier.setof(0, enable);
		//
		InterruptControl::setAble(IRQ_UART0, enable);
	}

	bool UART_t::setMode(stduint _baudrate) {
		self[XARTReg::IER] = 0x00;// disable interrupts
		// Setting baud rate
		// Some registers have same address. To change what the base address points to,  open the "divisor latch" by writing 1 into the Divisor Latch Access Bit (DLAB), which is bit index 7 of the Line Control Register (LCR).
		//{TEMP} use 38.4K when 1.8432 MHZ crystal (0x0003)
		self[XARTReg::LCR] |= _IMM1S(7);
		self[XARTReg::DLL] = 0x03;// Low
		self[XARTReg::DLM] = 0x00;// Hig
		// Setting the asynchronous data communication format
		//{TEMP} 8 bits word-length, stop bits：1 bit when word length is 8 bits, no parity, no break control, disabled baud latch
		self[XARTReg::LCR] = 3;
		return true;
	}

	// waiting-method. another method return false when no data
	bool UART_t::operator>> (int& res) {
		while (!(self[XARTReg::LSR] & _IMM1S(_BITPOS_LSR_RX_READY)));
		res = self[XARTReg::RHR];
		return true;
	}

	// waiting-method
	bool UART_t::operator<< (stduint dat) {
		while (!(self[XARTReg::LSR] & _IMM1S(_BITPOS_LSR_TX_IDLE)));
		self[XARTReg::THR] = dat;
		return true;
	}


}
#endif

_Comment("Interrupt") namespace uni {
	#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
	void UART_t::setInterrupt(Handler_t f) const {
		FUNC_XART[XART_ID] = f;
	}
	static Request_t XART_Request_list[] = { Request_None,
		IRQ_USART1, IRQ_USART2, IRQ_USART3, IRQ_UART4,
		IRQ_UART5,
		#ifdef _MCU_STM32F4x
		IRQ_USART6, Request_None, Request_None
		#endif
	};
	void UART_t::setInterruptPriority(byte preempt, byte sub_priority) const {
		NVIC.setPriority(XART_Request_list[XART_ID], preempt, sub_priority);
	}
	#endif
}


#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
#ifndef APB1PERIPH_BASE
#define APB1PERIPH_BASE 0x40000000U
#endif
#ifndef APB2PERIPH_BASE
#define APB2PERIPH_BASE 0x40010000U
#endif
// bi: 8 or 16
//:not check detailedly for F1
inline static stduint UART_DIV_SAMPLING(stduint _PCLK_, stduint _BAUD_, byte bi) {
	bi >>= 2;// div-by 4
	return (_PCLK_ * 25U) / (bi * _BAUD_);
}
inline static stduint UART_DIVMANT_SAMPLING(stduint _PCLK_, stduint _BAUD_, byte bi) {
	return (UART_DIV_SAMPLING(_PCLK_, _BAUD_, bi) / 100U);
}
inline static stduint UART_DIVFRAQ_SAMPLING(stduint _PCLK_, stduint _BAUD_, byte bi) {
	return (((UART_DIV_SAMPLING(_PCLK_, _BAUD_, bi) - (UART_DIVMANT_SAMPLING(_PCLK_, _BAUD_, bi) * 100U)) * bi + 50U) / 100U);
}
// 16: UART BRR = mantissa + overflow + fraction = (UART DIVMANT << 4) + (UART DIVFRAQ & 0xF0) + (UART DIVFRAQ & 0x0FU)
//  8: UART BRR = mantissa + overflow + fraction = (UART DIVMANT << 4) + ((UART DIVFRAQ & 0xF8) << 1) + (UART DIVFRAQ & 0x07U)
inline static stduint UART_BRR_SAMPLING(stduint _PCLK_, stduint _BAUD_, byte bi) {
	if (bi == 16)
		return (((UART_DIVMANT_SAMPLING(_PCLK_, _BAUD_, 16) << 4U) + (UART_DIVFRAQ_SAMPLING(_PCLK_, _BAUD_, 16) & 0xF0U)) + (UART_DIVFRAQ_SAMPLING(_PCLK_, _BAUD_, 16) & 0x0FU));
	else if (bi == 8)
		return (((UART_DIVMANT_SAMPLING(_PCLK_, _BAUD_, 8) << 4U) + ((UART_DIVFRAQ_SAMPLING(_PCLK_, _BAUD_, 8) & 0xF8U) << 1U)) + (UART_DIVFRAQ_SAMPLING(_PCLK_, _BAUD_, 8) & 0x07U));
	return 0;
}
namespace uni {
	#if defined(_MCU_STM32F1x)
	USART_t XART1(APB2PERIPH_BASE + 0x3800, 1),
		XART2(APB1PERIPH_BASE + 0x4400, 2),
		XART3(APB1PERIPH_BASE + 0x4800, 3);
	UART_t XART4(APB1PERIPH_BASE + 0x4C00, 4),
		XART5(APB1PERIPH_BASE + 0x5000, 5);
	#elif defined(_MCU_STM32F4x)
	USART_t XART1(APB2PERIPH_BASE + 0x1000, 1),
		XART2(APB1PERIPH_BASE + 0x4400, 2),
		XART3(APB1PERIPH_BASE + 0x4800, 3),
		XART6(APB2PERIPH_BASE + 0x1400, 6);
	UART_t XART4(APB1PERIPH_BASE + 0x4C00, 4),
		XART5(APB1PERIPH_BASE + 0x5000, 5),
		XART7(APB1PERIPH_BASE + 0x7800, 7),
		XART8(APB1PERIPH_BASE + 0x7C00, 8);
	#endif

	static void setMode_initGPIO(byte XART_ID);
	//aka HAL_UART_Init
	bool UART_t::setMode(stduint band_rate) {
		using namespace XARTReg;
		{
			//{TODO} busy flag
			if (!enClock()) return false;
			if (XART_ID == 1) setMode_initGPIO(XART_ID);// HAL_UART_MspInit
			enAble(false);
			//aka UART_SetConfig(UART_HandleTypeDef *huart)
			{
				self[CR2] &= ~_IMM(0x3 << 12);// set zero, aka UartHandle.Init.StopBits = UART_STOPBITS_1;
				if (stopbit == UARTStopBit::Two)
					self[CR2] |= _IMM(0x2 << 12);
				else if (stopbit == UARTStopBit::OneHalf)
					self[CR2] |= _IMM(0x3 << 12);
				self[CR1] &= ~_IMM(0x3 << 9);// aka UartHandle.Init.Parity = UART_PARITY_NONE; (USART_CR1_PCE and USART_CR1_PS)
				if (check != UARTCheck::None) {
					self[CR1].setof(10);
					self[CR1].setof(9, check == UARTCheck::Odd);
				}
				self[CR1].setof(12, databits == 9);// aka UartHandle.Init.WordLength = UART_WORDLENGTH_8B/9B; (USART_CR1_M)
				self[CR1] |= 0xC;//aka UartHandle.Init.Mode = UART_MODE_TX_RX; (USART_CR1_TE | USART_CR1_RE)
				self[CR1].rstof(15);// (USART_CR1_OVER8)
				self[CR3] &= ~_IMM(0x3 << 8);//aka UartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE; (USART_CR3_RTSE | USART_CR3_CTSE)
				// : byte over_sampling = _TEMP 16;// or 8
				stduint freq = XART_ID == 1 || XART_ID == 6 ? RCC.getFrequencyPCLK2() : RCC.getFrequencyPCLK1();
				last_baudrate = band_rate;
				self[BRR] = UART_BRR_SAMPLING(freq, band_rate, 16);
			}
			// In asynchronous mode, the following bits must be kept cleared:
			// - LINEN and CLKEN bits in the USART_CR2 register,
			// - SCEN, HDSEL and IREN  bits in the USART_CR3 register.
			self[CR2].rstof(_USART_CR2_POSI_CLKEN);
			self[CR2].rstof(_USART_CR2_POSI_LINEN);
			self[CR3].rstof(_USART_CR3_POSI_IREN);
			self[CR3].rstof(_USART_CR3_POSI_HDSEL);
			self[CR3].rstof(_USART_CR3_POSI_SCEN);
			enAble(true);
		}
		return true;
	}

	//aka HAL_UART_Receive
	//{TODO} conflict
	int UART_t::operator>> (int& res) {
		using namespace XARTReg;
		bool len9b = databits == 9;// or 8b
		bool parity_enable = check != UARTCheck::None;
		stduint mask = len9b ?
			(parity_enable ? 0xFF : 0x1FF) :
			(parity_enable ? 0x7F : 0xFF);
		int d = int(self[DR]);
		return res = d & mask;
	}

	int UART_t::inn() {
		int res;
		return operator>>(res) ? res : -1;
	}

	int UART_t::out(const char* str, stduint len) {
		for0(i, len) self << stduint(str[i]);
		return len;
	}

	void UART_t::Delay_unit() {
		for (volatile stduint i = 0; i < SystemCoreClock / last_baudrate; i++);
	}

	//aka HAL_UART_Transmit
	//{TODO} conflict
	UART_t& UART_t::operator << (stduint dat) {
		using namespace XARTReg;
		bool len9b = databits == 9;// or 8b
		while (!self[SR].bitof(7));// TXE
		self[DR] = dat & (len9b ? 0x1FF : 0x0FF);
		return self;
	}
	UART_t& UART_t::operator << (const char* p) {
		while (*p) {
			self << stduint(*p++);
			Delay_unit();
		}
		return self;
	}

	bool UART_t::enAble(bool ena) {
		using namespace XARTReg;
		self[CR1].setof(13, ena);// 13 is USART_CR1_UE_Pos
		return true;
	}

	bool UART_t::enClock(bool ena) {
		using namespace RCCReg;
		switch (XART_ID) {
		case 4:
			RCC[APB1ENR].setof(19, ena);
			return RCC[APB1ENR].bitof(19) == ena;
		case 5:
			RCC[APB1ENR].setof(20, ena);
			return RCC[APB1ENR].bitof(20) == ena;
		#if defined(_MCU_STM32F4x)
		case 7:
			RCC[APB1ENR].setof(30, ena);
			return RCC[APB1ENR].bitof(30) == ena;
		case 8:
			RCC[APB1ENR].setof(31, ena);
			return RCC[APB1ENR].bitof(31) == ena;
		#endif
		default:
			return false;
		}
	}

	bool USART_t::enClock(bool ena) {
		using namespace RCCReg;
		switch (XART_ID) {
		case 1:
			RCC[APB2ENR].setof(_RCC_APB2ENR_POSI_ENCLK_USART1, ena);
			return RCC[APB2ENR].bitof(_RCC_APB2ENR_POSI_ENCLK_USART1) == ena;
		case 2:
			RCC[APB1ENR].setof(17, ena);
			return RCC[APB1ENR].bitof(17) == ena;
		case 3:
			RCC[APB1ENR].setof(18, ena);
			return RCC[APB1ENR].bitof(18) == ena;
		#if defined(_MCU_STM32F4x)
		case 6:
			RCC[APB2ENR].setof(_RCC_APB2ENR_POSI_ENCLK_USART6, ena);
			return RCC[APB2ENR].bitof(_RCC_APB2ENR_POSI_ENCLK_USART6) == ena;
		#endif
		default:
			return false;
		}
	}

	// ---- ---- INTSYS ---- ----



	void UART_t::enInterrupt(bool enable) const {
		using namespace XARTReg;
		if (enable)
		{
			NVIC.setAble(XART_Request_list[XART_ID]);
			stduint val = (_IMM1 << 28U | 0x00000020);// _TEMP only UART_IT_RXNE aka ((uint32_t)(UART_CR1_REG_INDEX << 28U | USART_CR1_RXNEIE)) aka this
			stduint UART_IT_MASK = 0x0000FFFFU;
			(((val >> 28U) == 1/*UART_CR1_REG_INDEX*/) ? (self[CR1] |= (val & UART_IT_MASK)) :
				((val >> 28U) == 2/*UART_CR2_REG_INDEX*/) ? (self[CR2] |= (val & UART_IT_MASK)) :
				(self[CR3] |= (val & UART_IT_MASK)));//aka __HAL_UART_ENABLE_IT
		}
		else _TODO;
	}
}
#endif

#if 0

#elif defined(_MCU_STM32F1x)

namespace uni {
	static void setMode_initGPIO(byte XART_ID) {
		GPIOA[9].setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::Atmost_50MHz);// Tx
		GPIOA[9].setPull(true);
		GPIOA[10].setMode(GPIOMode::IN_Floating);// Rx
	}
}

#elif defined(_MCU_STM32F4x)

namespace uni {

	//{ONLY}  F407 & F417
	static byte GPINs_AFs_XARTx[1 + 6] = { nil,
		7,7,7, 8,8,8
	};

	static void setMode_initGPIO(byte XART_ID) {
		GPIOA[9].setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::Veryhigh);// Tx
		GPIOA[9].setPull(true);
		GPIOA[10].setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::Veryhigh);// Rx
		GPIOA[10].setPull(true);
		(void)"GPIO Config for AUF regs"; {
			byte GPIO_AF7_USART1 = 0x07;
			GPIOA[ 9]._set_alternate(GPINs_AFs_XARTx[XART_ID]);
			GPIOA[10]._set_alternate(GPINs_AFs_XARTx[XART_ID]);
		}
	}





}

#endif

#if defined(_MPU_STM32MP13)

#include "../../../inc/c/driver/interrupt/GIC.h"

namespace uni {
	USART_t XART1(APB6_PERIPH_BASE + 0x0000, 1),
		XART2(APB6_PERIPH_BASE + 0x1000, 2),
		XART3(APB1_PERIPH_BASE + 0xF000, 3),
		XART6(APB2_PERIPH_BASE + 0x3000, 6);
	UART_t XART4(APB1_PERIPH_BASE + 0x10000, 4),
		XART5(APB1_PERIPH_BASE + 0x11000, 5),
		XART7(APB1_PERIPH_BASE + 0x18000, 7),
		XART8(APB1_PERIPH_BASE + 0x19000, 8);

	static Request_t XART_Request_list[] = {
		Request_None,
		IRQ_USART1, IRQ_USART2, IRQ_USART3, IRQ_UART4,
		IRQ_UART5, IRQ_USART6, IRQ_UART7, IRQ_UART8,
	};

	static stduint xart_mask(stduint databits, UARTCheck parity) {
		const bool parity_enable = parity != UARTCheck::None;
		if (databits == 9) return parity_enable ? 0xFF : 0x1FF;
		if (databits == 7) return parity_enable ? 0x3F : 0x7F;
		return parity_enable ? 0x7F : 0xFF;
	}
	static stduint xart_mp13_apbDiv(RCCReg::RCCReg reg) {
		stduint div = RCC[reg].masof(0, 3);
		return div <= 4 ? (_IMM1 << div) : 0;
	}
	static stduint xart_mp13_getPCLK(byte xart_id) {
		using namespace RCCReg;
		RCCReg::RCCReg reg;
		switch (xart_id) {
		case 1:
		case 2:
			reg = APB6DIVR;
			break;
		case 6:
			reg = APB2DIVR;
			break;
		case 3:
		case 4:
		case 5:
		case 7:
		case 8:
			reg = APB1DIVR;
			break;
		default:
			return 0;
		}
		stduint div = xart_mp13_apbDiv(reg);
		return div ? RCC.Sysclock.getCoreFrequency() / div : 0;
	}
	static stduint xart_divSampling16(stduint pclk, stduint baud, stduint prescaler = 1) {
		return baud ? (((pclk / prescaler) + (baud / 2)) / baud) : 0;
	}
	static bool xart_mp13_setClock(byte xart_id, bool ena) {
		using namespace RCCReg;
		RCCReg::RCCReg reg = ena ? MP_APB1ENSETR : MP_APB1ENCLRR;
		byte bit = 0;
		switch (xart_id) {
		case 1: reg = ena ? MP_APB6ENSETR : MP_APB6ENCLRR; bit = 0; break;
		case 2: reg = ena ? MP_APB6ENSETR : MP_APB6ENCLRR; bit = 1; break;
		case 3: bit = 15; break;
		case 4: bit = 16; break;
		case 5: bit = 17; break;
		case 6: reg = ena ? MP_APB2ENSETR : MP_APB2ENCLRR; bit = 13; break;
		case 7: bit = 18; break;
		case 8: bit = 19; break;
		default: return false;
		}
		RCC[reg] = _IMM1S(bit);
		return true;
	}

	void UART_t::setInterrupt(Handler_t f) const {
		FUNC_XART[XART_ID] = f;
	}
	void UART_t::setInterruptPriority(byte preempt, byte sub_priority) const {
		(void)sub_priority;
		GIC.setPriority(XART_Request_list[XART_ID], preempt);
	}
	void UART_t::enInterrupt(bool enable) const {
		GIC.enInterrupt(XART_Request_list[XART_ID], enable);
		self[XARTReg::CR1].setof(5, enable);// RXNEIE_RXFNEIE
	}
	int UART_t::inn() {
		return int(self[XARTReg::RDR] & xart_mask(databits, check));
	}
	int UART_t::out(const char* str, stduint len) {
		for0(i, len) {
			while (!self[XARTReg::ISR].bitof(7));// TXE_TXFNF
			self[XARTReg::TDR] = uint8(str[i]);
		}
		return len;
	}
	UART_t& UART_t::operator<< (stduint dat) {
		while (!self[XARTReg::ISR].bitof(7));// TXE_TXFNF
		self[XARTReg::TDR] = dat & xart_mask(databits, check);
		return self;
	}
	bool UART_t::setMode(stduint band_rate) {
		if (!band_rate || !enClock()) return false;
		stduint pclk = xart_mp13_getPCLK(XART_ID);
		if (!pclk) return false;
		enAble(false);
		self[XARTReg::CR1] = 0;
		self[XARTReg::CR2] = 0;
		self[XARTReg::CR3] = 0;
		stduint usartdiv = xart_divSampling16(pclk, band_rate);
		if (usartdiv < 0x10 || usartdiv > 0xFFFF) return false;
		self[XARTReg::BRR] = uint16(usartdiv);
		self[XARTReg::CR1].setof(3);// TE
		self[XARTReg::CR1].setof(2);// RE
		self[XARTReg::CR1].setof(12, databits == 9);// M0
		self[XARTReg::CR1].setof(28, databits == 7);// M1
		if (check != UARTCheck::None) {
			self[XARTReg::CR1].setof(10);// PCE
			self[XARTReg::CR1].setof(9, check == UARTCheck::Odd);
		}
		if (stopbit == UARTStopBit::Two)
			self[XARTReg::CR2] |= _IMM(0x2 << 12);
		else if (stopbit == UARTStopBit::OneHalf)
			self[XARTReg::CR2] |= _IMM(0x3 << 12);
		return enAble(true);
	}
	bool UART_t::enAble(bool ena) {
		self[XARTReg::CR1].setof(0, ena);
		return self[XARTReg::CR1].bitof(0) == ena;
	}
	bool UART_t::enClock(bool ena) {
		return xart_mp13_setClock(XART_ID, ena);
	}
	bool USART_t::enClock(bool ena) {
		return xart_mp13_setClock(XART_ID, ena);
	}
}

#endif

#if defined(_MCU_STM32H7x)

namespace uni {
	USART_t XART1(1), XART2(2), XART3(3), XART6(6);
	UART_t  XART4(4), XART5(5), XART7(7), XART8(8);

	static const stduint xart_addr[8]{
	D2_APB2PERIPH_BASE + 0x1000, D2_APB1PERIPH_BASE + 0x4400, D2_APB1PERIPH_BASE + 0x4800, D2_APB1PERIPH_BASE + 0x4C00,
	D2_APB1PERIPH_BASE + 0x5000, D2_APB2PERIPH_BASE + 0x1400, D2_APB1PERIPH_BASE + 0x7800, D2_APB1PERIPH_BASE + 0x7C00,
	};

	static Request_t XART_Request_list[] = {
		Request_None,
		IRQ_USART1, IRQ_USART2, IRQ_USART3, IRQ_UART4,
		IRQ_UART5, IRQ_USART6, IRQ_UART7, IRQ_UART8,
	};

	static Request_t xart_getRequest(byte xart_id) {
		return Ranglin(xart_id, 1, 8) ? XART_Request_list[xart_id] : Request_None;
	}

	static bool xart_enClock_USART(byte xart_id, bool ena) {
		if (!XART.isSync(xart_id)) return false;
		switch (xart_id) {
		case 1:
			RCC[RCCReg::APB2ENR].setof(4, ena);
			return RCC[RCCReg::APB2ENR].bitof(4) == ena;
		case 2:
			RCC[RCCReg::APB1LENR].setof(RCC_APB1LENR_USART2EN_Pos, ena);
			return RCC[RCCReg::APB1LENR].bitof(RCC_APB1LENR_USART2EN_Pos) == ena;
		case 3:
			RCC[RCCReg::APB1LENR].setof(RCC_APB1LENR_USART3EN_Pos, ena);
			return RCC[RCCReg::APB1LENR].bitof(RCC_APB1LENR_USART3EN_Pos) == ena;
		case 6:
			RCC[RCCReg::APB2ENR].setof(RCC_APB2ENR_USART6EN_Pos, ena);
			return RCC[RCCReg::APB2ENR].bitof(RCC_APB2ENR_USART6EN_Pos) == ena;
		default:
			return false;
		}
	}

	static bool xart_enClock_UART(byte xart_id, bool ena) {
		if (XART.isSync(xart_id)) return false;
		switch (xart_id) {
		case 4:
			RCC[RCCReg::APB1LENR].setof(RCC_APB1LENR_UART4EN_Pos, ena);
			return RCC[RCCReg::APB1LENR].bitof(RCC_APB1LENR_UART4EN_Pos) == ena;
		case 5:
			RCC[RCCReg::APB1LENR].setof(RCC_APB1LENR_UART5EN_Pos, ena);
			return RCC[RCCReg::APB1LENR].bitof(RCC_APB1LENR_UART5EN_Pos) == ena;
		case 7:
			RCC[RCCReg::APB1LENR].setof(RCC_APB1LENR_UART7EN_Pos, ena);
			return RCC[RCCReg::APB1LENR].bitof(RCC_APB1LENR_UART7EN_Pos) == ena;
		case 8:
			RCC[RCCReg::APB1LENR].setof(RCC_APB1LENR_UART8EN_Pos, ena);
			return RCC[RCCReg::APB1LENR].bitof(RCC_APB1LENR_UART8EN_Pos) == ena;
		default:
			return false;
		}
	}

	static UART_CLKSRC xart_getClockSource(byte xart_id) {
		switch (xart_id) {
		case 1: case 6:
			return UART_CLKSRC _IMM(RCC_D2CCIP2R_USART16SEL);
		case 2: case 3: case 4: case 5: case 7: case 8:
			return UART_CLKSRC _IMM(RCC_D2CCIP2R_USART28SEL);
		default:
			return UART_CLKSRC::UNDEFINED;
		}
	}
	static bool xart_parityEnable(XARTParity_E parity) {
		return parity != XARTParity_E::None;
	}
	static stduint xart_mask(WordLength_E wordlen, bool parity_enable) {
		switch (wordlen) {
		case WordLength_E::Bits7:
			return parity_enable ? 0x3F : 0x7F;
		case WordLength_E::Bits8:
			return parity_enable ? 0x7F : 0xFF;
		case WordLength_E::Bits9:
			return parity_enable ? 0xFF : 0x1FF;
		default:
			return 0;
		}
	}
	static stduint xart_rxUnit(WordLength_E wordlen, bool parity_enable) {
		return (wordlen == WordLength_E::Bits9 && !parity_enable) ? 2 : 1;
	}
	static WordLength_E xart_wordLengthFromDataBits(stduint bits) {
		switch (bits) {
		case 7:
			return WordLength_E::Bits7;
		case 9:
			return WordLength_E::Bits9;
		default:
			return WordLength_E::Bits8;
		}
	}
	static XARTParity_E xart_parityFromCheck(UARTCheck val) {
		switch (val) {
		case UARTCheck::Odd:
			return XARTParity_E::Odd;
		case UARTCheck::Even:
			return XARTParity_E::Even;
		default:
			return XARTParity_E::None;
		}
	}
	static XARTStopBits_E xart_stopBitsFromStopBit(UARTStopBit val) {
		switch (val) {
		case UARTStopBit::Two:
			return XARTStopBits_E::Two;
		case UARTStopBit::OneHalf:
			return XARTStopBits_E::OneHalf;
		default:
			return XARTStopBits_E::One;
		}
	}
	static void xart_setHwFlow(Reference& cr3, XARTHwFlowCtl_E hwflow) {
		USART_CR3_RTSE_REF(cr3) = _IMM(hwflow) & 1;
		USART_CR3_CTSE_REF(cr3) = (_IMM(hwflow) >> 1) & 1;
	}
	static void xart_setFIFOThresholds(Reference& cr3, XARTFIFOThreshold_E tx, XARTFIFOThreshold_E rx) {
		USART_CR3_RXFTCFG_REF(cr3) = _IMM(rx);
		USART_CR3_TXFTCFG_REF(cr3) = _IMM(tx);
	}

	static byte GPINs_AFs_XARTx[8] = {
		7,7,7, 8,8, 7,7,8
	};

	stduint UART_t::getAddress() const {
		return Ranglin(XART_ID, 1, 8) ? xart_addr[XART_ID - 1] : nil;
	}

	Reference UART_t::operator[](XARTReg::USARTReg idx) const { return getAddress() + _IMM(idx); }
	Reference_T<uint16> UART_t::operator[](XARTReg::USARTReg16 idx) const { return getAddress() + _IMM(idx); }
	static bool setMode_initGPIO(byte xart_id) {
		if (!Ranglin(xart_id, 1, 8)) return false;
		xart_id--;
		static GPIN* RPins[8] _TODO{
			&GPIOA[10], &GPIOD[6], &GPIOB[11], &GPIOC[11],
			&GPIOD[2], &GPIOC[7], &GPIOE[7], &GPIOE[0],
		};
		static GPIN* TPins[8] _TODO {
			&GPIOA[9], &GPIOD[5], &GPIOB[10], &GPIOC[10],
			&GPIOC[12], &GPIOC[6], &GPIOE[8], &GPIOE[1],
		};
		if (!RPins[xart_id] || !TPins[xart_id]) return false;
		RPins[xart_id]->setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::Veryhigh).setPull(true);// Rx
		RPins[xart_id]->_set_alternate(GPINs_AFs_XARTx[xart_id]);
		TPins[xart_id]->setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::Veryhigh).setPull(true);// Tx
		TPins[xart_id]->_set_alternate(GPINs_AFs_XARTx[xart_id]);
		return true;
	}
	static bool setMode_initSyncGPIO(byte xart_id) {
		if (!XART.isSync(xart_id)) return false;
		xart_id--;
		static GPIN* CKPins[8] _TODO{
			&GPIOA[8], &GPIOD[7], &GPIOB[12], nullptr,
			nullptr, &GPIOC[8], nullptr, nullptr,
		};
		static byte CKAFs[8] _TODO{
			7, 7, 8, 0, 0, 7, 0, 0
		};
		if (!CKPins[xart_id]) return false;
		CKPins[xart_id]->setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::Veryhigh).setPull(true);
		CKPins[xart_id]->_set_alternate(CKAFs[xart_id]);
		return true;
	}

	// ----

	void UART_t::setInterrupt(Handler_t _func) const {
		FUNC_XART[XART_ID] = _func;
	}
	void UART_t::setInterruptPriority(byte preempt, byte sub_priority) const {
		NVIC.setPriority(xart_getRequest(XART_ID), preempt, sub_priority);
	}
	void UART_t::enInterrupt(bool enable) const {
		NVIC.setAble(xart_getRequest(XART_ID), enable);
	}
	// AKA HAL_UART_Receive_IT
	// AKA UART_Receive_IT
	void UART_t::innByInterrupt() {
		if (!rx_buffer.address || rx_pointer >= rx_buffer.length) return;
		while (lock_r);
		lock_r = true;
		// (frame error, noise error, overrun error)
		USART_CR3_EIE(self) = true;
		// Enable the UART Parity Error interrupt and RX FIFO Threshold interrupt
		// (if FIFO mode is enabled) or Data Register Not Empty interrupt
		// (if FIFO mode is disabled)
		USART_CR1_PEIE(self) = xart_parityEnable(parity);
		if _IMM(USART_CR1_FIFOEN(self))
			USART_CR3_RXFTIE(self) = true;
		else
			USART_CR1_RXNEIE(self) = true;
	}
	// Disable the UART Parity Error Interrupt, RXNE/RXFT interrupt and UART Error Interrupt.
	void UART_t::abortReceive() {
		USART_CR1_RXNEIE(self) = 0;
		USART_CR1_PEIE(self) = 0;
		USART_CR3_EIE(self) = 0;
		USART_CR3_RXFTIE(self) = 0;
		lock_r = false;
	}

	void UART_t::innHandlerByInterrupt() {
		const bool parity_enable = xart_parityEnable(parity);
		const stduint rx_unit = xart_rxUnit(wordlen, parity_enable);
		bool rx_done = false;
		/* Check that a Rx process is ongoing */
		if (lock_r) {
			if (rx_pointer + rx_unit <= rx_buffer.length) {
				if (rx_unit == 2) {
					*(uint16*)(rx_buffer.address + rx_pointer) = inn();
					rx_pointer += 2;
				}
				else {
					*(uint8*)(rx_buffer.address + rx_pointer) = inn();
					rx_pointer++;
				}
				// now the buffer slot is the data received
				rx_done = rx_pointer >= rx_buffer.length;
			}
			else {
				inn();// drop
				rx_done = true;
			}
			if (rx_done) {
				abortReceive();
			}
		}
	}

	// ----

	static int xart_inn(stduint address, stduint mask, bool wide) {
		if (!address) return 0;
		stduint uhdata = (uint16)Reference_T<uint16>(address + _IMM(XARTReg::RDR));
		return wide ? uint16(uhdata & mask) : uint8(uhdata & mask);
	}

	static int xart_out(stduint address, const char* str, stduint len, stduint mask, bool wide) {
		if (!address) return 0;
		if (wide && (len & 1)) return 0;
		for (stduint i = 0; i < len; i += wide ? 2 : 1) {
			while (!Reference(address + _IMM(XARTReg::ISR)).bitof(6));
			Reference_T<uint16>(address + _IMM(XARTReg::TDR)) =
				wide ? (*(uint16*)(str + i) & mask) : (*(uint8*)(str + i) & mask);
		}
		return len;
	}
	static bool xart_outByInterrupt(stduint address, Slice& tx_buffer, stduint& tx_pointer, bool& lock_t, const char* str, stduint len) {
		if (!address || !str || !len || lock_t) return false;
		tx_buffer.address = (stduint)str;
		tx_buffer.length = len;
		tx_pointer = 0;
		lock_t = true;
		if (Reference(address + _IMM(XARTReg::CR1)).bitof(29))
			Reference(address + _IMM(XARTReg::CR3)) |= USART_CR3_TXFTIE;
		else
			Reference(address + _IMM(XARTReg::CR1)) |= USART_CR1_TXEIE;
		return true;
	}
	static void xart_abortTransmit(stduint address, bool& lock_t) {
		if (!address) return;
		Reference(address + _IMM(XARTReg::CR1)) &= ~(USART_CR1_TXEIE | USART_CR1_TCIE);
		Reference(address + _IMM(XARTReg::CR3)) &= ~USART_CR3_TXFTIE;
		lock_t = false;
	}
	static void xart_outHandlerByInterrupt(stduint address, Slice& tx_buffer, stduint& tx_pointer, bool& lock_t, WordLength_E wordlen, bool parity_enable) {
		if (!address || !lock_t) return;
		const bool wide = xart_rxUnit(wordlen, parity_enable) == 2;
		const stduint tx_unit = wide ? 2 : 1;
		if (tx_pointer + tx_unit <= tx_buffer.length) {
			Reference_T<uint16>(address + _IMM(XARTReg::TDR)) =
				wide ? (*(uint16*)(tx_buffer.address + tx_pointer) & 0x1FF) : (*(uint8*)(tx_buffer.address + tx_pointer) & 0xFF);
			tx_pointer += tx_unit;
		}
		if (tx_pointer >= tx_buffer.length) {
			if (Reference(address + _IMM(XARTReg::CR1)).bitof(29))
				Reference(address + _IMM(XARTReg::CR3)) &= ~USART_CR3_TXFTIE;
			else
				Reference(address + _IMM(XARTReg::CR1)) &= ~USART_CR1_TXEIE;
			Reference(address + _IMM(XARTReg::CR1)) |= USART_CR1_TCIE;
		}
	}
	static void xart_outDoneHandlerByInterrupt(stduint address, bool& lock_t) {
		if (!address) return;
		Reference(address + _IMM(XARTReg::CR1)) &= ~USART_CR1_TCIE;
		lock_t = false;
	}

	int UART_t::inn() {
		const bool parity_enable = xart_parityEnable(parity);
		return xart_inn(getAddress(), xart_mask(wordlen, parity_enable), xart_rxUnit(wordlen, parity_enable) == 2);
	}
	int UART_t::out(const char* str, stduint len) {
		const bool parity_enable = xart_parityEnable(parity);
		return xart_out(getAddress(), str, len, xart_mask(wordlen, parity_enable), xart_rxUnit(wordlen, parity_enable) == 2);
	}
	UART_t& UART_t::operator<< (stduint dat) {
		out((const char*)&dat, xart_rxUnit(wordlen, xart_parityEnable(parity)));
		return self;
	}
	UART_t& UART_t::setDataBits(stduint bits) {
		wordlen = xart_wordLengthFromDataBits(bits);
		return self;
	}
	UART_t& UART_t::setCheck(UARTCheck val) {
		parity = xart_parityFromCheck(val);
		return self;
	}
	UART_t& UART_t::setStopBit(UARTStopBit val) {
		stopbits = xart_stopBitsFromStopBit(val);
		return self;
	}
	bool UART_t::outByInterrupt(const char* str, stduint len) {
		if (xart_rxUnit(wordlen, xart_parityEnable(parity)) == 2 && (len & 1)) return false;
		return xart_outByInterrupt(getAddress(), tx_buffer, tx_pointer, lock_t, str, len);
	}
	void UART_t::abortTransmit() {
		xart_abortTransmit(getAddress(), lock_t);
	}
	void UART_t::outHandlerByInterrupt() {
		xart_outHandlerByInterrupt(getAddress(), tx_buffer, tx_pointer, lock_t, wordlen, xart_parityEnable(parity));
	}
	void UART_t::outDoneHandlerByInterrupt() {
		xart_outDoneHandlerByInterrupt(getAddress(), lock_t);
	}

	// ----

	#undef D3PCLK1
	#define UART_DIV_SAMPLING8(__PCLK__, __BAUD__, __PRESCALER__)   (((((__PCLK__)/uart_prescalers[(__PRESCALER__)])*2) + ((__BAUD__)/2)) / (__BAUD__))
	#define UART_DIV_SAMPLING16(__PCLK__, __BAUD__, __PRESCALER__)  ((((__PCLK__)/uart_prescalers[(__PRESCALER__)]) + ((__BAUD__)/2)) / (__BAUD__))
	static const uint16 uart_prescalers[] {
		1, 2, 4, 6, 8, 10, 12, 16, 32, 64, 128, 256
	};

	// HAL_UART_Init / UART_SetConfig reference:
	// - UART_InitTypeDef: WordLength, StopBits, Parity, Mode, HwFlowCtl, OverSampling, OneBitSampling, FIFOMode.
	// - USART_InitTypeDef sync-only fields: CLKPolarity, CLKPhase, CLKLastBit.
	// - UART clock prescaler is encoded by PRESC according to uart_prescalers[presval].
	// - UART BRR must be computed from the selected kernel clock source and the prescaler.
	// - LPUART is intentionally not handled here.
	// - AdvancedInit / UART_AdvFeatureConfig is still TODO.
	bool UART_t::setMode(stduint band_rate) {
		bool mode_tx = true;
		bool mode_rx = true;
		stduint presval = 0;
		uint16 usartdiv = 0;

		if (!enClock()) return false;
		if (!setMode_initGPIO(XART_ID)) {
			enClock(false);
			return false;
		}
		enAble(false);
		{
			Reflocal(cr1) = self[XARTReg::CR1];
			cr1 = (cr1 & ~_IMM(USART_CR1_M)) | _IMM(wordlen);
			cr1 &= ~_IMM(USART_CR1_PCE | USART_CR1_PS);
			cr1 |= _IMM(parity);
			USART_CR1_TE_REF(cr1) = mode_tx;
			USART_CR1_RE_REF(cr1) = mode_rx;
			USART_CR1_OVER8_REF(cr1) = oversampling8;
			USART_CR1_FIFOEN_REF(cr1) = fifo_mode;
			self[XARTReg::CR1] = cr1;

			USART_CR2_STOP(self) = _IMM(stopbits) >> 12;
			self[XARTReg::CR2] &= ~(USART_CR2_LINEN | USART_CR2_CLKEN);

			Reflocal(cr3) = 0;
			xart_setHwFlow(cr3, hwflow);
			USART_CR3_ONEBIT_REF(cr3) = onebit_sampling;
			if (fifo_mode) {
				xart_setFIFOThresholds(cr3, tx_fifo_threshold, rx_fifo_threshold);
			}
			self[XARTReg::CR3] = cr3;

			USART_PRESC_PRESCALER(self) = presval;
			switch (xart_getClockSource(XART_ID)) {
			case UART_CLKSRC::DxPCLKx:
				if (Ranglin(XART_ID, 2, 4) || Ranglin(XART_ID, 7, 2)) {
					if (oversampling8)
						usartdiv = UART_DIV_SAMPLING8(RCC.getFrequencyPCLK1(), band_rate, presval);
					else
						self[XARTReg::BRR] = (uint16)UART_DIV_SAMPLING16(RCC.getFrequencyPCLK1(), band_rate, presval);
				}
				else if (XART_ID == 1 || XART_ID == 6) {
					if (oversampling8)
						usartdiv = UART_DIV_SAMPLING8(RCC.getFrequencyPCLK2(), band_rate, presval);
					else
						self[XARTReg::BRR] = (uint16)UART_DIV_SAMPLING16(RCC.getFrequencyPCLK2(), band_rate, presval);
				}
				break;
			case UART_CLKSRC::PLL2:
				if (oversampling8)
					usartdiv = UART_DIV_SAMPLING8(RCC.PLL2.getFrequencyQ(), band_rate, presval);
				else
					self[XARTReg::BRR] = (uint16)UART_DIV_SAMPLING16(RCC.PLL2.getFrequencyQ(), band_rate, presval);
				break;
			case UART_CLKSRC::PLL3:
				if (oversampling8)
					usartdiv = UART_DIV_SAMPLING8(RCC.PLL3.getFrequencyQ(), band_rate, presval);
				else
					self[XARTReg::BRR] = (uint16)UART_DIV_SAMPLING16(RCC.PLL3.getFrequencyQ(), band_rate, presval);
				break;
			case UART_CLKSRC::HSI:
				if (oversampling8)
					usartdiv = UART_DIV_SAMPLING8(RCC.HSI.getFrequency_ToCore(), band_rate, presval);
				else
					self[XARTReg::BRR] = (uint16)UART_DIV_SAMPLING16(RCC.HSI.getFrequency_ToCore(), band_rate, presval);
				break;
			case UART_CLKSRC::CSI:
				if (oversampling8)
					usartdiv = UART_DIV_SAMPLING8(RCC.CSI.getFrequency(), band_rate, presval);
				else
					self[XARTReg::BRR] = (uint16)UART_DIV_SAMPLING16(RCC.CSI.getFrequency(), band_rate, presval);
				break;
			case UART_CLKSRC::LSE:
				if (oversampling8)
					usartdiv = UART_DIV_SAMPLING8(RCC.LSE.getFrequency(), band_rate, presval);
				else
					self[XARTReg::BRR] = (uint16)UART_DIV_SAMPLING16(RCC.LSE.getFrequency(), band_rate, presval);
				break;
			default:
				enClock(false);
				return false;
			}
			if (oversampling8) {
				uint16 brrtemp = usartdiv & 0xFFF0U;
				brrtemp |= (uint16)((usartdiv & 0x000FU) >> 1U);
				self[XARTReg::BRR] = brrtemp;
			}
		}
		/* In asynchronous mode, the following bits must be kept cleared:
		- LINEN and CLKEN bits in the USART_CR2 register,
		- SCEN, HDSEL and IREN  bits in the USART_CR3 register. */
		enAble(true);
		/* TEACK and/or REACK to check before moving huart->gState and huart->RxState to Ready */
		if _IMM(USART_CR1_TE(self)) {
			while (!_IMM(USART_ISR_TEACK(self)));
		}
		if _IMM(USART_CR1_RE(self)) {
			while (!_IMM(USART_ISR_REACK(self)));
		}
		// no consider timeout
		// AKA UART_MASK_COMPUTATION
		mask = xart_mask(wordlen, xart_parityEnable(parity));
		return mask != 0;
	}

	// HAL USART synchronous mode reference:
	// - enables CLKEN and configures CPOL/CPHA/LBCL on CR2.
	// - clears LINEN and the smartcard/half-duplex/IrDA async-incompatible CR3 bits.
	// - BRR computation still follows UART_SetConfig clock-source handling.
	bool USART_t::setModeSync(stduint band_rate, bool clock_polarity, bool clock_phase, bool lastbit_ena, bool slave_ena) {
		if (!enClock()) return false;
		if (!setMode_initGPIO(XART_ID) || !setMode_initSyncGPIO(XART_ID)) {
			enClock(false);
			return false;
		}
		enAble(false);

		bool mode_tx = true;
		bool mode_rx = true;
		stduint presval = 0;
		uint16 usartdiv = 0;
		uint16 brrtemp = 0;
		const bool parity_enable = xart_parityEnable(parity);

		Reflocal(cr1) = self[XARTReg::CR1];
		cr1 &= ~_IMM(USART_CR1_M | USART_CR1_PCE | USART_CR1_PS);
		cr1 |= _IMM(wordlen) | _IMM(parity);
		USART_CR1_TE_REF(cr1) = mode_tx;
		USART_CR1_RE_REF(cr1) = mode_rx;
		USART_CR1_OVER8_REF(cr1) = true;
		USART_CR1_FIFOEN_REF(cr1) = fifo_mode;
		self[XARTReg::CR1] = cr1;

		Reflocal(cr2) = self[XARTReg::CR2];
		cr2 &= ~_IMM(USART_CR2_CPHA | USART_CR2_CPOL | USART_CR2_DIS_NSS |
			USART_CR2_CLKEN | USART_CR2_LBCL | (0x3U << 12) | USART_CR2_SLVEN | USART_CR2_LINEN);
		cr2 |= USART_CR2_CLKEN | _IMM(stopbits);
		if (clock_polarity) cr2 |= USART_CR2_CPOL;
		if (clock_phase) cr2 |= USART_CR2_CPHA;
		if (lastbit_ena) cr2 |= USART_CR2_LBCL;
		if (slave_ena) cr2 |= USART_CR2_SLVEN;
		self[XARTReg::CR2] = cr2;

		self[XARTReg::CR3] &= ~_IMM((0x7U << 25) | (0x7U << 29) |
			USART_CR3_SCEN | USART_CR3_HDSEL | USART_CR3_IREN);
		if (fifo_mode) {
			Reflocal(cr3) = self[XARTReg::CR3];
			xart_setFIFOThresholds(cr3, tx_fifo_threshold, rx_fifo_threshold);
			self[XARTReg::CR3] = cr3;
		}
		USART_PRESC_PRESCALER(self) = presval;

		switch (getClockSource()) {
		case UART_CLKSRC::DxPCLKx:
			if (Ranglin(XART_ID, 2, 4) || Ranglin(XART_ID, 7, 2))
				usartdiv = UART_DIV_SAMPLING8(RCC.getFrequencyPCLK1(), band_rate, presval);
			else if (XART_ID == 1 || XART_ID == 6)
				usartdiv = UART_DIV_SAMPLING8(RCC.getFrequencyPCLK2(), band_rate, presval);
			else
				return false;
			break;
		case UART_CLKSRC::PLL2:
			usartdiv = UART_DIV_SAMPLING8(RCC.PLL2.getFrequencyQ(), band_rate, presval);
			break;
		case UART_CLKSRC::PLL3:
			usartdiv = UART_DIV_SAMPLING8(RCC.PLL3.getFrequencyQ(), band_rate, presval);
			break;
		case UART_CLKSRC::HSI:
			usartdiv = UART_DIV_SAMPLING8(RCC.HSI.getFrequency_ToCore(), band_rate, presval);
			break;
		case UART_CLKSRC::CSI:
			usartdiv = UART_DIV_SAMPLING8(RCC.CSI.getFrequency(), band_rate, presval);
			break;
		case UART_CLKSRC::LSE:
			usartdiv = UART_DIV_SAMPLING8(RCC.LSE.getFrequency(), band_rate, presval);
			break;
		default:
			return false;
		}
		brrtemp = usartdiv & 0xFFF0U;
		brrtemp |= (uint16)((usartdiv & 0x000FU) >> 1U);
		self[XARTReg::BRR] = brrtemp;

		enAble(true);
		/* TEACK and/or REACK to check before moving huart->gState and huart->RxState to Ready */
		if _IMM(USART_CR1_TE(self)) {
			while (!_IMM(USART_ISR_TEACK(self)));
		}
		if _IMM(USART_CR1_RE(self)) {
			while (!_IMM(USART_ISR_REACK(self)));
		}
		// no consider timeout
		// AKA UART_MASK_COMPUTATION
		mask = xart_mask(wordlen, parity_enable);
		return mask != 0;
	}
	//
	bool UART_t::enAble(bool ena) {
		self[XARTReg::CR1].setof(0, ena);// USART_CR1_UE, USART Enable
		return self[XARTReg::CR1].bitof(0);
	}
	//
	bool UART_t::enClock(bool ena) {
		return xart_enClock_UART(XART_ID, ena);
	}
	//
	bool USART_t::enClock(bool ena) {
		return xart_enClock_USART(XART_ID, ena);
	}
	//

	UART_CLKSRC USART_t::getClockSource() {
		return xart_getClockSource(XART_ID);
	}


	
}

#endif

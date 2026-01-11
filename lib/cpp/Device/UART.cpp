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

	_TEMP
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

	bool UART_t::operator>> (int& res) {
		return _TODO false;
	}

	bool UART_t::operator<< (stduint dat) {
		while (!(self[XARTReg::LSR] & _IMM1S(_BITPOS_LSR_TX_IDLE)));
		self[XARTReg::THR] = dat;
		return true;
	}


}
#endif

_Comment("Interrupt") namespace uni {
	#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)
	void USART_t::setInterrupt(Handler_t f) const {
		FUNC_XART[XART_ID] = f;
	}
	static Request_t XART_Request_list[] = { Request_None,
		IRQ_USART1, IRQ_USART2, IRQ_USART3, IRQ_UART4,
		IRQ_UART5,
		#ifdef _MCU_STM32F4x
		IRQ_USART6, Request_None, Request_None
		#elif defined(_MCU_STM32H7x)
		IRQ_USART6, IRQ_UART7, IRQ_UART8,
		#endif
	};
	void USART_t::setInterruptPriority(byte preempt, byte sub_priority) const {
		NVIC.setPriority(XART_Request_list[XART_ID], preempt, sub_priority);
	}
	#endif
}


#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
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
	static void setMode_initGPIO(byte XART_ID);
	//aka HAL_UART_Init
	void USART_t::setMode(stduint band_rate) {
		using namespace XARTReg;
		_TEMP if (XART_ID != 1) return;
		{
			//{TODO} busy flag
			enClock();
			setMode_initGPIO(XART_ID);// HAL_UART_MspInit
			enAble(false);
			//aka UART_SetConfig(UART_HandleTypeDef *huart)
			{
				self[CR2] &= ~_IMM(0x3 << 12);//set zero, aka UartHandle.Init.StopBits = UART_STOPBITS_1;
				self[CR1] &= ~_IMM(0x3 << 9);//aka UartHandle.Init.Parity = UART_PARITY_NONE; (USART_CR1_PCE and USART_CR1_PS)
				self[CR1].rstof(12); //aka UartHandle.Init.WordLength = UART_WORDLENGTH_8B; (USART_CR1_M)
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
	}

	//aka HAL_UART_Receive
	//{TODO} conflict
	int USART_t::operator>> (int& res) {
		using namespace XARTReg;
		bool len9b = false;// or 8b
		bool parity = false;
		stduint mask = len9b ?
			(parity ? 0x1FF : 0xFF) :
			(parity ? 0x0FF : 0x7F);
		int d = int(self[DR]);
		return res = d & mask;
	}

	void USART_t::Delay_unit() {
		for (volatile stduint i = 0; i < SystemCoreClock / last_baudrate; i++);
	}
	
	//aka HAL_UART_Transmit
	//{TODO} conflict
	USART_t& USART_t::operator << (stduint dat) {
		using namespace XARTReg;
		bool len9b = false;// or 8b
		self[DR] = dat & (len9b ? 0x1FF : 0x0FF);
		return self;
	}
	USART_t& USART_t::operator << (const char* p) {
		while (*p) {
			self << stduint(*p++);
			Delay_unit();
		}
		return self;
	}

	bool USART_t::enAble(bool ena) {
		using namespace XARTReg;
		self[CR1].setof(13, ena);// 13 is USART_CR1_UE_Pos
		return true;
	}

	bool USART_t::enClock(bool ena) {
		//{!} only for usart1
		using namespace RCCReg;
		RCC[APB2ENR].setof(_RCC_APB2ENR_POSI_ENCLK_USART1, ena);
		return RCC[APB2ENR].bitof(_RCC_APB2ENR_POSI_ENCLK_USART1) == ena;
	}

	// ---- ---- INTSYS ---- ----



	void USART_t::enInterrupt(bool enable) const {
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

#elif defined(_MCU_STM32H7x)

namespace uni {
	USART_t XART1(1), XART2(2), XART3(3), XART6(6);
	UART_t  XART4(4), XART5(5), XART7(7), XART8(8);

	static const stduint xart_addr[8]{ 
	D2_APB2PERIPH_BASE + 0x1000, D2_APB1PERIPH_BASE + 0x4400, D2_APB1PERIPH_BASE + 0x4800, D2_APB1PERIPH_BASE + 0x4C00,
	D2_APB1PERIPH_BASE + 0x5000, D2_APB2PERIPH_BASE + 0x1400, D2_APB1PERIPH_BASE + 0x7800, D2_APB1PERIPH_BASE + 0x7C00,
	};

	static byte GPINs_AFs_XARTx[8] = {
		7,7,7, 8,8, 7,7,8
	};

	Reference USART_t::operator[](XARTReg::USARTReg idx) const { return xart_addr[XART_ID - 1] + _IMM(idx); }
	Reference_T<uint16> USART_t::operator[](XARTReg::USARTReg16 idx) const { return xart_addr[XART_ID - 1] + _IMM(idx); }

	static void setMode_initGPIO(byte xart_id) {
		if (!Ranglin(xart_id, 1, 8)) return;
		xart_id--;
		static GPIN* RPins[8] _TODO{
			&GPIOA[10], nullptr, nullptr, nullptr,
			nullptr, nullptr, nullptr, nullptr,
		};
		static GPIN* TPins[8] _TODO {
			&GPIOA[9], nullptr, nullptr, nullptr,
			nullptr, nullptr, nullptr, nullptr,
		};
		RPins[xart_id]->setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::Veryhigh).setPull(true);// Rx
		RPins[xart_id]->_set_alternate(GPINs_AFs_XARTx[xart_id]);
		TPins[xart_id]->setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::Veryhigh).setPull(true);// Tx
		TPins[xart_id]->_set_alternate(GPINs_AFs_XARTx[xart_id]);
	}

	// ----

	void UART_t::setInterrupt(Handler_t _func) const { _TODO }
	void UART_t::setInterruptPriority(byte preempt, byte sub_priority) const { _TODO }
	void UART_t::enInterrupt(bool enable) const { _TODO }

	// HAL_NVIC_EnableIRQ
	void USART_t::enInterrupt(bool enable) const {
		NVIC.setAble(_TEMP IRQ_USART1, enable);
	}

	void USART_t::innByInterrupt() {
		while (lock_r);
		lock_r = true;
		// (frame error, noise error, overrun error)
		USART_CR3_EIE(self) = true;
		// Enable the UART Parity Error interupt and RX FIFO Threshold interrupt (if FIFO mode is enabled) or Data Register Not Empty interrupt (if FIFO mode is disabled)
		USART_CR1_PEIE(self) = true;
		if _IMM(USART_CR1_FIFOEN(self))
			USART_CR3_RXFTIE(self) = true;
		else
			USART_CR1_RXNEIE(self) = true;
		// rx_pointer = nil; => Dame
		// lock_r = false; => unlock by rx-handler
	}

	void USART_t::innHandlerByInterrupt() {
		uint16* tmp = (uint16*)(rx_buffer.address + rx_pointer);
		/* Check that a Rx process is ongoing */
		if (lock_r) {
			if (wordlen == WordLength_E::Bits9 && !parity_enable) {
				*tmp = inn();
				rx_pointer += 2;
			}
			else {
				*(uint8*)tmp = inn();
				rx_pointer++;
			}
			// now *tmp is the data received
			if (rx_pointer >= rx_buffer.length) {
				// Disable the UART Parity Error Interrupt and RXNE interrupt
				USART_CR1_RXNEIE(self) = 0;
				USART_CR1_PEIE(self) = 0;
				// Disable the UART Error Interrupt: (Frame error, noise error, overrun error)
				USART_CR3_EIE(self) = 0;
				// no consider HAL_UART_RxCpltCallback
			}
			lock_r = false;
		}
		else {
			// Clear RXNE interrupt flag
			self[XARTReg::RQR] |= USART_RQR_RXFRQ;// __HAL_UART_SEND_REQ
		}
	}


	// ----

	int UART_t::inn() {
		return _TODO 0;
	}
	int UART_t::out(const char* str, stduint len) {
		return _TODO 0;
	}

	int USART_t::inn() {
		stduint uhdata = (uint16)self[XARTReg::RDR];
		if (wordlen == WordLength_E::Bits9 && !parity_enable) {
			return uint16(uhdata & mask);
		}
		else {
			return uint8(uhdata & mask);
		}
	}
	int USART_t::out(const char* str, stduint len) {
		for0 (i, len) {
			while (!USART_ISR_TC(self));
			self[XARTReg::TDR] = str[i];
		}
		return len;
	}


	// ----

	#undef D3PCLK1
	#define UART_DIV_SAMPLING8(__PCLK__, __BAUD__, __PRESCALER__)   (((((__PCLK__)/uart_prescalers[(__PRESCALER__)])*2) + ((__BAUD__)/2)) / (__BAUD__))
	#define UART_DIV_SAMPLING16(__PCLK__, __BAUD__, __PRESCALER__)  ((((__PCLK__)/uart_prescalers[(__PRESCALER__)]) + ((__BAUD__)/2)) / (__BAUD__))
	static const uint16 uart_prescalers[] {
		1, 2, 4, 6, 8, 10, 12, 16, 32, 64, 128, 256
	};
	
	// HAL_UART_Init
	bool USART_t::setMode(stduint band_rate) {
		//[TEMP]
		// UART_WORDLENGTH_8B UART_STOPBITS_1 UART_PARITY_NONE UART_HWCONTROL_NONE UART_MODE_TX_RX
		//
		stduint wordlen_mask = 0x10001000;// for CR1
		_TEMP wordlen = WordLength_E::Bits8;
		//
		_Comment(USART_CR2_STOP should= 0);
		// Appended MSB Parity, CR1
		_TEMP parity_enable = false;
		bool parity_odd = false;
		#define USART_PARITY_NONE 0
		#define USART_PARITY_EVEN (USART_CR1_PCE)
		#define USART_PARITY_ODD  ((USART_CR1_PCE | USART_CR1_PS))
		#define USART_PARITY_MASK USART_PARITY_ODD
		// Mode (Parallel Setting), CR1
		bool mode_tx = true;// USART_CR1_TE
		bool mode_rx = true;// USART_CR1_RE
		// USART_InitTypeDef CLKPolarity
		bool clock_polarity;// true for USART_CR2_CPOL else 0
		// USART_InitTypeDef CLKPhase: clock transition on which the bit capture is made.
		bool clock_phase;// true for (USART_CR2_CPHA) second clock transition but (0) first
		// USART_InitTypeDef CLKLastBit: whether the clock pulse corresponding to the last transmitted data bit (MSB) has to be output on the SCLK pin in synchronous mode
		bool lastbit_ena;// true for USART_CR2_LBCL
		// Prescaler
		stduint presval = 1;
		
		bool state = false;
		for0a(i, uart_prescalers) if (uart_prescalers[i] == presval) { state = true; presval = i; break; }
		asrtret (state);
		// UART_InitTypeDef: HwFlowCtl, 4=2*2 states:
		bool HwFlowCtl_RTS = false;// USART_CR3_RTSE
		bool HwFlowCtl_CTS = false;// USART_CR3_CTSE
		// UART_InitTypeDef: OverSampling, whether the Over sampling 8 is enabled or disabled, to achieve higher speed (up to f_PCLK/8)
		bool OverSampling = false;// true for USART_CR1_OVER8 but 16
		// UART_InitTypeDef: OneBitSampling, whether a single sample or three samples' majority vote is selected. Selecting the single sample method increases the receiver tolerance to clock deviations
		bool OneBitSampling_ena = false;// USART_CR3_ONEBIT
		// UART_InitTypeDef: FIFOMode
		bool FIFOMode = false;// USART_CR1_FIFOEN
		//
		enum class Threshold_E {
			_1_8,// R/TXFIFO reaches 1/8 of its depth
			_1_4,//                  1/4
			_1_2,//                  1/2
			_3_4,//                  3/4
			_7_8,//                  7/8
			_8_8,// R/TXFIFO becomes empt
		};
		Threshold_E TXFIFOThreshold;// USART_CR3_TXFTCFG
		Threshold_E RXFIFOThreshold;// USART_CR3_RXFTCFG
		// ---- ---- above are info about USART_InitTypeDef and UART_InitTypeDef ---- ----
		// if (HwFlowCtl_RTS || HwFlowCtl_CTS) { // HwFlowCtl != UART_HWCONTROL_NONE
		// 	//{} for XART1~8 + LPUART1 // IS_UART_HWFLOW_INSTANCE
		// }
		// else {
		// 	//{} for XART1~8 + LPUART1 // IS_UART_INSTANCE || IS_LPUART_INSTANCE
		// }
		{
			enClock();
			setMode_initGPIO(XART_ID);
		}
		enAble(false);
		/* Set the UART Communication parameters */
		// special parameters
		//{} UART_INSTANCE_LOWPOWER =>    IS_LPUART_STOPBITS 
		//                          else  IS_UART_STOPBITS,  IS_UART_ONE_BIT_SAMPLE
		//{} UART_FIFOMODE_ENABLE   =>    IS_UART_TXFIFO_THRESHOLD,  IS_UART_RXFIFO_THRESHOLD

		// UART_SetConfig
		{
			uint32_t tmpreg = 0x00000000U;
			UART_CLKSRC clocksource = UART_CLKSRC::UNDEFINED;
			uint16 brrtemp = 0x0000U;
			uint16 usartdiv = 0x0000U;
			// ---- USART CR1 Configuration ---- //
			/* Clear M, PCE, PS, TE, RE and OVER8 bits and configure */
			Reflocal (cr1) = self[XARTReg::CR1];
			cr1 = (cr1 & ~wordlen_mask) | _IMM(wordlen);
			cr1 &= ~USART_PARITY_MASK;
			if (parity_enable) {
				cr1 |= parity_odd ? USART_PARITY_ODD : USART_PARITY_EVEN;
			}
			USART_CR1_TE_REF(cr1) = mode_tx;
			USART_CR1_RE_REF(cr1) = mode_rx;
			USART_CR1_OVER8_REF(cr1) = OverSampling;
			USART_CR1_FIFOEN_REF(cr1) = FIFOMode;
			self[XARTReg::CR1] = cr1;

			// ---- USART CR2 Configuration ---- //
			USART_CR2_STOP(self) = 0;
			
			// ---- USART CR3 Configuration ---- //
			Reflocal(cr3) = 0;
			// - UART HardWare Flow Control: set CTSE and RTSE bits
			USART_CR3_RTSE_REF(cr3) = HwFlowCtl_RTS;
			USART_CR3_CTSE_REF(cr3) = HwFlowCtl_CTS;
			// - one-bit sampling method versus three samples' majority rule according
			if (_TEMP true) // (!(UART_INSTANCE_LOWPOWER(huart)))
			{
				_Comment("We did not consider LPUART now");
				USART_CR3_ONEBIT_REF(cr3) = OneBitSampling_ena;// (not applicable to LPUART)
			}
			//
			if (FIFOMode) {
				USART_CR3_RXFTCFG_REF(cr3) = _IMM(RXFIFOThreshold);
				USART_CR3_TXFTCFG_REF(cr3) = _IMM(TXFIFOThreshold);
			}
			self[XARTReg::CR3] = cr3;

			// ---- USART PRESC Configuration ---- //
			/* Configure
			 * - UART Clock Prescaler : set PRESCALER according to uart_prescalers[presval] value */
			USART_PRESC_PRESCALER(self) = presval;

			// ---- USART BRR Configuration ---- //
			clocksource = getClockSource();

			// ---- Check LPUART instance ---- //
			#if 0//{TODO}
			if (UART_INSTANCE_LOWPOWER(huart))
			{
				/* Retrieve frequency clock */
				tmpreg = 0U;
				switch (clocksource)
				{
				case UART_CLOCKSOURCE_D3PCLK1:
					tmpreg = HAL_RCCEx_GetD3PCLK1Freq();
					break;
				case UART_CLOCKSOURCE_PLL2:
					HAL_RCCEx_GetPLL2ClockFreq(&pll2_clocks);
					tmpreg = pll2_clocks.PLL2_Q_Frequency;
					break;
				case UART_CLOCKSOURCE_PLL3:
					HAL_RCCEx_GetPLL3ClockFreq(&pll3_clocks);
					tmpreg = pll3_clocks.PLL3_Q_Frequency;
					break;
				case UART_CLOCKSOURCE_HSI:
					if (__HAL_RCC_GET_FLAG(RCC_FLAG_HSIDIV) != 0U)
					{
						tmpreg = (uint32_t)(HSI_VALUE >> (__HAL_RCC_GET_HSI_DIVIDER() >> 3U));
					}
					else
					{
						tmpreg = (uint32_t)HSI_VALUE;
					}
					break;
				case UART_CLOCKSOURCE_CSI:
					tmpreg = (uint32_t)CSI_VALUE;
					break;
				case UART_CLOCKSOURCE_LSE:
					tmpreg = (uint32_t)LSE_VALUE;
					break;
				case UART_CLOCKSOURCE_UNDEFINED:
				default:
					ret = HAL_ERROR;
					break;
				}
				/* if proper clock source reported */
				if (tmpreg != 0U)
				{
				  /* ensure that Frequency clock is in the range [3 * baudrate, 4096 * baudrate] */
					if ((tmpreg < (3 * band_rate)) ||
						(tmpreg > (4096 * band_rate)))
					{
						ret = HAL_ERROR;
					}
					else
					{
						switch (clocksource)
						{
						case UART_CLOCKSOURCE_D3PCLK1:
							tmpreg = (uint32_t)(UART_DIV_LPUART(HAL_RCCEx_GetD3PCLK1Freq(), band_rate, uart_prescalers[presval]));
							break;
						case UART_CLOCKSOURCE_PLL2:
							HAL_RCCEx_GetPLL2ClockFreq(&pll2_clocks);
							tmpreg = (uint32_t)(UART_DIV_LPUART(pll2_clocks.PLL2_Q_Frequency, band_rate, uart_prescalers[presval]));
							break;
						case UART_CLOCKSOURCE_PLL3:
							HAL_RCCEx_GetPLL3ClockFreq(&pll3_clocks);
							tmpreg = (uint32_t)(UART_DIV_LPUART(pll3_clocks.PLL3_Q_Frequency, band_rate, uart_prescalers[presval]));
							break;
						case UART_CLOCKSOURCE_HSI:
							if (__HAL_RCC_GET_FLAG(RCC_FLAG_HSIDIV) != 0U)
							{
								tmpreg = (uint32_t)(UART_DIV_LPUART((HSI_VALUE >> (__HAL_RCC_GET_HSI_DIVIDER() >> 3)), band_rate, uart_prescalers[presval]));
							}
							else
							{
								tmpreg = (uint32_t)(UART_DIV_LPUART(HSI_VALUE, band_rate, uart_prescalers[presval]));
							}
							break;
						case UART_CLOCKSOURCE_CSI:
							tmpreg = (uint32_t)(UART_DIV_LPUART(CSI_VALUE, band_rate, uart_prescalers[presval]));
							break;
						case UART_CLOCKSOURCE_LSE:
							tmpreg = (uint32_t)(UART_DIV_LPUART(LSE_VALUE, band_rate, uart_prescalers[presval]));
							break;
						case UART_CLOCKSOURCE_UNDEFINED:
						default:
							ret = HAL_ERROR;
							break;
						}

						if ((tmpreg >= UART_LPUART_BRR_MIN) && (tmpreg <= UART_LPUART_BRR_MAX))
						{
							self[XARTReg::BRR] = tmpreg;
						}
						else
						{
							ret = HAL_ERROR;
						}
					}  /*   if ( (tmpreg < (3 * band_rate) ) || (tmpreg > (4096 * band_rate) )) */
				} /* if (tmpreg != 0) */
			}
			#else
			if (0);
			#endif
			// ---- Check UART Over Sampling to set Baud Rate Register ---- //
			else
			{
				switch (clocksource)
				{
				case UART_CLKSRC::DxPCLKx:
					if (Ranglin(XART_ID, 2, 4) || Ranglin(XART_ID, 7, 2)) {
						if (OverSampling)
							usartdiv = (UART_DIV_SAMPLING8(RCC.getFrequencyPCLK1(), band_rate, presval));
						else
							self[XARTReg::BRR] = (uint16)(UART_DIV_SAMPLING16(RCC.getFrequencyPCLK1(), band_rate, presval));
					}
					else if (XART_ID == 1 || XART_ID == 6) {
						if (OverSampling)
							usartdiv = (UART_DIV_SAMPLING8(RCC.getFrequencyPCLK2(), band_rate, presval));
						else
							self[XARTReg::BRR] = (uint16)(UART_DIV_SAMPLING16(RCC.getFrequencyPCLK2(), band_rate, presval));
					}
					break;
				case UART_CLKSRC::PLL2:
					if (OverSampling)
					{
						usartdiv = (UART_DIV_SAMPLING8(RCC.PLL2.getFrequencyQ(), band_rate, presval));
					}
					else {
						self[XARTReg::BRR] = (uint16)(UART_DIV_SAMPLING16(RCC.PLL2.getFrequencyQ(), band_rate, presval));
					}
					break;
				case UART_CLKSRC::PLL3:
					if (OverSampling)
					{
						usartdiv = (UART_DIV_SAMPLING8(RCC.PLL3.getFrequencyQ(), band_rate, presval));
					}
					else {
						self[XARTReg::BRR] = (uint16)(UART_DIV_SAMPLING16(RCC.PLL3.getFrequencyQ(), band_rate, presval));
					}
					break;
				case UART_CLKSRC::HSI:
					// ST Origin Code: if HSION&HSIRDY, use getFrequency_ToCore or use HSI_VALUE
					if (OverSampling)
					{
						usartdiv = (UART_DIV_SAMPLING8(RCC.HSI.getFrequency_ToCore(), band_rate, presval));
					}
					else {
						self[XARTReg::BRR] = (uint16)(UART_DIV_SAMPLING16(RCC.HSI.getFrequency_ToCore(), band_rate, presval));
					}
					break;
				case UART_CLKSRC::CSI:
					if (OverSampling)
						usartdiv = (UART_DIV_SAMPLING8(RCC.CSI.getFrequency(), band_rate, presval));
					else
						self[XARTReg::BRR] = (uint16)(UART_DIV_SAMPLING16(RCC.CSI.getFrequency(), band_rate, presval));
					break;
				case UART_CLKSRC::LSE:
					if (OverSampling)
						usartdiv = (UART_DIV_SAMPLING8(RCC.LSE.getFrequency(), band_rate, presval));
					else
						self[XARTReg::BRR] = (uint16)(UART_DIV_SAMPLING16(RCC.LSE.getFrequency(), band_rate, presval));
					break;
				case UART_CLKSRC::UNDEFINED:
				default:
					return false;
					break;
				}
				if (OverSampling)
				{
					brrtemp = usartdiv & 0xFFF0U;
					brrtemp |= (uint16)((usartdiv & 0x000FU) >> 1U);
					self[XARTReg::BRR] = brrtemp;
				}
			}

		}
		//{TODO} Adv Init
		//{} if (huart->AdvancedInit.AdvFeatureInit != UART_ADVFEATURE_NO_INIT)
		//{} {
		//{} 	UART_AdvFeatureConfig(huart);
		//{} }
		/* In asynchronous mode, the following bits must be kept cleared:
		- LINEN and CLKEN bits in the USART_CR2 register,
		- SCEN, HDSEL and IREN  bits in the USART_CR3 register.*/


		enAble(true);
		/* TEACK and/or REACK to check before moving huart->gState and huart->RxState to Ready */
		_Comment("UART_CheckIdleState(huart)") {
			if _IMM(USART_CR1_TE(self))
			{
				while (!_IMM(USART_ISR_TEACK(self)));
			}
			if _IMM(USART_CR1_RE(self))
			{
				while (!_IMM(USART_ISR_REACK(self)));
			}
		}
		// no consider timeout
		// AKA UART_MASK_COMPUTATION
		switch (wordlen) {
		case WordLength_E::Bits7:
			mask = parity_enable ? 0x7F : 0x3F;// one bit is for parity
			break;
		case WordLength_E::Bits8:
			mask = parity_enable ? 0xFF : 0x7F;
			break;
		case WordLength_E::Bits9:
			mask = parity_enable ? 0x1FF : 0xFF;
			break;
		default:
			return false;
		}
		return true;
	}
	//
	bool USART_t::enAble(bool ena) {
		self[XARTReg::CR1].setof(0, ena);// USART_CR1_UE, USART Enable
		return self[XARTReg::CR1].bitof(0);
	}
	//
	bool USART_t::enClock(bool ena) {
		//{TEMP} for XART1
		RCC[RCCReg::APB2ENR].setof(4, ena);// RCC_APB2ENR_USART1EN
		return RCC[RCCReg::APB2ENR].bitof(4);
	}
	//

	UART_CLKSRC USART_t::getClockSource() {
		switch (XART_ID) {
		case 1: case 6:
			return UART_CLKSRC _IMM(RCC_D2CCIP2R_USART16SEL);
		case 2: case 3: case 4: case 5: case 7: case 8:
			return UART_CLKSRC _IMM(RCC_D2CCIP2R_USART28SEL);
		default:
			return UART_CLKSRC::UNDEFINED;
		}
	}


	
}

#endif

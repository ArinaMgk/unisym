
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

#endif

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
		Delay_unit();
		return self;
	}
	USART_t& USART_t::operator << (const char* p) {
		while (*p) self << stduint(*p++);
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

	void USART_t::setInterrupt(Handler_t f) const {
		FUNC_XART[XART_ID] = f;
	}
	static Request_t XART_Request_list[8] = {
		Request_None, IRQ_USART1, IRQ_USART2, IRQ_USART3,
		IRQ_UART4, IRQ_UART5
		#ifdef _MCU_STM32F4x
		, IRQ_USART6, Request_None
		#endif
	};
	void USART_t::setInterruptPriority(byte preempt, byte sub_priority) const {
		NVIC.setPriority(XART_Request_list[XART_ID], preempt, sub_priority);
	}

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


#endif

// UTF-8 C/C++11 TAB4 CRLF
// Docutitle: (Protocol) Inter-Integrated Circuit, I2C
// Codifiers: @dosconio: 20240429~;
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

#if !defined(_INC_Standard_IIC)
#define _INC_Standard_IIC
#include "../stdinc.h"
#if defined(_MCU_Intel8051)

void I2C_Start(void);

void I2C_SetByte(unsigned char Byte);
unsigned char I2C_GetAck(void);

unsigned char I2C_GetByte(void);
void I2C_SetAck(unsigned char AckBit);

void I2C_Stop(void);

#elif defined(_INC_CPP) // Below are C++ Area
#include "../../cpp/reference"
#include "../../cpp/Device/GPIO"
#include "../../cpp/trait/XstreamTrait.hpp"

#define DA_PP 1

// AKA HAL_I2C_ERROR_* (bitmask, H7/MP13 base set)
#define ERR_IIC_NONE     0x00
#define ERR_IIC_BERR     0x01
#define ERR_IIC_ARLO     0x02
#define ERR_IIC_NACK     0x04   // AKA HAL_I2C_ERROR_AF (Acknowledge Failure)
#define ERR_IIC_OVR      0x08
#define ERR_IIC_DMA      0x10
#define ERR_IIC_TIMEOUT  0x20
#define ERR_IIC_SIZE     0x40

namespace uni {

	// AKA I2C_MEMADD_SIZE_8BIT / I2C_MEMADD_SIZE_16BIT
	enum class MemAddrSize : byte { Byte1 = 1, Byte2 = 2 };

	// AKA I2C_FIRST_FRAME / I2C_NEXT_FRAME / I2C_LAST_FRAME / I2C_FIRST_AND_LAST_FRAME
	enum class XferOption : byte {
		FirstFrame,        // START, no STOP（SOFTEND）
		NextFrame,         // no START/STOP（继续）
		LastFrame,         // no START, STOP（AUTOEND）
		FirstAndLastFrame  // START + STOP（默认）
	};

	// AKA master / slave role
	enum class IICRole : byte { Master, Slave };

	class DMAStream;// forward declaration for IIC DMA integration

#undef IIC
#if defined(_SUPPORT_GPIO)
	
	class IIC_t : public OstreamTrait, public IstreamTrait {
	protected:
		bool last_ack_accepted;
		bool push_pull = false;
		stduint errcode = ERR_IIC_NONE;// AKA HAL ErrorCode (bitmask ERR_IIC_*), read via getError()
	public:
		virtual int out(const char* str, stduint len) {
			for0(i, len) Send(((const byte*)str)[i], true);// do ... while ACK
			return self.last_ack_accepted;
		}
		virtual int inn() {
			return ReadByte(true, true);
		}
	public:
		bool operator<<(byte txt) { return out((char*)&txt, 1); }
		stduint getError() const { return errcode; }// AKA HAL_I2C_GetError
		//
		virtual void SendStart(stduint addr = 0) {}// addr==0 sentinel = bare START; else send 7-bit addr (<<1 internal)
		virtual void SendStop(void) {}
		virtual bool WaitAcknowledge() { return false; }
		virtual void SendAcknowledge(bool ack = true) {}

		//{TODO} send return ACK status

		void Send(byte txt, bool auto_wait_ack = false) { Send(&txt, _BYTE_BITS_, auto_wait_ack); }
		virtual void Send(byte* txt, stduint len, bool auto_wait_ack = false) {}
		virtual byte ReadByte(bool feedback = true, bool ack = true) { return 0; }
		//{} Read(&bits, bitlen, ...)
	};

	class IIC_SOFT : public IIC_t {
	protected:
		GPIO_Pin& SDA, & SCL;
	public:
		Handler_t func_delay;

		IIC_SOFT(GPIO_Pin& SDA, GPIO_Pin& SCL, bool init_now = true)
			: SDA(SDA), SCL(SCL) { if (init_now) setMode(); }

		void setMode() {
			SDA.setMode(push_pull ? GPIOMode::OUT_PushPull : GPIOMode::OUT_OpenDrain);
			SCL.setMode(GPIOMode::OUT_PushPull);
			SCL = true;
			SDA = true;
		}

		virtual void SendStart(stduint addr = 0) override;
		virtual void SendStop(void) override;
		virtual bool WaitAcknowledge() override;
		virtual void SendAcknowledge(bool ack = true) override;
		virtual void Send(byte* txt, stduint len, bool auto_wait_ack = false) override;
		virtual byte ReadByte(bool feedback = true, bool ack = true) override;//[to-be-outdated]
	};

	#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
	#ifdef _MCU_IIC_TEMP
	#include "../../cpp/Device/_inner/IIC-STM32H7.hpp"
	#endif
	enum class IICReg // V2 register offsets, H7 & MP13 identical
	{
		CR1, CR2,
		OAR1, OAR2,
		TIMINGR,
		TIMEOUTR,
		ISR,
		ICR,
		PECR,
		RXDR, TXDR,
	};
	#if defined(_MCU_STM32H7x)
	enum class IIC_Clksrc_E {
		DxPCLK1,// D2 for IIC123, D3 for IIC4
		PLL3, HSI, CSI
	};// for field RCC_D2CCIP2R_I2C123SEL
	#endif

	#endif
	class IIC_HARD : public IIC_t {
		//
		//{TEMP} only <= 8 bit, aka BITS8
	protected:
		byte id;// 1..4 for H7, 1..5 for MP13
		
	public:
		// debug failed!
		// AKA HAL_I2C_Mem_Write(self, dest_addr, &txt, 1, &txt, 1, uint32_t Timeout), split into:
		virtual void SendStart(stduint addr = 0) override;
		virtual void SendStop(void) override;
		virtual bool WaitAcknowledge() override;
		virtual void Send(byte* txt, stduint len, bool auto_wait_ack = false) override;

		//
		virtual byte ReadByte(bool feedback = true, bool ack = true) override;
		virtual void SendAcknowledge(bool ack = true) override;
		
		//
		IIC_HARD(byte _id) : id(_id) {}
		#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
	protected:
		stduint dest_addr = 0;
		bool send_start_just = false;
		const byte* tx_ptr = nullptr;
		byte* rx_ptr = nullptr;
		stduint xfer_count = 0;
		stduint xfer_size = 0;
		bool lock = false;// AKA HAL State != READY (busy lock)
		IICRole role = IICRole::Master;
		bool dma_xfer = false;// DMA path active（RELOAD 握手）
		bool xfer_stop = false;// Rupt 帧是否以 STOP（AUTOEND）结束，AKA XferOptions 的 stop 语义
		// AKA HAL I2C_TransferConfig (conditional RD_WRN clear)
		void _TransferConfig(stduint size, stduint mode, stduint request);
		// AKA HAL I2C_WaitOnXxxFlagUntilTimeout (generic flag wait)
		bool _WaitFlag(stduint flag, bool expect_set, stduint timeout, bool check_nack);
		// AKA I2C_RESET_CR2 + clear STOPF
		void _FinishXfer(void);
		// DMA 启动（memory↔TXDR/RXDR）；H7 I2C4 路由到 BDMA
		bool _DmaTxStart(pureptr_t mem, pureptr_t peri, stduint ndtr);
		bool _DmaRxStart(pureptr_t peri, pureptr_t mem, stduint ndtr);
	public:
		Reference operator[] (IICReg reg);
		bool enClock(bool ena = true);
		bool enAble(bool ena = true);

		void setDestination(stduint _dest_addr) { dest_addr = _dest_addr; }
		void setRole(IICRole r) { role = r; }

		// AKA HAL_I2C_GetState / GetMode（简化：0=Ready/2=Busy；0=Master/1=Slave）
		byte getState(void) const { return lock ? 2 : 0; }
		byte getMode(void) const { return _IMM(role); }
		// AKA HAL_I2C_IsDeviceReady
		bool IsDeviceReady(stduint dev, byte trials = 3, stduint timeout = 25);

		// DMA stream handles (bound externally, like UART/SPI)
		const DMAStream* hdmatx = nullptr;
		const DMAStream* hdmarx = nullptr;
		// H7 I2C4 走 BDMA 通道（0xFF = 无效）
		byte bdma_tx_channel = 0xFF;
		byte bdma_rx_channel = 0xFF;
		// AKA HAL_I2C_GetDMARequestID — DMAMUX1 request line（H7 I2C4 走 DMAMUX2/BDMA）
		stduint getDMARequestID(bool is_tx) const;
		// AKA I2C_DMAMasterTransmitCplt / I2C_DMAMasterReceiveCplt
		void dmaTransmitCplt(void);
		void dmaReceiveCplt(void);
		
		// none or single address
		bool setMode(stduint self_addr0 = 0);

		// AKA HAL_I2CEx_ConfigAnalogFilter
		bool setAnalogFilter(bool ena = true);
		// AKA HAL_I2CEx_ConfigDigitalFilter
		bool setDigitalFilter(byte coeff);
		// AKA HAL_I2CEx_EnableWakeUp / DisableWakeUp
		bool setWakeUp(bool ena = true);
		// AKA HAL_I2CEx_EnableFastModePlus / DisableFastModePlus
		bool setFastModePlus(bool ena = true);

		// AKA HAL_I2C_Master_Transmit (device addr via setDestination / SendStart)
		stduint Transmit(const byte* tx, stduint size, IOMethod method = IOMethod::Loop, XferOption opt = XferOption::FirstAndLastFrame);
		// AKA HAL_I2C_Master_Receive
		stduint Receive(byte* rx, stduint size, IOMethod method = IOMethod::Loop, XferOption opt = XferOption::FirstAndLastFrame);
		// AKA HAL_I2C_Slave_Transmit_IT / HAL_I2C_Slave_Receive_IT（非阻塞，从机 IT）
		stduint SlaveTransmit(const byte* tx, stduint size);
		stduint SlaveReceive(byte* rx, stduint size);

		// AKA HAL_I2C_Mem_Write (device addr via setDestination / SendStart)
		stduint MemWrite(stduint mem_addr, MemAddrSize mem_size, const byte* tx, stduint size, IOMethod method = IOMethod::Loop);
		// AKA HAL_I2C_Mem_Read
		stduint MemRead(stduint mem_addr, MemAddrSize mem_size, byte* rx, stduint size, IOMethod method = IOMethod::Loop);

		// AKA HAL_I2C_EV_IRQHandler (event: TXIS/RXNE/TCR/STOPF/NACKF); true = transfer done/error
		bool evByInterrupt(void);
		// AKA HAL_I2C_ER_IRQHandler (error: BERR/ARLO/OVR); true = error
		bool erByInterrupt(void);
	protected:
		// AKA HAL I2C_Slave_ISR_IT（从机 EV：ADDR/RXNE/TXIS/NACKF/STOPF）
		bool slaveByInterrupt(void);
		#endif
	};
	#if defined(_MCU_STM32H7x)
	extern IIC_HARD IIC1, IIC2, IIC3, IIC4;
	#define I2C1 IIC1
	#define I2C2 IIC2
	#define I2C3 IIC3
	#define I2C4 IIC4
	#elif defined(_MPU_STM32MP13)
	extern IIC_HARD IIC1, IIC2, IIC3, IIC4, IIC5;
	#define I2C1 IIC1
	#define I2C2 IIC2
	#define I2C3 IIC3
	#define I2C4 IIC4
	#define I2C5 IIC5
	#endif


#endif

}

#endif
#endif

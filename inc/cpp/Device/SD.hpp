// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Stroage) SDCard
// Codifiers: @dosconio: 20250107
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


#ifndef _INCPP_Storage_SDMMC
#define _INCPP_Storage_SDMMC

#include "../unisym"

#ifdef _INC_CPP

#include "../trait/StorageTrait.hpp"
#include "../interrupt"
#include "../string"

#if defined(_MPU_STM32MP13)

namespace uni {

	#include "Storage/SD-DEPEND.h"

	enum class SDReg {
		POWER, CLKCR, ARG, CMD,
		RESPCMD, RESP1, RESP2, RESP3,
		RESP4, DTIMER, DLEN, DCTRL,
		DCOUNT, STA, ICR, MASK,
		ACKTIME,
		IDMACTRL = 0x50 / 4, IDMABSIZE, IDMABASER,
		IDMALAR = 0x64 / 4, IDMABAR,
		FIFO_Start = 0x80 / 4, // 0x80 - 0xBC
		VERR = 0x3F4 / 4, IPIDR, SIDR
	};

	// for SDMMC 1 and 2
	enum class SDMMC_CLKSRC { HCLK6 = 0b00, PLL3, PLL4, HSI };

	class SecureDigitalCard_t : public StorageTrait, public RuptTrait
	{
		byte SDMMC_ID;
		bool Inner_Rupt_Handler;
	public:
		SecureDigitalCard_t(byte _SDMMC_ID) : SDMMC_ID(_SDMMC_ID) {}
		byte getID() const { return SDMMC_ID; }
		Reference operator[](SDReg idx) const;
		// ---- StorageTrait ----
		// - stduint Block_Size;
		// - void* Block_buffer;
		virtual bool Read(stduint BlockIden, void* Dest) { return false; }
		virtual bool Write(stduint BlockIden, const void* Sors) { return false; }//{unchecked}
		virtual byte operator[](uint64 bytid) { return false; }// byte read
		// ---- RuptTrait ----
		virtual void setInterrupt(Handler_t _func) const override { _TODO }
		virtual void setInterruptPriority(byte preempt, byte sub_priority) const override { _TODO }
		virtual void enInterrupt(bool enable = true) const override { _TODO }
		// clock_edge: true for posedge
		bool setMode(
			SDMMC_CLKSRC clk_src = SDMMC_CLKSRC::HCLK6,
			bool clock_edge = false,
			bool powersave_enable = false,
			byte bus_width = 4,
			bool hardware_flow_control_enable = false);
	protected:
		// AKA HAL_SD_InitCard
		bool setModeSub();
	_TEMP public:
		bool CARD_V2X_else_V1X;// CardVersion
	public:
		enum class WaitForInterrupt_E {
			None, Interrupt = 0b01, Pending = 0b10
		};
		enum class CardType_E {
			SDSC = 0,// SD Standard Capacity <2Go
			SDHC_SDXC = 1,// SD High Capacity <32Go, SD Extended Capacity <2To
			SECURED = 3,
		};
	public:
		CardType_E CardType;
		// ----: SD_HandleTypeDef :----
		uint32 CSD[4];// SD card specific data table
		uint32 CID[4];// SD card identification number table
	protected:

		struct CmdInitType {
			stduint argument;// Specifies the SDMMC command argument which is sent to a card as part of a command message. If a command contains an argument, it must be loaded into this register before writing the command to the command register
			stduint CmdIndex;// Specifies the SDMMC command index. It must be Min_Data = 0 and 			Max_Data = 64
			stduint Response;// Specifies the SDMMC response type.
			WaitForInterrupt_E WaitForInterrupt;// self[CMD]. Specifies whether SDMMC wait for interrupt request is enabled or disabled.
			bool CPSM;// SDMMC_CMD_CPSMEN. Specifies whether SDMMC Command path state machine (CPSM) is enabled or disabled.
		};// AKA SDMMC_CmdInitTypeDef
	_TEMP public:
		
		struct HAL_SD_CardInfoTypeDef
		{
			uint32 Class;
			uint32 RelCardAdd;// Relative Card Address
			//uint32 CardType;
			// uint32 CardVersion;
			uint32 BlockSize;
			uint32 BlockNbr;// Card Capacity in blocks
			uint32 LogBlockSize;
			uint32 LogBlockNbr;
			uint32 CardSpeed;
		} CardInfo;
	
		
		void setClockEdge(bool clock_edge) const;
		void setClockPowerSave(bool powersave_enable) const;
		void setBusWide(byte bus_width) const;
		void setHardwareFlowControl(bool hardware_flow_control_enable) const;
		void setClockDiv(stduint Init_ClockDiv) const;
		//
		// Identify card operating voltage
		// Enquires cards about their operating voltage and configures clock controls and stores SD information that will be needed in future in the SD handle.
		bool SD_PowerON(uint32* feedback);
		bool SD_InitCard(uint32* feedback);
		bool SDMMC_CmdBlockLength(uint32 BlockSize) const;
		//
		bool SDMMC_CmdGoIdleState() const;
		// 
		bool SDMMC_SendCommand(const CmdInitType& cit) const;
		bool SDMMC_SendCommand(
			stduint argument,
			stduint cmdindex,
			byte response,
			WaitForInterrupt_E waitforinterrupt = WaitForInterrupt_E::None,
			bool cpsm = true
			) const;
		bool SDMMC_GetCmdError() const;
		bool SDMMC_CmdOperCond() const;
		//
		//
		//
		bool HAL_SD_GetCardCSD(HAL_SD_CardCSDTypeDef* pCSD);
		// Send the Send CID command and check the response
		bool SDMMC_CmdSendCID(uint32* feedback) const;
		// Send the Send CSD command and check the response
		bool SDMMC_CmdSendCSD(uint32 Argument, uint32* feedback) const;
		// ???
		bool SDMMC_CmdSetRelAdd(uint16* pRCA, uint32* feedback) const;

		// [17 18] Send the Read Single/Multi Block command and check the response
		uint32 SDMMC_CmdReadSingleBlock(uint32 ReadAdd, uint32* feedback);
		uint32 SDMMC_CmdReadMultiBlock(uint32 ReadAdd, uint32* feedback);

		// [24 25] Send the Write Single/Multi Block command and check the response
		uint32 SDMMC_CmdWriteSingleBlock(uint32 ReadAdd, uint32* feedback);
		uint32 SDMMC_CmdWriteMultiBlock(uint32 ReadAdd, uint32* feedback);

		// [32 35]
		bool SDMMC_CmdSDEraseStartAdd(uint32 StartAdd, uint32* feedback);
		bool SDMMC_CmdEraseStartAdd(uint32 StartAdd, uint32* feedback);
		// [33 36]
		bool SDMMC_CmdSDEraseEndAdd(uint32 EndAdd, uint32* feedback);
		bool SDMMC_CmdEraseEndAdd(uint32 EndAdd, uint32* feedback);

		bool SDMMC_CmdErase(uint32 EraseType, uint32* feedback);

		bool SDMMC_CmdStopTransfer(uint32* feedback);

		bool SDMMC_CmdBusWidth(uint32 BusWidth, uint32* feedback);

		// Send the Select Deselect command and check the response
		bool SDMMC_CmdSelDesel(uint32 Addr, uint32* feedback) const;

		bool SDMMC_CmdSendSCR(uint32* feedback);
		bool SDMMC_CmdSetRelAddMmc(uint16 RCA, uint32* feedback);
		uint32 SDMMC_CmdSleepMmc(uint32 Argument, uint32* feedback);
		bool SDMMC_CmdSendStatus(uint32 Argument, uint32* feedback);
		bool SDMMC_CmdStatusRegister(uint32* feedback);
		bool SDMMC_CmdOpCondition(uint32 Argument, uint32* feedback);
		bool SDMMC_CmdSwitch(uint32 Argument, uint32* feedback);
		bool SDMMC_CmdVoltageSwitch(uint32* feedback);
		bool SDMMC_CmdSendEXTCSD(uint32 Argument, uint32* feedback);



		//
		bool SDMMC_GetCmdResp1(uint8 SD_CMD, uint32 Timeout, uint32* feedback) const;
		bool SDMMC_GetCmdResp2(uint32* feedback) const;
		bool SDMMC_GetCmdResp3(uint32* feedback) const;
		bool SDMMC_GetCmdResp6(uint8 SD_CMD, uint16* pRCA, uint32* feedback) const;
		bool SDMMC_GetCmdResp7() const;
		//
		bool SDMMC_CmdAppCommand(uint32 Argument, uint32* feedback) const;
		bool SDMMC_CmdAppOperCommand(uint32 Argument, uint32* feedback) const;
		//
		bool SDMMC_ConfigData(const SDMMC_DataInitTypeDef& Data);

	_Comment("Linked List management functions") protected:

		bool SD_SendSDStatus(uint32* pSDstatus, uint32* feedback);

	public://[debug]
		inline bool SendCommand(stduint argument, stduint cmdindex, byte response) {
			return SDMMC_SendCommand(argument, cmdindex, response, WaitForInterrupt_E::None, true);
		}
	public:
		// AKA __HAL_RCC_GET_SDMMC1_SOURCE = __HAL_RCC_GET_SDIO_SOURCE
		SDMMC_CLKSRC getClockSource() const;
		// AKA __HAL_RCC_SDMMC1_CONFIG
		void setClockSource(SDMMC_CLKSRC clk_src) const;
		// AKA HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SDMMCx)
		stduint getFrequency() const;

	};
	typedef SecureDigitalCard_t SDCard_t;
	extern SecureDigitalCard_t SDCard1;
	extern SecureDigitalCard_t SDCard2;
	


	
}
#endif

#endif

#endif


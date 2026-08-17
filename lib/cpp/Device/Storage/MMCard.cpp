// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Stroage) MMCard
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

#if defined(_MCU_STM32)
#include "../../../../inc/cpp/Device/SD.hpp"
#include "../../../../inc/cpp/Device/Storage/SD-PARA.h"
#include "../../../../inc/cpp/Device/RCC/RCC"
#include "../../../../inc/cpp/Device/GPIO"
#include "../../../../inc/cpp/Device/SysTick"
#include "../../../../inc/c/driver/interrupt/GIC.h"
#include "../../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"

#define BLOCKSIZE 512
#define SD_INIT_FREQ 400000U
#define MMC_HIGH_VOLTAGE_RANGE 0xC0FF8000U
#define SDMMC_MAX_VOLT_TRIAL 0x0000FFFFU
#define SDMMC_CMDTIMEOUT 5000U

#define bitmatch(bits,mask) (((bits) & (mask)) == (mask))

namespace uni {
#if defined(_MPU_STM32MP13) || defined(_MCU_STM32H7x)

	MultiMediaCard_t MMC1(1);
	MultiMediaCard_t MMC2(2);

	// AKA HAL_MMC_Init + HAL_MMC_ConfigWideBusOperation
#if defined(_MPU_STM32MP13)
	bool MultiMediaCard_t::setMode(SDMMC_CLKSRC clk_src, bool clock_edge, bool powersave_enable, SDMMC_BusWidth bus_width, bool hardware_flow_control_enable) {
#elif defined(_MCU_STM32H7x)
	bool MultiMediaCard_t::setMode(SDMMC1_CLKSRC clk_src, bool clock_edge, bool powersave_enable, SDMMC_BusWidth bus_width, bool hardware_flow_control_enable) {
#endif
		byte _id = getID() - 1;
		asrtret(_id < 2);
		// AKA HAL_MMC_MspInit
#if defined(_MPU_STM32MP13)
		setClockSource(clk_src);
		RCC[RCCReg::MP_AHB6ENSETR].setof(16 + _id);
#elif defined(_MCU_STM32H7x)
		if (getID() == 1)
			setClockSource(clk_src);
		else
			setClockSource2(clk_src == SDMMC1_CLKSRC::PLL2R ? SDMMC2_CLKSRC::PLL2R : SDMMC2_CLKSRC::PLL1Q);
		if (getID() == 1)
			RCC[RCCReg::AHB3ENR].setof(16);// SDMMC1EN
		else
			RCC[RCCReg::AHB2ENR].setof(9);// SDMMC2EN
#endif
		if (getID() == 1) {
			for (unsigned i = 8; i < 13; i++) {
				GPIOC[i].setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::High);
				GPIOC[i]._set_alternate(12);// GPIO_AF12_SDIO1
			}
			GPIOD[2].setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::High);
			GPIOD[2]._set_alternate(12);// GPIO_AF12_SDIO1
#if defined(_MPU_STM32MP13)
			GIC.setPriority(IRQ_SDMMC1, 0x00);
			GIC.enInterrupt(IRQ_SDMMC1);
#elif defined(_MCU_STM32H7x)
			NVIC.setPriority(IRQ_SDMMC1, 0x00);
			NVIC.setAble(IRQ_SDMMC1);
#endif
		}
#if defined(_MCU_STM32H7x)
		else {
			GPIOB[14].setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::High); GPIOB[14]._set_alternate(9);// SDMMC2_D0
			GPIOB[15].setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::High); GPIOB[15]._set_alternate(9);// SDMMC2_D1
			GPIOB[3].setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::High);  GPIOB[3]._set_alternate(9);// SDMMC2_D2
			GPIOB[4].setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::High);  GPIOB[4]._set_alternate(9);// SDMMC2_D3
			GPIOC[1].setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::High);  GPIOC[1]._set_alternate(9);// SDMMC2_CK
			GPIOA[0].setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::High);  GPIOA[0]._set_alternate(9);// SDMMC2_CMD
			NVIC.setPriority(IRQ_SDMMC2, 0x00);
			NVIC.setAble(IRQ_SDMMC2);
		}
#endif
		// Initialize the card parameters
		asrtret(setModeSub());
		// Configure the bus wide
		asrtret(HAL_MMC_ConfigWideBusOperation(bus_width, nullptr));
		// Verify card ready
		while (HAL_MMC_GetCardState() != HAL_SD_CardStateTypeDef::TRANSFER);
		Context = SDContext::NONE;
		return true;
	}

	bool MultiMediaCard_t::setModeSub() {
		stduint sdmmc_clk = getFrequency();
		asrtret(sdmmc_clk);
		stduint Init_ClockDiv = sdmmc_clk / (2U * SD_INIT_FREQ);
		// AKA SDMMC_Init with 1-bit / 400kHz default configuration
		{
			Reflocal(clkcr) = 0;
			setClockEdge(clkcr, true);
			setClockPowerSave(clkcr, false);
			setBusWide(clkcr, SDMMC_BusWidth::Bits1);
			setHardwareFlowControl(clkcr, false);
			setClockDiv(clkcr, Init_ClockDiv);
			self[SDReg::CLKCR] = clkcr;
		}
		self[SDReg::POWER].maset(0, 2, 0b11);// PWRCTRL ON
		// wait 74 cycles required before starting the MMC initialization sequence
		if (Init_ClockDiv) sdmmc_clk /= 2 * Init_ClockDiv;
		if (sdmmc_clk) SysDelay(1U + (74U * SysTickHz / sdmmc_clk));

		asrtret(MMC_PowerON(nullptr));
		asrtret(MMC_InitCard(nullptr));
		asrtret(SDMMC_CmdBlockLength(BLOCKSIZE));
		return true;
	}

	bool MultiMediaCard_t::canMode() {
		SDMMC_PowerState_OFF();
		return true;
	}

	// AKA MMC_PowerON: CMD0 + CMD1 loop (no CMD8/ACMD41, unlike SD)
	bool MultiMediaCard_t::MMC_PowerON(uint32* feedback) {
		uint32 count = 0;
		uint32 response = 0;
		uint32 validvoltage = 0;
		asserv(feedback)[nil] = SDMMC_ERROR_NONE;
		asrtret(SDMMC_CmdGoIdleState());
		while (validvoltage == 0U) {
			if (count++ == SDMMC_MAX_VOLT_TRIAL) {
				asserv(feedback)[nil] = SDMMC_ERROR_INVALID_VOLTRANGE;
				return false;
			}
			asrtret(SDMMC_CmdOpCondition(MMC_HIGH_VOLTAGE_RANGE, feedback));
			response = self.SDMMC_GetResponse(1);
			validvoltage = (((response >> 31U) == 1U) ? 1U : 0U);
		}
		CardType = (((response & 0xFF000000U) >> 24) == 0xC0U) ? CardType_E::HIGH_CAPACITY : CardType_E::LOW_CAPACITY;
		return true;
	}

	// AKA MMC_InitCard: CMD2/3/9/7 + EXT_CSD (CMD8)
	bool MultiMediaCard_t::MMC_InitCard(uint32* feedback) {
		uint16 mmc_rca = 1;
		SDMMC_DataInitTypeDef config;
		asserv(feedback)[nil] = SDMMC_ERROR_NONE;

		if (!SDMMC_GetPowerState()) { asserv(feedback)[nil] = SDMMC_ERROR_REQUEST_NOT_APPLICABLE; return false; }

		// CMD2 ALL_SEND_CID
		asrtret(SDMMC_CmdSendCID(feedback));
		for0(i, 4) CID[i] = self.SDMMC_GetResponse(i + 1);
		// CMD3 SET_REL_ADDR
		asrtret(SDMMC_CmdSetRelAdd(&mmc_rca, feedback));
		CardInfo.RelCardAdd = mmc_rca;
		// CMD9 SEND_CSD
		asrtret(SDMMC_CmdSendCSD((uint32)(mmc_rca << 16U), feedback));
		for0(i, 4) CSD[i] = self.SDMMC_GetResponse(i + 1);
		CardInfo.Class = self.SDMMC_GetResponse(2) >> 20;
		// CMD7 SELECT_CARD
		asrtret(SDMMC_CmdSelDesel((uint32)(mmc_rca << 16U), feedback));
		// CMD8 SEND_EXT_CSD: read 512 bytes EXT_CSD
		{
			self[SDReg::DCTRL] = 0;
			config.DataTimeOut = SDMMC_DATATIMEOUT;
			config.DataLength = 512U;
			config.DataBlockSize = 9;// 2**9 == 512B
			config.TransferDir = SDMMC_DataTransferDir::toSDMMC;
			config.TransferMode = SDMMC_DataTransferMode::Block;
			config.DPSM = false;
			(void)SDMMC_ConfigData(config);
			self[SDReg::CMD].setof(6);// CMDTRANS enable
			asrtret(SDMMC_CmdSendEXTCSD(nil, feedback));
			uint32 tickstart = SysTick::getTick();
			uint32 mask = (_IMM1S(5U)) | (_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(8U));// RXOVERR | DCRCFAIL | DTIMEOUT | DATAEND
			uint32 idx = 0;
			while (!(self[SDReg::STA] & mask)) {
				if (self[SDReg::STA].bitof(15) && (idx < 128)) {// RXFIFOHF
					for (uint32 k = 0; k < 8; k++) Ext_CSD[idx++] = self.SDMMC_ReadFIFO();
				}
				if (((SysTick::getTick() - tickstart) >= SDMMC_CMDTIMEOUT)) {
					asserv(feedback)[nil] = SDMMC_ERROR_TIMEOUT;
					return false;
				}
			}
			if (self[SDReg::STA].bitof(3)) { asserv(feedback)[nil] = SDMMC_ERROR_DATA_TIMEOUT; return false; }
			if (self[SDReg::STA].bitof(1)) { asserv(feedback)[nil] = SDMMC_ERROR_DATA_CRC_FAIL; return false; }
			self[SDReg::CMD].rstof(6);// CMDTRANS disable
			self[SDReg::ICR] = (_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) | (_IMM1S(8U)) | (_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) | (_IMM1S(27U)) | (_IMM1S(28U));
		}
		// CMD13 SEND_STATUS
		asrtret(SDMMC_CmdSendStatus((uint32)(mmc_rca << 16U), feedback));

		// Card capacity from EXT_CSD SEC_COUNT field (bytes 212..215, little-endian)
		{
			uint32 sec_count = Ext_CSD[212 / 4] & 0xFFFFFFFFU;// EXT_CSD is big-endian words in HAL; keep raw
			CardInfo.BlockSize = BLOCKSIZE;
			CardInfo.BlockNbr = sec_count;
			CardInfo.LogBlockNbr = sec_count;
			CardInfo.LogBlockSize = BLOCKSIZE;
			CardInfo.CardType = _IMM(CardType);
		}
		return true;
	}

	bool MultiMediaCard_t::Read(stduint BlockIden, void* Dest) {
		HAL_MMC_ReadBlocks((uint8_t*)Dest, BlockIden, 1, 1000, nullptr);
		while (HAL_MMC_GetCardState() != HAL_SD_CardStateTypeDef::TRANSFER);
		return true;
	}

	bool MultiMediaCard_t::Write(stduint BlockIden, const void* Sors) {
		HAL_MMC_WriteBlocks((uint8_t*)Sors, BlockIden, 1, 1000, nullptr);
		while (HAL_MMC_GetCardState() != HAL_SD_CardStateTypeDef::TRANSFER);
		return true;
	}

	// AKA HAL_MMC_ReadBlocks (polling)
	bool MultiMediaCard_t::HAL_MMC_ReadBlocks(uint8_t* pData, uint32 BlockAdd, uint32 NumberOfBlocks, uint32 Timeout, uint32* feedback) {
		SDMMC_DataInitTypeDef config;
		uint32 tickstart = SysTick::getTick();
		uint32 count;
		uint32 data;
		uint32 dataremaining;
		uint32 add = BlockAdd;
		uint8_t* tempbuff = pData;

		asserv(feedback)[nil] = SDMMC_ERROR_NONE;

		if ((add + NumberOfBlocks) > CardInfo.LogBlockNbr) {
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}

		self[SDReg::DCTRL] = 0;
		if (CardType != CardType_E::HIGH_CAPACITY)
			add *= BLOCKSIZE;

		config.DataTimeOut = SDMMC_DATATIMEOUT;
		config.DataLength = BLOCKSIZE * NumberOfBlocks;
		config.DataBlockSize = 9;// 2**9 == 512B
		config.TransferDir = SDMMC_DataTransferDir::toSDMMC;
		config.TransferMode = SDMMC_DataTransferMode::Block;
		config.DPSM = false;
		(void)SDMMC_ConfigData(config);
		self[SDReg::CMD].setof(6);// CMDTRANS enable

		uint32 (SDMMC_t::*fn_read)(uint32, uint32*);
		fn_read = (NumberOfBlocks > 1U ? &SDMMC_t::SDMMC_CmdReadMultiBlock : &SDMMC_t::SDMMC_CmdReadSingleBlock);
		Context = (NumberOfBlocks > 1U) ? SDContext::READ_MULTIPLE_BLOCK : SDContext::READ_SINGLE_BLOCK;
		if (!(self.*fn_read)(add, feedback)) {
			Context = SDContext::NONE;
			return false;
		}

		dataremaining = config.DataLength;
		uint32 mask = (_IMM1S(5U)) | (_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(8U));
		while (!(self[SDReg::STA] & mask)) {
			if (self[SDReg::STA].bitof(15) && (dataremaining >= 32U)) {
				for (count = 0U; count < 8U; count++) {
					data = self.SDMMC_ReadFIFO();
					*tempbuff++ = (uint8_t)(data & 0xFFU);
					*tempbuff++ = (uint8_t)((data >> 8U) & 0xFFU);
					*tempbuff++ = (uint8_t)((data >> 16U) & 0xFFU);
					*tempbuff++ = (uint8_t)((data >> 24U) & 0xFFU);
				}
				dataremaining -= 32U;
			}
			if (((SysTick::getTick() - tickstart) >= Timeout) || (Timeout == 0U)) {
				asserv(feedback)[nil] = SDMMC_ERROR_TIMEOUT;
				Context = SDContext::NONE;
				return false;
			}
		}
		self[SDReg::CMD].rstof(6);

		if (self[SDReg::STA].bitof(8) && (NumberOfBlocks > 1U)) {
			if (!SDMMC_CmdStopTransfer(feedback)) {
				Context = SDContext::NONE;
				return false;
			}
		}
		if (self[SDReg::STA].bitof(3)) { asserv(feedback)[nil] = SDMMC_ERROR_TIMEOUT; Context = SDContext::NONE; return false; }
		if (self[SDReg::STA].bitof(1)) { asserv(feedback)[nil] = SDMMC_ERROR_DATA_CRC_FAIL; Context = SDContext::NONE; return false; }
		if (self[SDReg::STA].bitof(5)) { asserv(feedback)[nil] = SDMMC_ERROR_RX_OVERRUN; Context = SDContext::NONE; return false; }

		self[SDReg::ICR] = (_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) | (_IMM1S(8U)) | (_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) | (_IMM1S(27U)) | (_IMM1S(28U));
		return true;
	}

	// AKA HAL_MMC_WriteBlocks (polling)
	bool MultiMediaCard_t::HAL_MMC_WriteBlocks(const uint8_t* pData, uint32 BlockAdd, uint32 NumberOfBlocks, uint32 Timeout, uint32* feedback) {
		SDMMC_DataInitTypeDef config;
		uint32 tickstart = SysTick::getTick();
		uint32 count;
		uint32 data;
		uint32 dataremaining;
		uint32 add = BlockAdd;
		const uint8_t* tempbuff = pData;

		asserv(feedback)[nil] = SDMMC_ERROR_NONE;

		if ((add + NumberOfBlocks) > CardInfo.LogBlockNbr) {
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}

		self[SDReg::DCTRL] = 0;
		if (CardType != CardType_E::HIGH_CAPACITY)
			add *= BLOCKSIZE;

		config.DataTimeOut = SDMMC_DATATIMEOUT;
		config.DataLength = BLOCKSIZE * NumberOfBlocks;
		config.DataBlockSize = 9;// 2**9 == 512B
		config.TransferDir = SDMMC_DataTransferDir::toCard;
		config.TransferMode = SDMMC_DataTransferMode::Block;
		config.DPSM = false;
		(void)SDMMC_ConfigData(config);
		self[SDReg::CMD].setof(6);// CMDTRANS enable

		uint32 (SDMMC_t::*fn_write)(uint32, uint32*);
		fn_write = (NumberOfBlocks > 1U ? &SDMMC_t::SDMMC_CmdWriteMultiBlock : &SDMMC_t::SDMMC_CmdWriteSingleBlock);
		Context = (NumberOfBlocks > 1U) ? SDContext::WRITE_MULTIPLE_BLOCK : SDContext::WRITE_SINGLE_BLOCK;
		if (!(self.*fn_write)(add, feedback)) {
			Context = SDContext::NONE;
			return false;
		}

		dataremaining = config.DataLength;
		uint32 mask = (_IMM1S(4U)) | (_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(8U));
		while (!(self[SDReg::STA] & mask)) {
			if (self[SDReg::STA].bitof(14) && (dataremaining >= 32U)) {
				for (count = 0U; count < 8U; count++) {
					data = (uint32)(*tempbuff++);
					data |= ((uint32)(*tempbuff++) << 8U);
					data |= ((uint32)(*tempbuff++) << 16U);
					data |= ((uint32)(*tempbuff++) << 24U);
					self.SDMMC_WriteFIFO(&data);
				}
				dataremaining -= 32U;
			}
			if (((SysTick::getTick() - tickstart) >= Timeout) || (Timeout == 0U)) {
				asserv(feedback)[nil] = SDMMC_ERROR_TIMEOUT;
				Context = SDContext::NONE;
				return false;
			}
		}
		self[SDReg::CMD].rstof(6);

		if (self[SDReg::STA].bitof(8) && (NumberOfBlocks > 1U)) {
			if (!SDMMC_CmdStopTransfer(feedback)) {
				Context = SDContext::NONE;
				return false;
			}
		}
		if (self[SDReg::STA].bitof(3)) { asserv(feedback)[nil] = SDMMC_ERROR_TIMEOUT; Context = SDContext::NONE; return false; }
		if (self[SDReg::STA].bitof(1)) { asserv(feedback)[nil] = SDMMC_ERROR_DATA_CRC_FAIL; Context = SDContext::NONE; return false; }
		if (self[SDReg::STA].bitof(4)) { asserv(feedback)[nil] = SDMMC_ERROR_TX_UNDERRUN; Context = SDContext::NONE; return false; }

		self[SDReg::ICR] = (_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) | (_IMM1S(8U)) | (_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) | (_IMM1S(27U)) | (_IMM1S(28U));
		return true;
	}

	bool MultiMediaCard_t::HAL_MMC_Erase(uint32 BlockStartAdd, uint32 BlockEndAdd, uint32* feedback) {
		uint32 start_add = BlockStartAdd;
		uint32 end_add = BlockEndAdd;
		asserv(feedback)[nil] = SDMMC_ERROR_NONE;
		if (end_add < start_add) { asserv(feedback)[nil] = SDMMC_ERROR_INVALID_PARAMETER; return false; }
		if (end_add > CardInfo.LogBlockNbr) { asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE; return false; }
		if (CardType != CardType_E::HIGH_CAPACITY) {
			start_add *= BLOCKSIZE;
			end_add *= BLOCKSIZE;
		}
		// MMC erase uses ERASE_GRP_START/END (CMD35/36), not SD_ERASE (CMD32/33)
		asrtret(SDMMC_CmdEraseStartAdd(start_add, feedback));
		asrtret(SDMMC_CmdEraseEndAdd(end_add, feedback));
		asrtret(SDMMC_CmdErase(0UL, feedback));
		return true;
	}

	bool MultiMediaCard_t::HAL_MMC_GetCardCSD(HAL_SD_CardCSDTypeDef* pCSD) {
		// CSD layout is shared between SD and MMC for the common fields; keep it minimal.
		asrtret(pCSD);
		pCSD->CSDStruct = (uint8_t)((CSD[0] & 0xC0000000U) >> 30U);
		pCSD->SysSpecVersion = (uint8_t)((CSD[0] & 0x3C000000U) >> 26U);
		pCSD->TAAC = (uint8_t)((CSD[0] & 0x00FF0000U) >> 16U);
		pCSD->NSAC = (uint8_t)((CSD[0] & 0x0000FF00U) >> 8U);
		pCSD->MaxBusClkFrec = (uint8_t)(CSD[0] & 0x000000FFU);
		pCSD->CardComdClasses = (uint16_t)((CSD[1] & 0xFFF00000U) >> 20U);
		pCSD->RdBlockLen = (uint8_t)((CSD[1] & 0x000F0000U) >> 16U);
		return true;
	}

	void MultiMediaCard_t::HAL_MMC_GetCardCID(HAL_SD_CardCIDTypeDef* pCID) {
		pCID->ManufacturerID = (uint8_t)((CID[0] & 0xFF000000U) >> 24U);
		pCID->OEM_AppliID = (uint16_t)((CID[0] & 0x00FFFF00U) >> 8U);
		pCID->ProdName1 = ((CID[0] & 0x000000FFU) << 24U) | ((CID[1] & 0xFFFFFF00U) >> 8U);
		pCID->ProdName2 = (uint8_t)(CID[1] & 0x000000FFU);
		pCID->ProdRev = (uint8_t)((CID[2] & 0xFF000000U) >> 24U);
		pCID->ProdSN = ((CID[2] & 0x00FFFFFFU) << 8U) | ((CID[3] & 0xFF000000U) >> 24U);
		pCID->ManufactDate = (uint16_t)((CID[3] & 0x0FFF0000U) >> 16U);
		pCID->CID_CRC = (uint8_t)((CID[3] & 0x0000FF00U) >> 8U);
	}

	void MultiMediaCard_t::HAL_MMC_GetCardInfo(HAL_MMC_CardInfoTypeDef* pCardInfo) {
		pCardInfo->CardType = CardInfo.CardType;
		pCardInfo->Class = CardInfo.Class;
		pCardInfo->RelCardAdd = CardInfo.RelCardAdd;
		pCardInfo->BlockNbr = CardInfo.BlockNbr;
		pCardInfo->BlockSize = CardInfo.BlockSize;
		pCardInfo->LogBlockNbr = CardInfo.LogBlockNbr;
		pCardInfo->LogBlockSize = CardInfo.LogBlockSize;
	}

	HAL_SD_CardStateTypeDef MultiMediaCard_t::HAL_MMC_GetCardState() {
		uint32 cardstate;
		if (!SDMMC_CmdSendStatus((uint32)(CardInfo.RelCardAdd << 16U), nullptr)) {
			return HAL_SD_CardStateTypeDef::ERROR;
		}
		cardstate = self.SDMMC_GetResponse(1);
		return (HAL_SD_CardStateTypeDef)((cardstate >> 9U) & 0x0FU);
	}

	// AKA HAL_MMC_ConfigWideBusOperation: switch bus width via EXT_CSD BUS_WIDTH (byte 183)
	bool MultiMediaCard_t::HAL_MMC_ConfigWideBusOperation(SDMMC_BusWidth bus_width, uint32* feedback) {
		uint32 sw_arg;
		asserv(feedback)[nil] = SDMMC_ERROR_NONE;
		switch (bus_width) {
		case SDMMC_BusWidth::Bits1:
			sw_arg = (0x03U << 24U) | (183U << 16U) | (0x00U << 8U) | 0x00U;
			break;
		case SDMMC_BusWidth::Bits4:
			sw_arg = (0x03U << 24U) | (183U << 16U) | (0x01U << 8U) | 0x00U;
			break;
		case SDMMC_BusWidth::Bits8:
			sw_arg = (0x03U << 24U) | (183U << 16U) | (0x02U << 8U) | 0x00U;
			break;
		default:
			asserv(feedback)[nil] = SDMMC_ERROR_INVALID_PARAMETER;
			return false;
		}
		// SWITCH (CMD6) to set EXT_CSD BUS_WIDTH
		asrtret(SDMMC_CmdSwitch(sw_arg, feedback));
		// Apply bus width to CLKCR
		Reference clkcr = self[SDReg::CLKCR];
		setBusWide(clkcr, bus_width);
		self[SDReg::CLKCR] = clkcr;
		return true;
	}

	bool MultiMediaCard_t::HAL_MMC_Abort() {
		// Disable all interrupts + internal DMA
		self[SDReg::MASK] &= ~(((0x1UL << 8U) | (0x1UL << 1U) | (0x1UL << 3U) | (0x1UL << 4U) | (0x1UL << 5U)));
		self[SDReg::IDMACTRL] = 0;
		self[SDReg::ICR] = (_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) | (_IMM1S(8U)) | (_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) | (_IMM1S(27U)) | (_IMM1S(28U));
		Context = SDContext::NONE;
		return true;
	}

	bool MultiMediaCard_t::HAL_MMC_Abort_IT() {
		uint32 error_code = SDMMC_ERROR_NONE;
		self[SDReg::MASK] &= ~(((0x1UL << 8U) | (0x1UL << 1U) | (0x1UL << 3U) | (0x1UL << 4U) | (0x1UL << 5U)));
		self[SDReg::IDMACTRL] = 0;
		self[SDReg::ICR] = (_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) | (_IMM1S(8U)) | (_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) | (_IMM1S(27U)) | (_IMM1S(28U));
		HAL_SD_CardStateTypeDef CardState = HAL_MMC_GetCardState();
		if ((CardState == HAL_SD_CardStateTypeDef::RECEIVING) || (CardState == HAL_SD_CardStateTypeDef::SENDING)) {
			error_code = SDMMC_CmdStopTransfer(&error_code);
		}
		if (error_code != SDMMC_ERROR_NONE) return false;
		if (AbortCpltHandler) AbortCpltHandler();
		return true;
	}

	// AKA HAL_MMC_ReadBlocks_IT
	bool MultiMediaCard_t::HAL_MMC_ReadBlocks_IT(uint8_t* pData, uint32 BlockAdd, uint32 NumberOfBlocks, uint32* feedback) {
		SDMMC_DataInitTypeDef config;
		uint32 add = BlockAdd;
		asserv(feedback)[nil] = SDMMC_ERROR_NONE;
		if ((add + NumberOfBlocks) > CardInfo.LogBlockNbr) {
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}
		self[SDReg::DCTRL] = 0;
		RxBuff.address = _IMM(pData);
		RxBuff.length = BLOCKSIZE * NumberOfBlocks;
		if (CardType != CardType_E::HIGH_CAPACITY)
			add *= BLOCKSIZE;
		config.DataTimeOut = SDMMC_DATATIMEOUT;
		config.DataLength = BLOCKSIZE * NumberOfBlocks;
		config.DataBlockSize = 9;
		config.TransferDir = SDMMC_DataTransferDir::toSDMMC;
		config.TransferMode = SDMMC_DataTransferMode::Block;
		config.DPSM = false;
		(void)SDMMC_ConfigData(config);
		self[SDReg::CMD].setof(6);
		uint32 (SDMMC_t::*fn_read)(uint32, uint32*);
		fn_read = (NumberOfBlocks > 1U ? &SDMMC_t::SDMMC_CmdReadMultiBlock : &SDMMC_t::SDMMC_CmdReadSingleBlock);
		Context = (NumberOfBlocks > 1U) ? SDContext::READ_MULTIPLE_BLOCK_IT : SDContext::READ_SINGLE_BLOCK_IT;
		if (!(self.*fn_read)(add, feedback)) {
			Context = SDContext::NONE;
			return false;
		}
		self[SDReg::MASK] |= (((_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(5U)) | (_IMM1S(8U)) | (_IMM1S(15U))));
		return true;
	}

	// AKA HAL_MMC_WriteBlocks_IT
	bool MultiMediaCard_t::HAL_MMC_WriteBlocks_IT(const uint8_t* pData, uint32 BlockAdd, uint32 NumberOfBlocks, uint32* feedback) {
		SDMMC_DataInitTypeDef config;
		uint32 add = BlockAdd;
		asserv(feedback)[nil] = SDMMC_ERROR_NONE;
		if ((add + NumberOfBlocks) > CardInfo.LogBlockNbr) {
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}
		self[SDReg::DCTRL] = 0;
		TxBuff.address = _IMM(pData);
		TxBuff.length = BLOCKSIZE * NumberOfBlocks;
		if (CardType != CardType_E::HIGH_CAPACITY)
			add *= BLOCKSIZE;
		config.DataTimeOut = SDMMC_DATATIMEOUT;
		config.DataLength = BLOCKSIZE * NumberOfBlocks;
		config.DataBlockSize = 9;
		config.TransferDir = SDMMC_DataTransferDir::toCard;
		config.TransferMode = SDMMC_DataTransferMode::Block;
		config.DPSM = false;
		(void)SDMMC_ConfigData(config);
		self[SDReg::CMD].setof(6);
		uint32 (SDMMC_t::*fn_write)(uint32, uint32*);
		fn_write = (NumberOfBlocks > 1U ? &SDMMC_t::SDMMC_CmdWriteMultiBlock : &SDMMC_t::SDMMC_CmdWriteSingleBlock);
		Context = (NumberOfBlocks > 1U) ? SDContext::WRITE_MULTIPLE_BLOCK_IT : SDContext::WRITE_SINGLE_BLOCK_IT;
		if (!(self.*fn_write)(add, feedback)) {
			Context = SDContext::NONE;
			return false;
		}
		self[SDReg::MASK] |= (((_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(8U)) | (_IMM1S(14U))));
		return true;
	}

	// AKA HAL_MMC_ReadBlocks_DMA (single-buffer internal DMA)
	bool MultiMediaCard_t::HAL_MMC_ReadBlocks_DMA(uint8_t* pData, uint32 BlockAdd, uint32 NumberOfBlocks, uint32* feedback) {
		SDMMC_DataInitTypeDef config;
		uint32 add = BlockAdd;
		asserv(feedback)[nil] = SDMMC_ERROR_NONE;
		if ((add + NumberOfBlocks) > CardInfo.LogBlockNbr) {
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}
		self[SDReg::DCTRL] = 0;
		RxBuff.address = _IMM(pData);
		RxBuff.length = BLOCKSIZE * NumberOfBlocks;
		if (CardType != CardType_E::HIGH_CAPACITY)
			add *= BLOCKSIZE;
		config.DataTimeOut = SDMMC_DATATIMEOUT;
		config.DataLength = BLOCKSIZE * NumberOfBlocks;
		config.DataBlockSize = 9;
		config.TransferDir = SDMMC_DataTransferDir::toSDMMC;
		config.TransferMode = SDMMC_DataTransferMode::Block;
		config.DPSM = false;
		(void)SDMMC_ConfigData(config);
		self[SDReg::CMD].setof(6);
		self[SDReg::IDMABASER] = (uint32)pData;
		self[SDReg::IDMACTRL] = 1;// IDMAEN single buffer
		uint32 (SDMMC_t::*fn_read)(uint32, uint32*);
		fn_read = (NumberOfBlocks > 1U ? &SDMMC_t::SDMMC_CmdReadMultiBlock : &SDMMC_t::SDMMC_CmdReadSingleBlock);
		Context = (NumberOfBlocks > 1U) ? SDContext::READ_MULTIPLE_BLOCK_DMA : SDContext::READ_SINGLE_BLOCK_DMA;
		if (!(self.*fn_read)(add, feedback)) {
			Context = SDContext::NONE;
			return false;
		}
		self[SDReg::MASK] |= (((_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(5U)) | (_IMM1S(8U))));
		return true;
	}

	// AKA HAL_MMC_WriteBlocks_DMA (single-buffer internal DMA)
	bool MultiMediaCard_t::HAL_MMC_WriteBlocks_DMA(const uint8_t* pData, uint32 BlockAdd, uint32 NumberOfBlocks, uint32* feedback) {
		SDMMC_DataInitTypeDef config;
		uint32 add = BlockAdd;
		asserv(feedback)[nil] = SDMMC_ERROR_NONE;
		if ((add + NumberOfBlocks) > CardInfo.LogBlockNbr) {
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}
		self[SDReg::DCTRL] = 0;
		TxBuff.address = _IMM(pData);
		TxBuff.length = BLOCKSIZE * NumberOfBlocks;
		if (CardType != CardType_E::HIGH_CAPACITY)
			add *= BLOCKSIZE;
		config.DataTimeOut = SDMMC_DATATIMEOUT;
		config.DataLength = BLOCKSIZE * NumberOfBlocks;
		config.DataBlockSize = 9;
		config.TransferDir = SDMMC_DataTransferDir::toCard;
		config.TransferMode = SDMMC_DataTransferMode::Block;
		config.DPSM = false;
		(void)SDMMC_ConfigData(config);
		self[SDReg::CMD].setof(6);
		self[SDReg::IDMABASER] = (uint32)pData;
		self[SDReg::IDMACTRL] = 1;// IDMAEN single buffer
		uint32 (SDMMC_t::*fn_write)(uint32, uint32*);
		fn_write = (NumberOfBlocks > 1U ? &SDMMC_t::SDMMC_CmdWriteMultiBlock : &SDMMC_t::SDMMC_CmdWriteSingleBlock);
		Context = (NumberOfBlocks > 1U) ? SDContext::WRITE_MULTIPLE_BLOCK_DMA : SDContext::WRITE_SINGLE_BLOCK_DMA;
		if (!(self.*fn_write)(add, feedback)) {
			Context = SDContext::NONE;
			return false;
		}
		self[SDReg::MASK] |= (((_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(8U))));
		return true;
	}

	// AKA MMC_Read_IT
	void MultiMediaCard_t::SD_Read_IT() {
		uint32 count;
		uint32 data;
		uint8_t* tmp = (uint8_t*)RxBuff.address;
		if (RxBuff.length >= 32U) {
			for (count = 0U; count < 8U; count++) {
				data = self.SDMMC_ReadFIFO();
				*tmp++ = (uint8_t)(data & 0xFFU);
				*tmp++ = (uint8_t)((data >> 8U) & 0xFFU);
				*tmp++ = (uint8_t)((data >> 16U) & 0xFFU);
				*tmp++ = (uint8_t)((data >> 24U) & 0xFFU);
			}
			RxBuff.address = _IMM(tmp);
			RxBuff.length -= 32U;
		}
	}

	// AKA MMC_Write_IT
	void MultiMediaCard_t::SD_Write_IT() {
		uint32 count;
		uint32 data;
		const uint8_t* tmp = (uint8_t*)TxBuff.address;
		if (TxBuff.length >= 32U) {
			for (count = 0U; count < 8U; count++) {
				data = (uint32)(*tmp++);
				data |= ((uint32)(*tmp++) << 8U);
				data |= ((uint32)(*tmp++) << 16U);
				data |= ((uint32)(*tmp++) << 24U);
				self.SDMMC_WriteFIFO(&data);
			}
			TxBuff.address = _IMM(tmp);
			TxBuff.length -= 32U;
		}
	}

#endif
}

#endif // _MCU_STM32

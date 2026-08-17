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

#include "../../../../inc/cpp/Device/SD.hpp"
#include "../../../../inc/cpp/Device/Storage/SD-PARA.h"
#include "../../../../inc/cpp/Device/SysTick"

#define BLOCKSIZE 512
#define SDMMC_CMDTIMEOUT 5000U

namespace uni {
#if defined(_MPU_STM32MP13)

	// AKA HAL_MMCEx_DMALinkedList_ReadBlocks
	bool MultiMediaCard_t::HAL_MMCEx_DMALinkedList_ReadBlocks(SDMMC_DMALinkedList* pLinkedList, uint32 BlockAddr, uint32 NumberOfBlocks, uint32* feedback) {
		SDMMC_DataInitTypeDef config;
		uint32 DmaBase0_reg;
		uint32 DmaBase1_reg;
		uint32 add = BlockAddr;

		if ((add + NumberOfBlocks) > CardInfo.LogBlockNbr) {
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}

		self[SDReg::IDMABASER] = pLinkedList->pHeadNode->IDMABASER;
		self[SDReg::IDMABSIZE] = pLinkedList->pHeadNode->IDMABSIZE;
		self[SDReg::IDMABAR] = pLinkedList->pHeadNode;
		self[SDReg::IDMALAR] = _IMM1S(29) | _IMM1S(30) | _IMM1S(31)
			| sizeof(SDMMC_DMALinkNode);// ABR | ULS | ULA

		DmaBase0_reg = self[SDReg::IDMABASER];
		DmaBase1_reg = self[SDReg::IDMABAR];
		if ((self[SDReg::IDMABSIZE] == 0U) || (DmaBase0_reg == 0U) || (DmaBase1_reg == 0U)) {
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}

		self[SDReg::DCTRL] = 0;
		self[SDReg::ICR] = (_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) | (_IMM1S(8U)) | (_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) | (_IMM1S(27U)) | (_IMM1S(28U));

		asserv(feedback)[nil] = SDMMC_ERROR_NONE;

		if (CardType != CardType_E::HIGH_CAPACITY)
			add *= 512U;

		config.DataTimeOut = SDMMC_DATATIMEOUT;
		config.DataLength = BLOCKSIZE * NumberOfBlocks;
		config.DataBlockSize = 9;// 2**9 == 512B
		config.TransferDir = SDMMC_DataTransferDir::toSDMMC;
		config.TransferMode = SDMMC_DataTransferMode::Block;
		config.DPSM = false;
		(void)SDMMC_ConfigData(config);

		self[SDReg::DCTRL].setof(13);// FIFORST
		self[SDReg::CMD].setof(6);// CMDTRANS enable
		self[SDReg::IDMACTRL] = 0b11;// IDMAEN | IDMABMODE

		Context = SDContext::READ_MULTIPLE_BLOCK_DMA;

		asrtret(SDMMC_CmdReadMultiBlock(add, feedback));

		self[SDReg::MASK] |= (((0x1UL << 1U) | (0x1UL << 3U) | (0x1UL << 5U) | (0x1UL << 8U) | (0x1UL << 28U)));
		return true;
	}

	// AKA HAL_MMCEx_DMALinkedList_WriteBlocks
	bool MultiMediaCard_t::HAL_MMCEx_DMALinkedList_WriteBlocks(SDMMC_DMALinkedList* pLinkedList, uint32 BlockAddr, uint32 NumberOfBlocks, uint32* feedback) {
		SDMMC_DataInitTypeDef config;
		uint32 DmaBase0_reg;
		uint32 DmaBase1_reg;
		uint32 add = BlockAddr;

		if ((add + NumberOfBlocks) > CardInfo.LogBlockNbr) {
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}

		self[SDReg::IDMABASER] = pLinkedList->pHeadNode->IDMABASER;
		self[SDReg::IDMABSIZE] = pLinkedList->pHeadNode->IDMABSIZE;
		self[SDReg::IDMABAR] = pLinkedList->pHeadNode;
		self[SDReg::IDMALAR] = _IMM1S(29) | _IMM1S(30) | _IMM1S(31)
			| sizeof(SDMMC_DMALinkNode);// ABR | ULS | ULA

		DmaBase0_reg = self[SDReg::IDMABASER];
		DmaBase1_reg = self[SDReg::IDMABAR];
		if ((self[SDReg::IDMABSIZE] == 0U) || (DmaBase0_reg == 0U) || (DmaBase1_reg == 0U)) {
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}

		self[SDReg::DCTRL] = 0;
		asserv(feedback)[nil] = SDMMC_ERROR_NONE;

		if (CardType != CardType_E::HIGH_CAPACITY)
			add *= 512U;

		config.DataTimeOut = SDMMC_DATATIMEOUT;
		config.DataLength = BLOCKSIZE * NumberOfBlocks;
		config.DataBlockSize = 9;// 2**9 == 512B
		config.TransferDir = SDMMC_DataTransferDir::toCard;
		config.TransferMode = SDMMC_DataTransferMode::Block;
		config.DPSM = false;
		(void)SDMMC_ConfigData(config);

		self[SDReg::CMD].setof(6);// CMDTRANS enable
		self[SDReg::IDMACTRL] = 0b11;// IDMAEN | IDMABMODE

		Context = SDContext::WRITE_MULTIPLE_BLOCK_DMA;

		asrtret(SDMMC_CmdWriteMultiBlock(add, feedback));

		self[SDReg::MASK] |= (((0x1UL << 1U) | (0x1UL << 3U) | (0x1UL << 4U) | (0x1UL << 8U) | (0x1UL << 28U)));
		return true;
	}

	// AKA HAL_MMC_GetCardExtCSD: read 512-byte EXT_CSD via CMD8
	bool MultiMediaCard_t::HAL_MMC_GetCardExtCSD(uint32* pExtCSD, uint32 Timeout) {
		SDMMC_DataInitTypeDef config;
		asrtret(pExtCSD);
		self[SDReg::DCTRL] = 0;
		config.DataTimeOut = SDMMC_DATATIMEOUT;
		config.DataLength = 512U;
		config.DataBlockSize = 9;// 2**9 == 512B
		config.TransferDir = SDMMC_DataTransferDir::toSDMMC;
		config.TransferMode = SDMMC_DataTransferMode::Block;
		config.DPSM = false;
		(void)SDMMC_ConfigData(config);
		self[SDReg::CMD].setof(6);// CMDTRANS enable
		asrtret(SDMMC_CmdSendEXTCSD(nil, nullptr));

		uint32 tickstart = SysTick::getTick();
		uint32 mask = (_IMM1S(5U)) | (_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(8U));
		uint32 idx = 0;
		while (!(self[SDReg::STA] & mask)) {
			if (self[SDReg::STA].bitof(15) && (idx < 128)) {
				for (uint32 k = 0; k < 8; k++) pExtCSD[idx++] = self.SDMMC_ReadFIFO();
			}
			if (((SysTick::getTick() - tickstart) >= Timeout) || (Timeout == 0U)) {
				self[SDReg::CMD].rstof(6);
				return false;
			}
		}
		self[SDReg::CMD].rstof(6);
		if (self[SDReg::STA].bitof(3)) return false;
		if (self[SDReg::STA].bitof(1)) return false;
		self[SDReg::ICR] = (_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) | (_IMM1S(8U)) | (_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) | (_IMM1S(27U)) | (_IMM1S(28U));
		return true;
	}

	// position value of the lowest set bit (AKA POSITION_VAL)
	inline static stduint mmc_posval(stduint x) {
		stduint r = 0;
		if (!x) return 0;
		while (x >>= 1) r++;
		return r;
	}

	// AKA HAL_MMC_GetSupportedSecRemovalType: read SECURE_REMOVAL_TYPE [16]
	bool MultiMediaCard_t::HAL_MMC_GetSupportedSecRemovalType(uint32* SupportedSRT) {
		asrtret(SupportedSRT);
		*SupportedSRT = Ext_CSD[4] & 0x0FU;
		return true;
	}

	// AKA HAL_MMC_ConfigSecRemovalType: write SECURE_REMOVAL_TYPE [16]
	bool MultiMediaCard_t::HAL_MMC_ConfigSecRemovalType(uint32 SRTMode, uint32* feedback) {
		uint32 srt;
		uint32 response = 0;
		uint32 count;
		asserv(feedback)[nil] = SDMMC_ERROR_NONE;
		asrtret(HAL_MMC_GetSupportedSecRemovalType(&srt));
		if ((SRTMode & srt) == 0U) {
			asserv(feedback)[nil] = SDMMC_ERROR_UNSUPPORTED_FEATURE;
			return false;
		}
		srt |= (mmc_posval(SRTMode) << 4U);
		asrtret(SDMMC_CmdSwitch(0x03100000U | (srt << 8U), feedback));
		count = 0xFFFFU;
		do {
			asrtret(SDMMC_CmdSendStatus((uint32)(CardInfo.RelCardAdd << 16U), feedback));
			response = self.SDMMC_GetResponse(1);
			count--;
		} while (((response & 0x100U) == 0U) && (count != 0U));
		if (count == 0U) { asserv(feedback)[nil] = SDMMC_ERROR_TIMEOUT; return false; }
		if ((response & 0x80U) != 0U) { asserv(feedback)[nil] = SDMMC_ERROR_GENERAL_UNKNOWN_ERR; return false; }
		return true;
	}

	// AKA HAL_MMC_EraseSequence: CMD35/36/38 with erase type (SECURE_ERASE etc.)
	bool MultiMediaCard_t::HAL_MMC_EraseSequence(uint32 EraseType, uint32 BlockStartAdd, uint32 BlockEndAdd, uint32* feedback) {
		uint32 start_add = BlockStartAdd;
		uint32 end_add = BlockEndAdd;
		uint32 tickstart = SysTick::getTick();
		asserv(feedback)[nil] = SDMMC_ERROR_NONE;

		if (end_add < start_add) { asserv(feedback)[nil] = SDMMC_ERROR_INVALID_PARAMETER; return false; }
		if (end_add > CardInfo.LogBlockNbr) { asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE; return false; }
		// 4KB block check: field DATA_SEC_SIZE [61] != 0 -> addresses must be 8-aligned
		if (((Ext_CSD[61 / 4] >> 0) & 0xFFU) != 0U) {
			if (((start_add % 8U) != 0U) || ((end_add % 8U) != 0U)) {
				asserv(feedback)[nil] = SDMMC_ERROR_ADDR_MISALIGNED;
				return false;
			}
		}
		if ((CardInfo.Class & 0x20U) == 0U) { asserv(feedback)[nil] = SDMMC_ERROR_REQUEST_NOT_APPLICABLE; return false; }

		if (CardType != CardType_E::HIGH_CAPACITY) {
			start_add *= BLOCKSIZE;
			end_add *= BLOCKSIZE;
		}

		asrtret(SDMMC_CmdEraseStartAdd(start_add, feedback));
		asrtret(SDMMC_CmdEraseEndAdd(end_add, feedback));
		asrtret(SDMMC_CmdErase(EraseType, feedback));

		// SECURE_ERASE / SECURE_TRIM_STEP2 wait BUSYD0END
		if ((EraseType == 0x80000000U) || (EraseType == 0x00000004U)) {
			while (!self[SDReg::STA].bitof(21)) {
				if ((SysTick::getTick() - tickstart) >= 63000U) { asserv(feedback)[nil] = SDMMC_ERROR_TIMEOUT; return false; }
			}
			self[SDReg::ICR] = _IMM1S(21);// BUSYD0END
		}
		return true;
	}

	// AKA HAL_MMC_Sanitize: CMD6 SWITCH SANITIZE_START [165] = 1
	bool MultiMediaCard_t::HAL_MMC_Sanitize(uint32* feedback) {
		uint32 response = 0;
		uint32 count;
		uint32 tickstart = SysTick::getTick();
		asserv(feedback)[nil] = SDMMC_ERROR_NONE;
		asrtret(SDMMC_CmdSwitch(0x03A50100U, feedback));
		while (!self[SDReg::STA].bitof(21)) {
			if ((SysTick::getTick() - tickstart) >= 63000U) { asserv(feedback)[nil] = SDMMC_ERROR_TIMEOUT; return false; }
		}
		self[SDReg::ICR] = _IMM1S(21);// BUSYD0END
		count = 0xFFFFU;
		do {
			asrtret(SDMMC_CmdSendStatus((uint32)(CardInfo.RelCardAdd << 16U), feedback));
			response = self.SDMMC_GetResponse(1);
			count--;
		} while (((response & 0x100U) == 0U) && (count != 0U));
		if (count == 0U) { asserv(feedback)[nil] = SDMMC_ERROR_TIMEOUT; return false; }
		if ((response & 0x80U) != 0U) { asserv(feedback)[nil] = SDMMC_ERROR_GENERAL_UNKNOWN_ERR; return false; }
		return true;
	}

	// AKA HAL_MMC_SleepDevice: switch device from Standby to Sleep (CMD5)
	bool MultiMediaCard_t::HAL_MMC_SleepDevice(uint32* feedback) {
		uint32 response = 0;
		uint32 count;
		uint32 tickstart = SysTick::getTick();
		asserv(feedback)[nil] = SDMMC_ERROR_NONE;
		// POWER_OFF_NOTIFICATION [34] = 1 (powered on)
		asrtret(SDMMC_CmdSwitch(0x03220100U, feedback));
		count = 0xFFFFU;
		do {
			asrtret(SDMMC_CmdSendStatus((uint32)(CardInfo.RelCardAdd << 16U), feedback));
			response = self.SDMMC_GetResponse(1);
			count--;
		} while (((response & 0x100U) == 0U) && (count != 0U));
		if (count == 0U) { asserv(feedback)[nil] = SDMMC_ERROR_TIMEOUT; return false; }
		if ((response & 0x80U) != 0U) { asserv(feedback)[nil] = SDMMC_ERROR_UNSUPPORTED_FEATURE; return false; }
		// POWER_OFF_NOTIFICATION [34] = 4 (sleep notification)
		asrtret(SDMMC_CmdSwitch(0x03220400U, feedback));
		while (!self[SDReg::STA].bitof(21)) {
			if ((SysTick::getTick() - tickstart) >= 1000U) { asserv(feedback)[nil] = SDMMC_ERROR_TIMEOUT; return false; }
		}
		self[SDReg::ICR] = _IMM1S(21);// BUSYD0END
		// deselect card (standby)
		asrtret(SDMMC_CmdSelDesel(0U, feedback));
		// CMD5 sleep with RCA | 0x1<<15
		asrtret(SDMMC_CmdSleepMmc(((uint32)CardInfo.RelCardAdd << 16U) | (0x1U << 15U), feedback));
		while (!self[SDReg::STA].bitof(21)) {
			if ((SysTick::getTick() - tickstart) >= 1000U) { asserv(feedback)[nil] = SDMMC_ERROR_TIMEOUT; return false; }
		}
		self[SDReg::ICR] = _IMM1S(21);// BUSYD0END
		return true;
	}

	// AKA HAL_MMC_AwakeDevice: switch device from Sleep to Standby (CMD5)
	bool MultiMediaCard_t::HAL_MMC_AwakeDevice(uint32* feedback) {
		uint32 tickstart = SysTick::getTick();
		asserv(feedback)[nil] = SDMMC_ERROR_NONE;
		// CMD5 awake with RCA (no sleep bit)
		asrtret(SDMMC_CmdSleepMmc(((uint32)CardInfo.RelCardAdd << 16U), feedback));
		while (!self[SDReg::STA].bitof(21)) {
			if ((SysTick::getTick() - tickstart) >= 1000U) { asserv(feedback)[nil] = SDMMC_ERROR_TIMEOUT; return false; }
		}
		self[SDReg::ICR] = _IMM1S(21);// BUSYD0END
		return true;
	}

	// AKA HAL_MMC_ConfigSpeedBusOperation: switch HS_TIMING [185] (high speed / DDR)
	bool MultiMediaCard_t::HAL_MMC_ConfigSpeedBusOperation(SDMMC_SPEED_MODE SpeedMode) {
		uint32 device_type = Ext_CSD[49] & 0xFFU;// DEVICE_TYPE [196]
		switch (SpeedMode) {
		case SDMMC_SPEED_MODE::HIGH:
			if ((device_type & 0x02U) == 0U) return false;
			asrtret(SDMMC_CmdSwitch(0x03B90100U, nullptr));// HS_TIMING [185] = 1
			self[SDReg::CLKCR].setof(19, true);// BUSSPEED
			return true;
		case SDMMC_SPEED_MODE::DDR:
			if ((device_type & 0x04U) == 0U) return false;
			asrtret(SDMMC_CmdSwitch(0x03B90200U, nullptr));// HS_TIMING [185] = 2 (DDR)
			self[SDReg::CLKCR].setof(19, true);// BUSSPEED
			self[SDReg::CLKCR].setof(18, true);// DDR
			return true;
		case SDMMC_SPEED_MODE::DEFAULT:
			self[SDReg::CLKCR].setof(19, false);// BUSSPEED
			self[SDReg::CLKCR].setof(18, false);// DDR
			asrtret(SDMMC_CmdSwitch(0x03B90000U, nullptr));// HS_TIMING [185] = 0
			return true;
		default:
			return true;// AUTO / unsupported: keep current
		}
	}

#endif
}

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

#define BLOCKSIZE 512

namespace uni {
#if defined(_MCU_STM32H7x)

	// AKA HAL_MMCEx_ConfigDMAMultiBuffer
	bool MultiMediaCard_t::HAL_MMCEx_ConfigDMAMultiBuffer(uint32* pDataBuffer0, uint32* pDataBuffer1, uint32 BufferSize) {
		if (Context != SDContext::NONE) return false;// busy
		self[SDReg::IDMABASER] = (uint32)pDataBuffer0;// IDMABASE0
		self[SDReg::IDMABASE1] = (uint32)pDataBuffer1;// IDMABASE1
		self[SDReg::IDMABSIZE] = (uint32)(BLOCKSIZE * BufferSize);
		return true;
	}

	// AKA HAL_MMCEx_ChangeDMABuffer
	bool MultiMediaCard_t::HAL_MMCEx_ChangeDMABuffer(SDMMC_DMABuffer Buffer, uint32* pDataBuffer) {
		if (Buffer == SDMMC_DMABuffer::Buffer0)
			self[SDReg::IDMABASER] = (uint32)pDataBuffer;// IDMABASE0
		else
			self[SDReg::IDMABASE1] = (uint32)pDataBuffer;// IDMABASE1
		return true;
	}

	// AKA HAL_MMCEx_ReadBlocksDMAMultiBuffer
	bool MultiMediaCard_t::HAL_MMCEx_ReadBlocksDMAMultiBuffer(uint32 BlockAdd, uint32 NumberOfBlocks, uint32* feedback) {
		SDMMC_DataInitTypeDef config;
		uint32 add = BlockAdd;

		asserv(feedback)[nil] = SDMMC_ERROR_NONE;

		if ((add + NumberOfBlocks) > CardInfo.LogBlockNbr) {
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}
		if (((uint32)self[SDReg::IDMABSIZE] == 0U) || ((uint32)self[SDReg::IDMABASER] == 0U) || ((uint32)self[SDReg::IDMABASE1] == 0U)) {
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}

		self[SDReg::DCTRL] = 0;
		self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
			(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
			(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
			(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
			(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
			(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
			(_IMM1S(27U)) | (_IMM1S(28U));

		if (CardType != CardType_E::HIGH_CAPACITY)
			add *= BLOCKSIZE;

		config.DataTimeOut = SDMMC_DATATIMEOUT;
		config.DataLength = BLOCKSIZE * NumberOfBlocks;
		config.DataBlockSize = 9;// 2**9 == 512B
		config.TransferDir = SDMMC_DataTransferDir::toSDMMC;
		config.TransferMode = SDMMC_DataTransferMode::Block;
		config.DPSM = false;
		(void)SDMMC_ConfigData(config);

		self[SDReg::DCTRL] |= _IMM1S(13);// SDMMC_DCTRL_FIFORST
		self[SDReg::CMD].setof(6);// __SDMMC_CMDTRANS_ENABLE
		self[SDReg::IDMACTRL] = _IMM1S(0) | _IMM1S(1);// IDMAEN | IDMABMODE (double buffer, start with buffer0)

		self[SDReg::MASK] |= (((_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(5U)) | (_IMM1S(8U)) | (_IMM1S(28U))));

		Context = SDContext::READ_MULTIPLE_BLOCK_DMA;

		if (!SDMMC_CmdReadMultiBlock(add, feedback)) {
			Context = SDContext::NONE;
			return false;
		}
		return true;
	}

	// AKA HAL_MMCEx_WriteBlocksDMAMultiBuffer
	bool MultiMediaCard_t::HAL_MMCEx_WriteBlocksDMAMultiBuffer(uint32 BlockAdd, uint32 NumberOfBlocks, uint32* feedback) {
		SDMMC_DataInitTypeDef config;
		uint32 add = BlockAdd;

		asserv(feedback)[nil] = SDMMC_ERROR_NONE;

		if ((add + NumberOfBlocks) > CardInfo.LogBlockNbr) {
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}
		if (((uint32)self[SDReg::IDMABSIZE] == 0U) || ((uint32)self[SDReg::IDMABASER] == 0U) || ((uint32)self[SDReg::IDMABASE1] == 0U)) {
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}

		self[SDReg::DCTRL] = 0;
		self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
			(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
			(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
			(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
			(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
			(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
			(_IMM1S(27U)) | (_IMM1S(28U));

		if (CardType != CardType_E::HIGH_CAPACITY)
			add *= BLOCKSIZE;

		config.DataTimeOut = SDMMC_DATATIMEOUT;
		config.DataLength = BLOCKSIZE * NumberOfBlocks;
		config.DataBlockSize = 9;// 2**9 == 512B
		config.TransferDir = SDMMC_DataTransferDir::toCard;
		config.TransferMode = SDMMC_DataTransferMode::Block;
		config.DPSM = false;
		(void)SDMMC_ConfigData(config);

		self[SDReg::CMD].setof(6);// __SDMMC_CMDTRANS_ENABLE
		self[SDReg::IDMACTRL] = _IMM1S(0) | _IMM1S(1);// IDMAEN | IDMABMODE (double buffer, start with buffer0)

		self[SDReg::MASK] |= (((_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(8U)) | (_IMM1S(28U))));

		Context = SDContext::WRITE_MULTIPLE_BLOCK_DMA;

		if (!SDMMC_CmdWriteMultiBlock(add, feedback)) {
			Context = SDContext::NONE;
			return false;
		}
		return true;
	}

#endif
}

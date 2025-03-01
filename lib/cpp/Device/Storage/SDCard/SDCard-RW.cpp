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

#include "../../../../../inc/cpp/Device/SD.hpp"
#include "../../../../../inc/cpp/Device/Storage/SD-PARA.h"
#include "../../../../../inc/cpp/Device/RCC/RCC"
#include "../../../../../inc/cpp/Device/GPIO"
#include "../../../../../inc/cpp/Device/SysTick"
#include "../../../../../inc/c/driver/interrupt/GIC.h"
#include "../../../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"

#define statin inline static
#define bitmatch(bits,mask) (((bits) & (mask)) == (mask))
#define whenis(x) case(x):// when is

// SDMMC_Internal_DMA_Mode  SDMMC Internal DMA Mode
#define SDMMC_DISABLE_IDMA              ((uint32)0x00000000)
#define SDMMC_ENABLE_IDMA_SINGLE_BUFF   (SDMMC_IDMA_IDMAEN)
#define SDMMC_ENABLE_IDMA_DOUBLE_BUFF0  (SDMMC_IDMA_IDMAEN | SDMMC_IDMA_IDMABMODE)
#define SDMMC_ENABLE_IDMA_DOUBLE_BUFF1  (SDMMC_IDMA_IDMAEN | SDMMC_IDMA_IDMABMODE | SDMMC_IDMA_IDMABACT)

#define BLOCKSIZE 512
#define SDMMC_FIFO_SIZE 32U

namespace uni {
#if defined(_MPU_STM32MP13)

	// Read data(word) from Rx FIFO in blocking mode(polling)
	// <=> SDMMC_GetFIFOCount
	statin uint32 SDMMC_ReadFIFO(const SecureDigitalCard_t& sd) {
		return sd[SDReg::FIFO_Start];
	}

	// Write data (word) to Tx FIFO in blocking mode (polling)
	statin void SDMMC_WriteFIFO(const SecureDigitalCard_t& sd, uint32* data) {
		sd[SDReg::FIFO_Start] = *data;
	}

	// Return the response received from the card for the last command
	inline static uint32 SDMMC_GetResponse(const SecureDigitalCard_t& sd, uint32 response) {
		if (--response >= 4) return 0;// 1 ~ 4
		return (&sd[SDReg::RESP1])[response];
	}

	bool SecureDigitalCard_t::HAL_SD_ReadBlocks(uint8_t* pData, uint32 BlockAdd, uint32 NumberOfBlocks, uint32 Timeout, uint32* feedback)
	{
		SDMMC_DataInitTypeDef config;
		uint32 errorstate;
		uint32 tickstart = SysTick::getTick();
		uint32 count;
		uint32 data;
		uint32 dataremaining;
		uint32 add = BlockAdd;
		uint8_t* tempbuff = pData;

		asserv(feedback)[nil] = SDMMC_ERROR_NONE;

		if ((add + NumberOfBlocks) > (CardInfo.LogBlockNbr))
		{
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}

		// Initialize data control register
		self[SDReg::DCTRL] = 0;

		if (CardType != CardType_E::SDHC_SDXC)
			add *= BLOCKSIZE;

		// Configure the SD DPSM (Data Path State Machine)
		config.DataTimeOut = SDMMC_DATATIMEOUT;
		config.DataLength = BLOCKSIZE * NumberOfBlocks;
		config.DataBlockSize = 9; // 2**9 == 512B
		config.TransferDir = SDMMC_DataTransferDir::toSDMMC;
		config.TransferMode = SDMMC_DataTransferMode::Block;
		config.DPSM = false;
		(void)SDMMC_ConfigData(config);
		self[SDReg::CMD].setof(6); // CMDTRANS __SDMMC_CMDTRANS_ENABLE(hsd->Instance);

		uint32 (SecureDigitalCard_t::*fn_read)(uint32, uint32*);
		fn_read = (NumberOfBlocks > 1U ?
			&SecureDigitalCard_t::SDMMC_CmdReadMultiBlock :
			&SecureDigitalCard_t::SDMMC_CmdReadSingleBlock);
		Context = (NumberOfBlocks > 1U) ?
			SDContext::READ_MULTIPLE_BLOCK :
			SDContext::READ_SINGLE_BLOCK;
		if (!(self.*fn_read)(add, feedback))
		{
			// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
			self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
				(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
				(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
				(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
				(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
				(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
				(_IMM1S(27U)) | (_IMM1S(28U));
			Context = SDContext::NONE;
			return false;
		}

		// Poll on SDMMC flags
		dataremaining = config.DataLength;
		uint32 mask = (_IMM1S(5U)) | (_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(8U));// (!__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_RXOVERR | SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DATAEND))
		while (!(self[SDReg::STA] & mask))
		{
			if (self[SDReg::STA].bitof(15)// SDMMC_FLAG_RXFIFOHF
				&& (dataremaining >= SDMMC_FIFO_SIZE))
			{
				/* Read data from SDMMC Rx FIFO */
				for (count = 0U; count < (SDMMC_FIFO_SIZE / 4U); count++)
				{
					data = SDMMC_ReadFIFO(self);
					*tempbuff = (uint8_t)(data & 0xFFU);
					tempbuff++;
					*tempbuff = (uint8_t)((data >> 8U) & 0xFFU);
					tempbuff++;
					*tempbuff = (uint8_t)((data >> 16U) & 0xFFU);
					tempbuff++;
					*tempbuff = (uint8_t)((data >> 24U) & 0xFFU);
					tempbuff++;
				}
				dataremaining -= SDMMC_FIFO_SIZE;
			}
			if (((SysTick::getTick() - tickstart) >= Timeout) || (Timeout == 0U))
			{
				// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
				self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
					(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
					(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
					(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
					(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
					(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
					(_IMM1S(27U)) | (_IMM1S(28U));
				asserv(feedback)[nil] = SDMMC_ERROR_TIMEOUT;
				Context = SDContext::NONE;
				return false;
			}
		}
		self[SDReg::CMD].rstof(6);// __SDMMC_CMDTRANS_DISABLE(hsd->Instance);

		// Send stop transmission command in case of multiblock read
		if (self[SDReg::STA].bitof(8) && // SDMMC_FLAG_DATAEND
			(NumberOfBlocks > 1U))
		{
			if (CardType != CardType_E ::SECURED)
			{
				// Send stop transmission command
				if (!SDMMC_CmdStopTransfer(feedback))
				{
					// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
					self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
						(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
						(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
						(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
						(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
						(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
						(_IMM1S(27U)) | (_IMM1S(28U));
					Context = SDContext::NONE;
					return false;
				}
			}
		}
		if (self[SDReg::STA].bitof(3))// SDMMC_FLAG_DTIMEOUT
		{
			// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
			self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
				(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
				(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
				(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
				(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
				(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
				(_IMM1S(27U)) | (_IMM1S(28U));
			
			asserv(feedback)[nil] = SDMMC_ERROR_TIMEOUT;

			Context = SDContext::NONE;
			return false;
		}
		else if (self[SDReg::STA].bitof(1)) { // DCRCFAIL
			// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
			self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
				(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
				(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
				(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
				(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
				(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
				(_IMM1S(27U)) | (_IMM1S(28U));
			asserv(feedback)[nil] = SDMMC_ERROR_DATA_CRC_FAIL;
			Context = SDContext::NONE;
			return false;
		}
		else if (self[SDReg::STA].bitof(5)) { // RXOVERR
			// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
			self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
				(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
				(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
				(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
				(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
				(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
				(_IMM1S(27U)) | (_IMM1S(28U));
			asserv(feedback)[nil] = SDMMC_ERROR_RX_OVERRUN;
			Context = SDContext::NONE;
			return false;
		}

		// Clear all the static flags
		// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_DATA_FLAGS)
		self[SDReg::ICR] = (_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) | (_IMM1S(8U)) | (_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) | (_IMM1S(27U)) | (_IMM1S(28U));
		return true;
	}

	// Allows to write block(s) to a specified address in a card. The Data transfer is managed by polling mode.
	// This API should be followed by a check on the card state through HAL_SD_GetCardState().
	bool SecureDigitalCard_t::HAL_SD_WriteBlocks(const uint8_t* pData, uint32 BlockAdd, uint32 NumberOfBlocks, uint32 Timeout, uint32* feedback)
	{
		SDMMC_DataInitTypeDef config;
		uint32 errorstate;
		uint32 tickstart = SysTick::getTick();
		uint32 count;
		uint32 data;
		uint32 dataremaining;
		uint32 add = BlockAdd;
		const uint8_t* tempbuff = pData;

		errorstate = SDMMC_ERROR_NONE;

		if ((add + NumberOfBlocks) > (CardInfo.LogBlockNbr))
		{
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}

		// Initialize data control register
		self[SDReg::DCTRL] = 0;

		if (CardType != CardType_E::SDHC_SDXC)
			add *= BLOCKSIZE;

		// Configure the SD DPSM (Data Path State Machine)
		config.DataTimeOut = SDMMC_DATATIMEOUT;
		config.DataLength = BLOCKSIZE * NumberOfBlocks;
		config.DataBlockSize = 9; // 2**9 == 512B
		config.TransferDir = SDMMC_DataTransferDir::toCard;
		config.TransferMode = SDMMC_DataTransferMode::Block;
		config.DPSM = false;
		SDMMC_ConfigData(config);
		self[SDReg::CMD].setof(6); // CMDTRANS __SDMMC_CMDTRANS_ENABLE(hsd->Instance);

		uint32(SecureDigitalCard_t:: * fn_write)(uint32, uint32*);
		fn_write = (NumberOfBlocks > 1U ?
			&SecureDigitalCard_t::SDMMC_CmdWriteMultiBlock :
			&SecureDigitalCard_t::SDMMC_CmdWriteSingleBlock);
		Context = (NumberOfBlocks > 1U) ?
			SDContext::WRITE_MULTIPLE_BLOCK :
			SDContext::WRITE_SINGLE_BLOCK;
		if (!(self.*fn_write)(add, feedback))
		{
			// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
			self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
				(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
				(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
				(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
				(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
				(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
				(_IMM1S(27U)) | (_IMM1S(28U));
			Context = SDContext::NONE;
			return false;
		}

		// Write block(s) in polling mode
		dataremaining = config.DataLength;
		uint32 mask = (_IMM1S(4U)) | (_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(8U));// SDMMC_FLAG_TXUNDERR | SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DATAEND
		while (!(self[SDReg::STA] & mask))
		{
			if (self[SDReg::STA].bitof(14) // SDMMC_FLAG_TXFIFOHE
				&& (dataremaining >= SDMMC_FIFO_SIZE))
			{
				/* Write data to SDMMC Tx FIFO */
				for (count = 0U; count < (SDMMC_FIFO_SIZE / 4U); count++)
				{
					data = (uint32)(*tempbuff);
					tempbuff++;
					data |= ((uint32)(*tempbuff) << 8U);
					tempbuff++;
					data |= ((uint32)(*tempbuff) << 16U);
					tempbuff++;
					data |= ((uint32)(*tempbuff) << 24U);
					tempbuff++;
					SDMMC_WriteFIFO(self, &data);
				}
				dataremaining -= SDMMC_FIFO_SIZE;
			}

			if (((SysTick::getTick() - tickstart) >= Timeout) || (Timeout == 0U))
			{
				/* Clear all the static flags */
				// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
				self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
					(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
					(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
					(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
					(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
					(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
					(_IMM1S(27U)) | (_IMM1S(28U));
				
				Context = SDContext::NONE;
				return false;
			}
		}
		self[SDReg::CMD].rstof(6);// __SDMMC_CMDTRANS_DISABLE(hsd->Instance);

		/* Send stop transmission command in case of multiblock write */
		if (self[SDReg::STA].bitof(8) // SDMMC_FLAG_DATAEND
			&& (NumberOfBlocks > 1U))
		{
			if (CardType != CardType_E::SECURED)
			{
				/* Send stop transmission command */
				if (!SDMMC_CmdStopTransfer(feedback))
				{
					// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
					self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
						(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
						(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
						(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
						(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
						(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
						(_IMM1S(27U)) | (_IMM1S(28U));
					Context = SDContext::NONE;
					return false;
				}
			}
		}

		/* Get error state */
		if (self[SDReg::STA].bitof(3))// SDMMC_FLAG_DTIMEOUT
		{
			// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
			self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
				(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
				(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
				(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
				(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
				(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
				(_IMM1S(27U)) | (_IMM1S(28U));
			errorstate |= SDMMC_ERROR_TIMEOUT;
			Context = SDContext::NONE;
			return false;
		}
		else if (self[SDReg::STA].bitof(1)) { // DCRCFAIL
			// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
			self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
				(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
				(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
				(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
				(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
				(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
				(_IMM1S(27U)) | (_IMM1S(28U));
			errorstate |= SDMMC_ERROR_DATA_CRC_FAIL;
			Context = SDContext::NONE;
			return false;
		}
		else if (self[SDReg::STA].bitof(4))// TXUNDERR
		{
		  /* Clear all the static flags */// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
			self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
				(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
				(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
				(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
				(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
				(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
				(_IMM1S(27U)) | (_IMM1S(28U));
			errorstate |= SDMMC_ERROR_TX_UNDERRUN;
			Context = SDContext::NONE;
			return false;
		}

		// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_DATA_FLAGS)
		self[SDReg::ICR] = (_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) | (_IMM1S(8U)) | (_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) | (_IMM1S(27U)) | (_IMM1S(28U));
		return true;
	}


	bool SecureDigitalCard_t::HAL_SD_ReadBlocks_IT(uint8_t* pData, uint32 BlockAdd, uint32 NumberOfBlocks, uint32* feedback)
	{
		SDMMC_DataInitTypeDef config;
		uint32 errorstate = SDMMC_ERROR_NONE;
		uint32 add = BlockAdd;

		if ((add + NumberOfBlocks) > (CardInfo.LogBlockNbr))
		{
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}
		// Initialize data control register
		self[SDReg::DCTRL] = 0;
		RxBuff.address = _IMM(pData);
		RxBuff.length = BLOCKSIZE * NumberOfBlocks;

		if (CardType != CardType_E::SDHC_SDXC)
			add *= BLOCKSIZE;

		// Configure the SD DPSM (Data Path State Machine)
		config.DataTimeOut = SDMMC_DATATIMEOUT;
		config.DataLength = BLOCKSIZE * NumberOfBlocks;
		config.DataBlockSize = 9; // 2**9 == 512B
		config.TransferDir = SDMMC_DataTransferDir::toSDMMC;
		config.TransferMode = SDMMC_DataTransferMode::Block;
		config.DPSM = false;
		(void)SDMMC_ConfigData(config);
		self[SDReg::CMD].setof(6); // CMDTRANS __SDMMC_CMDTRANS_ENABLE(hsd->Instance);

		/* Read Blocks in IT mode */
		uint32(SecureDigitalCard_t:: * fn_read)(uint32, uint32*);
		fn_read = (NumberOfBlocks > 1U ?
			&SecureDigitalCard_t::SDMMC_CmdReadMultiBlock :
			&SecureDigitalCard_t::SDMMC_CmdReadSingleBlock);
		Context = (NumberOfBlocks > 1U) ?
			SDContext::READ_MULTIPLE_BLOCK_IT :
			SDContext::READ_SINGLE_BLOCK_IT;
		if (!(self.*fn_read)(add, feedback))
		{
			// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
			self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
				(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
				(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
				(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
				(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
				(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
				(_IMM1S(27U)) | (_IMM1S(28U));
			Context = SDContext::NONE;
			return false;
		}
		// __HAL_SD_ENABLE_IT(hsd, (SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT | SDMMC_IT_RXOVERR | SDMMC_IT_DATAEND | SDMMC_FLAG_RXFIFOHF));
		self[SDReg::MASK] |= ((((_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(5U)) | (_IMM1S(8U)) | (_IMM1S(15U)))));
		return true;
	}

	bool SecureDigitalCard_t::HAL_SD_WriteBlocks_IT(const uint8_t* pData, uint32 BlockAdd, uint32 NumberOfBlocks, uint32* feedback)
	{
		SDMMC_DataInitTypeDef config;
		uint32 errorstate = SDMMC_ERROR_NONE;
		uint32 add = BlockAdd;
		if ((add + NumberOfBlocks) > (CardInfo.LogBlockNbr))
		{
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}

		// Initialize data control register
		self[SDReg::DCTRL] = 0;

		TxBuff.address = _IMM(pData);
		TxBuff.length = BLOCKSIZE * NumberOfBlocks;

		if (CardType != CardType_E::SDHC_SDXC)
			add *= BLOCKSIZE;

		// Configure the SD DPSM (Data Path State Machine)
		config.DataTimeOut = SDMMC_DATATIMEOUT;
		config.DataLength = BLOCKSIZE * NumberOfBlocks;
		config.DataBlockSize = 9; // 2**9 == 512B
		config.TransferDir = SDMMC_DataTransferDir::toCard;
		config.TransferMode = SDMMC_DataTransferMode::Block;
		config.DPSM = false;
		SDMMC_ConfigData(config);
		self[SDReg::CMD].setof(6); // CMDTRANS __SDMMC_CMDTRANS_ENABLE(hsd->Instance);

		uint32(SecureDigitalCard_t:: * fn_write)(uint32, uint32*);
		fn_write = (NumberOfBlocks > 1U ?
			&SecureDigitalCard_t::SDMMC_CmdWriteMultiBlock :
			&SecureDigitalCard_t::SDMMC_CmdWriteSingleBlock);
		Context = (NumberOfBlocks > 1U) ?
			SDContext::WRITE_MULTIPLE_BLOCK_IT :
			SDContext::WRITE_SINGLE_BLOCK_IT;
		if (!(self.*fn_write)(add, feedback))
		{
			// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
			self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
				(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
				(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
				(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
				(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
				(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
				(_IMM1S(27U)) | (_IMM1S(28U));
			Context = SDContext::NONE;
			return false;
		}

		//__HAL_SD_ENABLE_IT(hsd, (SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT | SDMMC_IT_TXUNDERR | SDMMC_IT_DATAEND | SDMMC_FLAG_TXFIFOHE));
		self[SDReg::MASK] |= ((((_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(8U)) | (_IMM1S(14U)))));
		return true;
	}


	bool SecureDigitalCard_t::HAL_SD_ReadBlocks_DMA(uint8_t* pData, uint32 BlockAdd, uint32 NumberOfBlocks, uint32* feedback)
	{
		SDMMC_DataInitTypeDef config;
		uint32 errorstate = SDMMC_ERROR_NONE;
		uint32 add = BlockAdd;

		if ((add + NumberOfBlocks) > (CardInfo.LogBlockNbr))
		{
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}

		// Initialize data control register
		self[SDReg::DCTRL] = 0;
		RxBuff.address = _IMM(pData);
		RxBuff.length = BLOCKSIZE * NumberOfBlocks;

		if (CardType != CardType_E::SDHC_SDXC)
			add *= BLOCKSIZE;

		// Configure the SD DPSM (Data Path State Machine)
		config.DataTimeOut = SDMMC_DATATIMEOUT;
		config.DataLength = BLOCKSIZE * NumberOfBlocks;
		config.DataBlockSize = 9; // 2**9 == 512B
		config.TransferDir = SDMMC_DataTransferDir::toSDMMC;
		config.TransferMode = SDMMC_DataTransferMode::Block;
		config.DPSM = false;
		(void)SDMMC_ConfigData(config);
		self[SDReg::CMD].setof(6); // CMDTRANS __SDMMC_CMDTRANS_ENABLE(hsd->Instance);

		self[SDReg::IDMABASER] = (uint32)pData;
		self[SDReg::IDMACTRL] = 1; //SDMMC_ENABLE_IDMA_SINGLE_BUFF;

		/* Read Blocks in DMA mode */
		uint32(SecureDigitalCard_t:: * fn_read)(uint32, uint32*);
		fn_read = (NumberOfBlocks > 1U ?
			&SecureDigitalCard_t::SDMMC_CmdReadMultiBlock :
			&SecureDigitalCard_t::SDMMC_CmdReadSingleBlock);
		Context = (NumberOfBlocks > 1U) ?
			SDContext::READ_MULTIPLE_BLOCK_DMA :
			SDContext::READ_SINGLE_BLOCK_DMA;
		if (!(self.*fn_read)(add, feedback))
		{
			// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
			self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
				(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
				(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
				(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
				(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
				(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
				(_IMM1S(27U)) | (_IMM1S(28U));
			Context = SDContext::NONE;
			return false;
		}
		// __HAL_SD_ENABLE_IT(hsd, (SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT | SDMMC_IT_RXOVERR | SDMMC_IT_DATAEND));
		self[SDReg::MASK] |= ((((_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(5U)) | (_IMM1S(8U)))));
		return true;
	}


	bool SecureDigitalCard_t::HAL_SD_WriteBlocks_DMA(const uint8_t* pData, uint32 BlockAdd, uint32 NumberOfBlocks, uint32* feedback)
	{
		SDMMC_DataInitTypeDef config;
		uint32 errorstate;
		uint32 add = BlockAdd;

		errorstate = SDMMC_ERROR_NONE;

		if ((add + NumberOfBlocks) > (CardInfo.LogBlockNbr))
		{
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}

		// Initialize data control register
		self[SDReg::DCTRL] = 0;

		TxBuff.address = _IMM(pData);
		TxBuff.length = BLOCKSIZE * NumberOfBlocks;

		if (CardType != CardType_E::SDHC_SDXC)
			add *= BLOCKSIZE;

		/* Configure the SD DPSM (Data Path State Machine) */
		config.DataTimeOut = SDMMC_DATATIMEOUT;
		config.DataLength = BLOCKSIZE * NumberOfBlocks;
		config.DataBlockSize = 9; // 2**9 == 512B
		config.TransferDir = SDMMC_DataTransferDir::toCard;
		config.TransferMode = SDMMC_DataTransferMode::Block;
		config.DPSM = false;
		SDMMC_ConfigData(config);
		self[SDReg::CMD].setof(6); // CMDTRANS __SDMMC_CMDTRANS_ENABLE(hsd->Instance);


		self[SDReg::IDMABASER] = (uint32)pData;
		self[SDReg::IDMACTRL] = 1; // SDMMC_ENABLE_IDMA_SINGLE_BUFF;

		/* Write Blocks in Polling mode */
		uint32(SecureDigitalCard_t:: * fn_write)(uint32, uint32*);
		fn_write = (NumberOfBlocks > 1U ?
			&SecureDigitalCard_t::SDMMC_CmdWriteMultiBlock :
			&SecureDigitalCard_t::SDMMC_CmdWriteSingleBlock);
		Context = (NumberOfBlocks > 1U) ?
			SDContext::WRITE_MULTIPLE_BLOCK_DMA :
			SDContext::WRITE_SINGLE_BLOCK_DMA;
		if (!(self.*fn_write)(add, feedback))
		{
			// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
			self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
				(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
				(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
				(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
				(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
				(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
				(_IMM1S(27U)) | (_IMM1S(28U));
			Context = SDContext::NONE;
			return false;
		}
		/* Enable transfer interrupts */
		// __HAL_SD_ENABLE_IT(hsd, (SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT | SDMMC_IT_TXUNDERR | SDMMC_IT_DATAEND));
		self[SDReg::MASK] |= ((((_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(8U)))));
		return true;
	}


	bool SecureDigitalCard_t::HAL_SD_Erase(uint32 BlockStartAdd, uint32 BlockEndAdd, uint32* feedback)
	{
		uint32 errorstate = SDMMC_ERROR_NONE;
		uint32 start_add = BlockStartAdd;
		uint32 end_add = BlockEndAdd;
		if (end_add < start_add) {
			errorstate |= SDMMC_ERROR_INVALID_PARAMETER;
			return false;
		}
		if (end_add > (CardInfo.LogBlockNbr))
		{
			errorstate |= SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}

		/* Check if the card command class supports erase command */
		if (((CardInfo.Class) &
			0x00000020U) // SDMMC_CCCC_ERASE
			== 0U)
		{
			// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
			self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
				(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
				(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
				(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
				(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
				(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
				(_IMM1S(27U)) | (_IMM1S(28U));
			errorstate |= SDMMC_ERROR_REQUEST_NOT_APPLICABLE;
			
			return false;
		}
		if (bitmatch(SDMMC_GetResponse(self, 1), SDMMC_CARD_LOCKED))
		{
			// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
			self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
				(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
				(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
				(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
				(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
				(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
				(_IMM1S(27U)) | (_IMM1S(28U));
			errorstate |= SDMMC_ERROR_LOCK_UNLOCK_FAILED;
			return false;
		}
		/* Get start and end block for high capacity cards */
		if (CardType != CardType_E ::SDHC_SDXC)
		{
			start_add *= BLOCKSIZE;
			end_add *= BLOCKSIZE;
		}
		/* According to sd-card spec 1.0 ERASE_GROUP_START (CMD32) and erase_group_end(CMD33) */
		if (CardType != CardType_E::SECURED)
		{
			/* Send CMD32 SD_ERASE_GRP_START with argument as addr  */
			if (!SDMMC_CmdSDEraseStartAdd(start_add, &errorstate))
			{
				// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
				self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
					(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
					(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
					(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
					(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
					(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
					(_IMM1S(27U)) | (_IMM1S(28U));
				return false;
			}
			/* Send CMD33 SD_ERASE_GRP_END with argument as addr  */
			if (!SDMMC_CmdSDEraseEndAdd(end_add, feedback))
			{
				// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
				self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
					(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
					(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
					(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
					(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
					(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
					(_IMM1S(27U)) | (_IMM1S(28U));
				return false;
			}
		}
		/* Send CMD38 ERASE */
		if (!SDMMC_CmdErase(0UL, feedback))
		{
			// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
			self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
				(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
				(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
				(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
				(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
				(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
				(_IMM1S(27U)) | (_IMM1S(28U));
			return false;
		}
		return true;
	}


	void SecureDigitalCard_t::SD_Read_IT()
	{
		uint32 count;
		uint32 data;
		uint8_t* tmp;
		tmp = (uint8_t*)RxBuff.address;
		if (RxBuff.length >= SDMMC_FIFO_SIZE)
		{
		  /* Read data from SDMMC Rx FIFO */
			for (count = 0U; count < (SDMMC_FIFO_SIZE / 4U); count++)
			{
				data = SDMMC_ReadFIFO(self);
				*tmp = (uint8_t)(data & 0xFFU);
				tmp++;
				*tmp = (uint8_t)((data >> 8U) & 0xFFU);
				tmp++;
				*tmp = (uint8_t)((data >> 16U) & 0xFFU);
				tmp++;
				*tmp = (uint8_t)((data >> 24U) & 0xFFU);
				tmp++;
			}
			RxBuff.address = _IMM(tmp);
			RxBuff.length -= SDMMC_FIFO_SIZE;
		}
	}


	void SecureDigitalCard_t::SD_Write_IT()
	{
		uint32 count;
		uint32 data;
		const uint8_t* tmp;
		tmp = (uint8_t*) TxBuff.address;
		if (TxBuff.length >= SDMMC_FIFO_SIZE)
		{
		  /* Write data to SDMMC Tx FIFO */
			for (count = 0U; count < (SDMMC_FIFO_SIZE / 4U); count++)
			{
				data = (uint32)(*tmp);
				tmp++;
				data |= ((uint32)(*tmp) << 8U);
				tmp++;
				data |= ((uint32)(*tmp) << 16U);
				tmp++;
				data |= ((uint32)(*tmp) << 24U);
				tmp++;
				(void)SDMMC_WriteFIFO(self, &data);
			}
			TxBuff.address = _IMM(tmp);
			TxBuff.length -= SDMMC_FIFO_SIZE;
		}
	}

#endif
}

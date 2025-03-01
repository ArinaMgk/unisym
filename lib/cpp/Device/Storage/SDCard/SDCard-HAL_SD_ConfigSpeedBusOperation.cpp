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

#define BLOCKSIZE 512

namespace uni {
#if defined(_MPU_STM32MP13)

	// Read data(word) from Rx FIFO in blocking mode(polling)
	// <=> SDMMC_GetFIFOCount
	statin uint32 SDMMC_ReadFIFO(const SecureDigitalCard_t& sd) {
		return sd[SDReg::FIFO_Start];
	}
	
	bool SecureDigitalCard_t::SD_SwitchSpeed(uint32 SwitchSpeedMode, uint32* feedback) {
		uint32 temp_error;
		if (!feedback) feedback = &temp_error;
		(feedback)[nil] = SDMMC_ERROR_NONE;
		uint32 SD_hs[16] = { 0 };
		uint32 loops = 0;
		uint32 Timeout = SysTick::getTick();
		if (CardInfo.CardSpeed == CARD_NORMAL_SPEED)// Standard Speed Card <= 12.5Mhz
		{
			(feedback)[nil] = SDMMC_ERROR_REQUEST_NOT_APPLICABLE;
			return false;
		}

		if (CardInfo.CardSpeed < CARD_HIGH_SPEED)
			return *feedback == SDMMC_ERROR_NONE;
		// Initialize the Data control register
		self[SDReg::DCTRL] = 0;
		asrtret(SDMMC_CmdBlockLength(64U, feedback));
		// Configure the SD DPSM (Data Path State Machine)
		SDMMC_DataInitTypeDef sdmmc_datainitstructure{
			.DataTimeOut = SDMMC_DATATIMEOUT,
			.DataLength = 64U,
			.DataBlockSize = 6,// 64 == 2**6
			.TransferDir = SDMMC_DataTransferDir::toSDMMC,
			.TransferMode = SDMMC_DataTransferMode::Block,
			.DPSM = true
		};
		SDMMC_ConfigData(sdmmc_datainitstructure);
		asrtret(SDMMC_CmdSwitch(SwitchSpeedMode, feedback));
		//
		stduint mask = (_IMM1S(5U)) | (_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(10U)) | (_IMM1S(8U));
		while (!(self[SDReg::STA] & mask))// !__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_RXOVERR | SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DBCKEND | SDMMC_FLAG_DATAEND)
		{
			if (self[SDReg::STA].bitof(15)) // RXFIFOHF
			{
				for0(count, 8)
					SD_hs[(8 * loops) + count] = SDMMC_ReadFIFO(self);
				loops++;
			}
			if (SysTick::getTick() - Timeout > SDMMC_DATATIMEOUT) {
				asserv(feedback)[nil] = SDMMC_ERROR_TIMEOUT;
				return false;
			}
		}
		if (self[SDReg::STA].bitof(3)) { // DTIMEOUT
			asserv(feedback)[nil] = SDMMC_ERROR_DATA_TIMEOUT;
			self[SDReg::ICR] = _IMM1S(3);
			return false;
		}
		else if (self[SDReg::STA].bitof(1)) { // DCRCFAIL
			asserv(feedback)[nil] = SDMMC_ERROR_DATA_CRC_FAIL;
			self[SDReg::ICR] = _IMM1S(1);
			return false;
		}
		else if (self[SDReg::STA].bitof(5)) { // RXOVERR
			asserv(feedback)[nil] = SDMMC_ERROR_RX_OVERRUN;
			self[SDReg::ICR] = _IMM1S(5);
			return false;
		}
		// Clear all the static flags
		// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_DATA_FLAGS)
		self[SDReg::ICR] = (_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) | (_IMM1S(8U)) | (_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) | (_IMM1S(27U)) | (_IMM1S(28U));
		// Test if the switch mode HS is ok
		if ((((uint8_t*)SD_hs)[13] & 2U) != 2U)
		{
			asserv(feedback)[nil] = SDMMC_ERROR_UNSUPPORTED_FEATURE;
		}
		return *feedback == SDMMC_ERROR_NONE;
	}

	bool SecureDigitalCard_t::HAL_SD_ConfigSpeedBusOperation(SDMMC_SPEED_MODE SpeedMode) {
		bool status{ true };
		uint32 feed_error = SDMMC_ERROR_NONE;

	#if (USE_SD_TRANSCEIVER != 0U)//{TODO}
		if (hsd->Init.TranceiverPresent == SDMMC_TRANSCEIVER_PRESENT)
		{
			switch (SpeedMode)
			{
			case SDMMC_SPEED_MODE_AUTO:
			{
				if ((hsd->SdCard.CardSpeed == CARD_ULTRA_HIGH_SPEED) ||
					(hsd->SdCard.CardType == CARD_SDHC_SDXC))
				{
					self[SDReg::CLKCR |= SDMMC_CLKCR_BUSSPEED;
					/* Enable Ultra High Speed */
					if (SD_UltraHighSpeed(hsd, SDMMC_SDR104_SWITCH_PATTERN) != SDMMC_ERROR_NONE)
					{
						if (SD_SwitchSpeed(hsd, SDMMC_SDR25_SWITCH_PATTERN) != SDMMC_ERROR_NONE)
						{
							hsd->ErrorCode |= HAL_SD_ERROR_UNSUPPORTED_FEATURE;
							status = HAL_ERROR;
						}
					}
				}
				else if (hsd->SdCard.CardSpeed == CARD_HIGH_SPEED)
				{
				  /* Enable High Speed */
					if (SD_SwitchSpeed(hsd, SDMMC_SDR25_SWITCH_PATTERN) != SDMMC_ERROR_NONE)
					{
						hsd->ErrorCode |= HAL_SD_ERROR_UNSUPPORTED_FEATURE;
						status = HAL_ERROR;
					}
				}
				else _Comment("Nothing to do, Use defaultSpeed");
				break;
			}
			case SDMMC_SPEED_MODE_ULTRA_SDR104:
			{
				if ((hsd->SdCard.CardSpeed == CARD_ULTRA_HIGH_SPEED) ||
					(hsd->SdCard.CardType == CARD_SDHC_SDXC))
				{
				  /* Enable UltraHigh Speed */
					if (SD_UltraHighSpeed(hsd, SDMMC_SDR104_SWITCH_PATTERN) != SDMMC_ERROR_NONE)
					{
						hsd->ErrorCode |= HAL_SD_ERROR_UNSUPPORTED_FEATURE;
						status = HAL_ERROR;
					}
					self[SDReg::CLKCR |= SDMMC_CLKCR_BUSSPEED;
				}
				else
				{
					hsd->ErrorCode |= HAL_SD_ERROR_UNSUPPORTED_FEATURE;
					status = HAL_ERROR;
				}
				break;
			}
			case SDMMC_SPEED_MODE_ULTRA_SDR50:
			{
				if ((hsd->SdCard.CardSpeed == CARD_ULTRA_HIGH_SPEED) ||
					(hsd->SdCard.CardType == CARD_SDHC_SDXC))
				{
				  /* Enable UltraHigh Speed */
					if (SD_UltraHighSpeed(hsd, SDMMC_SDR50_SWITCH_PATTERN) != SDMMC_ERROR_NONE)
					{
						hsd->ErrorCode |= HAL_SD_ERROR_UNSUPPORTED_FEATURE;
						status = HAL_ERROR;
					}
					self[SDReg::CLKCR |= SDMMC_CLKCR_BUSSPEED;
				}
				else
				{
					hsd->ErrorCode |= HAL_SD_ERROR_UNSUPPORTED_FEATURE;
					status = HAL_ERROR;
				}
				break;
			}
			case SDMMC_SPEED_MODE_DDR:
			{
				if ((hsd->SdCard.CardSpeed == CARD_ULTRA_HIGH_SPEED) ||
					(hsd->SdCard.CardType == CARD_SDHC_SDXC))
				{
				  /* Enable DDR Mode*/
					if (SD_DDR_Mode(hsd) != SDMMC_ERROR_NONE)
					{
						hsd->ErrorCode |= HAL_SD_ERROR_UNSUPPORTED_FEATURE;
						status = HAL_ERROR;
					}
					self[SDReg::CLKCR |= SDMMC_CLKCR_BUSSPEED | SDMMC_CLKCR_DDR;
				}
				else
				{
					hsd->ErrorCode |= HAL_SD_ERROR_UNSUPPORTED_FEATURE;
					status = HAL_ERROR;
				}
				break;
			}
			case SDMMC_SPEED_MODE_HIGH:
			{
				if ((hsd->SdCard.CardSpeed == CARD_ULTRA_HIGH_SPEED) ||
					(hsd->SdCard.CardSpeed == CARD_HIGH_SPEED) ||
					(hsd->SdCard.CardType == CARD_SDHC_SDXC))
				{
				  /* Enable High Speed */
					if (SD_SwitchSpeed(hsd, SDMMC_SDR25_SWITCH_PATTERN) != SDMMC_ERROR_NONE)
					{
						hsd->ErrorCode |= HAL_SD_ERROR_UNSUPPORTED_FEATURE;
						status = HAL_ERROR;
					}
				}
				else
				{
					hsd->ErrorCode |= HAL_SD_ERROR_UNSUPPORTED_FEATURE;
					status = HAL_ERROR;
				}
				break;
			}
			case SDMMC_SPEED_MODE_DEFAULT:
			{
			  /* Switch to default Speed */
				if (SD_SwitchSpeed(hsd, SDMMC_SDR12_SWITCH_PATTERN) != SDMMC_ERROR_NONE)
				{
					hsd->ErrorCode |= HAL_SD_ERROR_UNSUPPORTED_FEATURE;
					status = HAL_ERROR;
				}

				break;
			}
			default:
				hsd->ErrorCode |= HAL_SD_ERROR_PARAM;
				status = HAL_ERROR;
				break;
			}
		}
		else
		{
			switch (SpeedMode)
			{
			case SDMMC_SPEED_MODE_AUTO:
			{
				if ((hsd->SdCard.CardSpeed == CARD_ULTRA_HIGH_SPEED) ||
					(hsd->SdCard.CardSpeed == CARD_HIGH_SPEED) ||
					(hsd->SdCard.CardType == CARD_SDHC_SDXC))
				{
				  /* Enable High Speed */
					if (SD_SwitchSpeed(hsd, SDMMC_SDR25_SWITCH_PATTERN) != SDMMC_ERROR_NONE)
					{
						hsd->ErrorCode |= HAL_SD_ERROR_UNSUPPORTED_FEATURE;
						status = HAL_ERROR;
					}
				}
				else
				{
				  /*Nothing to do, Use defaultSpeed */
				}
				break;
			}
			case SDMMC_SPEED_MODE_HIGH:
			{
				if ((hsd->SdCard.CardSpeed == CARD_ULTRA_HIGH_SPEED) ||
					(hsd->SdCard.CardSpeed == CARD_HIGH_SPEED) ||
					(hsd->SdCard.CardType == CARD_SDHC_SDXC))
				{
				  /* Enable High Speed */
					if (SD_SwitchSpeed(hsd, SDMMC_SDR25_SWITCH_PATTERN) != SDMMC_ERROR_NONE)
					{
						hsd->ErrorCode |= HAL_SD_ERROR_UNSUPPORTED_FEATURE;
						status = HAL_ERROR;
					}
				}
				else
				{
					hsd->ErrorCode |= HAL_SD_ERROR_UNSUPPORTED_FEATURE;
					status = HAL_ERROR;
				}
				break;
			}
			case SDMMC_SPEED_MODE_DEFAULT:
			{
			  /* Switch to default Speed */
				if (SD_SwitchSpeed(hsd, SDMMC_SDR12_SWITCH_PATTERN) != SDMMC_ERROR_NONE)
				{
					hsd->ErrorCode |= HAL_SD_ERROR_UNSUPPORTED_FEATURE;
					status = HAL_ERROR;
				}

				break;
			}
			case SDMMC_SPEED_MODE_ULTRA: /*not valid without transceiver*/
			default:
				hsd->ErrorCode |= HAL_SD_ERROR_PARAM;
				status = HAL_ERROR;
				break;
			}
		}
	#else
		switch (SpeedMode) {
			whenis(SDMMC_SPEED_MODE::AUTO)
			{
				if ((CardInfo.CardSpeed == CARD_ULTRA_HIGH_SPEED) ||
					(CardInfo.CardSpeed == CARD_HIGH_SPEED) ||
					(CardType == CardType_E::SDHC_SDXC))
				{
					// Enable High Speed
					if (!SD_SwitchSpeed(SDMMC_SDR25_SWITCH_PATTERN, &feed_error))
						status = false;
				}
				else _Comment("Nothing to do, Use defaultSpeed");
				break;
			}
			whenis(SDMMC_SPEED_MODE::HIGH)
			{
				if ((CardInfo.CardSpeed == CARD_ULTRA_HIGH_SPEED) ||
					(CardInfo.CardSpeed == CARD_HIGH_SPEED) ||
					(CardType == CardType_E::SDHC_SDXC))
				{
					// Enable High Speed
					if (!SD_SwitchSpeed(SDMMC_SDR25_SWITCH_PATTERN, &feed_error)) {
						// SDMMC_ERROR_UNSUPPORTED_FEATURE
						status = false;
					}
				}
				else {
					// SDMMC_ERROR_UNSUPPORTED_FEATURE
					status = false;
				}
				break;
			}
			whenis(SDMMC_SPEED_MODE::DEFAULT)// Switch to default Speed
			{
				if (!SD_SwitchSpeed(SDMMC_SDR12_SWITCH_PATTERN, &feed_error)) {
					// SDMMC_ERROR_UNSUPPORTED_FEATURE
					status = false;
				}
				break;
			}
		case SDMMC_SPEED_MODE::ULTRA: // not valid without transceiver
		default:
			// SDMMC_ERROR_INVALID_PARAMETER
			status = false;
			break;
		}
	#endif // USE_SD_TRANSCEIVER
		// Verify that SD card is ready to use after Speed mode switch*/
		stduint tickstart = SysTick::getTick();
		while ((HAL_SD_GetCardState() != HAL_SD_CardStateTypeDef::TRANSFER))
		{
			if ((SysTick::getTick() - tickstart) >= SDMMC_DATATIMEOUT)
			{
				feed_error = SDMMC_ERROR_TIMEOUT;
				return false;
			}
		}
		// Set Block Size for Card
		if (!SDMMC_CmdBlockLength(BLOCKSIZE, &feed_error))
		{
			// Clear all the static flags */
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
		return status;
	}

#endif
}

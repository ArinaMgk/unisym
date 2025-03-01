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

#if (USE_SD_TRANSCEIVER != 0U)
	
	//{TODO}
	// Switches the SD card to Ultra High Speed mode. This API must be used after "Transfer State"
	// This operation should be followed by the configuration of PLL to have SDMMCCK clock between 50 and 120 MHz
	static uint32 SD_UltraHighSpeed(SD_HandleTypeDef* hsd, uint32 UltraHighSpeedMode)
	{
		uint32 errorstate = SDMMC_ERROR_NONE;
		SDMMC_DataInitTypeDef sdmmc_datainitstructure;
		uint32 SD_hs[16] = { 0 };
		uint32 count;
		uint32 loop = 0;
		uint32 Timeout = SysTick::getTick();

		if (hsd->SdCard.CardSpeed == CARD_NORMAL_SPEED)
		{
		  /* Standard Speed Card <= 12.5Mhz  */
			return HAL_SD_ERROR_REQUEST_NOT_APPLICABLE;
		}

		if (hsd->SdCard.CardSpeed == CARD_ULTRA_HIGH_SPEED)
		{
		  /* Initialize the Data control register */
			self[SDReg::DCTRL = 0;
			errorstate = SDMMC_CmdBlockLength(hsd->Instance, 64U);

			if (errorstate != SDMMC_ERROR_NONE)
			{
				return errorstate;
			}

			/* Configure the SD DPSM (Data Path State Machine) */
			sdmmc_datainitstructure.DataTimeOut = SDMMC_DATATIMEOUT;
			sdmmc_datainitstructure.DataLength = 64U;
			sdmmc_datainitstructure.DataBlockSize = SDMMC_DATABLOCK_SIZE_64B;
			sdmmc_datainitstructure.TransferDir = SDMMC_TRANSFER_DIR_TO_SDMMC;
			sdmmc_datainitstructure.TransferMode = SDMMC_TRANSFER_MODE_BLOCK;
			sdmmc_datainitstructure.DPSM = SDMMC_DPSM_ENABLE;

			if (SDMMC_ConfigData(hsd->Instance, &sdmmc_datainitstructure) != HAL_OK)
			{
				return (HAL_SD_ERROR_GENERAL_UNKNOWN_ERR);
			}

			errorstate = SDMMC_CmdSwitch(hsd->Instance, UltraHighSpeedMode);
			if (errorstate != SDMMC_ERROR_NONE)
			{
				return errorstate;
			}

			while (!__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_RXOVERR | SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DBCKEND |
				SDMMC_FLAG_DATAEND))
			{
				if (__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_RXFIFOHF))
				{
					for (count = 0U; count < 8U; count++)
					{
						SD_hs[(8U * loop) + count] = SDMMC_ReadFIFO(hsd->Instance);
					}
					loop++;
				}

				if ((SysTick::getTick() - Timeout) >= SDMMC_DATATIMEOUT)
				{
					hsd->ErrorCode = SDMMC_ERROR_TIMEOUT;
					hsd->State = HAL_SD_STATE_READY;
					return SDMMC_ERROR_TIMEOUT;
				}
			}

			if (__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_DTIMEOUT))
			{
				__HAL_SD_CLEAR_FLAG(hsd, SDMMC_FLAG_DTIMEOUT);

				return errorstate;
			}
			else if (__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_DCRCFAIL))
			{
				__HAL_SD_CLEAR_FLAG(hsd, SDMMC_FLAG_DCRCFAIL);

				errorstate = SDMMC_ERROR_DATA_CRC_FAIL;

				return errorstate;
			}
			else if (__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_RXOVERR))
			{
				__HAL_SD_CLEAR_FLAG(hsd, SDMMC_FLAG_RXOVERR);

				errorstate = SDMMC_ERROR_RX_OVERRUN;

				return errorstate;
			}
			else
			{
			  /* No error flag set */
			}

			/* Clear all the static flags */
			__HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_DATA_FLAGS);

			/* Test if the switch mode HS is ok */
			if ((((uint8_t*)SD_hs)[13] & 2U) != 2U)
			{
				errorstate = SDMMC_ERROR_UNSUPPORTED_FEATURE;
			}
			else
			{
			#if defined (USE_HAL_SD_REGISTER_CALLBACKS) && (USE_HAL_SD_REGISTER_CALLBACKS == 1U)
				hsd->DriveTransceiver_1_8V_Callback(SET);
			#else
				HAL_SD_DriveTransceiver_1_8V_Callback(SET);
			#endif /* USE_HAL_SD_REGISTER_CALLBACKS */
			#if defined (DLYB_SDMMC1) || defined (DLYB_SDMMC2)
				  /* Enable DelayBlock Peripheral */
				  /* SDMMC_FB_CLK tuned feedback clock selected as receive clock, for SDR104 */
				MODIFY_REG(self[SDReg::CLKCR, SDMMC_CLKCR_SELCLKRX, SDMMC_CLKCR_SELCLKRX_1);
				if (DelayBlock_Enable(SD_GET_DLYB_INSTANCE(hsd->Instance)) != HAL_OK)
				{
					return (HAL_SD_ERROR_GENERAL_UNKNOWN_ERR);
				}
			#endif /* (DLYB_SDMMC1) || (DLYB_SDMMC2) */
			}
		}

		return errorstate;
	}

	//{TODO}
	// Switches the SD card to Double Data Rate (DDR) mode. This API must be used after "Transfer State"
	// This operation should be followed by the configuration of PLL to have SDMMCCK clock less than 50MHz
	static uint32 SD_DDR_Mode(SD_HandleTypeDef* hsd)
	{
		uint32 errorstate = SDMMC_ERROR_NONE;
		SDMMC_DataInitTypeDef sdmmc_datainitstructure;
		uint32 SD_hs[16] = { 0 };
		uint32 count;
		uint32 loop = 0;
		uint32 Timeout = SysTick::getTick();

		if (hsd->SdCard.CardSpeed == CARD_NORMAL_SPEED)
		{
		  /* Standard Speed Card <= 12.5Mhz  */
			return HAL_SD_ERROR_REQUEST_NOT_APPLICABLE;
		}

		if (hsd->SdCard.CardSpeed == CARD_ULTRA_HIGH_SPEED)
		{
		  /* Initialize the Data control register */
			self[SDReg::DCTRL = 0;
			errorstate = SDMMC_CmdBlockLength(hsd->Instance, 64U);

			if (errorstate != SDMMC_ERROR_NONE)
			{
				return errorstate;
			}

			/* Configure the SD DPSM (Data Path State Machine) */
			sdmmc_datainitstructure.DataTimeOut = SDMMC_DATATIMEOUT;
			sdmmc_datainitstructure.DataLength = 64U;
			sdmmc_datainitstructure.DataBlockSize = SDMMC_DATABLOCK_SIZE_64B;
			sdmmc_datainitstructure.TransferDir = SDMMC_TRANSFER_DIR_TO_SDMMC;
			sdmmc_datainitstructure.TransferMode = SDMMC_TRANSFER_MODE_BLOCK;
			sdmmc_datainitstructure.DPSM = SDMMC_DPSM_ENABLE;

			if (SDMMC_ConfigData(hsd->Instance, &sdmmc_datainitstructure) != HAL_OK)
			{
				return (HAL_SD_ERROR_GENERAL_UNKNOWN_ERR);
			}

			errorstate = SDMMC_CmdSwitch(hsd->Instance, SDMMC_DDR50_SWITCH_PATTERN);
			if (errorstate != SDMMC_ERROR_NONE)
			{
				return errorstate;
			}

			while (!__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_RXOVERR | SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DBCKEND |
				SDMMC_FLAG_DATAEND))
			{
				if (__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_RXFIFOHF))
				{
					for (count = 0U; count < 8U; count++)
					{
						SD_hs[(8U * loop) + count] = SDMMC_ReadFIFO(hsd->Instance);
					}
					loop++;
				}

				if ((SysTick::getTick() - Timeout) >= SDMMC_DATATIMEOUT)
				{
					hsd->ErrorCode = SDMMC_ERROR_TIMEOUT;
					hsd->State = HAL_SD_STATE_READY;
					return SDMMC_ERROR_TIMEOUT;
				}
			}

			if (__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_DTIMEOUT))
			{
				__HAL_SD_CLEAR_FLAG(hsd, SDMMC_FLAG_DTIMEOUT);

				return errorstate;
			}
			else if (__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_DCRCFAIL))
			{
				__HAL_SD_CLEAR_FLAG(hsd, SDMMC_FLAG_DCRCFAIL);

				errorstate = SDMMC_ERROR_DATA_CRC_FAIL;

				return errorstate;
			}
			else if (__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_RXOVERR))
			{
				__HAL_SD_CLEAR_FLAG(hsd, SDMMC_FLAG_RXOVERR);

				errorstate = SDMMC_ERROR_RX_OVERRUN;

				return errorstate;
			}
			else
			{
			  /* No error flag set */
			}

			/* Clear all the static flags */
			__HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_DATA_FLAGS);

			/* Test if the switch mode  is ok */
			if ((((uint8_t*)SD_hs)[13] & 2U) != 2U)
			{
				errorstate = SDMMC_ERROR_UNSUPPORTED_FEATURE;
			}
			else
			{
			#if defined (USE_HAL_SD_REGISTER_CALLBACKS) && (USE_HAL_SD_REGISTER_CALLBACKS == 1U)
				hsd->DriveTransceiver_1_8V_Callback(SET);
			#else
				HAL_SD_DriveTransceiver_1_8V_Callback(SET);
			#endif /* USE_HAL_SD_REGISTER_CALLBACKS */
			#if defined (DLYB_SDMMC1) || defined (DLYB_SDMMC2)
				  /* Enable DelayBlock Peripheral */
				  /* SDMMC_CKin feedback clock selected as receive clock, for DDR50 */
				MODIFY_REG(self[SDReg::CLKCR, SDMMC_CLKCR_SELCLKRX, SDMMC_CLKCR_SELCLKRX_0);
				if (DelayBlock_Enable(SD_GET_DLYB_INSTANCE(hsd->Instance)) != HAL_OK)
				{
					return (HAL_SD_ERROR_GENERAL_UNKNOWN_ERR);
				}
			#endif /* (DLYB_SDMMC1) || (DLYB_SDMMC2) */
			}
		}

		return errorstate;
	}

#endif /* USE_SD_TRANSCEIVER */
	
#endif
}

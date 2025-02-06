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
#if defined(_MPU_STM32MP13)
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

// __weak void HAL_SD_TxCpltCallback(SD_HandleTypeDef* hsd);
// __weak void HAL_SD_RxCpltCallback(SD_HandleTypeDef* hsd);
// __weak void HAL_SD_ErrorCallback(SD_HandleTypeDef* hsd);
// __weak void HAL_SD_AbortCallback(SD_HandleTypeDef* hsd);
// __weak def(USE_SD_TRANSCEIVER) void HAL_SD_DriveTransceiver_1_8V_Callback(FlagStatus status);
// __weak void HAL_SDEx_Read_DMALnkLstBufCpltCallback(SD_HandleTypeDef * hsd);
// __weak void HAL_SDEx_Write_DMALnkLstBufCpltCallback(SD_HandleTypeDef * hsd);


namespace uni {

#if defined (USE_HAL_SD_REGISTER_CALLBACKS) && (USE_HAL_SD_REGISTER_CALLBACKS == 1U)
/**
  * @brief  Register a User SD Callback
  *         To be used instead of the weak (overridden) predefined callback
  * @note   The HAL_SD_RegisterCallback() may be called before HAL_SD_Init in
  *         HAL_SD_STATE_RESET to register callbacks for HAL_SD_MSP_INIT_CB_ID
  *         and HAL_SD_MSP_DEINIT_CB_ID.
  * @param hsd : SD handle
  * @param CallbackID : ID of the callback to be registered
  *        This parameter can be one of the following values:
  *          @arg @ref HAL_SD_TX_CPLT_CB_ID                 SD Tx Complete Callback ID
  *          @arg @ref HAL_SD_RX_CPLT_CB_ID                 SD Rx Complete Callback ID
  *          @arg @ref HAL_SD_ERROR_CB_ID                   SD Error Callback ID
  *          @arg @ref HAL_SD_ABORT_CB_ID                   SD Abort Callback ID
  *          @arg @ref HAL_SD_READ_DMA_LNKLST_BUF_CPLT_CB_ID  SD DMA Rx Linked List Node buffer Callback ID
  *          @arg @ref HAL_SD_WRITE_DMA_LNKLST_BUF_CPLT_CB_ID SD DMA Tx Linked List Node buffer Callback ID
  *          @arg @ref HAL_SD_MSP_INIT_CB_ID                SD MspInit Callback ID
  *          @arg @ref HAL_SD_MSP_DEINIT_CB_ID              SD MspDeInit Callback ID
  * @param pCallback : pointer to the Callback function
  * @retval status
  */
	HAL_StatusTypeDef HAL_SD_RegisterCallback(SD_HandleTypeDef* hsd, HAL_SD_CallbackIDTypeDef CallbackID,
		pSD_CallbackTypeDef pCallback)
	{
		HAL_StatusTypeDef status = HAL_OK;

		if (pCallback == NULL)
		{
		  /* Update the error code */
			hsd->ErrorCode |= HAL_SD_ERROR_INVALID_CALLBACK;
			return HAL_ERROR;
		}

		if (hsd->State == HAL_SD_STATE_READY)
		{
			switch (CallbackID)
			{
			case HAL_SD_TX_CPLT_CB_ID:
				hsd->TxCpltCallback = pCallback;
				break;
			case HAL_SD_RX_CPLT_CB_ID:
				hsd->RxCpltCallback = pCallback;
				break;
			case HAL_SD_ERROR_CB_ID:
				hsd->ErrorCallback = pCallback;
				break;
			case HAL_SD_ABORT_CB_ID:
				hsd->AbortCpltCallback = pCallback;
				break;
			case HAL_SD_READ_DMA_LNKLST_BUF_CPLT_CB_ID:
				hsd->Read_DMALnkLstBufCpltCallback = pCallback;
				break;
			case HAL_SD_WRITE_DMA_LNKLST_BUF_CPLT_CB_ID:
				hsd->Write_DMALnkLstBufCpltCallback = pCallback;
				break;
			case HAL_SD_MSP_INIT_CB_ID:
				hsd->MspInitCallback = pCallback;
				break;
			case HAL_SD_MSP_DEINIT_CB_ID:
				hsd->MspDeInitCallback = pCallback;
				break;
			default:
			  /* Update the error code */
				hsd->ErrorCode |= HAL_SD_ERROR_INVALID_CALLBACK;
				/* update return status */
				status = HAL_ERROR;
				break;
			}
		}
		else if (hsd->State == HAL_SD_STATE_RESET)
		{
			switch (CallbackID)
			{
			case HAL_SD_MSP_INIT_CB_ID:
				hsd->MspInitCallback = pCallback;
				break;
			case HAL_SD_MSP_DEINIT_CB_ID:
				hsd->MspDeInitCallback = pCallback;
				break;
			default:
			  /* Update the error code */
				hsd->ErrorCode |= HAL_SD_ERROR_INVALID_CALLBACK;
				/* update return status */
				status = HAL_ERROR;
				break;
			}
		}
		else
		{
		  /* Update the error code */
			hsd->ErrorCode |= HAL_SD_ERROR_INVALID_CALLBACK;
			/* update return status */
			status = HAL_ERROR;
		}

		return status;
	}

	/**
	  * @brief  Unregister a User SD Callback
	  *         SD Callback is redirected to the weak (overridden) predefined callback
	  * @note   The HAL_SD_UnRegisterCallback() may be called before HAL_SD_Init in
	  *         HAL_SD_STATE_RESET to register callbacks for HAL_SD_MSP_INIT_CB_ID
	  *         and HAL_SD_MSP_DEINIT_CB_ID.
	  * @param hsd : SD handle
	  * @param CallbackID : ID of the callback to be unregistered
	  *        This parameter can be one of the following values:
	  *          @arg @ref HAL_SD_TX_CPLT_CB_ID                 SD Tx Complete Callback ID
	  *          @arg @ref HAL_SD_RX_CPLT_CB_ID                 SD Rx Complete Callback ID
	  *          @arg @ref HAL_SD_ERROR_CB_ID                   SD Error Callback ID
	  *          @arg @ref HAL_SD_ABORT_CB_ID                   SD Abort Callback ID
	  *          @arg @ref HAL_SD_READ_DMA_LNKLST_BUF_CPLT_CB_ID  SD DMA Rx Linked List Node buffer Callback ID
	  *          @arg @ref HAL_SD_WRITE_DMA_LNKLST_BUF_CPLT_CB_ID SD DMA Tx Linked List Node buffer Callback ID
	  *          @arg @ref HAL_SD_MSP_INIT_CB_ID                SD MspInit Callback ID
	  *          @arg @ref HAL_SD_MSP_DEINIT_CB_ID              SD MspDeInit Callback ID
	  * @retval status
	  */
	HAL_StatusTypeDef HAL_SD_UnRegisterCallback(SD_HandleTypeDef* hsd, HAL_SD_CallbackIDTypeDef CallbackID)
	{
		HAL_StatusTypeDef status = HAL_OK;

		if (hsd->State == HAL_SD_STATE_READY)
		{
			switch (CallbackID)
			{
			case HAL_SD_TX_CPLT_CB_ID:
				hsd->TxCpltCallback = HAL_SD_TxCpltCallback;
				break;
			case HAL_SD_RX_CPLT_CB_ID:
				hsd->RxCpltCallback = HAL_SD_RxCpltCallback;
				break;
			case HAL_SD_ERROR_CB_ID:
				hsd->ErrorCallback = HAL_SD_ErrorCallback;
				break;
			case HAL_SD_ABORT_CB_ID:
				hsd->AbortCpltCallback = HAL_SD_AbortCallback;
				break;
			case HAL_SD_READ_DMA_LNKLST_BUF_CPLT_CB_ID:
				hsd->Read_DMALnkLstBufCpltCallback = HAL_SDEx_Read_DMALnkLstBufCpltCallback;
				break;
			case HAL_SD_WRITE_DMA_LNKLST_BUF_CPLT_CB_ID:
				hsd->Write_DMALnkLstBufCpltCallback = HAL_SDEx_Write_DMALnkLstBufCpltCallback;
				break;
			case HAL_SD_MSP_INIT_CB_ID:
				hsd->MspInitCallback = HAL_SD_MspInit;
				break;
			case HAL_SD_MSP_DEINIT_CB_ID:
				hsd->MspDeInitCallback = HAL_SD_MspDeInit;
				break;
			default:
			  /* Update the error code */
				hsd->ErrorCode |= HAL_SD_ERROR_INVALID_CALLBACK;
				/* update return status */
				status = HAL_ERROR;
				break;
			}
		}
		else if (hsd->State == HAL_SD_STATE_RESET)
		{
			switch (CallbackID)
			{
			case HAL_SD_MSP_INIT_CB_ID:
				hsd->MspInitCallback = HAL_SD_MspInit;
				break;
			case HAL_SD_MSP_DEINIT_CB_ID:
				hsd->MspDeInitCallback = HAL_SD_MspDeInit;
				break;
			default:
			  /* Update the error code */
				hsd->ErrorCode |= HAL_SD_ERROR_INVALID_CALLBACK;
				/* update return status */
				status = HAL_ERROR;
				break;
			}
		}
		else
		{
		  /* Update the error code */
			hsd->ErrorCode |= HAL_SD_ERROR_INVALID_CALLBACK;
			/* update return status */
			status = HAL_ERROR;
		}

		return status;
	}

#if (USE_SD_TRANSCEIVER != 0U)
/**
  * @brief  Register a User SD Transceiver Callback
  *         To be used instead of the weak (overridden) predefined callback
  * @param hsd : SD handle
  * @param pCallback : pointer to the Callback function
  * @retval status
  */
	HAL_StatusTypeDef HAL_SD_RegisterTransceiverCallback(SD_HandleTypeDef* hsd, pSD_TransceiverCallbackTypeDef pCallback)
	{
		HAL_StatusTypeDef status = HAL_OK;

		if (pCallback == NULL)
		{
		  /* Update the error code */
			hsd->ErrorCode |= HAL_SD_ERROR_INVALID_CALLBACK;
			return HAL_ERROR;
		}

		/* Process locked */
		__HAL_LOCK(hsd);

		if (hsd->State == HAL_SD_STATE_READY)
		{
			hsd->DriveTransceiver_1_8V_Callback = pCallback;
		}
		else
		{
		  /* Update the error code */
			hsd->ErrorCode |= HAL_SD_ERROR_INVALID_CALLBACK;
			/* update return status */
			status = HAL_ERROR;
		}

		/* Release Lock */
		__HAL_UNLOCK(hsd);
		return status;
	}

	/**
	  * @brief  Unregister a User SD Transceiver Callback
	  *         SD Callback is redirected to the weak (overridden) predefined callback
	  * @param hsd : SD handle
	  * @retval status
	  */
	HAL_StatusTypeDef HAL_SD_UnRegisterTransceiverCallback(SD_HandleTypeDef* hsd)
	{
		HAL_StatusTypeDef status = HAL_OK;

		/* Process locked */
		__HAL_LOCK(hsd);

		if (hsd->State == HAL_SD_STATE_READY)
		{
			hsd->DriveTransceiver_1_8V_Callback = HAL_SD_DriveTransceiver_1_8V_Callback;
		}
		else
		{
		  /* Update the error code */
			hsd->ErrorCode |= HAL_SD_ERROR_INVALID_CALLBACK;
			/* update return status */
			status = HAL_ERROR;
		}

		/* Release Lock */
		__HAL_UNLOCK(hsd);
		return status;
	}
#endif /* USE_SD_TRANSCEIVER */
#endif /* USE_HAL_SD_REGISTER_CALLBACKS */

}
#endif

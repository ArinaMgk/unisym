
#include "../../../../inc/cpp/interrupt"
#include "../../../../inc/cpp/Device/SD.hpp"
#include "../../../../inc/cpp/Device/Storage/SD-PARA.h"

using namespace uni;

#if defined(_MPU_STM32MP13)

namespace uni {
	void _HandlerIRQ_SDMMCx(SecureDigitalCard_t& sd);
}

_WEAK void SDMMC1_IRQHandler(void) { _HandlerIRQ_SDMMCx(SDCard1); }
_WEAK void SDMMC2_IRQHandler(void) { _HandlerIRQ_SDMMCx(SDCard1); }

//{TODO} BUSY BIT

// AKA HAL_SD_IRQHandler
void uni::_HandlerIRQ_SDMMCx(SecureDigitalCard_t& sd)
{
	uint32 errorstate;
	stduint context = _IMM(sd.Context);
	/* Check for SDMMC interrupt flags */
	if (sd[SDReg::STA].bitof(15) //SDMMC_FLAG_RXFIFOHF
		&& ((context & _IMM(SDContext::IT))))
		sd.SD_Read_IT();

	else if (sd[SDReg::STA].bitof(8))// SDMMC_FLAG_DATAEND
	{
		sd[SDReg::ICR] = _IMM1S(8); // SDMMC_CLEAR_FLAG_DATAEND

		// __HAL_SD_DISABLE_IT(hsd, SDMMC_IT_DATAEND | SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT | SDMMC_IT_TXUNDERR | SDMMC_IT_RXOVERR | SDMMC_IT_TXFIFOHE | SDMMC_IT_RXFIFOHF);
		sd[SDReg::MASK] &= ~(((0x1UL << (8U)) | (0x1UL << (1U)) | (0x1UL << (3U)) | (0x1UL << (4U)) | (0x1UL << (5U)) | (0x1UL << (14U)) | (0x1UL << (15U))));

		// __HAL_SD_DISABLE_IT(hsd, SDMMC_IT_IDMABTC);
		sd[SDReg::MASK] &= ~(((0x1UL << (28U))));

		sd[SDReg::CMD].rstof(6);// __SDMMC_CMDTRANS_DISABLE: SDMMC_CMD_CMDTRANS

		if ((context & _IMM(SDContext::IT)))
		{
			if (((context & _IMM(SDContext::READ_MULTIPLE_BLOCK)) != 0U)
				|| ((context & _IMM(SDContext::WRITE_MULTIPLE_BLOCK)) != 0U))
			{
				if (!sd.SDMMC_CmdStopTransfer(&errorstate))
				{
					//{TODO} Error Callback
					return;
				}
			}
			// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_DATA_FLAGS)
			sd[SDReg::ICR] = (_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) | (_IMM1S(8U)) | (_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) | (_IMM1S(27U)) | (_IMM1S(28U));

			sd.Context = (SDContext::NONE);
			if (((context & _IMM(SDContext::READ_SINGLE_BLOCK)) != 0U) || ((context & _IMM(SDContext::READ_MULTIPLE_BLOCK)) != 0U))
			{
				//{TODO} HAL_SD_RxCpltCallback
			}
			else
			{
				//{TODO} HAL_SD_TxCpltCallback
			}
		}
		else if ((context & _IMM(SDContext::DMA)) != 0U)
		{
			sd[SDReg::DLEN] = 0;
			sd[SDReg::DCTRL] = 0;
			sd[SDReg::IDMACTRL] = 0; // SDMMC_DISABLE_IDMA;

			/* Stop Transfer for Write Multi blocks or Read Multi blocks */
			if (((context & _IMM(SDContext::READ_MULTIPLE_BLOCK)) != 0U)
				|| ((context & _IMM(SDContext::WRITE_MULTIPLE_BLOCK)) != 0U))
			{
				if (!sd.SDMMC_CmdStopTransfer(&errorstate))
				{
					//{TODO} SDMMC_ERRORCallback
					return;
				}
			}
			sd.Context = (SDContext::NONE);
			if (((context & _IMM(SDContext::WRITE_SINGLE_BLOCK)) != 0U)
				|| ((context & _IMM(SDContext::WRITE_MULTIPLE_BLOCK)) != 0U))
			{
				//{TODO} HAL_SD_TxCpltCallback
			}
			if (((context & _IMM(SDContext::READ_SINGLE_BLOCK)) != 0U) || ((context & _IMM(SDContext::READ_MULTIPLE_BLOCK)) != 0U))
			{
				//{TODO} HAL_SD_RxCpltCallback
			}
		}
	}
	else if (sd[SDReg::STA].bitof(14) // SDMMC_FLAG_TXFIFOHE
		&& ((context & _IMM(SDContext::IT))))
		sd.SD_Write_IT();

	// else if (__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_RXOVERR | SDMMC_FLAG_TXUNDERR) != RESET)
	else if (((sd[SDReg::STA] & (((0x1UL << (1U)) | (0x1UL << (3U)) | (0x1UL << (5U)) | (0x1UL << (4U))))) != 0U))
	{
		/* Set Error code */
		if (sd[SDReg::STA].bitof(1))// DCRCFAIL
		{
			errorstate |= SDMMC_ERROR_DATA_CRC_FAIL;
		}
		if (sd[SDReg::STA].bitof(3))// SDMMC_FLAG_DTIMEOUT
		{
			errorstate |= SDMMC_ERROR_DATA_TIMEOUT;
		}
		if (sd[SDReg::STA].bitof(5))// RXOVERR
		{
			errorstate |= SDMMC_ERROR_RX_OVERRUN;
		}
		if (sd[SDReg::STA].bitof(4))// TXUNDERR
		{
			errorstate |= SDMMC_ERROR_TX_UNDERRUN;
		}
		// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_DATA_FLAGS)
		sd[SDReg::ICR] = (_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) | (_IMM1S(8U)) | (_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) | (_IMM1S(27U)) | (_IMM1S(28U));

		/* Disable all interrupts */
		// __HAL_SD_DISABLE_IT(hsd, SDMMC_IT_DATAEND | SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT | SDMMC_IT_TXUNDERR | SDMMC_IT_RXOVERR);
		sd[SDReg::MASK] &= ~(((0x1UL << (8U)) | (0x1UL << (1U)) | (0x1UL << (3U)) | (0x1UL << (4U)) | (0x1UL << (5U))));

		sd[SDReg::CMD].rstof(6);// __SDMMC_CMDTRANS_DISABLE(hsd->Instance);
		sd[SDReg::DCTRL] |= _IMM1S(13);// SDMMC_DCTRL_FIFORST;
		sd[SDReg::CMD] |= _IMM1S(7);//SDMMC_CMD_CMDSTOP;
		sd.SDMMC_CmdStopTransfer(&errorstate);
		sd[SDReg::CMD] &= ~_IMM1S(7);//(SDMMC_CMD_CMDSTOP);
		sd[SDReg::ICR] = (((0x1UL << (11U))));// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_FLAG_DABORT);

		if ((context & _IMM(SDContext::IT)) != 0U)
		{
			sd.Context = (SDContext::NONE);
			//{TODO} SDMMC_ERRORCallback
			return;
		}
		else if ((context & _IMM(SDContext::DMA)) != 0U)
		{
			if (errorstate != SDMMC_ERROR_NONE)
			{
				/* Disable Internal DMA */
				sd[SDReg::MASK].rstof(28); // __HAL_SD_DISABLE_IT(hsd, SDMMC_IT_IDMABTC);
				sd[SDReg::IDMACTRL] = 0; // SDMMC_DISABLE_IDMA;
				//{TODO} SDMMC_ERRORCallback
			}
		}
	}

	else if (sd[SDReg::STA].bitof(28))
	{
		// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_FLAG_IDMABTC);
		sd[SDReg::ICR] = _IMM1S(28);// SDMMC_FLAG_IDMABTC
		if ((context & _IMM(SDContext::WRITE_MULTIPLE_BLOCK)))
		{
			//{TODO} HAL_SDEx_Write_DMALnkLstBufCpltCallback
		}
		else /* _IMM(SDContext::READ_MULTIPLE_BLOCK */
		{
			//{TODO} HAL_SDEx_Read_DMALnkLstBufCpltCallback
		}
	}
}

#endif




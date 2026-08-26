
#include "../../../../inc/cpp/Device/Flash"

#if defined(_MCU_STM32H7x)
_ESYM_C{
	// AKA HAL: FLASH_IRQHandler - single global FLASH IRQ (IRQ_FLASH = 4).
	Handler_t FUNC_FLASH[1] = { 0 };

	void FLASH_IRQHandler(void) {
		// AKA HAL_FLASH_IRQHandler: friend of Flash_t; dispatch EOP/error to callbacks.
		uni::Flash_t& fl = uni::Flash;

		// ---- Bank1 end-of-operation ----
		if (fl[uni::FlashReg::SR1].bitof(_FLASH_SR_POS_EOP)) {
			stduint proc = fl.Procedure;
			if (proc == _FLASH_PROC_PROGRAM_BANK1 || proc == _FLASH_PROC_SECTERASE_BANK1 || proc == _FLASH_PROC_MASSERASE_BANK1) {
				fl.Procedure = _FLASH_PROC_NONE;
				fl[uni::FlashReg::CCR1] |= _FLASH_CCR_CLEAR;
				if (proc == _FLASH_PROC_PROGRAM_BANK1) fl[uni::FlashReg::CR1].rstof(_FLASH_CR_POS_PG);
				else if (proc == _FLASH_PROC_SECTERASE_BANK1) fl[uni::FlashReg::CR1].rstof(_FLASH_CR_POS_SER);
				else fl[uni::FlashReg::CR1].rstof(_FLASH_CR_POS_BER);
				if (fl.EndOfOperationHandler) fl.EndOfOperationHandler();
			}
		}
		// ---- Bank2 end-of-operation ----
		if (fl[uni::FlashReg::SR2].bitof(_FLASH_SR_POS_EOP)) {
			stduint proc = fl.Procedure;
			if (proc == _FLASH_PROC_PROGRAM_BANK2 || proc == _FLASH_PROC_SECTERASE_BANK2 || proc == _FLASH_PROC_MASSERASE_BANK2) {
				fl.Procedure = _FLASH_PROC_NONE;
				fl[uni::FlashReg::CCR2] |= _FLASH_CCR_CLEAR;
				if (proc == _FLASH_PROC_PROGRAM_BANK2) fl[uni::FlashReg::CR2].rstof(_FLASH_CR_POS_PG);
				else if (proc == _FLASH_PROC_SECTERASE_BANK2) fl[uni::FlashReg::CR2].rstof(_FLASH_CR_POS_SER);
				else fl[uni::FlashReg::CR2].rstof(_FLASH_CR_POS_BER);
				if (fl.EndOfOperationHandler) fl.EndOfOperationHandler();
			}
		}
		// ---- Bank1 operation error ----
		if ((stduint)fl[uni::FlashReg::SR1] & _FLASH_SR_ERRORS) {
			fl.ErrorCode = (stduint)fl[uni::FlashReg::SR1] & _FLASH_SR_ERRORS;
			fl.Procedure = _FLASH_PROC_NONE;
			fl[uni::FlashReg::CCR1] |= _FLASH_CCR_CLEAR;
			if (fl.OperationErrorHandler) fl.OperationErrorHandler();
		}
		// ---- Bank2 operation error ----
		if ((stduint)fl[uni::FlashReg::SR2] & _FLASH_SR_ERRORS) {
			fl.ErrorCode = (stduint)fl[uni::FlashReg::SR2] & _FLASH_SR_ERRORS;
			fl.Procedure = _FLASH_PROC_NONE;
			fl[uni::FlashReg::CCR2] |= _FLASH_CCR_CLEAR;
			if (fl.OperationErrorHandler) fl.OperationErrorHandler();
		}

		if (fl.Procedure == _FLASH_PROC_NONE) {
			// no procedure running: disarm both banks' operation interrupts
			fl[uni::FlashReg::CR1] &= ~_FLASH_CR_OPER_IT;
			fl[uni::FlashReg::CR2] &= ~_FLASH_CR_OPER_IT;
		}
		if (FUNC_FLASH[0]) FUNC_FLASH[0]();// user callback registered by setInterrupt()
	}
}
#endif

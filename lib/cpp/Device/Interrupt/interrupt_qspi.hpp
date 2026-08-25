
#include "../../../../inc/cpp/Device/SPI-Quad.hpp"

#if defined(_MCU_STM32H7x)
_ESYM_C{
	// AKA HAL: QUADSPI_IRQHandler - single global QSPI IRQ (IRQ_QUADSPI = 92).
	Handler_t FUNC_QSPI[1] = { 0 };

	void QUADSPI_IRQHandler(void) {
		// AKA HAL_QSPI_IRQHandler; friend of uni::QSPI
		uni::QSPI& q = uni::QSPI1;
		stduint flag = q[uni::QSPIReg::SR];
		stduint itsrc = q[uni::QSPIReg::CR];

		// FIFO threshold interrupt
		if ((flag & (1U << _QSPI_SR_POS_FTF)) && (itsrc & (1U << _QSPI_CR_POS_FTIE))) {
			// DR is 32-bit FIFO-backed: byte-wise access pops/pushes one byte only
			uni::Reference_T<byte> qdr(_QSPI_ADDR + _IMMx4(uni::QSPIReg::DR));
			if (q.state == uni::QSPIState::BusyIndirectTx) {
				while (q[uni::QSPIReg::SR].bitof(_QSPI_SR_POS_FTF)) {
					if (q.tx_count > 0) {
						qdr = *q.tx_buff++;
						q.tx_count--;
					} else {
						q[uni::QSPIReg::CR].rstof(_QSPI_CR_POS_FTIE);
						break;
					}
				}
			} else if (q.state == uni::QSPIState::BusyIndirectRx) {
				while (q[uni::QSPIReg::SR].bitof(_QSPI_SR_POS_FTF)) {
					if (q.rx_count > 0) {
						*q.rx_buff++ = (byte)qdr;
						q.rx_count--;
					} else {
						q[uni::QSPIReg::CR].rstof(_QSPI_CR_POS_FTIE);
						break;
					}
				}
			}
			if (q.FifoThresholdHandler) q.FifoThresholdHandler();
		}
		// transfer complete interrupt
		else if ((flag & (1U << _QSPI_SR_POS_TCF)) && (itsrc & (1U << _QSPI_CR_POS_TCIE))) {
			q[uni::QSPIReg::FCR].setof(_QSPI_FCR_POS_CTCF);
			q[uni::QSPIReg::CR].rstof(_QSPI_CR_POS_TCIE);
			q[uni::QSPIReg::CR].rstof(_QSPI_CR_POS_TEIE);
			q[uni::QSPIReg::CR].rstof(_QSPI_CR_POS_FTIE);
			if (q.state == uni::QSPIState::BusyIndirectTx) {
				q.state = uni::QSPIState::Ready;
				if (q.TxCpltHandler) q.TxCpltHandler();
			} else if (q.state == uni::QSPIState::BusyIndirectRx) {
				q.state = uni::QSPIState::Ready;
				if (q.RxCpltHandler) q.RxCpltHandler();
			} else if (q.state == uni::QSPIState::Busy) {
				q.state = uni::QSPIState::Ready;
				if (q.CmdCpltHandler) q.CmdCpltHandler();
			} else if (q.state == uni::QSPIState::Abort) {
				q.state = uni::QSPIState::Ready;
				if (q.error_code == _QSPI_ERROR_NONE) {
					if (q.AbortCpltHandler) q.AbortCpltHandler();
				} else {
					if (q.ErrorHandler) q.ErrorHandler();
				}
			}
		}
		// status match interrupt
		else if ((flag & (1U << _QSPI_SR_POS_SMF)) && (itsrc & (1U << _QSPI_CR_POS_SMIE))) {
			q[uni::QSPIReg::FCR].setof(_QSPI_FCR_POS_CSMF);
			if (q[uni::QSPIReg::CR].bitof(_QSPI_CR_POS_APMS)) {
				q[uni::QSPIReg::CR].rstof(_QSPI_CR_POS_SMIE);
				q[uni::QSPIReg::CR].rstof(_QSPI_CR_POS_TEIE);
				q.state = uni::QSPIState::Ready;
			}
			if (q.StatusMatchHandler) q.StatusMatchHandler();
		}
		// transfer error interrupt
		else if ((flag & (1U << _QSPI_SR_POS_TEF)) && (itsrc & (1U << _QSPI_CR_POS_TEIE))) {
			q[uni::QSPIReg::FCR].setof(_QSPI_FCR_POS_CTEF);
			q[uni::QSPIReg::CR].rstof(_QSPI_CR_POS_SMIE);
			q[uni::QSPIReg::CR].rstof(_QSPI_CR_POS_TCIE);
			q[uni::QSPIReg::CR].rstof(_QSPI_CR_POS_TEIE);
			q[uni::QSPIReg::CR].rstof(_QSPI_CR_POS_FTIE);
			q.error_code |= _QSPI_ERROR_TRANSFER;
			q.state = uni::QSPIState::Ready;
			if (q.ErrorHandler) q.ErrorHandler();
		}
		// timeout interrupt
		else if ((flag & (1U << _QSPI_SR_POS_TOF)) && (itsrc & (1U << _QSPI_CR_POS_TOIE))) {
			q[uni::QSPIReg::FCR].setof(_QSPI_FCR_POS_CTOF);
			if (q.TimeOutHandler) q.TimeOutHandler();
		}
		if (FUNC_QSPI[0]) FUNC_QSPI[0]();// user callback registered by setInterrupt()
	}
}
#endif

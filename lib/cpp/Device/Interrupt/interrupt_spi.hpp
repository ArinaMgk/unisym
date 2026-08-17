
#include "../../../../inc/cpp/Device/SPI"
#include "interrupt_i2s.hpp"

using namespace uni;

#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)

static SPI_HARD& _spi_inst(byte spid) {
	switch (spid) {
	case 1: return SPI1;
	case 2: return SPI2;
	case 3: return SPI3;
	case 4: return SPI4;
	case 5: return SPI5;
	#if defined(_MCU_STM32H7x)
	case 6: return SPI6;
	#endif
	default: return SPI1;
	}
}

// AKA HAL_SPI_IRQHandler
void _HandlerIRQ_SPI(byte spid) {
	SPI_HARD& sp = _spi_inst(spid);
	// I2S mode: SPI1/2/3 in I2S mode share the same IRQ line, route to the I2S driver
	if (sp[SPIReg::I2SCFGR].bitof(SPI_I2SCFGR_I2SMOD_Pos)) {
		_HandlerIRQ_I2S(spid);
		return;
	}
	stduint itsource = sp[SPIReg::IER];
	stduint itflag = sp[SPIReg::SR];
	stduint trigger = itsource & itflag;
	SPIXState st = sp.getState();
	bool handled = false;

	// SPI in mode Receiver
	if (!(trigger & SPI_SR_OVR) && (trigger & SPI_SR_RXP)) {
		sp.innHandlerByInterrupt();
		handled = true;
	}
	// SPI in mode Transmitter
	if (!(trigger & SPI_SR_UDR) && (trigger & SPI_SR_TXP)) {
		sp.outHandlerByInterrupt();
		handled = true;
	}
	if (handled) return;

	// End Of Transfer
	if (trigger & SPI_SR_EOT) {
		sp[SPIReg::IFCR] = SPI_IFCR_EOTC | SPI_IFCR_TXTFC;
		sp[SPIReg::IER].rstof(SPI_IER_EOTIE_Pos);
		// IT-based transfer: drain remaining RX data (AKA HAL EOT extra polling)
		if (!(sp[SPIReg::CFG1] & (SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN)))
			sp.innHandlerByInterrupt();
		sp.closeTransfer();
		sp.setState(SPIXState::Ready);
		if (st == SPIXState::BusyTxRx) { if (sp.TxRxCpltHandler) sp.TxRxCpltHandler(); }
		else if (st == SPIXState::BusyRX) { if (sp.RxCpltHandler) sp.RxCpltHandler(); }
		else if (st == SPIXState::BusyTX) { if (sp.TxCpltHandler) sp.TxCpltHandler(); }
		return;
	}

	// Error treatment
	if (trigger & (SPI_SR_MODF | SPI_SR_OVR | SPI_SR_TIFRE | SPI_SR_UDR)) {
		if (trigger & SPI_SR_OVR) { sp.setError(_SPI_ERR_OVR); sp[SPIReg::IFCR] = SPI_IFCR_OVRC; }
		if (trigger & SPI_SR_MODF) { sp.setError(_SPI_ERR_MODF); sp[SPIReg::IFCR] = SPI_IFCR_MODFC; }
		if (trigger & SPI_SR_TIFRE) { sp.setError(_SPI_ERR_FRE); sp[SPIReg::IFCR] = SPI_IFCR_TIFREC; }
		if (trigger & SPI_SR_UDR) { sp.setError(_SPI_ERR_UDR); sp[SPIReg::IFCR] = SPI_IFCR_UDRC; }
		sp.closeTransfer();
		sp.setState(SPIXState::Ready);
		if (sp.ErrorHandler) sp.ErrorHandler();
		return;
	}
}

_ESYM_C{
	void SPI1_IRQHandler(void) { _HandlerIRQ_SPI(1); }
	void SPI2_IRQHandler(void) { _HandlerIRQ_SPI(2); }
	void SPI3_IRQHandler(void) { _HandlerIRQ_SPI(3); }
	#if defined(_MCU_STM32H7x)
	void SPI4_IRQHandler(void) { _HandlerIRQ_SPI(4); }
	void SPI5_IRQHandler(void) { _HandlerIRQ_SPI(5); }
	void SPI6_IRQHandler(void) { _HandlerIRQ_SPI(6); }
	#elif defined(_MPU_STM32MP13)
	void SPI4_IRQHandler(void) { _HandlerIRQ_SPI(4); }
	void SPI5_IRQHandler(void) { _HandlerIRQ_SPI(5); }
	#endif
}

#endif

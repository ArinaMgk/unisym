
#include "../../../../inc/cpp/Device/I2S"

using namespace uni;

#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)

static I2S_HARD& _i2s_inst(byte i2sid) {
	switch (i2sid) {
	case 1: return I2S1;
	case 2: return I2S2;
	case 3: return I2S3;
	default: return I2S1;
	}
}

// AKA HAL_I2S_IRQHandler (split RX/TX dispatch)
// I2S shares the SPI1/2/3 IRQ lines; interrupt_spi.hpp routes here when I2SMOD is set.
// The inn/out helpers are self-contained (data pump + close + OVR/UDR error handling),
// so this dispatcher only gates them by the enabled interrupt + pending flags.
void _HandlerIRQ_I2S(byte i2sid) {
	I2S_HARD& i2s = _i2s_inst(i2sid);
	stduint itsource = i2s[SPIReg::IER];
	stduint i2ssr = i2s[SPIReg::SR];
	// Receiver: RXPIE enabled and (RXP pending or OVR error)
	if ((itsource & SPI_IER_RXPIE) && ((i2ssr & SPI_SR_RXP) || (i2ssr & SPI_SR_OVR)))
		i2s.innHandlerByInterrupt();
	// Transmitter: TXPIE enabled and (TXP pending or UDR error)
	if ((itsource & SPI_IER_TXPIE) && ((i2ssr & SPI_SR_TXP) || (i2ssr & SPI_SR_UDR)))
		i2s.outHandlerByInterrupt();
}

#endif

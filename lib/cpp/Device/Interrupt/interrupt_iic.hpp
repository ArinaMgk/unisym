

#include "../../../../inc/c/stdinc.h"
#include "../../../../inc/c/driver/IIC.h"

_ESYM_C{
	#if defined(_MCU_STM32H7x)
	void I2C1_EV_IRQHandler(void) {
		stduint itflags = IIC1[IICReg::ISR];
		stduint itsources = IIC1[IICReg::CR1];

	}

	//{TODO} more...

	#elif defined(_MPU_STM32MP13)
	void I2C1_EV_IRQHandler(void) {}
	void I2C1_ER_IRQHandler(void) {}
	void I2C2_EV_IRQHandler(void) {}
	void I2C2_ER_IRQHandler(void) {}
	void I2C3_EV_IRQHandler(void) {}
	void I2C3_ER_IRQHandler(void) {}
	void I2C4_EV_IRQHandler(void) {}
	void I2C4_ER_IRQHandler(void) {}
	void I2C5_EV_IRQHandler(void) {}
	void I2C5_ER_IRQHandler(void) {}
	#endif
}


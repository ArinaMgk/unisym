#define _MCU_IIC_TEMP
#include "../../../../inc/cpp/Device/IIC"

using namespace uni;

#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
void _HandlerIRQ_IIC_EV(byte iic_id);
void _HandlerIRQ_IIC_ER(byte iic_id);
#endif

_ESYM_C{
#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
	Handler_t FUNC_IIC[16] = { 0 };

	#if defined(_MCU_STM32H7x)
	void I2C1_EV_IRQHandler(void) { _HandlerIRQ_IIC_EV(1); }
	void I2C1_ER_IRQHandler(void) { _HandlerIRQ_IIC_ER(1); }
	void I2C2_EV_IRQHandler(void) { _HandlerIRQ_IIC_EV(2); }
	void I2C2_ER_IRQHandler(void) { _HandlerIRQ_IIC_ER(2); }
	void I2C3_EV_IRQHandler(void) { _HandlerIRQ_IIC_EV(3); }
	void I2C3_ER_IRQHandler(void) { _HandlerIRQ_IIC_ER(3); }
	void I2C4_EV_IRQHandler(void) { _HandlerIRQ_IIC_EV(4); }
	void I2C4_ER_IRQHandler(void) { _HandlerIRQ_IIC_ER(4); }
	#elif defined(_MPU_STM32MP13)
	void I2C1_EV_IRQHandler(void) { _HandlerIRQ_IIC_EV(1); }
	void I2C1_ER_IRQHandler(void) { _HandlerIRQ_IIC_ER(1); }
	void I2C2_EV_IRQHandler(void) { _HandlerIRQ_IIC_EV(2); }
	void I2C2_ER_IRQHandler(void) { _HandlerIRQ_IIC_ER(2); }
	void I2C3_EV_IRQHandler(void) { _HandlerIRQ_IIC_EV(3); }
	void I2C3_ER_IRQHandler(void) { _HandlerIRQ_IIC_ER(3); }
	void I2C4_EV_IRQHandler(void) { _HandlerIRQ_IIC_EV(4); }
	void I2C4_ER_IRQHandler(void) { _HandlerIRQ_IIC_ER(4); }
	void I2C5_EV_IRQHandler(void) { _HandlerIRQ_IIC_EV(5); }
	void I2C5_ER_IRQHandler(void) { _HandlerIRQ_IIC_ER(5); }
	#endif
#endif
}

#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
static IIC_HARD& _IICById(byte iic_id) {
	switch (iic_id) {
	case 1: return IIC1;
	case 2: return IIC2;
	case 3: return IIC3;
	case 4: return IIC4;
	#if defined(_MPU_STM32MP13)
	case 5: return IIC5;
	#endif
	default: return IIC1;
	}
}
void _HandlerIRQ_IIC_EV(byte iic_id) _Comment("AKA HAL_I2C_EV_IRQHandler") {
	#if defined(_HIS_INT)
	asserv(FUNC_IIC[iic_id])();
	#else
	if (_IICById(iic_id).evByInterrupt()) asserv(FUNC_IIC[iic_id])();
	#endif
}
void _HandlerIRQ_IIC_ER(byte iic_id) _Comment("AKA HAL_I2C_ER_IRQHandler") {
	#if defined(_HIS_INT)
	asserv(FUNC_IIC[iic_id])();
	#else
	if (_IICById(iic_id).erByInterrupt()) asserv(FUNC_IIC[iic_id])();
	#endif
}
#endif

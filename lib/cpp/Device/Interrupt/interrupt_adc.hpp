#include "../../../../inc/cpp/Device/ADC"
#include "../../../../inc/c/driver/ADConverter/Register-ADC.h"

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MPU_STM32MP13)
_ESYM_C{
	Handler_t FUNC_ADCx[4] = { 0 };

	void ADC1_2_IRQHandler(void);
	void ADC3_IRQHandler(void);

	void ADC_IRQHandler(void);

	symbol_t ADC1_IRQHandler;
	symbol_t ADC2_IRQHandler;
}
#endif


#if defined(_MCU_STM32F1x)
void ADC1_2_IRQHandler(void) {
		// 1 Check End of Conversion flag for regular group
	for1(i, 2) {
		ADC_t& crt = i == 1 ? uni::ADC1 : uni::ADC2;
		if (crt[ADCReg::CR1].bitof(_ADC_CR1_POS_EOCIE) && crt[ADCReg::SR].bitof(_ADC_SR_POS_EOC)) {
			// 确定是否有即将到来的外部触发、连续模式或扫描序列转换，适用于组内常规转换。对于STM32F1设备，如果启用了序列器（选择了多个通道），则在序列结束后会触发转换结束标志。
			if (crt.isTriggeredBySoftware() && crt.getConfigLastDiscCount()) {
				crt[ADCReg::CR1].rstof(_ADC_CR1_POS_EOCIE);// Disable ADC end of conversion interrupt on group regular
			}
			asserv(FUNC_ADCx[i])();
			unchecked{//aka __HAL_ADC_CLEAR_FLAG
				crt[ADCReg::SR].rstof(_ADC_SR_POS_STRT);
				crt[ADCReg::SR].rstof(_ADC_SR_POS_STRT);
			}
			break;
		}
	}
	//{TODO} 2 Check End of Conversion flag for injected group
	//{TODO} 3 Check Analog watchdog flags
}

void ADC3_IRQHandler(void) { _TODO }

#elif defined(_MCU_STM32F4x)

static bool ADC_IRQHandler_sub(ADC_t& sel) {
	static bool busy;
	using namespace ADCReg;
	if (busy) return false; else busy = true;
	bool flg1, flg2;
	flg1 = sel[SR].bitof(1);// EOC
	flg2 = sel[CR1].bitof(5);// EOCIE
	if (flg1 && flg2) {
		// > Determine whether any further conversion upcoming on group regular by external trigger, continuous mode or scan sequence on going.  
		// > Note: On STM32F4, there is no independent flag of end of sequence.The test of scan sequence on going is done either with scan sequence disabled or with end of conversion flag set to of end of sequence.
		// > Disable ADC end of single conversion interrupt on group regular Note: Overrun interrupt was enabled with EOC interrupt in HAL_ADC_Start_IT(), but is not disabled here because can be used by overrun IRQ process below.
		bool SOFTWARE_START_INJECTED = sel[CR2].mask(20, 2);// JEXTEN
		bool ContiConvMode = sel[CR2].bitof(_ADC_CR2_POS_CONT);
		if (!SOFTWARE_START_INJECTED && !ContiConvMode &&
			!sel[SQR1].mask(20, 4)/*L*/ && !sel[CR2].bitof(10)/*EOCS*/) {
			sel[CR1].setof(5, false);// EOCIE
		}
		asserv(FUNC_ADCx[sel.getID()])();// Conversion complete callback
		sel[SR] &= ~_IMM(0x10);// Clear STRT flags
		busy = false;
		return true;
	}
	//{TODO} MORE
	busy = false;
	return false;
}
void ADC_IRQHandler(void) {
	//_TEMP NVIC.setAble(IRQ_ADC, false);
	if (ADC_IRQHandler_sub(ADC1)) return;
	if (ADC_IRQHandler_sub(ADC2)) return;
	if (ADC_IRQHandler_sub(ADC3)) return;
	//_TEMP NVIC.setAble(IRQ_ADC, true);
}

#elif defined(_MPU_STM32MP13)

void ADC1_IRQHandler(void) {}
void ADC2_IRQHandler(void) {}

#endif


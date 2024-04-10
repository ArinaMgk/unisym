// ASCII CPP TAB4 CRLF
// Attribute: <ArnCovenant> <Env> <bin^%> <CPU()> [Allocation]
// LastCheck: <date>
// AllAuthor: @dosconio
// ModuTitle: ...
// Copyright: ...

#include "../../../inc/cpp/Device/GPIO"

namespace uni
{
	#ifdef _MCU_STM32F103VE_X
	GeneralPurposeInputOutputPort GPIOA(0,0,0);//{}
	GeneralPurposeInputOutputPort GPIOB(0x40021018, 0x40010C0C, 0x40010C00);
	GeneralPurposeInputOutputPort GPIOC(0,0,0);//{}
	GeneralPurposeInputOutputPort GPIOD(0,0,0);//{}
	GeneralPurposeInputOutputPort GPIOE(0,0,0);//{}

	GeneralPurposeInputOutput GPIO;

	GeneralPurposeInputOutputPin& GeneralPurposeInputOutputPin::operator=(bool val) {
		// G'DP or D'DP
		if (val)
			parent->OutpdPort |=  (1 << bitposi);
		else
			parent->OutpdPort &= ~(1 << bitposi);
		return *this;
	}
	
	void GeneralPurposeInputOutputPin::setMode(GPIOMode::Mode mod) {
		if (mod == GPIOMode::OUT_PushPull)
			parent->CnrglPort &= ~(0xf << (4 * bitposi));
	}
	
	bool GeneralPurposeInputOutputPin::setSpeedM(uint32 MHz) {
		if (MHz == 2) {
			parent->CnrglPort |=  (0x2 << (4*bitposi));
			return true;
		}
		return false;
	}
	
	
	
	#endif
}


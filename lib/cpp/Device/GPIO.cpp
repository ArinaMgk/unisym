// ASCII CPP TAB4 CRLF
// Attribute: <ArnCovenant> <Env> <bin^%> <CPU()> [Allocation]
// LastCheck: <date>
// AllAuthor: @dosconio
// ModuTitle: ...
// Copyright: ...

#include "../../../inc/cpp/Device/GPIO"
#include "../../../inc/c/binary.h"

namespace uni
{
	#ifdef _MCU_STM32F103VE
	GeneralPurposeInputOutputPort GPIOA(0,0,0);//{}
	GeneralPurposeInputOutputPort GPIOB(0x40021018, 0x40010C0C, 0x40010C00, 3);
	GeneralPurposeInputOutputPort GPIOC(0,0,0);//{}
	GeneralPurposeInputOutputPort GPIOD(0,0,0);//{}
	GeneralPurposeInputOutputPort GPIOE(0,0,0);//{}

	GeneralPurposeInputOutput GPIO;

	GeneralPurposeInputOutputPin& GeneralPurposeInputOutputPin::operator=(bool val) {
		// G'DP or D'DP
		if (val)
			BitSet(parent->OutpdPort, bitposi);
		else
			BitClr(parent->OutpdPort, bitposi);
		return *this;
	}
	
	void GeneralPurposeInputOutputPin::setMode(GPIOMode::Mode mode, GPIOSpeed::Speed speed) {
		uint32 bposi = bitposi << 2; // mul by 4
		uint32 bmode = (uint32)mode;
		uint32 state = (bmode & 1) ? GPIOSpeed::Atmost_Input : speed;
		state |= (bmode & 0xC);// 0b1100
		parent->CnrglPort = (parent->CnrglPort & ~(0xf << bposi)) | (state << bposi);
	}
	
	
	#endif
}


// ASCII CPP TAB4 CRLF
// Attribute: <ArnCovenant> <Env> <bin^%> <CPU()> [Allocation]
// LastCheck: <date>
// AllAuthor: @dosconio
// ModuTitle: ...
// Copyright: ...

#include "../../../inc/cpp/Device/GPIO"
#include "../../../inc/cpp/Device/RCC/RCC"
#include "../../../inc/c/binary.h"

namespace uni
{
	#ifdef _MCU_STM32F103VE
	#define _OFFSET_GPIO_CRL 0x00
	#define _OFFSET_GPIO_IDR 0x08
	#define _OFFSET_GPIO_ODR 0x0C

	GeneralPurposeInputOutputPort::GeneralPurposeInputOutputPort(uint32 ADDR, uint32 CLK, uint32 Enap) :
		EnablPosi(Enap), // (RCC_APB2ENR)
		ClockPort(CLK), // Enable Clock
		CnrglPort(_OFFSET_GPIO_CRL + ADDR),
		InnpdPort(_OFFSET_GPIO_IDR + ADDR),
		OutpdPort(_OFFSET_GPIO_ODR + ADDR)
	{
		for0(i, numsof(OutpdPins))
			OutpdPins[i] = GeneralPurposeInputOutputPin(this, i);
	}

	GeneralPurposeInputOutputPort GPIOA(0x40010800, _RCC_APB2ENR_ADDR, _RCC_APB2ENR_POSI_ENCLK_GPIOA);
	GeneralPurposeInputOutputPort GPIOB(0x40010C00, _RCC_APB2ENR_ADDR, _RCC_APB2ENR_POSI_ENCLK_GPIOB);
	GeneralPurposeInputOutputPort GPIOC(0x40011000, _RCC_APB2ENR_ADDR, _RCC_APB2ENR_POSI_ENCLK_GPIOC);
	GeneralPurposeInputOutputPort GPIOD(0,0,0);//{}
	GeneralPurposeInputOutputPort GPIOE(0,0,0);//{}

	GeneralPurposeInputOutput GPIO;

	GeneralPurposeInputOutputPin::operator bool() {
		return (innput ? parent->InnpdPort : parent->OutpdPort) & (1 << bitposi);
	}
	
	GeneralPurposeInputOutputPin& GeneralPurposeInputOutputPin::operator=(bool val) {
		if (innput) return *this;
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
		innput = (bmode & 1);
		uint32 state = innput ? GPIOSpeed::Atmost_Input : speed;
		state |= (bmode & 0xC);// 0b1100
		parent->CnrglPort = (parent->CnrglPort & ~(0xf << bposi)) | (state << bposi);
	}

	void GeneralPurposeInputOutputPin::setPull(bool pullup) {
		parent->OutpdPort = pullup ? 1 : 0;
	}

	void GeneralPurposeInputOutputPin::Toggle() {
		parent->OutpdPort ^= 1 << bitposi;
	}

	
	#endif
}


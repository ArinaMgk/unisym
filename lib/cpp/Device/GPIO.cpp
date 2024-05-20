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
	#ifdef _MCU_STM32F10x

	#define _OFFSET_GPIO_CRL 0x00
	#define _OFFSET_GPIO_CRH 0x04
	#define _OFFSET_GPIO_IDR 0x08
	#define _OFFSET_GPIO_ODR 0x0C

	GeneralPurposeInputOutputPort::GeneralPurposeInputOutputPort(uint32 ADDR, uint32 CLK, uint32 Enap) :
		EnablPosi(Enap), // (RCC_APB2ENR)
		ClockPort(CLK), // Enable Clock
		CnrglPort(_OFFSET_GPIO_CRL + ADDR),
		CnrghPort(_OFFSET_GPIO_CRH + ADDR),
		InnpdPort(_OFFSET_GPIO_IDR + ADDR),
		OutpdPort(_OFFSET_GPIO_ODR + ADDR),
		base(ADDR)
	{
		for0(i, numsof(OutpdPins))
			OutpdPins[i] = GeneralPurposeInputOutputPin(this, i);
	}

	GeneralPurposeInputOutputPort GPIOA(0x40010800, _RCC_APB2ENR_ADDR, _RCC_APB2ENR_POSI_ENCLK_GPIOA);
	GeneralPurposeInputOutputPort GPIOB(0x40010C00, _RCC_APB2ENR_ADDR, _RCC_APB2ENR_POSI_ENCLK_GPIOB);
	GeneralPurposeInputOutputPort GPIOC(0x40011000, _RCC_APB2ENR_ADDR, _RCC_APB2ENR_POSI_ENCLK_GPIOC);
	GeneralPurposeInputOutputPort GPIOD(0x40011400, _RCC_APB2ENR_ADDR, _RCC_APB2ENR_POSI_ENCLK_GPIOD);
	GeneralPurposeInputOutputPort GPIOE(0x40011800, _RCC_APB2ENR_ADDR, _RCC_APB2ENR_POSI_ENCLK_GPIOE);
	GeneralPurposeInputOutputPort GPIOF(0x40011C00, _RCC_APB2ENR_ADDR, _RCC_APB2ENR_POSI_ENCLK_GPIOF);
	GeneralPurposeInputOutputPort GPIOG(0x40012000, _RCC_APB2ENR_ADDR, _RCC_APB2ENR_POSI_ENCLK_GPIOG);

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

	void GeneralPurposeInputOutputPin::setMode(GPIOMode::Mode mode, GPIOSpeed::Speed speed, bool autoEnClk) {
		Reference& ref = (bitposi < 8) ? parent->CnrglPort : parent->CnrghPort;
		uint32 bposi = (bitposi < 8) ? bitposi << 2 : (bitposi - 8) << 2; // mul by 4
		uint32 bmode = (uint32)mode;
		innput = (bmode & 1);
		uint32 state = innput ? GPIOSpeed::Atmost_Input : speed;
		state |= (bmode & 0xC);// 0b1100
		ref = (ref & ~(0xf << bposi)) | (state << bposi);// or treat it a double-length Reference (64-bit)
		if (autoEnClk) parent->enClock();
	}
	// for F1, only for input (?)
	void GeneralPurposeInputOutputPin::setPull(bool pullup) {
		(parent->OutpdPort).setof(bitposi, pullup);
	}

	void GeneralPurposeInputOutputPin::Toggle() {
		parent->OutpdPort ^= 1 << bitposi;
	}

// ---- ---- ---- ----
#elif defined(_MCU_STM32F4x)

	#define _OFFSET_GPIO_MODER 0x00
	#define _OFFSET_GPIO_OTYPE 0x04
	#define _OFFSET_GPIO_SPEED 0x08
	#define _OFFSET_GPIO_PULLS 0x0C
	#define _OFFSET_GPIO_IDR   0x10
	#define _OFFSET_GPIO_ODR   0x14

	GeneralPurposeInputOutputPort::GeneralPurposeInputOutputPort(uint32 ADDR, uint32 CLK, uint32 Enap) :
		EnablPosi(Enap), // (RCC_APB2ENR)
		ClockPort(CLK), // Enable Clock
		InnpdPort(_OFFSET_GPIO_IDR + ADDR),
		OutpdPort(_OFFSET_GPIO_ODR + ADDR),
		//
		ModerPort(_OFFSET_GPIO_MODER + ADDR),
		OtypePort(_OFFSET_GPIO_OTYPE + ADDR),
		SpeedPort(_OFFSET_GPIO_SPEED + ADDR),
		PullsPort(_OFFSET_GPIO_PULLS + ADDR)
		//{TODO} base(ADDR),
		
	{
		for0(i, numsof(OutpdPins))
			OutpdPins[i] = GeneralPurposeInputOutputPin(this, i);
	}

	GeneralPurposeInputOutputPort GPIOA(0x40020000, _RCC_AHB1ENR_ADDR, _RCC_AHB1ENR_POSI_ENCLK_GPIOA);// ~ 0x400203FF
	GeneralPurposeInputOutputPort GPIOB(0x40020400, _RCC_AHB1ENR_ADDR, _RCC_AHB1ENR_POSI_ENCLK_GPIOB);
	GeneralPurposeInputOutputPort GPIOC(0x40020800, _RCC_AHB1ENR_ADDR, _RCC_AHB1ENR_POSI_ENCLK_GPIOC);
	GeneralPurposeInputOutputPort GPIOD(0x40020C00, _RCC_AHB1ENR_ADDR, _RCC_AHB1ENR_POSI_ENCLK_GPIOD);//
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

	void GeneralPurposeInputOutputPin::setMode(GPIOMode::Mode mode, GPIOSpeed::Speed speed, bool autoEnClk) {
		parent->ModerPort &= ~(uint32)(0x3 << (bitposi << 1));
		parent->ModerPort |= (((stduint)mode)>>1) << (bitposi << 1);
		if ((stduint)mode & 1)
			BitSet(parent->OtypePort, bitposi);
		else
			BitClr(parent->OtypePort, bitposi);
		parent->SpeedPort &= ~(uint32)(0x3 << (bitposi << 1));
		parent->SpeedPort |= (stduint)speed << (bitposi << 1);
		if (mode == GPIOMode::IN_Floating) parent->PullsPort &= ~(uint32)(0x3 << (bitposi << 1));
		innput = (GPIOMode::IN_Floating == mode) || (GPIOMode::IN_Analog == mode) || (GPIOMode::IN_Pull == mode); // KEPT
		if (autoEnClk) parent->enClock();
	}

	void GeneralPurposeInputOutputPin::setPull(bool pullup) {
		if (pullup) {
			parent->PullsPort |= 0x1 << (bitposi << 1);// pull-up
			parent->PullsPort &= ~(uint32)(0x2 << (bitposi << 1));
		}
		else {
			parent->PullsPort |= 0x2 << (bitposi << 1);// pull-dn
			parent->PullsPort &= ~(uint32)(0x1 << (bitposi << 1));
		}
	}

	void GeneralPurposeInputOutputPin::Toggle() {
		parent->OutpdPort ^= 1 << bitposi;
	}
	
	#endif
}


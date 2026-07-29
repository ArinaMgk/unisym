// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Alternate Function I/O
// Codifiers: @dosconio: 20240527
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#include "../../../inc/cpp/Device/AFIO"
namespace uni {
#if 0

#elif defined(_MCU_STM32F1x)

	Reference AFIO::EventCtrlReg(_AFIO_ADDR + 0x00);// AFIO_EVCR
	Reference AFIO::MapReg(_AFIO_ADDR + 0x04);// AFIO_MAPR
	Reference AFIO::MapReg2(_AFIO_ADDR + 0x2C);// AFIO_MAPR2
	Reference AFIO::ExternInterruptCfgs[4] = {// AFIO_EXTICRx
		Reference(_AFIO_ADDR + 0x08),Reference(_AFIO_ADDR + 0x0C),
		Reference(_AFIO_ADDR + 0x10),Reference(_AFIO_ADDR + 0x14)
	};

	// ---- AFIO Remap ----
	// Compact table: { bitPos, width } for each AFIORemap enum value
	// Register is implied by position (MAPR vs MAPR2)
	struct RemapEntry { byte pos, wid; };
	static constexpr RemapEntry _remap_tbl[] = {
		// MAPR single-bit
		{0, 1},  // SPI1
		{1, 1},  // I2C1
		{2, 1},  // USART1
		{3, 1},  // USART2
		{12,1},  // TIM4
		{16,1},  // PD01
		{17,1},  // TIM5CH4
		{28,1},  // SPI3 (connectivity line only)
		{21,1},  // ETH (connectivity line only)
		{22,1},  // CAN2 (connectivity line only)
		{23,1},  // MII_RMII (connectivity line only)
		{18,1},  // ADC1_ETRGINJ
		{19,1},  // ADC1_ETRGREG
		{20,1},  // ADC2_ETRGINJ
		{21,1},  // ADC2_ETRGREG (same bit as ETH, different sub-models)
		{29,1},  // TIM2ITR1 (connectivity line only)
		{30,1},  // PTP_PPS (connectivity line only)
		// MAPR2 single-bit
		{5, 1},  // TIM9 (HD value line / XL only)
		{6, 1},  // TIM10 (HD value line / XL only)
		{7, 1},  // TIM11 (HD value line / XL only)
		{12,1},  // TIM12 (HD value line only)
		{8, 1},  // TIM13
		{9, 1},  // TIM14
		{0, 1},  // TIM15
		{1, 1},  // TIM16
		{2, 1},  // TIM17
		{3, 1},  // CEC
		{4, 1},  // TIM1DMA
		{11,1},  // TIM67DACDMA
		{10,1},  // FSMC_NADV
		{13,1},  // MISC (HD value line only)
		// MAPR multi-bit fields
		{4, 2},  // USART3: 0=NoRemap, 1=Partial, 2=Full
		{6, 2},  // TIM1:   0=NoRemap, 1=Partial, 3=Full
		{8, 2},  // TIM2:   0=NoRemap, 1=Partial1, 2=Partial2, 3=Full
		{10,2},  // TIM3:   0=NoRemap, 2=Partial, 3=Full
		{13,2},  // CAN1:   0=Remap1(PA11), 2=Remap2(PB8), 3=Remap3(PD0)
		// SWJ special
		{24,3},  // SWJ:    0=Full, 1=NoJNTRST, 2=NoJTAG, 4=Disable
	};

	static constexpr int _REMAP_MAPR2_START = 17;// index where MAPR2 entries begin
	static constexpr int _REMAP_MULTIBIT_START = 32;// index where multi-bit entries begin

	void AFIO_Remap(AFIORemap remap, bool enable) {
		int idx = (int)remap;
		auto& e = _remap_tbl[idx];
		bool is_mapr2 = idx >= _REMAP_MAPR2_START && idx < _REMAP_MULTIBIT_START;
		Reference& reg = is_mapr2 ? AFIO::MapReg2 : AFIO::MapReg;
		if (enable) reg.setof(e.pos);
		else        reg.rstof(e.pos);
	}

	void AFIO_RemapConfig(AFIORemap remap, byte value) {
		int idx = (int)remap;
		auto& e = _remap_tbl[idx];
		bool is_mapr2 = idx >= _REMAP_MAPR2_START;
		Reference& reg = is_mapr2 ? AFIO::MapReg2 : AFIO::MapReg;
		reg.maset(e.pos, e.wid, value);
	}

	// ---- EVENTOUT (Cortex EVENTOUT via AFIO_EVCR) ----
	void EVENTOUT_Config(SelfGpin pin) { AFIO::EventCtrlReg.maset(0, 7, (pin.getParent().getID() << 4) | pin.getID()); }
	void EVENTOUT_Enable()  { AFIO::EventCtrlReg.setof(7); }
	void EVENTOUT_Disable() { AFIO::EventCtrlReg.rstof(7); }

#elif defined(_MCU_STM32F4x)

	Reference SYSCFG::MEMRM(_SYSCFG_ADDR);
	Reference SYSCFG::PMC(_SYSCFG_ADDR + 0x04);
	Reference SYSCFG::ExternInterruptCfgs[4] = {
		Reference(_SYSCFG_ADDR + 0x08),
		Reference(_SYSCFG_ADDR + 0x0C),
		Reference(_SYSCFG_ADDR + 0x10),
		Reference(_SYSCFG_ADDR + 0x14)
	};
	Reference SYSCFG::CMPCR(_SYSCFG_ADDR + 0x20);

#endif
}

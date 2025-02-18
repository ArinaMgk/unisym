// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Partial) RCC PLL Control
// Codifiers: @dosconio: 20241116
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

// inner included file ----

#if 0

#elif defined(_MCU_STM32F4x)

bool RCCPLL::isReady() {
	return RCC[CR].bitof(_RCC_CR_POSI_PLLReady);
}

void RCCPLL::enAble(bool ena) {
	Reference Conreg = RCC[CR];
	Conreg.setof(_RCC_CR_POSI_PLLEnable, ena);
	while (Conreg.bitof(_RCC_CR_POSI_PLLReady) ^ ena); // wait for PLL locked
}

bool RCCPLL::setMode() {
	using namespace RCCReg;
	// part PLL Configuration of HAL_RCC_OscConfig
	// if ((RCC_OscInitStruct->PLL.PLLState) != RCC_PLL_NONE) call this
	bool enabl_it = true;// if ((RCC_OscInitStruct->PLL.PLLState) == RCC_PLL_ON)
	if (RCC.Sysclock.CurrentSource() == SysclkSource::PLL) {
		return false;//{TEMP}
	}
	if (enabl_it)
	{
		enAble(false);
		stduint _m = 8;
		stduint _p = 0x00000002U;//aka RCC_PLLP_DIV2
		stduint _n = (168 / 8 * _p * _m);// = 336;
		stduint _q = 7;
		RCC[PLLCFGR] = (_TEMP PLLSource::HSE |
			(_m << _RCC_PLLCFGR_POSI_PLLM) |
			(_n << _RCC_PLLCFGR_POSI_PLLN) |
			(((_p >> 1U) - 1U) << _RCC_PLLCFGR_POSI_PLLP) |
			(_q << _RCC_PLLCFGR_POSI_PLLQ));
		enAble();
	}
	else
	{
		enAble(false);// aka __HAL_RCC_PLL_DISABLE();
		//{TODO} while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLLRDY) != RESET);
	}
	return true;
}

#elif defined(_MCU_STM32H7x)

stduint RCCPLL::getFrequency_ToCore() const {
	uint32_t pllp = 2, pllsource = 0, pllm = 2, pllfracen = 0, hsivalue = 0;
	float fracn1, pllvco = 0;
	// PLL_VCO = (HSE_VALUE or HSI_VALUE or CSI_VALUE/ PLLM) * PLLN
	// SYSCLK = PLL_VCO / PLLR
	Stdfield PLLCKSELR_DIVM1(RCC[PLLCKSELR], 4, 6);
	Stdfield PLL1FRACR_FRACN1(RCC[PLL1FRACR], 3, 13);
	pllm = _IMM(PLLCKSELR_DIVM1);
	pllfracen = RCC[PLLCFGR].bitof(0); // RCC_PLLCFGR_PLL1FRACEN
	fracn1 = pllfracen * _IMM(PLL1FRACR_FRACN1);;
	Stdfield PLL1DIVR_N1(RCC[PLL1DIVR], 0, 9);
	Stdfield PLL1DIVR_P1(RCC[PLL1DIVR], 9, 7);
	// Stdfield PLL1DIVR_Q1(RCC[PLL1DIVR], 16, 7);
	// Stdfield PLL1DIVR_R1(RCC[PLL1DIVR], 24, 7);
	stduint freq = 0;
	switch (CurrentSource()) {
	case PLLSource::HSI:
		freq = RCC.HSI.getFrequency_ToCore(); break;
	case PLLSource::HSE:
		freq = RCC.HSE.getFrequency(); break;
	default:
	case PLLSource::CSI:
		freq = RCC.CSI.getFrequency(); break;
	}
	pllvco = (freq / pllm) * (_IMM(PLL1DIVR_N1) + (fracn1 / 0x2000) + 1);
	return SystemCoreClock = (pllvco / (1 + _IMM(PLL1DIVR_P1)));
}

PLLSource::PLLSource RCCPLL::CurrentSource() const {
	return PLLSource::PLLSource(RCC[PLLCKSELR] & 0b11);
}



#elif defined(_MPU_STM32MP13)
const static RCCReg::RCCReg PLLxCR[4] = {
	RCCReg::PLL1CR, RCCReg::PLL2CR, RCCReg::PLL3CR, RCCReg::PLL4CR};
const static RCCReg::RCCReg PLLxCFGR1[4] = {
	RCCReg::PLL1CFGR1, RCCReg::PLL2CFGR1,
	RCCReg::PLL3CFGR1, RCCReg::PLL4CFGR1 };
const static RCCReg::RCCReg PLLxCFGR2[4] = {
	RCCReg::PLL1CFGR2, RCCReg::PLL2CFGR2,
	RCCReg::PLL3CFGR2, RCCReg::PLL4CFGR2 };
const static RCCReg::RCCReg PLLxFRACR[4] = {
	RCCReg::PLL1FRACR, RCCReg::PLL2FRACR,
	RCCReg::PLL3FRACR, RCCReg::PLL4FRACR };
const static RCCReg::RCCReg PLLxCSGR[4] = {
	RCCReg::PLL1CSGR, RCCReg::PLL2CSGR,
	RCCReg::PLL3CSGR, RCCReg::PLL4CSGR };
const static RCCReg::RCCReg RCKxSELR[4] = {
	RCCReg::RCK12SELR, RCCReg::RCK12SELR,
	RCCReg::RCK3SELR, RCCReg::RCK4SELR };
const static stduint _RST_PLLxCFGR1[4] = {
	0x00010031U, 0x00010063U, 0x00010031U, 0x00010031U
};
const static stduint _RST_PLLxCFGR2[4] = {
	0x00010100U, 0x00010101U, 0x00010101U, 0x00000000U
};

void RCCPLL::enAble(bool ena) const {
	using namespace RCCReg;
	RCC[PLLxCR[getID() - 1]].setof(0, ena);// PLLON
}

bool RCCPLL::isUsed() const {
	if (getID() == 1) {
		SysclkSource::RCCSysclockSource src = RCCSystemClock::CurrentSource();
		bool tmp = src == SysclkSource::PLL1 || src == SysclkSource::MPUDIV;
		asserv(tmp) = RCCSystemClock::isReady();
		return tmp;
	}
	else if (getID() == 2) {
		bool axis_used = RCC.AXIS.getSource() == AxisSource::PLL2;
		asserv(axis_used) = RCC.AXIS.isReady();
		return axis_used;
	}
	else if (getID() == 3) {
		bool mlahb_used = RCC.MLAHB.getSource() == MLAHBSource::PLL3;
		asserv(mlahb_used) = RCC.MLAHB.isReady();
		return mlahb_used;
	}
	return false;
}

bool RCCPLL::isReady() const {
	using namespace RCCReg;
	return RCC[PLLxCR[getID() - 1]].bitof(1);// PLLxRDY
}

static void setModeSub(const RCCPLL& sel, PLLMode mod) {
	byte id = sel.getID() - 1;
	if (mod == PLLMode::SpreadSpectrum) {
		// check valid of INC_STEP, SSCG_MODE, RPDFN_DIS, TPDFN_DIS, MOD_PER
		//{} __HAL_RCC_PLL1CSGCONFIG(pll1->MOD_PER, pll1->TPDFN_DIS, pll1->RPDFN_DIS, pll1->SSCG_MODE, pll1->INC_STEP);
	}
	RCC[PLLxCR[id]].setof(2, mod == PLLMode::SpreadSpectrum);// SSCG_CTRL AKA __HAL_RCC_PLL1/2_SSMODE_xABLE
	sel.enAble(true); while (true != sel.isReady());
	RCC[PLLxCR[id]].maset(4, 3, 0b111);// AKA __HAL_RCC_PLLxCLKOUT_ENABLE, 0b111 for MSB{R Q P}LSB
}
bool RCCPLL::setMode(PLLMode mod, PLL1Source::PLL1SourceType src, PLLPara_t para) const {
	byte id = getID() - 1;
	// range of N depends on whether Integer/Fractional
	if (getID() == 2) { // PLL2
		// Check if PLL1 set
		PLL1Source::PLL1SourceType crtsrc = (PLL1Source::PLL1SourceType)CurrentSource();
		if (isUsed() || (crtsrc != PLL1Source::HSE && crtsrc != PLL1Source::HSI))
			return false;
	}
	if (id == 0 || id == 1) {
		if (!canMode(false)) return false;
		if (!setSource(_IMM(src))) return false;
		setPara(para);
		// In integer or clock spreading mode the application shall ensure that a 0 is loaded into the SDM
		if (mod == PLLMode::Integer || mod == PLLMode::SpreadSpectrum)
			para.frac = nil;
		ConfigFracv(para.frac);
		setModeSub(self, mod);
	}
	else return false;
	return true;
}
bool RCCPLL::setMode(PLLMode mod, PLL3Source::PLL3SourceType src, bool if_range, PLLPara_t para) const {
	byte id = getID() - 1;
	if (id == 2 || id == 3) {
		if (!canMode(false)) return false;
		if (!setSource(_IMM(src))) return false;
		RCC[PLL3CFGR1].maset(24, 2, if_range ? 1 : 0); // IFRGE AKA __HAL_RCC_PLL3/4_IFRANGE
		setPara(para);
		// In integer or clock spreading mode the application shall ensure that a 0 is loaded into the SDM
		if (mod == PLLMode::Integer || mod == PLLMode::SpreadSpectrum)
			para.frac = nil;
		ConfigFracv(para.frac);
		setModeSub(self, mod);
	}
	else return false;
	return true;
}

bool RCCPLL::canMode(bool reset) const {
	using namespace RCCReg;
	byte id = getID() - 1;
	if (reset) {
		Reference cr = RCC[PLLxCR[id]];
		cr.maset(4, 3, 0);// DIV P/Q/R EN
		enAble(false); while (false != isReady());
		RCC[PLLxCR[id]].setof(2, false);// SSCG_CTRL
		//
		RCC[RCKxSELR[id]].maset(0, 2, 0);// SRC
		RCC[PLLxCFGR1[id]] = _RST_PLLxCFGR1[id];
		RCC[PLLxCFGR2[id]] = _RST_PLLxCFGR2[id];
		RCC[PLLxFRACR[id]] = 0;
		RCC[PLLxCSGR[id]] = 0;
		return true;
	}
	else if (!isUsed()) {
		RCC[PLLxCR[id]].maset(4, 3, nil);// AKA __HAL_RCC_PLLxCLKOUT_DISABLE(DIVP|DIVQ|DIVR)
		enAble(false); while (false != isReady());
		return true;
	}
	return false;
}


void RCCPLL::setPara(PLLPara_t para) const {
	byte id = getID() - 1;
	RCC[PLLxCFGR1[id]].maset(16, 6, para.m - 1);// DIVM
	RCC[PLLxCFGR1[id]].maset(0, 9, para.n - 1);// DIVN
	RCC[PLLxCFGR2[id]].maset(0, 7, para.p - 1);// DIVP
	RCC[PLLxCFGR2[id]].maset(8, 7, para.q - 1);// DIVQ
	RCC[PLLxCFGR2[id]].maset(16, 7, para.r - 1);// DIVR
}
void RCCPLL::ConfigFracv(stduint frac) const {
	byte id = getID() - 1;
	// AKA __HAL_RCC_PLL1FRACV_DISABLE
	RCC[PLLxFRACR[id]].setof(16, false);// FRACLE
	// AKA __HAL_RCC_PLL1FRACV_CONFIG
	RCC[PLLxFRACR[id]].maset(3, 13, frac);// FRACV
	// AKA __HAL_RCC_PLL1FRACV_ENABLE
	RCC[PLLxFRACR[id]].setof(16, true);// FRACLE
}

stduint RCCPLL::CurrentSource() const {
	using namespace RCCReg;
	static RCCReg::RCCReg RCKxSELR[4] = {
		RCK12SELR, RCK12SELR, RCK3SELR, RCK4SELR
	};
	if (Ranglin(getID(), 1, 4)) {
		return RCC[RCKxSELR[getID() - 1]].masof(0, 2);// PLLxSRC
	}
	return nil;
}
bool RCCPLL::setSource(stduint src) const {
	byte id = getID() - 1;
	if (!Ranglin(id, 0, 4)) return false;
	// Do not change pll src if already in use
	if ((id == 0 && RCC.PLL2.isUsed()) || (id == 1 && RCC.PLL1.isUsed())) { // PLL1&2
		if (CurrentSource() != (src))
			return false;
	}
	else {
		RCC[RCKxSELR[id]].maset(0, 2, (src));// PLLxSRC
		while (true != RCC[RCKxSELR[id]].bitof(31));// PLLxSRCRDY
	}
	return true;
}


float32 RCCPLL::getVCO() const {
	byte id = getID() - 1;
	float32 pll_m = RCC[PLLxCFGR1[id]].masof(16, 6) + 1;// DIVM1
	bool pll1fracen = RCC[PLLxFRACR[id]].bitof(16);// FRACLE
	float32 fracn1_us = pll1fracen ? RCC[PLLxFRACR[id]].masof(3, 13) : 0;// FRACV
	float32 DIVN = RCC[PLLxCFGR1[id]].masof(0, 9) + 1;
	float32 pll_vco = DIVN + (fracn1_us / (float32)0x2000);// Intermediary value
	switch (PLL1Source::PLL1SourceType(CurrentSource())) {
	case PLL1Source::HSI:
		pll_vco *= (float32)RCC.HSI.getFrequency() / pll_m;
		break;
	case PLL1Source::HSE:
		pll_vco *= (float32)HSE_VALUE / pll_m;
		break;
	case PLL1Source::OFF: default:
		pll_vco = 0;
		break;
	}
	return pll_vco;
}
stduint RCCPLL::getFrequencyP() const {
	using namespace RCCReg;
	stduint tmp = 0;
	byte id = getID() - 1;
	if (Ranglin(getID(), 1, 4)) { // no for PLL 3 4
		const stduint P_Div = RCC[PLLxCFGR2[id]].masof(0, 7) + 1;
		return getVCO() / (float32)P_Div;// aka PLL1/2_Clocks->PLL1_P/Q/R_Frequency
	}
	return 0;
}

stduint RCCPLL::getFrequencyQ() const {
	using namespace RCCReg;
	stduint tmp = 0;
	byte id = getID() - 1;
	if (Ranglin(getID(), 1, 4)) { // no for PLL 3 4
		const stduint Q_Div = RCC[PLLxCFGR2[id]].masof(8, 7) + 1;
		return getVCO() / (float32)Q_Div;// aka PLL1/2_Clocks->PLL1_P/Q/R_Frequency
	}
	return 0;
}

stduint RCCPLL::getFrequencyR() const {
	using namespace RCCReg;
	stduint tmp = 0;
	byte id = getID() - 1;
	if (Ranglin(getID(), 1, 4)) { // no for PLL 3 4
		const stduint R_Div = RCC[PLLxCFGR2[id]].masof(16, 7) + 1;
		return getVCO() / (float32)R_Div;// aka PLL1/2_Clocks->PLL1_P/Q/R_Frequency
	}
	return 0;
}

#endif

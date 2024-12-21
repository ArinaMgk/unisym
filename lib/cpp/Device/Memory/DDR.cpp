// UTF-8 CPP-ISO20 TAB4 CRLF
// Docutitle: (Device) Double Data Rate (DDR) Memory
// Codifiers: @dosconio: 20241213 ~ <Last-check> 
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

#include "../../../../inc/cpp/Device/DDR"
#include "../../../../inc/c/driver/SRAM.h"
#include "../../../../inc/cpp/Device/_Power.hpp"
#include "../../../../inc/cpp/Device/RCC/RCC"
#include "../../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"
#include "../../../../inc/c/ustring.h"

#define re(x) Reference(&localcfg.x)

inline static stduint loc_getTimeout(stduint us) { // AKA ddr_timeout_init_us
	return us * (SystemCoreClock / 2000000);
}
inline static void loc_delayus(stduint us) { // AKA ddr_delay_us
	volatile stduint loop_times = loc_getTimeout(us);
	while (loop_times--);
}

#ifdef _MPU_STM32MP13
#include "../../../../inc/cpp/Device/RAM/DDRDefault.hpp"
#include "../../../../inc/cpp/Device/RAM/DDRLoading.hpp"
#include "../../../../inc/c/proctrl/ARM/cortex_a7.h"

#define _IMM32P(x) ((uint32*)_IMM(x))
#define DDR_BASE_ADDR (uint32*)0xC0000000
#define DDR_MAX_SIZE 0x40000000
#define DDR_TRAINING_AREA_SIZE 64U


// Start quasi dynamic register update
#define start_sw_done() self[DDRControl::SWCTL].rstof(0/*SW_DONE*/)

_TEMP stduint _dbg_reg;

namespace uni {
	DDR_t DDR;

	Reference DDR_t::operator[](DDRControl idx) const {
		return APB4_PERIPH_BASE + 0x3000 + _IMMx4(idx);
	}
	Reference DDR_t::operator[](DDRPerform idx) const {
		return APB4_PERIPH_BASE + 0x7000 + _IMMx4(idx);
	}
	Reference DDR_t::operator[](DDRPhyziks idx) const {
		return APB4_PERIPH_BASE + 0x4000 + _IMMx4(idx);
	}

	#define impl(x) x DDR_t

	
	/**
	 * * reset/clock/power management (i.e. access to other IPs),
	 * * DDRCTRL and DDRPHY configuration and initialization,
	 * * self-refresh mode setup,
	 * * data/addr tests execution after training.
	 */
	#define isDDR3 (re(c_reg.MSTR).bitof(0))// DDRCTRL_MSTR_DDR3
	bool DDR_t::setMode(DDRType typ, DDR_Size siz) {
		using namespace RCCReg;
		//
		if (typ != DDRType::DDR3 || siz != DDRSize_256M) _TEMP return false;
		_TEMP localcfg.c_map[6] = 0x0F0F0707U;
		//
		self_refresh = wakeup_from_standby;
		RCC[DDRITFCR].setof(8, false);// AXIDCGEN, Disable axidcg clock gating during init
		stduint bus_width;
		_dbg_reg = re(c_reg.MSTR);
		switch (re(c_reg.MSTR).masof(12, 2)) {// DDRCTRL_MSTR_DATA_BUS_WIDTH [MP135D]
		case 0b10: bus_width = 0x08; break;
		case 0b01: bus_width = 0x10; break;
		default: bus_width = 0x20; break;
		}
		_dbg_reg = localcfg.c_reg.MSTR;
		// [ using bitposi of mp135d
		// - STM32MP_DDR3
		// - STM32MP_LPDDR2_32 STM32MP_LPDDR2_16
		// - STM32MP_LPDDR3_32 STM32MP_LPDDR3_16
		//{TODO} Compared with typ
		if (isDDR3);
		else if (re(c_reg.MSTR).bitof(2));// DDRCTRL_MSTR_LPDDR2
		else if (re(c_reg.MSTR).bitof(3));//DDRCTRL_MSTR_LPDDR3
		else return false;
		// ]
		// Check DDR PHY pads retention
		bool ddr_reten = PWR[PWRReg::CR3].bitof(12);// DDRRETEN
		if (self_refresh) {
			if (!ddr_reten) self_refresh = false;
		}
		else if (ddr_reten) {
			PWR[PWRReg::CR3].setof(11, true);// DDRSRDIS
			PWR[PWRReg::CR3].setof(12, false);// DDRRETEN
		}
		// ---- 1. Program the DWC_ddr_umctl2 registers ----
		// : check DFIMISC.dfi_init_complete = 0
		// ---- ---- 1.1 RESETS: presetn, core_ddrc_rstn, aresetn ---- ----
		{
			using namespace RCCReg;
			RCC[DDRITFCR].setof(14);// DDRCAPBRST
			RCC[DDRITFCR].setof(15);// DDRCAXIRST
			RCC[DDRITFCR].setof(16);// DDRCORERST
			RCC[DDRITFCR].setof(17);// DPHYAPBRST
			RCC[DDRITFCR].setof(18);// DPHYRST
			RCC[DDRITFCR].setof(19);// DPHYCTLRST
		}
		// ---- ---- 1.2 ---- ----
		asrtret(ddr_clk_enable(localcfg.info.speed));
		// ---- ---- 1.3 deassert reset ---- ----
		{
			using namespace RCCReg;
			// De-assert PHY rstn and ctl_rstn via DPHYRST and DPHYCTLRST.
			RCC[DDRITFCR].rstof(18);// DPHYRST
			RCC[DDRITFCR].rstof(19);// DPHYCTLRST
			// De-assert presetn once the clocks are active and stable via DDRCAPBRST bit.
			RCC[DDRITFCR].rstof(14);// DDRCAPBRST
		}
		// ---- ---- 1.4 wait 128 cycles to permit initialization of end logic ---- ----
		loc_delayus(2);// 2us. For PCLK = 133MHz => 1 us is enough, 2 to allow lower frequency
		// ---- ---- 1.5 initialize registers ddr_umctl2 ---- ----
		{
			// Stop uMCTL2 before PHY is ready
			DDR[DDRControl::DFIMISC].setof(0, false);// DFI_INIT_COMPLETE_EN
			for0a(i, ListLoadControl)
				DDR[ListLoadControl[i]] = _IMM32P(&localcfg.c_reg)[i];
			if (isDDR3 && re(c_reg.MSTR).bitof(15))// DDR3 = don't set DLLOFF for init mode, reset it
				re(c_reg.MSTR).rstof(15);// DLL_OFF_MODE
			for0a(i, ListLoadTiming)
				DDR[ListLoadTiming[i]] = _IMM32P(&localcfg.c_timing)[i];
			for0a(i, ListLoadMapping)
				DDR[ListLoadMapping[i]] = _IMM32P(&localcfg.c_map[1])[i];
			if (self_refresh) self[DDRControl::PWRCTL].setof(5);// SELFREF_SW
			self[DDRControl::INIT0].maset(30, 2, 0b01);// SKIP_DRAM_INIT -> NORMAL, Skip CTRL init, SDRAM init is done by PHY PUBL
			for0a(i, ListLoadPerform) if (!Ranglin(i, 6, 2)) // skip [7] and [8]
				DDR[ListLoadPerform[i]] = _IMM32P(&localcfg.c_perf)[i];
		}
		// ---- ---- 2 deassert reset signal core_ddrc_rstn, aresetn and presetn ---- ----
		{
			using namespace RCCReg;
			RCC[DDRITFCR].rstof(16);// DDRCORERST
			RCC[DDRITFCR].rstof(15);// DDRCAXIRST
			RCC[DDRITFCR].rstof(17);// DPHYAPBRST
		}
		// ---- 3 start PHY init by accessing relevant PUBL registers(DXGCR, DCR, PTR*, MR*, DTPR*) ----
		for0a(i, ListLoadFizical)
			DDR[ListLoadFizical[i]] = _IMM32P(&localcfg.p_reg)[i];
		for0a(i, ListLoadFizTime)
			DDR[ListLoadFizTime[i]] = _IMM32P(&localcfg.p_timing)[i];
		if (isDDR3 && re(c_reg.MSTR).bitof(15))
			self[DDRPhyziks::MR1].rstof(0);// DE, field of DDR3
		// ---- 4 Monitor PHY init status by polling PUBL register PGSR.IDONE ----
		// Perform DDR PHY DRAM initialization and Gate Training Evaluation
		asrtret(ddrphy_idone_wait());



		
		return true;


		// ---- 5 Indicate to PUBL that controller performs SDRAM initialization ----
		// by setting PIR.INIT and PIR CTLDINIT and pool PGSR.IDONE
		// DRAM init is done by PHY, init0.skip_dram.init = 1
		{
			Reflocal(pir) = 0;
			pir.setof(1);// DLLSRST
			pir.setof(2);// DLLLOCK
			pir.setof(3);// ZCAL
			pir.setof(4);// ITMSRST
			pir.setof(6);// DRAMINIT
			pir.setof(16);// ICPC
			pir.setof(5, isDDR3);// DRAMRST
			if (self_refresh) { // Treat self-refresh exit : hot boot
				// DDR in self refresh mode, remove zcal & reset & init
				pir.setof(3, false);// ZCAL
				pir.setof(5, false);// DRAMRST
				pir.setof(6, false);// DRAMINIT
				pir.setof(30);// ZCALBYP
			}
			asrtret(ddrphy_init(pir));
			if (self_refresh) asrtret(self_refresh_zcal(zdata));
		}
		// ---- 6 SET DFIMISC.dfi_init_complete_en to 1 Enable quasi-dynamic register programming ----
		start_sw_done();
		DDR[DDRControl::DFIMISC].setof(0, true);// DFI_INIT_COMPLETE_EN
		asrtret(wait_sw_done_ack());
		// ---- 7 Wait for DWC_ddr_umctl2 to move to normal operation mode by monitoring STAT.operating_mode signal ----
		// Wait uMCTL2 ready
		// Trigger self-refresh exit
		if (self_refresh) self[DDRControl::PWRCTL].setof(5, false);// SELFREF_SW

		// ---- 8 ----
		// ---- 9 ----
		// ---- 10 ----


		
		return true;
	}

	void DDR_t::enClock(bool ena) const {
		using namespace RCCReg;
		// _TEMP for ena == true
		// 0(DDRC1EN), 4(DDRPHYCEN), 9(DDRPHYCAPBEN), 6(DDRCAPBEN), ?(DDRC2EN: def-DDR_DUAL_AXI_PORT)
		RCC[DDRITFCR] |= _IMM1S(0) | _IMM1S(4) | _IMM1S(9) | _IMM1S(6);
	}
	
	bool DDR_t::ddr_clk_enable(stduint spd) {
		enClock(true);
		stduint mem_speed_hz = spd * 1000U;
		stduint ddrphy_clk = getFrequency();
		stduint ddr_clk = ABS(ddrphy_clk, mem_speed_hz);
		// Max 10% frequency delta
		return ddr_clk <= (mem_speed_hz / 10);
	}

	bool DDR_t::ddrphy_idone_wait() {
		stduint timeout = loc_getTimeout(50);// us
		stduint&& errmask = 0b1111100000;
		stduint err = 0;
		stduint pgsr;
		Reference _pgsr _IMM(&pgsr);
		do {
			asrtret(timeout--);
			pgsr = self[DDRPhyziks::PGSR];
			if (pgsr & errmask) err++;
		} while (!_pgsr.bitof(0) && !err);// IDONE
		return true;
	}
	bool DDR_t::wait_sw_done_ack() {
		stduint timeout = loc_getTimeout(1);// us
		self[DDRControl::SWCTL].setof(0/*SW_DONE*/);
		do {
			asrtret(timeout--);
		} while (!self[DDRControl::SWSTAT].bitof(0/*SW_DONE_ACK*/));
		return true;
	}

	bool DDR_t::ddrphy_init(Reference& locref) {
		locref.setof(0);// INIT
		self[DDRPhyziks::PIR] = _IMM(locref);
		loc_delayus(10);// 10us as DDR_DELAY_PHY_INIT_US, before start polling
		return ddrphy_idone_wait();// Wait DRAM initialization and Gate Training Evaluation complete
	}

	stduint DDR_t::getFrequency() const {
		return RCC.PLL2.getFrequencyR();
	}

	bool DDR_t::self_refresh_zcal(stduint _zdata) {
		// sequence for PUBL I/O Data Retention during Power-Down
		// ---- 10 Override ZQ calibration with previously (pre-retention) ----
		// calibrated values. This is done by writing 1 to ZQ0CRN.ZDEN and the override data to ZQ0CRN.ZDATA.
		self[DDRPhyziks::ZQ0CR0].setof(28);// ZDEN
		self[DDRPhyziks::ZQ0CR0].maset(0, 20, _zdata);// ZDATA
		// ---- 11 De-assert the PHY_top data retention enable signals (ret_en or ret_en_i/ret_en_n_i). ----
		PWR[PWRReg::CR3].setof(11, true);// DDRSRDIS
		PWR[PWRReg::CR3].setof(12, false);// DDRRETEN
		// ---- 12 Remove ZQ calibration override by writing 0 to ZQ0CRN.ZDEN ----
		self[DDRPhyziks::ZQ0CR0].setof(28, false);// ZDEN
		// ---- 13 Trigger ZQ calibration by writing 1 to PIR.INIT and '1' to PIR.ZCAL ----
		// ---- 14 Wait for ZQ calibration to finish by polling a 1 status on PGSR.IDONE. ----
		Reflocal(tmp) = 0x00000008;// DDRPHYC_PIR_ZCAL
		return ddrphy_init(tmp);
	}

	impl(void)::save_ddr_training_area() {
		using namespace RCCReg;
		RCC[MP_AHB5ENSETR] = _IMM1S(8);// BKPSRAMEN
		MemCopyN((uint32_t*)BKPSRAM.getAddress(), DDR_BASE_ADDR, DDR_TRAINING_AREA_SIZE);
		__DSB();
		RCC[MP_AHB5ENCLRR] = _IMM1S(8);// BKPSRAMEN
	}
	impl(void)::restore_ddr_training_area() {
		using namespace RCCReg;
		RCC[MP_AHB5ENSETR] = _IMM1S(8);// BKPSRAMEN
		MemCopyN(DDR_BASE_ADDR, (uint32_t*)BKPSRAM.getAddress(), DDR_TRAINING_AREA_SIZE);
		__DSB();
		RCC[MP_AHB5ENCLRR] = _IMM1S(8);// BKPSRAMEN
	}

	impl(bool)::wait_operating_mode(stduint mode) {
		// node: 0bXXX
		stduint timeout = loc_getTimeout(1);// us
		stduint&& DDRCTRL_STAT_OPERATING_MODE_NORMAL = 0b001;
		stduint&& DDRCTRL_STAT_OPERATING_MODE_SR = 0b011;
		stduint&& DDRCTRL_STAT_SELFREF_TYPE_SR = 0b10;
		stduint&& DDRCTRL_STAT_SELFREF_TYPE_ASR = 0b11;
		while (true) {
			stduint operating_mode = self[DDRControl::STAT].masof(0, 3);// OPERATING_MODE
			stduint selref_type = self[DDRControl::STAT].masof(4, 2);// SELFREF_TYPE
			asrtret(timeout--);
			if (DDRCTRL_STAT_OPERATING_MODE_SR == mode) {
				// Self-refresh due to software => checking also STAT.selfref_type.
				if (operating_mode == DDRCTRL_STAT_OPERATING_MODE_SR && selref_type == DDRCTRL_STAT_SELFREF_TYPE_SR)
					break;
			}
			else if (operating_mode == mode) break;
			else if (mode == DDRCTRL_STAT_OPERATING_MODE_NORMAL &&
				operating_mode == DDRCTRL_STAT_OPERATING_MODE_SR &&
				selref_type == DDRCTRL_STAT_SELFREF_TYPE_ASR)
				break;// Normal mode: handle also automatic self refresh
		}
		return true;
	}

}
#endif


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
#include "../../../../inc/cpp/Device/SysTick"
#include "../../../../inc/c/driver/SRAM.h"
#include "../../../../inc/cpp/Device/_Power.hpp"
#include "../../../../inc/cpp/Device/RCC/RCC"
#include "../../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"
#include "../../../../inc/c/ustring.h"

#define re(x) Reference(&localcfg.x)

#ifdef _MPU_STM32MP13
#include "../../../../inc/cpp/Device/RAM/DDRDefault.hpp"
#include "../../../../inc/cpp/Device/RAM/DDRLoading.hpp"
#include "../../../../inc/c/proctrl/ARM/cortex_a7.h"

#define _IMM32P(x) ((uint32*)_IMM(x))
#define DDR_BASE_ADDR (uint32*)0xC0000000
#define DDR_MAX_SIZE 0x40000000
#define DDR_TRAINING_AREA_SIZE 64U
#define SCTLR_C_BIT _IMM1S(2)


// Start quasi dynamic register update
#define start_sw_done() self[DDRControl::SWCTL].rstof(0/*SW_DONE*/)
inline static stduint loc_getTimeout(stduint us) { // AKA ddr_timeout_init_us
	return us * (SystemCoreClock / 2000000);
}
inline static void loc_delayus(stduint us) { // AKA ddr_delay_us
	volatile stduint loop_times = loc_getTimeout(us);
	while (loop_times--);
}

namespace uni {
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
		switch (re(c_reg.MSTR).masof(12, 2)) {// DDRCTRL_MSTR_DATA_BUS_WIDTH [MP135D]
		case 0b10: bus_width = 0x08; break;
		case 0b01: bus_width = 0x10; break;
		default: bus_width = 0x20; break;
		}
		//
		//{TODO} HERE do the AKA HAL_DDR_MspInit passing typ to it
		//
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
		stduint&& DDRCTRL_STAT_OPERATING_MODE_NORMAL = 0b001;
		asrtret(wait_operating_mode(DDRCTRL_STAT_OPERATING_MODE_NORMAL));
		// Switch to DLL OFF mode
		if (re(c_reg.MSTR).bitof(15))// DLL_OFF_MODE
			asrtret(ddr3_dll_off());
		// DDR DQS training done
		stduint ddrinit_time = SysTick::getTickPhysical() - (0);
		// ---- 8 Disable Auto refresh and power down by setting ----
		//   - RFSHCTL3.dis_au_refresh = 1
		//   - PWRCTL.powerdown_en = 0
		//   - DFIMISC.dfiinit_complete_en = 0
		asrtret(setRefresh(false));
		// ---- 9 Program PUBL PGCR to enable refresh during training and rank to train ----
		// not done => keep the programmed value in PGCR
		// ---- 10 configure PUBL PIR register to specify which training step to run ----
		// RVTRN is executed only on LPDDR2/LPDDR3
		{
			Reflocal(pir) = 0;
			pir.setof(7);// QSTRN
			if (!isDDR3) pir.setof(8);// RVTRN
			asrtret(ddrphy_init(pir));
		}
		// ---- 11 monitor PUB PGSR.IDONE to poll cpmpletion of training sequence ----
		asrtret(ddrphy_idone_wait());
		if (self_refresh) asrtret(refresh_compensation(ddrinit_time)); // Refresh compensation: forcing refresh command
		// ---- 12 set back registers in step 8 to the original values if desidered ----
		asrtret(setRefresh(true, localcfg.c_reg.RFSHCTL3, localcfg.c_reg.PWRCTL));
		enable_axi_port();
		RCC[DDRITFCR].setof(8, true);// Enable axidcg clock gating
		// check if DDR content is lost (self-refresh aborted)
		if (wakeup_from_standby && !self_refresh) clear_bkp = true;
		//{TEMP} OPT : ndef-DCACHE_OFF
		__set_SCTLR(__get_SCTLR() & ~SCTLR_C_BIT);
		// END OPT : ndef-DCACHE_OFF
		if (self_refresh) {
			asrtret(ddr_test_rw_access(self));
			restore_ddr_training_area();
		}
		else {
			asrtret(ddr_test_data_bus(self));
			asrtret(ddr_test_addr_bus(self));
			asrtret(ddr_check_size(self) >= localcfg.info.size);
		}
		asrtret(setModeSR(ddr_sr_read_mode()));// Switch to Self-Refresh mode defined by the settings
		//{TEMP} OPT : ndef-DCACHE_OFF
		__set_SCTLR(__get_SCTLR() | SCTLR_C_BIT);
		// END OPT : ndef-DCACHE_OFF
		return true;
	}
	

}

#endif

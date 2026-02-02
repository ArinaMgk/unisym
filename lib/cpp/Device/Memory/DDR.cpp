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
#include "../../../../inc/cpp/Device/RCC/RCC"
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
#include "../../../../inc/c/proctrl/ARM/cortex_a7.h"

#define _IMM32P(x) ((uint32*)_IMM(x))
#define DDR_BASE_ADDR (uint32*)0xC0000000
#define DDR_MAX_SIZE 0x40000000
#define DDR_TRAINING_AREA_SIZE 64U
#define ddr_supports_ssr_asr() self[DDRControl::MSTR].bitof(2) // LPDDR2

// Start quasi dynamic register update
#define start_sw_done() self[DDRControl::SWCTL].rstof(0/*SW_DONE*/)

namespace uni {
	DDR_t DDR;

	extern DDRConfig localcfg;

	Reference DDR_t::operator[](DDRControl idx) const {
		return APB4_PERIPH_BASE + 0x3000 + _IMMx4(idx);
	}
	Reference DDR_t::operator[](DDRPerform idx) const {
		return APB4_PERIPH_BASE + 0x7000 + _IMMx4(idx);
	}
	Reference DDR_t::operator[](DDRPhyziks idx) const {
		return APB4_PERIPH_BASE + 0x4000 + _IMMx4(idx);
	}

	volatile uint32* DDR_t::getRoleress(stduint off) const { return (uint32*)(_IMM(DDR_BASE_ADDR) + off); }

	#define impl(x) x DDR_t

	void DDR_t::enClock(bool ena) const {
		using namespace RCCReg;
		// _TEMP for ena == true
		// 0(DDRC1EN), 4(DDRPHYCEN), 9(DDRPHYCAPBEN), 6(DDRCAPBEN), ?(DDRC2EN: def-DDR_DUAL_AXI_PORT)
		RCC[DDRITFCR] |= _IMM1S(0) | _IMM1S(4) | _IMM1S(9) | _IMM1S(6);
	}
	
	bool DDR_t::ddr_clk_enable(stduint spd) {
		enClock(true);
		stdsint mem_speed_hz = spd * 1000U;
		stdsint ddrphy_clk = getFrequency() - mem_speed_hz;
		stduint ddr_clk = absof(ddrphy_clk);
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
		//{TODO} make into enum class.
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

	impl(bool)::enable_axi_port(bool ena) const {
		self[DDRControl::PCTRL_0].setof(0, ena);// PORT_EN;
		if (_TEMP 0) { // DDR_DUAL_AXI_PORT
			//self[DDRControl::PCTRL_1].setof(0, ena);// PORT_EN
		}
		// Waits unit all AXI ports are idle
		// Poll PSTAT.rd_port_busy_n = 0
		// Poll PSTAT.wr_port_busy_n = 0
		if (!ena) {
			stduint timeout = loc_getTimeout(1000000); // 1s
			while (self[DDRControl::PSTAT] != 0) asrtret(timeout--);
		}
		return true;
	}

	impl(bool)::enable_host_interface(bool ena) const {
		self[DDRControl::DBG1].setof(1, !ena);// DIS_HIF
		// Waits until all queues and pipelines are empty
		// Poll DBGCAM.dbg_wr_q_empty = 1
		// Poll DBGCAM.dbg_rd_q_empty = 1
		// Poll DBGCAM.dbg_wr_data_pipeline_empty = 1
		// Poll DBGCAM.dbg_rd_data_pipeline_empty = 1
		// data_pipeline fields must be polled twice to ensure value propoagation, so count is added to loop condition.
		if (!ena) {
			stduint&& mask = _IMM(0b11011) << 25;// DBG_WR/RD_Q_EMPTY + WR/RD_DATA_PIPELINE_EMPTY
			stduint timeout = loc_getTimeout(1000000); // 1s
			stduint preread = self[DDRControl::DBGCAM];
			while (self[DDRControl::DBGCAM] & mask != mask) asrtret(timeout--);
		}
		return true;
	}

	impl(bool)::sw_selfref_entry() {
		self[DDRControl::PWRCTL].setof(5);// SELFREF_SW
		// Wait operating mode change in self-refresh mode with STAT.operating_mode[1:0]==11.
		// Ensure transition to self-refresh was due to software by checking also that STAT.selfref_type[1:0]=2.
		stduint timeout = loc_getTimeout(500); // 500us
		while (timeout--) {
			stduint&& DDRCTRL_STAT_OPERATING_MODE_SR = 0b011;
			stduint&& DDRCTRL_STAT_SELFREF_TYPE_SR = 0b10;
			stduint operating_mode = self[DDRControl::STAT].masof(0, 3);// OPERATING_MODE
			stduint selref_type = self[DDRControl::STAT].masof(4, 2);// SELFREF_TYPE
			if (operating_mode == DDRCTRL_STAT_OPERATING_MODE_SR && selref_type == DDRCTRL_STAT_SELFREF_TYPE_SR)
				return true;
		}
		return false;
	}

	impl(bool)::set_qd3_update_conditions() {
		asrtret(enable_axi_port(false));
		asrtret(enable_host_interface(false));
		start_sw_done();
		return true;
	}
	impl(bool)::unset_qd3_update_conditions() {
		asrtret(wait_sw_done_ack());
		enable_host_interface();
		enable_axi_port();
		return true;
	}
	impl(bool)::wait_refresh_update_done_ack() {
		bool refresh_update_level;
		refresh_update_level = self[DDRControl::RFSHCTL3].bitof(1);
		self[DDRControl::RFSHCTL3].setof(1, !refresh_update_level);
		stduint timeout = loc_getTimeout(1000000); // 1s
		while (self[DDRControl::RFSHCTL3].bitof(1) == refresh_update_level)
			asrtret(timeout--);
		return true;
	}

	impl(bool)::mode_register_write(stduint addr, stduint data) {
		// 1. Poll MRSTAT.mr_wr_busy until it is '0'.
		// This checks that there is no outstanding MR transaction.
		// No write should be performed to MRCTRL0 and MRCTRL1 if MRSTAT.mr_wr_busy = 1.
		stduint timeout = loc_getTimeout(200000); // 200 ms
		while (self[DDRControl::MRSTAT].bitof(0)) // MR_WR_BUSY
			asrtret(timeout--);
		// 2. Write the MRCTRL0.mr_type, MRCTRL0.mr_addr, MRCTRL0.mr_rank and (for MRWs) MRCTRL1.mr_data to define the MR transaction.
		Reflocal(mrctrl0) = 0;
		mrctrl0.setof(0, 0);// ...
		mrctrl0.setof(31, 0);// MR_TYPE Write
		mrctrl0.setof(4);// DDRCTRL_MRCTRL0_MR_RANK_ALL
		mrctrl0.maset(12, 4, addr);// MR_ADDR
		self[DDRControl::MRCTRL0] = mrctrl0;
		self[DDRControl::MRCTRL1] = data;
		// 3. In a separate APB transaction, write the MRCTRL0.mr_wr to 1. 
		// This bit is self-clearing, and triggers the MR transaction.
		// The uMCTL2 then asserts the MRSTAT.mr_wr_busy while it performs
		// the MR transaction to SDRAM, and no further access can be initiated until it is deasserted.
		mrctrl0.setof(31, true);// MR_TYPE Write
		self[DDRControl::MRCTRL0] = mrctrl0;
		timeout = loc_getTimeout(200000); // 200 ms
		while (self[DDRControl::MRSTAT].bitof(0)) // MR_WR_BUSY
			asrtret(timeout--);
		return true;
	}

	impl(bool)::ddr3_dll_off() {
		Reflocal (mr1) = self[DDRPhyziks::MR1];
		Reflocal (mr2) = self[DDRPhyziks::MR2];
		stduint timeout = loc_getTimeout(200000); // 200 ms
		Reflocal (dbgcam);
		// 1. Set the DBG1.dis_hif = 1.
		// This prevents further reads/writes being received on the HIF.
		self[DDRControl::DBG1].setof(1, true);// DIS_HIF
		// 2. Ensure all commands have been flushed from the uMCTL2 by polling
		//  .   DBGCAM.wr_data_pipeline_empty = 1,
		//  .   DBGCAM.rd_data_pipeline_empty = 1,
		//  .   DBGCAM.dbg_wr_q_depth = 0 ,
		//  .   DBGCAM.dbg_lpr_q_depth = 0, and
		//  .   DBGCAM.dbg_hpr_q_depth = 0.
		do dbgcam = self[DDRControl::DBGCAM];
		while (timeout-- &&
			dbgcam.masof(28, 2) == 0b11 && // R&W DATA_PIPELINE_EMPTY
			dbgcam & 0b000111110001111100011111 == 0 // DBG_W/LPR/HPR_Q_DEPTH
			);
		// 3. Perform an MRS command (using MRCTRL0 and MRCTRL1 registers) to disable RTT_NOM:
		//    a. DDR3: Write to MR1[9], MR1[6] and MR1[2]
		//    b. DDR4: Write to MR1[10:8]
		mr1.setof(2, false);// RTT0
		mr1.setof(6, false);// RTT1
		mr1.setof(9, false);// RTT2
		asrtret(mode_register_write(1, _IMM(mr1)));
		// 4. For DDR4 only: Perform an MRS command
		//    (using MRCTRL0 and MRCTRL1 registers) to write to MR5[8:6]
		//    to disable RTT_PARK
		(void)"None";
		// 5. Perform an MRS command (using MRCTRL0 and MRCTRL1 registers)
		//    to write to MR2[10:9], to disable RTT_WR
		//    (and therefore disable dynamic ODT).
		//    This applies for both DDR3 and DDR4.
		mr2.maset(9, 2, 0);// RTTWR
		asrtret(mode_register_write(2, _IMM(mr2)));
		// 6. Perform an MRS command (using MRCTRL0 and MRCTRL1 registers)
		//    to disable the DLL. The timing of this MRS is automatically handled by the uMCTL2.
		//    a. DDR3: Write to MR1[0]
		//    b. DDR4: Write to MR1[0]
		mr1.setof(0);// DE, field of DDR3
		asrtret(mode_register_write(1, _IMM(mr1)));
		// 7. Put the SDRAM into self-refresh mode by setting PWRCTL.selfref_sw = 1,
		//     and polling STAT.operating_mode to ensure the DDRC has entered self-refresh.
		self[DDRControl::PWRCTL].setof(5);// SELFREF_SW
		// 8. Wait until STAT.operating_mode[1:0]==11 indicating that the DWC_ddr_umctl2 core is in self-refresh mode. 
		//    Ensure transition to self-refresh was due to software by checking that STAT.selfref_type[1:0]=2.
		{
			stduint&& DDRCTRL_STAT_OPERATING_MODE_SR = 0b011;
			asrtret(DDRCTRL_STAT_OPERATING_MODE_SR);
		}
		// 9. Set the MSTR.dll_off_mode = 1.
		//    warning: MSTR.dll_off_mode is a quasi-dynamic type 2 field
		start_sw_done();
		(void)"CORE"; self[DDRControl::MSTR].setof(15);// DLL_OFF_MODE
		asrtret(wait_sw_done_ack());
		// 10. Change the clock frequency to the desired value.
		(void)"None";
		// 11. Update any registers which may be required to change for the new
		//     frequency. This includes static and dynamic registers.
		//     This includes both uMCTL2 registers and PHY registers.
		// Change Bypass Mode Frequency Range (depends on DDRPHYC_CLK_RATE)
		self[DDRPhyziks::DLLGCR].setof(23, localcfg.info.speed >= 100000U);// BPS200
		//
		self[DDRPhyziks::ACDLLCR].setof(31, true);// DLLDIS
		self[DDRPhyziks::DX0DLLCR].setof(31, true);// DLLDIS
		self[DDRPhyziks::DX1DLLCR].setof(31, true);// DLLDIS
		// [not for mp13] DDR_32BIT_INTERFACE? : set DLLDIS of DX2DLLCR and DX3DLLCR
		//
		// 12. Exit the self-refresh state by setting PWRCTL.selfref_sw = 0.
		stduint&& DDRCTRL_STAT_OPERATING_MODE_NORMAL = 0b001;
		self[DDRControl::PWRCTL].setof(5, false);// SELFREF_SW
		asrtret(wait_operating_mode(DDRCTRL_STAT_OPERATING_MODE_NORMAL));
		// 13. If ZQCTL0.dis_srx_zqcl = 0, the uMCTL2 performs a ZQCL command
		//     at this point.
		(void)"None";
		// 14. Perform MRS commands as required to re-program timing registers
		//     in the SDRAM for the new frequency
		//     (in particular, CL, CWL and WR may need to be changed).
		(void)"None";
		// 15. Write DBG1.dis_hif = 0 to re-enable reads and writes.
		self[DDRControl::DBG1].setof(1, false);// DIS_HIF
		return true;
	}

	impl(bool)::setRefresh(bool ena, uint32_t rfshctl3, uint32_t pwrctl) {
		// manage quasi-dynamic registers modification
		// dfimisc.dfi_init_complete_en : Group 3
		// AXI ports not yet enabled, don't disable them
		asrtret(enable_host_interface(false));
		start_sw_done();
		if (!ena) {
			self[DDRControl::RFSHCTL3].setof(1);// REFRESH_UPDATE_LEVEL, Quasi-dynamic register update
			asrtret(wait_refresh_update_done_ack());
			self[DDRControl::PWRCTL].setof(1, false);// POWERDOWN_EN
			self[DDRControl::PWRCTL].setof(0, false);// SELFREF_EN
		}
		else {
			Reflocal(rf) = rfshctl3;
			Reflocal(ctl) = pwrctl;
			if (!rf.bitof(0)) { // DIS_AUTO_REFRESH
				self[DDRControl::RFSHCTL3].setof(0, false);// DIS_AUTO_REFRESH
				asrtret(wait_refresh_update_done_ack());
			}
			if (ctl.bitof(1)) self[DDRControl::PWRCTL].setof(1, true);// POWERDOWN_EN
			if (ctl.bitof(0)) self[DDRControl::PWRCTL].setof(0, true);// SELFREF_EN
		}
		self[DDRControl::DFIMISC].setof(0, ena);// DFI_INIT_COMPLETE_EN
		asrtret(wait_sw_done_ack());
		enable_host_interface();
		return true;
	}

	impl(bool)::refresh_cmd() {
		stduint timeout = loc_getTimeout(200000);// 200 ms
		do {
			asrtret(timeout--)
		} while (self[DDRControl::DBGSTAT].bitof(0));// RANK0_REFRESH_BUSY
		self[DDRControl::DBGCMD].setof(0);// RANK0_REFRESH
		return true;
	}

	impl(bool)::refresh_compensation(stduint start) {
		stduint d_time = SysTick::getTickPhysical() - start;
		stduint d_time_us = d_time / (__get_CNTFRQ() / 1000000U);
		stduint d_tick_ps = 1000000000U / localcfg.info.speed;
		if (!d_tick_ps) return true;
		stduint&& tref = d_time_us * 1000000U / d_tick_ps;// refresh time in tck
		stduint trefi = 32U * self[DDRControl::RFSHTMG].masof(16, 12);// T_RFC_NOM_X1_X32
		if (!trefi) return true;
		// div round up : number of refresh to compensate
		stduint refcomp = (tref + trefi - 1U) / trefi;
		for0(i, refcomp) asrtret(refresh_cmd());
		return true;
	}

#define ph(x) self[DDRPhyziks::x]
#define cn(x) self[DDRControl::x]
	impl(bool)::ddr_sw_self_refresh_in() {
		using namespace RCCReg;
		RCC[DDRITFCR].setof(8, false);// AXIDCGEN
		if (!enable_axi_port(false)) {
			enable_axi_port();
			return false;
		}
		if (!sw_selfref_entry()) { // SW Self-Refresh entry
			// selfref_sw_failed: restore DDR in its previous state
			sw_selfref_exit();
			enable_axi_port();
			return false;
		}
		// IOs powering down (PUBL registers)
		{
			ph(ACIOCR).setof(3);// ACPDD
			ph(ACIOCR).setof(4);// ACPDR
			ph(ACIOCR).maset(8, 3, 1);// CKPDD
			ph(ACIOCR).maset(11, 3, 1);// CKPDR
			ph(ACIOCR).setof(18);// CSPDD, //{} why the manual mark the field  4 blen ? --20241225
		}
		// Disable command / address output driver
		{
			ph(ACIOCR).setof(1, false);// ACOE
			ph(DXCCR).setof(2);// DXPDD
			ph(DXCCR).setof(3);// DXPDR
			ph(DSGCR).setof(20);// ODTPDD, should be 4b-field
			ph(DSGCR).setof(24);// NL2PD
			ph(DSGCR).setof(16);// CKEPDD, should be 4b-field
		}
		ph(ZQ0CR0).setof(31);// ZQPD, Disable PZQ cell (PUBL register)
		ph(DSGCR).rstof(28);// CKOE, set latch
		loc_delayus(10);// Additional delay to avoid early latch
		PWR[PWRReg::CR3].setof(12);// DDRRETEN, sw retention
		RCC[DDRITFCR].setof(24);// GSKPCTRL
		// Disable all DLLs: GLITCH window
		{
			self[DDRPhyziks::ACDLLCR].setof(31, true);// DLLDIS
			self[DDRPhyziks::DX0DLLCR].setof(31, true);// DLLDIS
			self[DDRPhyziks::DX1DLLCR].setof(31, true);// DLLDIS
			// [not for mp13] DDR_32BIT_INTERFACE? : set DLLDIS of DX2DLLCR and DX3DLLCR
		}
		RCC[DDRITFCR].setof(24, false);// GSKPCTRL, Switch controller clocks (uMCTL2/PUBL) to DLL output clock
		// Deactivate all DDR clocks
		{
			// 0(DDRC1EN), 9(DDRPHYCAPBEN), 6(DDRCAPBEN), ?(DDRC2EN: def-DDR_DUAL_AXI_PORT)
			RCC[DDRITFCR] ^= ~(_IMM1S(0) | _IMM1S(9) | _IMM1S(6));
		}
		return true;
	}

	impl(bool)::ddr_sr_mode_ssr() {
		if (!ddr_supports_ssr_asr())
			return true;
		{
			using namespace RCCReg;
			RCC[DDRITFCR].setof(1);// DDRC1LPEN
			RCC[DDRITFCR].setof(0);// DDRC1EN
			//[mp13 no this] DDR_DUAL_AXI_PORT : DDRC2LPEN DDRC2EN
			RCC[DDRITFCR].setof(7);// DDRCAPBLPEN
			RCC[DDRITFCR].setof(10);// DDRPHYCAPBLPEN
			RCC[DDRITFCR].setof(6);// DDRCAPBEN
			RCC[DDRITFCR].setof(9);// DDRPHYCAPBEN
			RCC[DDRITFCR].setof(4);// DDRPHYCEN
			RCC[DDRITFCR].setof(8, false);// AXIDCGEN
			RCC[DDRITFCR].maset(20, 3, 0);// DDRCKMOD
		}
		// manage quasi-dynamic registers modification
		// hwlpctl.hw_lp_en : Group 3
		// pwrtmg.selfref_to_x32 & powerdown_to_x32 : Group 4
		// Group 3 is the most restrictive, apply its conditions for all
		asrtret(set_qd3_update_conditions());
		cn(HWLPCTL).rstof(0);// HW_LP_EN
		cn(PWRTMG).maset(16, 8, 1);// SELFREF_TO_X32
		asrtret(unset_qd3_update_conditions());
		cn(PWRCTL).rstof(3);// EN_DFI_DRAM_CLK_DISABLE, Disable Clock disable with LP modes (used in RUN mode for LPDDR2 with specific timing).
		cn(PWRCTL).rstof(0);// SELFREF_EN, Disable automatic Self-Refresh mode
		return true;
	}

	impl(bool)::ddr_sr_mode_asr() {
		using namespace RCCReg;
		asrtret(ddr_supports_ssr_asr());
		RCC[DDRITFCR].setof(8);// AXIDCGEN
		RCC[DDRITFCR].setof(1);// DDRC1LPEN
		//[mp13 no this] DDR_DUAL_AXI_PORT : DDRC2LPEN
		RCC[DDRITFCR].setof(5);// DDRPHYCLPEN
		RCC[DDRITFCR].maset(20, 3, 1);// DDRCKMOD, ASR1
		// manage quasi-dynamic registers modification
		// hwlpctl.hw_lp_en : Group 3
		// pwrtmg.selfref_to_x32 & powerdown_to_x32 : Group 4
		// Group 3 is the most restrictive, apply its conditions for all
		asrtret(set_qd3_update_conditions());
		cn(HWLPCTL).setof(0);// HW_LP_EN
		cn(PWRTMG).maset(16, 8, 1);// SELFREF_TO_X32
		asrtret(unset_qd3_update_conditions());
		cn(PWRCTL).setof(3);// EN_DFI_DRAM_CLK_DISABLE
		cn(PWRCTL).setof(0);// SELFREF_EN
		return true;
	}

	impl(bool)::ddr_sr_mode_hsr() {
		using namespace RCCReg;
		RCC[DDRITFCR].setof(8);// AXIDCGEN
		RCC[DDRITFCR].setof(1);// DDRC1LPEN
		//[mp13 no this] DDR_DUAL_AXI_PORT : DDRC2LPEN
		RCC[DDRITFCR].setof(5);// DDRPHYCLPEN
		RCC[DDRITFCR].maset(20, 3, 2);// DDRCKMOD, HSR1
		// manage quasi-dynamic registers modification
		// hwlpctl.hw_lp_en : Group 3
		// pwrtmg.selfref_to_x32 & powerdown_to_x32 : Group 4
		// Group 3 is the most restrictive, apply its conditions for all
		asrtret(set_qd3_update_conditions());
		cn(HWLPCTL).setof(0);// HW_LP_EN
		cn(PWRTMG).maset(16, 8, 1);// SELFREF_TO_X32
		asrtret(unset_qd3_update_conditions());
		cn(PWRCTL).setof(3);// EN_DFI_DRAM_CLK_DISABLE
		return true;
	}

	impl(DDR_t::SelfRefreshMode)::ddr_sr_read_mode() {
		SelfRefreshMode ret = SelfRefreshMode::Invalid;
		bool EN_DFI_DRAM_CLK_DISABLE = cn(PWRCTL).bitof(3);
		bool SELFREF_EN = cn(PWRCTL).bitof(0);
		if (EN_DFI_DRAM_CLK_DISABLE) {
			ret = SELFREF_EN ?
				SelfRefreshMode::AutoSelfRefresh :
				SelfRefreshMode::HardSelfRefresh;
		}
		else if (!SELFREF_EN) {
			ret = SelfRefreshMode::SoftSelfRefresh;
		}
		return ret;
	}

	//

	impl(bool)::setModeSR(SelfRefreshMode mod) {
		switch (mod) {
		case SelfRefreshMode::SoftSelfRefresh:
			asrtret(ddr_sr_mode_ssr());
			break;
		case SelfRefreshMode::AutoSelfRefresh:
			asrtret(ddr_sr_mode_asr());
			break;
		case SelfRefreshMode::HardSelfRefresh:
			asrtret(ddr_sr_mode_hsr());
			break;
		default:
			return false;
		}
		return true;
	}

	impl(bool)::HAL_DDR_SR_Entry(
		uint32_t* zq0cr0_zdata // IO calibration value
	) {
		// Save IOs calibration values
		asserv(zq0cr0_zdata)[0] = ph(ZQ0CR0).masof(0, 20);// ZDATA
		// Put DDR in Self-Refresh
		asrtret(ddr_sw_self_refresh_in());
		// Enable I/O retention mode in standby
		PWR[PWRReg::CR3].setof(10);// DDRSREN
		return true;
	}

	impl(bool)::HAL_DDR_SR_Exit() {
		using namespace RCCReg;
		enClock();
		// manage quasi-dynamic registers modification
		// dfimisc.dfi_init_complete_en : Group 3
		asrtret(set_qd3_update_conditions());
		self[DDRControl::DFIMISC].setof(0, false);// DFI_INIT_COMPLETE_EN
		asrtret(unset_qd3_update_conditions());
		RCC[DDRITFCR].setof(24);// GSKPCTRL
		// Enable all DLLs: GLITCH window
		//
		self[DDRPhyziks::ACDLLCR].setof(31, false);// DLLDIS
		self[DDRPhyziks::DX0DLLCR].setof(31, false);// DLLDIS
		self[DDRPhyziks::DX1DLLCR].setof(31, false);// DLLDIS
		// [not for mp13] DDR_32BIT_INTERFACE? : set DLLDIS of DX2DLLCR and DX3DLLCR
		//
		// Additional delay to avoid early DLL clock switch
		loc_delayus(10);
		RCC[DDRITFCR].setof(24, false);// GSKPCTRL
		ph(ACDLLCR).setof(31, false);// DLLSRST
		loc_delayus(10);
		ph(ACDLLCR).setof(31, true);// DLLSRST
		// PHY partial init: (DLL lock and ITM reset)
		{
			Reflocal(pir) = 0;
			pir.setof(1);// DLLSRST
			pir.setof(2);// DLLLOCK
			pir.setof(4);// ITMSRST
			pir.setof(0);// INIT
			ph(PIR) = pir;
		}
		// Need to wait at least 10 clock cycles before accessing PGSR
		loc_delayus(1);
		{ // Pool end of init 
			stduint timeout = loc_getTimeout(500);
			while (!ph(PGSR).bitof(0)) asrtret(timeout--);
		}
		// manage quasi-dynamic registers modification
		// dfimisc.dfi_init_complete_en : Group 3
		asrtret(set_qd3_update_conditions());
		self[DDRControl::DFIMISC].setof(0, true);// DFI_INIT_COMPLETE_EN
		asrtret(unset_qd3_update_conditions());
		PWR[PWRReg::CR3].setof(12, false);// DDRRETEN
		ph(ZQ0CR0).setof(31, false);// ZQPD
		ph(ACIOCR).setof(3, false);// ACPDD
		// Enable command/address output driver
		ph(ACIOCR).setof(1);// ACOE
		ph(ACIOCR).maset(8, 3, nil);// CKPDD
		ph(ACIOCR).maset(18, 4, nil);// CSPDD
		ph(DXCCR).setof(2, false);// DXPDD
		ph(DXCCR).setof(3, false);// DXPDR
		// Release latch
		ph(DSGCR).setof(28);// CKOE, set latch
		ph(DSGCR).maset(20, 4, nil);// ODTPDD
		ph(DSGCR).setof(24, false);// NL2PD
		ph(DSGCR).maset(16, 4, nil);// CKEPDD
		//
		sw_selfref_exit();
		{
			stduint timeout = loc_getTimeout(500);
			stduint&& DDRCTRL_STAT_OPERATING_MODE_NORMAL = 0b001;
			while (cn(STAT).masof(0, 3) != DDRCTRL_STAT_OPERATING_MODE_NORMAL)// OPERATING_MODE
				asrtret(timeout--);
		}
		// AXI ports are no longer blocked from taking transactions
		enable_axi_port();
		RCC[DDRITFCR].setof(8);// AXIDCGEN
		return true;
	}



}
#endif


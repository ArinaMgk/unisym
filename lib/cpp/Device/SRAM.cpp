// ASCII CPP TAB4 CRLF
// Docutitle: (Driver) SRAM
// Codifiers: @dosconio: 20240723
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
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

#include "../../../inc/c/driver/SRAM.h"

#ifdef _MPU_STM32MP13
#include "../../../inc/c/proctrl/ARM/cortex_a7.h"
#include "../../../inc/cpp/Device/RCC/RCC"
#include "../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"
namespace uni {
	BKPSRAM_t BKPSRAM;

	#define impl(x) x BKPSRAM_t
	

	impl(stduint)::getAddress() const {
		return AHB5_PERIPH_BASE;
	}
	
	impl(void)::enClock(bool ena) const {
		RCC[ena ? RCCReg::MP_AHB5ENSETR : RCCReg::MP_AHB5ENCLRR] = _IMM1S(8);// BKPSRAMEN
	}




}
#endif

// ---- H7 external SRAM (FMC NORSRAM Bank1~4) ----
#ifdef _MCU_STM32H7x
#include "../../../inc/cpp/Device/SRAM"
#include "../../../inc/cpp/Device/RCC/RCC"
#include "../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"

namespace uni {

	SRAM_t SRAM(FMC_Bank1_R_BASE, FMC_Bank1E_R_BASE);

	// AKA HAL_SRAM_Init + FMC_NORSRAM_Init + FMC_NORSRAM_Timing_Init + FMC_NORSRAM_Extended_Timing_Init + __FMC_NORSRAM_ENABLE + __FMC_ENABLE
	bool SRAM_t::setMode(const SRAMInit& _init, const SRAMTiming& timing, const SRAMTiming& extTiming) {
		Init = _init;
		// HAL_SRAM_MspInit: enable FMC clock (D1 AHB3)
		RCC[RCCReg::AHB3ENR].setof(12);// FMCEN

		State = SRAMState::Busy;

		stduint bidx = (stduint)Init.bank << 1;// BCR index 0/2/4/6; BTR = bidx+1; BWTR 同 bidx

		// ---- FMC_NORSRAM_Init (BCR) ----
		Reference bcr = btcr(bidx);
		// clear config bits (AKA BCR_CLEAR_MASK)
		bcr.rstof(_SRAM_BCR_POS_MBKEN);
		bcr.rstof(_SRAM_BCR_POS_MUXEN);
		bcr.maset(_SRAM_BCR_POS_MTYP, 2, 0);
		bcr.maset(_SRAM_BCR_POS_MWID, 2, 0);
		bcr.rstof(_SRAM_BCR_POS_FACCEN);
		bcr.rstof(_SRAM_BCR_POS_BURSTEN);
		bcr.rstof(_SRAM_BCR_POS_WAITPOL);
		bcr.maset(_SRAM_BCR_POS_CPSIZE, 3, 0);
		bcr.rstof(_SRAM_BCR_POS_WAITCFG);
		bcr.rstof(_SRAM_BCR_POS_WREN);
		bcr.rstof(_SRAM_BCR_POS_WAITEN);
		bcr.rstof(_SRAM_BCR_POS_EXTMOD);
		bcr.rstof(_SRAM_BCR_POS_ASYNCWAIT);
		bcr.rstof(_SRAM_BCR_POS_CBURSTRW);
		bcr.rstof(_SRAM_BCR_POS_CCLKEN);
		bcr.rstof(_SRAM_BCR_POS_WFDIS);
		// set from Init
		bcr.setof(_SRAM_BCR_POS_MUXEN, Init.dataAddressMux);
		bcr.maset(_SRAM_BCR_POS_MTYP, 2, (stduint)Init.memory);
		bcr.maset(_SRAM_BCR_POS_MWID, 2, (stduint)Init.dataWidth);
		bcr.setof(_SRAM_BCR_POS_WREN, Init.writeOperation);
		bcr.setof(_SRAM_BCR_POS_EXTMOD, Init.extendedMode);

		// ---- FMC_NORSRAM_Timing_Init (BTR) ----
		Reference btr = btcr(bidx + 1);
		btr.maset(_SRAM_BTR_POS_ADDSET, 4, timing.addressSetup);
		btr.maset(_SRAM_BTR_POS_ADDHLD, 4, timing.addressHold);
		btr.maset(_SRAM_BTR_POS_DATAST, 8, timing.dataSetup);
		btr.maset(_SRAM_BTR_POS_BUSTURN, 4, timing.busTurnAround);
		btr.maset(_SRAM_BTR_POS_CLKDIV, 4, 0);// 同步专用, 异步 SRAM 恒 0
		btr.maset(_SRAM_BTR_POS_DATLAT, 4, 0);// 同步专用, 异步 SRAM 恒 0
		btr.maset(_SRAM_BTR_POS_ACCMOD, 2, (stduint)timing.accessMode);

		// ---- FMC_NORSRAM_Extended_Timing_Init (BWTR) ----
		Reference bwtrx = bwtr(bidx);
		if (Init.extendedMode) {
			bwtrx.maset(_SRAM_BTR_POS_ADDSET, 4, extTiming.addressSetup);
			bwtrx.maset(_SRAM_BTR_POS_ADDHLD, 4, extTiming.addressHold);
			bwtrx.maset(_SRAM_BTR_POS_DATAST, 8, extTiming.dataSetup);
			bwtrx.maset(_SRAM_BTR_POS_BUSTURN, 4, extTiming.busTurnAround);
			bwtrx.maset(_SRAM_BTR_POS_ACCMOD, 2, (stduint)extTiming.accessMode);
		} else {
			bwtrx = 0x0FFFFFFF;
		}

		// ---- enable NORSRAM device (per-bank MBKEN) + FMC IP (global FMCEN) ----
		bcr.setof(_SRAM_BCR_POS_MBKEN, true);
		Reference(FMC_R_BASE).setof(_FMC_BCR1_FMCEN, true);

		State = SRAMState::Ready;// H7 HAL 漏置 READY, 此处对齐 F4/MP13 正确语义
		return true;
	}

	// AKA HAL_SRAM_DeInit + FMC_NORSRAM_DeInit
	bool SRAM_t::canMode() {
		stduint bidx = (stduint)Init.bank << 1;
		// __FMC_NORSRAM_DISABLE: clear MBKEN
		btcr(bidx).rstof(_SRAM_BCR_POS_MBKEN);
		// FMC_NORSRAM_DeInit: reset BCR/BTR/BWTR
		btcr(bidx) = (Init.bank == SRAMBank::Bank1) ? 0x000030DB : 0x000030D2;
		btcr(bidx + 1) = 0x0FFFFFFF;
		bwtr(bidx) = 0x0FFFFFFF;
		// HAL_SRAM_MspDeInit: disable FMC clock
		RCC[RCCReg::AHB3ENR].setof(12, false);// FMCEN
		State = SRAMState::Reset;
		return true;
	}

	// AKA HAL_SRAM_Read_8b/16b/32b + HAL_SRAM_Read_DMA
	bool SRAM_t::Read(pureptr_t addr, pureptr_t buf, stduint size, SRAMWidth w, IOMethod method) {
		// H7 HAL Read 不查状态, 此处按更严谨语义拒绝 Busy
		if (State == SRAMState::Busy) return false;
		if (method == IOMethod::DMA) {
			// HAL_SRAM_Read_DMA: 32-bit words only, byte count = size*4
			if (w != SRAMWidth::Bits32) return false;
			return MDMA[mdmaChannel].Transfer(addr, buf, size * 4, 1, IOMethod::Rupt);
		}
		if (method != IOMethod::Loop) return false;// Rupt N/A for SRAM
		switch (w) {
		case SRAMWidth::Bits8: {
			volatile byte* p = (volatile byte*)addr;
			byte* d = (byte*)buf;
			for (stduint i = 0; i < size; i++) d[i] = p[i];
			return true;
		}
		case SRAMWidth::Bits16: {
			volatile uint16* p = (volatile uint16*)addr;
			uint16* d = (uint16*)buf;
			for (stduint i = 0; i < size; i++) d[i] = p[i];
			return true;
		}
		case SRAMWidth::Bits32: {
			volatile uint32* p = (volatile uint32*)addr;
			uint32* d = (uint32*)buf;
			for (stduint i = 0; i < size; i++) d[i] = p[i];
			return true;
		}
		}
		return false;
	}

	// AKA HAL_SRAM_Write_8b/16b/32b + HAL_SRAM_Write_DMA
	bool SRAM_t::Write(pureptr_t addr, pureptr_t buf, stduint size, SRAMWidth w, IOMethod method) {
		if (State == SRAMState::Busy || State == SRAMState::Protected) return false;
		if (method == IOMethod::DMA) {
			if (w != SRAMWidth::Bits32) return false;
			return MDMA[mdmaChannel].Transfer(buf, addr, size * 4, 1, IOMethod::Rupt);
		}
		if (method != IOMethod::Loop) return false;
		switch (w) {
		case SRAMWidth::Bits8: {
			volatile byte* p = (volatile byte*)addr;
			const byte* s = (const byte*)buf;
			for (stduint i = 0; i < size; i++) p[i] = s[i];
			return true;
		}
		case SRAMWidth::Bits16: {
			volatile uint16* p = (volatile uint16*)addr;
			const uint16* s = (const uint16*)buf;
			for (stduint i = 0; i < size; i++) p[i] = s[i];
			return true;
		}
		case SRAMWidth::Bits32: {
			volatile uint32* p = (volatile uint32*)addr;
			const uint32* s = (const uint32*)buf;
			for (stduint i = 0; i < size; i++) p[i] = s[i];
			return true;
		}
		}
		return false;
	}

	// AKA HAL_SRAM_WriteOperation_Enable/Disable
	bool SRAM_t::setWriteOperation(bool ena) {
		if (State == SRAMState::Busy) return false;
		State = SRAMState::Busy;
		btcr((stduint)Init.bank << 1).setof(_SRAM_BCR_POS_WREN, ena);
		State = ena ? SRAMState::Ready : SRAMState::Protected;
		return true;
	}

}
#endif

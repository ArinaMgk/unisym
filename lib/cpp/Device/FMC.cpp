// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Flexible Memory Controller
// Codifiers: @dosconio: 20240715~;
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Dependens: RCC
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

#include "../../../inc/cpp/Device/FMC"

namespace uni {

#if defined(_MCU_STM32H7x)

	// AKA HAL_SDRAM_Init + FMC_SDRAM_Init + FMC_SDRAM_Timing_Init + __FMC_ENABLE
	bool FMC_SDRAM_t::setMode(const SDRAMInit& _init, const SDRAMTiming& timing) {
		Init = _init;
		// HAL_SDRAM_MspInit: enable FMC clock (D1 AHB3)
		RCC[RCCReg::AHB3ENR].setof(12);// FMCEN

		State = SDRAMState::Busy;

		if (Init.bank != SDRAMBank::Bank2) {
			// FMC_SDRAM_Init (bank1): all fields to SDCR1
			Reference sdcr1 = self[FMC_SDRAMReg::SDCR1];
			sdcr1.maset(_SDRAM_SDCR_POS_NC, 2, (stduint)Init.column);
			sdcr1.maset(_SDRAM_SDCR_POS_NR, 2, (stduint)Init.row);
			sdcr1.maset(_SDRAM_SDCR_POS_MWID, 2, (stduint)Init.dataWidth);
			sdcr1.maset(_SDRAM_SDCR_POS_NB, 1, (stduint)Init.bankNum);
			sdcr1.maset(_SDRAM_SDCR_POS_CAS, 2, (stduint)Init.cas);
			sdcr1.setof(_SDRAM_SDCR_POS_WP, Init.writeProtection);
			sdcr1.maset(_SDRAM_SDCR_POS_SDCLK, 2, (stduint)Init.clockPeriod);
			sdcr1.setof(_SDRAM_SDCR_POS_RBURST, Init.readBurst);
			sdcr1.maset(_SDRAM_SDCR_POS_RPIPE, 2, (stduint)Init.readPipe);
		} else {
			// FMC_SDRAM_Init (bank2): SDCLK/RBURST/RPIPE shared to SDCR1, rest to SDCR2
			Reference sdcr1 = self[FMC_SDRAMReg::SDCR1];
			Reference sdcr2 = self[FMC_SDRAMReg::SDCR2];
			sdcr1.maset(_SDRAM_SDCR_POS_SDCLK, 2, (stduint)Init.clockPeriod);
			sdcr1.setof(_SDRAM_SDCR_POS_RBURST, Init.readBurst);
			sdcr1.maset(_SDRAM_SDCR_POS_RPIPE, 2, (stduint)Init.readPipe);
			sdcr2.maset(_SDRAM_SDCR_POS_NC, 2, (stduint)Init.column);
			sdcr2.maset(_SDRAM_SDCR_POS_NR, 2, (stduint)Init.row);
			sdcr2.maset(_SDRAM_SDCR_POS_MWID, 2, (stduint)Init.dataWidth);
			sdcr2.maset(_SDRAM_SDCR_POS_NB, 1, (stduint)Init.bankNum);
			sdcr2.maset(_SDRAM_SDCR_POS_CAS, 2, (stduint)Init.cas);
			sdcr2.setof(_SDRAM_SDCR_POS_WP, Init.writeProtection);
		}

		// FMC_SDRAM_Timing_Init
		if (Init.bank != SDRAMBank::Bank2) {
			Reference sdtr1 = self[FMC_SDRAMReg::SDTR1];
			sdtr1.maset(_SDRAM_SDTR_POS_TMRD, 4, (stduint)(timing.loadToActive - 1));
			sdtr1.maset(_SDRAM_SDTR_POS_TXSR, 4, (stduint)(timing.exitSelfRefresh - 1));
			sdtr1.maset(_SDRAM_SDTR_POS_TRAS, 4, (stduint)(timing.selfRefreshTime - 1));
			sdtr1.maset(_SDRAM_SDTR_POS_TRC, 4, (stduint)(timing.rowCycle - 1));
			sdtr1.maset(_SDRAM_SDTR_POS_TWR, 4, (stduint)(timing.writeRecovery - 1));
			sdtr1.maset(_SDRAM_SDTR_POS_TRP, 4, (stduint)(timing.rpDelay - 1));
			sdtr1.maset(_SDRAM_SDTR_POS_TRCD, 4, (stduint)(timing.rcdDelay - 1));
		} else {
			Reference sdtr1 = self[FMC_SDRAMReg::SDTR1];
			Reference sdtr2 = self[FMC_SDRAMReg::SDTR2];
			sdtr1.maset(_SDRAM_SDTR_POS_TRC, 4, (stduint)(timing.rowCycle - 1));
			sdtr1.maset(_SDRAM_SDTR_POS_TRP, 4, (stduint)(timing.rpDelay - 1));
			sdtr2.maset(_SDRAM_SDTR_POS_TMRD, 4, (stduint)(timing.loadToActive - 1));
			sdtr2.maset(_SDRAM_SDTR_POS_TXSR, 4, (stduint)(timing.exitSelfRefresh - 1));
			sdtr2.maset(_SDRAM_SDTR_POS_TRAS, 4, (stduint)(timing.selfRefreshTime - 1));
			sdtr2.maset(_SDRAM_SDTR_POS_TWR, 4, (stduint)(timing.writeRecovery - 1));
			sdtr2.maset(_SDRAM_SDTR_POS_TRCD, 4, (stduint)(timing.rcdDelay - 1));
		}

		// __FMC_ENABLE: FMC_Bank1->BTCR[0] |= FMC_BCR1_FMCEN
		Reference(FMC_R_BASE).setof(_FMC_BCR1_FMCEN, true);

		State = SDRAMState::Ready;
		return true;
	}

	// AKA HAL_SDRAM_DeInit + FMC_SDRAM_DeInit
	bool FMC_SDRAM_t::canMode() {
		// FMC_SDRAM_DeInit
		self[(FMC_SDRAMReg)((stduint)FMC_SDRAMReg::SDCR1 + (stduint)Init.bank)] = 0x000002D0;
		self[(FMC_SDRAMReg)((stduint)FMC_SDRAMReg::SDTR1 + (stduint)Init.bank)] = 0x0FFFFFFF;
		self[FMC_SDRAMReg::SDCMR] = 0;
		self[FMC_SDRAMReg::SDRTR] = 0;
		self[FMC_SDRAMReg::SDSR] = 0;
		// HAL_SDRAM_MspDeInit: disable FMC clock
		RCC[RCCReg::AHB3ENR].setof(12, false);// FMCEN
		State = SDRAMState::Reset;
		return true;
	}

	// AKA HAL_SDRAM_Read_8b/16b/32b + HAL_SDRAM_Read_DMA
	bool FMC_SDRAM_t::Read(pureptr_t addr, pureptr_t buf, stduint size, SDRAMWidth w, IOMethod method) {
		if (State == SDRAMState::Busy || State == SDRAMState::Precharged) return false;
		if (method == IOMethod::DMA) {
			// HAL_SDRAM_Read_DMA: 32-bit words only
			if (w != SDRAMWidth::Bits32) return false;
			return MDMA[mdmaChannel].Transfer(addr, buf, size * 4, 1, IOMethod::Rupt);
		}
		if (method != IOMethod::Loop) return false; // Rupt N/A for SDRAM
		switch (w) {
		case SDRAMWidth::Bits8: {
			volatile byte* p = (volatile byte*)addr;
			byte* d = (byte*)buf;
			for (stduint i = 0; i < size; i++) d[i] = p[i];
			return true;
		}
		case SDRAMWidth::Bits16: {
			volatile uint16* p = (volatile uint16*)addr;
			uint16* d = (uint16*)buf;
			for (stduint i = 0; i < size; i++) d[i] = p[i];
			return true;
		}
		case SDRAMWidth::Bits32: {
			volatile uint32* p = (volatile uint32*)addr;
			uint32* d = (uint32*)buf;
			for (stduint i = 0; i < size; i++) d[i] = p[i];
			return true;
		}
		}
		return false;
	}

	// AKA HAL_SDRAM_Write_8b/16b/32b + HAL_SDRAM_Write_DMA
	bool FMC_SDRAM_t::Write(pureptr_t addr, pureptr_t buf, stduint size, SDRAMWidth w, IOMethod method) {
		if (State == SDRAMState::Busy || State == SDRAMState::Precharged || State == SDRAMState::WriteProtected)
			return false;
		if (method == IOMethod::DMA) {
			// HAL_SDRAM_Write_DMA: 32-bit words only
			if (w != SDRAMWidth::Bits32) return false;
			return MDMA[mdmaChannel].Transfer(buf, addr, size * 4, 1, IOMethod::Rupt);
		}
		if (method != IOMethod::Loop) return false; // Rupt N/A for SDRAM
		switch (w) {
		case SDRAMWidth::Bits8: {
			volatile byte* p = (volatile byte*)addr;
			const byte* s = (const byte*)buf;
			for (stduint i = 0; i < size; i++) p[i] = s[i];
			return true;
		}
		case SDRAMWidth::Bits16: {
			volatile uint16* p = (volatile uint16*)addr;
			const uint16* s = (const uint16*)buf;
			for (stduint i = 0; i < size; i++) p[i] = s[i];
			return true;
		}
		case SDRAMWidth::Bits32: {
			volatile uint32* p = (volatile uint32*)addr;
			const uint32* s = (const uint32*)buf;
			for (stduint i = 0; i < size; i++) p[i] = s[i];
			return true;
		}
		}
		return false;
	}

	// AKA HAL_SDRAM_WriteProtection_Enable/Disable
	bool FMC_SDRAM_t::setWriteProtection(bool ena) {
		if (State == SDRAMState::Busy) return false;
		State = SDRAMState::Busy;
		self[(FMC_SDRAMReg)((stduint)FMC_SDRAMReg::SDCR1 + (stduint)Init.bank)].setof(_SDRAM_SDCR_POS_WP, ena);
		State = ena ? SDRAMState::WriteProtected : SDRAMState::Ready;
		return true;
	}

	// AKA HAL_SDRAM_SendCommand + FMC_SDRAM_SendCommand
	bool FMC_SDRAM_t::setCommand(const SDRAMCommand& cmd) {
		if (State == SDRAMState::Busy) return false;
		State = SDRAMState::Busy;
		self[FMC_SDRAMReg::SDCMR] =
			(stduint)cmd.mode | (stduint)cmd.target |
			((stduint)(cmd.autoRefreshNumber - 1) << _SDRAM_SDCMR_POS_NRFS) |
			((stduint)cmd.modeRegister << _SDRAM_SDCMR_POS_MRD);
		State = (cmd.mode == SDRAMCmd::PALL) ? SDRAMState::Precharged : SDRAMState::Ready;
		return true;
	}

	// AKA HAL_SDRAM_ProgramRefreshRate + FMC_SDRAM_ProgramRefreshRate
	//（HAL 用 |= 会累积旧值，此处按正确语义清后写）
	bool FMC_SDRAM_t::setRefreshRate(uint32 rate) {
		if (State == SDRAMState::Busy) return false;
		State = SDRAMState::Busy;
		self[FMC_SDRAMReg::SDRTR].maset(_SDRAM_SDRTR_POS_COUNT, 13, rate);// COUNT[12:0] @ bit1
		State = SDRAMState::Ready;
		return true;
	}

	// AKA HAL_SDRAM_SetAutoRefreshNumber + FMC_SDRAM_SetAutoRefreshNumber
	//（HAL 漏了 -1 且用 |=；此处统一为 N-1、清后写，与 SendCommand 的 (N-1)<<5 一致）
	bool FMC_SDRAM_t::setAutoRefreshNumber(uint32 n) {
		if (State == SDRAMState::Busy) return false;
		State = SDRAMState::Busy;
		self[FMC_SDRAMReg::SDCMR].maset(_SDRAM_SDCMR_POS_NRFS, 4, n - 1);// NRFS[3:0] @ bit5
		State = SDRAMState::Ready;
		return true;
	}

	// AKA HAL_SDRAM_GetModeStatus + FMC_SDRAM_GetModeStatus
	uint32 FMC_SDRAM_t::getModeStatus() {
		if (Init.bank == SDRAMBank::Bank1)
			return self[FMC_SDRAMReg::SDSR].masof(_SDRAM_SDSR_POS_MODES1, 2);
		else
			return self[FMC_SDRAMReg::SDSR].masof(_SDRAM_SDSR_POS_MODES2, 2);
	}

	// AKA HAL_SDRAM_IRQHandler
	void FMC_SDRAM_t::IRQHandler() {
		if (self[FMC_SDRAMReg::SDSR].bitof(_SDRAM_SDSR_POS_RE)) {
			if (RefreshErrorCallback) RefreshErrorCallback();
			self[FMC_SDRAMReg::SDRTR].setof(_SDRAM_SDRTR_POS_CRE, true);// clear refresh error
		}
	}

	FMC_t FMC;

#endif // _MCU_STM32H7x

}

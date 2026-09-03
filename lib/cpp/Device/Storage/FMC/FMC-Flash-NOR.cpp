// ASCII CPP TAB4 CRLF
// Docutitle: [Driver] External NOR Flash over FMC NORSRAM controller
// Codifiers: @ArinaMgk
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

#include "../../../../../inc/cpp/Device/Storage/FMC/FMC-Flash-NOR.hpp"
#include "../../../../../inc/cpp/Device/RCC/RCC"
#include "../../../../../inc/cpp/Device/SysTick"
#include "../../../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"
#ifdef _MCU_STM32H7x
#include "../../../../../inc/c/prochip/CortexM7.h"// __DSB (NOR command latch barrier)
#endif

#ifdef _MCU_STM32H7x

namespace uni {

	// ---- AKA NOR_Private_Defines (stm32h7xx_hal_nor.c) ----
	// command addresses
	static constexpr uint16 _NOR_CMD_ADDRESS_FIRST      = 0x0555;
	static constexpr uint16 _NOR_CMD_ADDRESS_FIRST_CFI  = 0x0055;
	static constexpr uint16 _NOR_CMD_ADDRESS_SECOND     = 0x02AA;
	static constexpr uint16 _NOR_CMD_ADDRESS_THIRD      = 0x0555;
	static constexpr uint16 _NOR_CMD_ADDRESS_FOURTH     = 0x0555;
	static constexpr uint16 _NOR_CMD_ADDRESS_FIFTH      = 0x02AA;
	static constexpr uint16 _NOR_CMD_ADDRESS_SIXTH      = 0x0555;
	// command data
	static constexpr uint16 _NOR_CMD_DATA_READ_RESET    = 0x00F0;
	static constexpr uint16 _NOR_CMD_DATA_FIRST         = 0x00AA;
	static constexpr uint16 _NOR_CMD_DATA_SECOND        = 0x0055;
	static constexpr uint16 _NOR_CMD_DATA_AUTO_SELECT   = 0x0090;
	static constexpr uint16 _NOR_CMD_DATA_PROGRAM       = 0x00A0;
	static constexpr uint16 _NOR_CMD_DATA_ERASE_THIRD   = 0x0080; // CHIP_BLOCK_ERASE_THIRD
	static constexpr uint16 _NOR_CMD_DATA_ERASE_FOURTH  = 0x00AA; // CHIP_BLOCK_ERASE_FOURTH
	static constexpr uint16 _NOR_CMD_DATA_ERASE_FIFTH   = 0x0055; // CHIP_BLOCK_ERASE_FIFTH
	static constexpr uint16 _NOR_CMD_DATA_CHIP_ERASE    = 0x0010;
	static constexpr uint16 _NOR_CMD_DATA_CFI           = 0x0098;
	static constexpr uint8  _NOR_CMD_DATA_BUFFER_PROG   = 0x25;   // BUFFER_AND_PROG
	static constexpr uint8  _NOR_CMD_DATA_BUFFER_CONFIRM= 0x29;   // BUFFER_AND_PROG_CONFIRM
	static constexpr uint8  _NOR_CMD_DATA_BLOCK_ERASE   = 0x30;
	// DQ status mask
	static constexpr uint16 _NOR_MASK_STATUS_DQ5        = 0x0020;
	static constexpr uint16 _NOR_MASK_STATUS_DQ6        = 0x0040;
	// ID offsets
	static constexpr uint16 _NOR_MC_ADDRESS             = 0x0000;
	static constexpr uint16 _NOR_DEVICE_CODE1_ADDR      = 0x0001;
	static constexpr uint16 _NOR_DEVICE_CODE2_ADDR      = 0x000E;
	static constexpr uint16 _NOR_DEVICE_CODE3_ADDR      = 0x000F;
	// CFI offsets
	static constexpr uint16 _NOR_CFI1_ADDRESS           = 0x61;
	static constexpr uint16 _NOR_CFI2_ADDRESS           = 0x62;
	static constexpr uint16 _NOR_CFI3_ADDRESS           = 0x63;
	static constexpr uint16 _NOR_CFI4_ADDRESS           = 0x64;

	// AKA NOR_WRITE / NOR read: half-word access to the memory-mapped NOR window.
	// (unisym memory-mapped drivers use direct volatile stores, like SRAM.cpp)
	static inline void _nor_write16(stduint a, uint16 d) { *(volatile uint16_t*)a = d; __DSB(); }
	static inline uint16 _nor_read16(stduint a) { return *(volatile uint16_t*)a; }

	FMC_NOR_t FMC_NOR(FMC_Bank1_R_BASE, FMC_Bank1E_R_BASE, 0x60000000);

	// AKA HAL_NOR_Init + FMC_NORSRAM_Init/Timing_Init/Extended_Timing_Init + __FMC_NORSRAM_ENABLE + __FMC_ENABLE
	bool FMC_NOR_t::setMode(const NORInit& _init, const NORTiming& timing, const NORTiming& ext_timing) {
		Init = _init;
		// HAL_NOR_MspInit: enable FMC clock (D1 AHB3)
		RCC[RCCReg::AHB3ENR].setof(12); // FMCEN

		State = NORState::Busy;

		// AKA NOR_MEMORY_ADRESS1..4: device memory base by bank
		devbase = 0x60000000 + ((stduint)Init.bank << 26); // bank * 0x04000000
		stduint bidx = (stduint)Init.bank << 1;             // BCR index 0/2/4/6

		// ---- FMC_NORSRAM_Init (BCR) ----
		Reference bcr = btcr(bidx);
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
		bcr.setof(_SRAM_BCR_POS_MUXEN, Init.data_address_mux);
		bcr.maset(_SRAM_BCR_POS_MTYP, 2, (stduint)Init.memory);
		bcr.maset(_SRAM_BCR_POS_MWID, 2, (stduint)Init.data_width);
		bcr.setof(_SRAM_BCR_POS_WREN, Init.write_operation);
		bcr.setof(_SRAM_BCR_POS_EXTMOD, Init.extended_mode);
		// AKA FMC_NORSRAM_Init: FACCEN set automatically when MemoryType==NOR
		bcr.setof(_SRAM_BCR_POS_FACCEN, true);

		// ---- FMC_NORSRAM_Timing_Init (BTR) ----
		Reference btr = btcr(bidx + 1);
		btr.maset(_SRAM_BTR_POS_ADDSET, 4, timing.address_setup);
		btr.maset(_SRAM_BTR_POS_ADDHLD, 4, timing.address_hold);
		btr.maset(_SRAM_BTR_POS_DATAST, 8, timing.data_setup);
		btr.maset(_SRAM_BTR_POS_BUSTURN, 4, timing.bus_turn_around);
		btr.maset(_SRAM_BTR_POS_CLKDIV, 4, 0);
		btr.maset(_SRAM_BTR_POS_DATLAT, 4, 0);
		btr.maset(_SRAM_BTR_POS_ACCMOD, 2, (stduint)timing.access_mode);

		// ---- FMC_NORSRAM_Extended_Timing_Init (BWTR) ----
		Reference bwtrx = bwtr(bidx);
		if (Init.extended_mode) {
			bwtrx.maset(_SRAM_BTR_POS_ADDSET, 4, ext_timing.address_setup);
			bwtrx.maset(_SRAM_BTR_POS_ADDHLD, 4, ext_timing.address_hold);
			bwtrx.maset(_SRAM_BTR_POS_DATAST, 8, ext_timing.data_setup);
			bwtrx.maset(_SRAM_BTR_POS_BUSTURN, 4, ext_timing.bus_turn_around);
			bwtrx.maset(_SRAM_BTR_POS_ACCMOD, 2, (stduint)ext_timing.access_mode);
		} else {
			bwtrx = 0x0FFFFFFF;
		}

		// ---- __FMC_NORSRAM_ENABLE + __FMC_ENABLE ----
		bcr.setof(_SRAM_BCR_POS_MBKEN, true);
		Reference(FMC_R_BASE).setof(_FMC_BCR1_FMCEN, true);

		// AKA uwNORMemoryDataWidth (8/16)
		data_width = (Init.data_width == SRAMDataWidth::W16) ? 1 : 0;
		Block_Size = Init.erase_block_size;

		State = NORState::Ready;
		return true;
	}

	// AKA HAL_NOR_DeInit + FMC_NORSRAM_DeInit
	bool FMC_NOR_t::canMode() {
		stduint bidx = (stduint)Init.bank << 1;
		btcr(bidx).rstof(_SRAM_BCR_POS_MBKEN);
		btcr(bidx) = (Init.bank == SRAMBank::Bank1) ? 0x000030DB : 0x000030D2;
		btcr(bidx + 1) = 0x0FFFFFFF;
		bwtr(bidx) = 0x0FFFFFFF;
		RCC[RCCReg::AHB3ENR].setof(12, false); // FMCEN off
		State = NORState::Reset;
		return true;
	}

	// AKA HAL_NOR_Read_ID
	bool FMC_NOR_t::ReadID(NORID& id) {
		if (State == NORState::Busy) return false;
		State = NORState::Busy;
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_FIRST), _NOR_CMD_DATA_FIRST);
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_SECOND), _NOR_CMD_DATA_SECOND);
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_THIRD), _NOR_CMD_DATA_AUTO_SELECT);
		id.manufacturer_code = _nor_read16(devbase + shift_addr(_NOR_MC_ADDRESS));
		id.device_code1      = _nor_read16(devbase + shift_addr(_NOR_DEVICE_CODE1_ADDR));
		id.device_code2      = _nor_read16(devbase + shift_addr(_NOR_DEVICE_CODE2_ADDR));
		id.device_code3      = _nor_read16(devbase + shift_addr(_NOR_DEVICE_CODE3_ADDR));
		State = NORState::Ready;
		return true;
	}

	// AKA HAL_NOR_ReturnToReadMode (no address shift, per HAL)
	bool FMC_NOR_t::ReturnToReadMode() {
		if (State == NORState::Busy) return false;
		_nor_write16(devbase, _NOR_CMD_DATA_READ_RESET);
		State = NORState::Ready;
		return true;
	}

	// AKA HAL_NOR_Read + HAL_NOR_ReadBuffer (converged; size = half-word count)
	bool FMC_NOR_t::Read(pureptr_t addr, void* buf, stduint size) {
		if (State == NORState::Busy) return false;
		State = NORState::Busy;
		// reset-to-read command sequence
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_FIRST), _NOR_CMD_DATA_FIRST);
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_SECOND), _NOR_CMD_DATA_SECOND);
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_THIRD), _NOR_CMD_DATA_READ_RESET);
		// NOR is random-access (no tR), but let the 0xF0 reset settle (reset recovery tRH)
		// before the data read, so it does not return stale/previous-mode data.
		uint64 tr_t0 = SysTick::getTick();
		while ((SysTick::getTick() - tr_t0) < 1) {}
		uint16* d = (uint16*)buf;
		volatile uint16* p = (volatile uint16_t*)addr;
		while (size--) *d++ = *p++;
		State = NORState::Ready;
		return true;
	}

	// AKA HAL_NOR_Program
	bool FMC_NOR_t::Program(pureptr_t addr, uint16 data) {
		if (State == NORState::Busy) return false;
		State = NORState::Busy;
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_FIRST), _NOR_CMD_DATA_FIRST);
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_SECOND), _NOR_CMD_DATA_SECOND);
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_THIRD), _NOR_CMD_DATA_PROGRAM);
		_nor_write16((stduint)addr, data);
		State = NORState::Ready;
		return true;
	}

	// AKA HAL_NOR_ProgramBuffer (S29GL128P device-specific)
	bool FMC_NOR_t::ProgramBuffer(pureptr_t addr, const uint16* data, stduint size) {
		if (State == NORState::Busy) return false;
		State = NORState::Busy;
		uint16_t* p_current = (uint16_t*)addr;
		uint16_t* p_end = p_current + (size - 1);
		stduint lastloaded = (stduint)addr;
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_FIRST), _NOR_CMD_DATA_FIRST);
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_SECOND), _NOR_CMD_DATA_SECOND);
		_nor_write16(devbase + shift_addr((stduint)addr), _NOR_CMD_DATA_BUFFER_PROG);
		_nor_write16(devbase + shift_addr((stduint)addr), (uint16)(size - 1));
		while (p_current <= p_end) {
			lastloaded = (stduint)p_current;
			_nor_write16((stduint)p_current, *data++);
			p_current++;
		}
		_nor_write16(lastloaded, _NOR_CMD_DATA_BUFFER_CONFIRM);
		State = NORState::Ready;
		return true;
	}

	// AKA HAL_NOR_Erase_Block
	bool FMC_NOR_t::EraseBlock(pureptr_t block_addr, pureptr_t dev_addr) {
		if (State == NORState::Busy) return false;
		State = NORState::Busy;
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_FIRST), _NOR_CMD_DATA_FIRST);
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_SECOND), _NOR_CMD_DATA_SECOND);
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_THIRD), _NOR_CMD_DATA_ERASE_THIRD);
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_FOURTH), _NOR_CMD_DATA_ERASE_FOURTH);
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_FIFTH), _NOR_CMD_DATA_ERASE_FIFTH);
		_nor_write16((stduint)block_addr + (stduint)dev_addr, _NOR_CMD_DATA_BLOCK_ERASE);
		State = NORState::Ready;
		return true;
	}

	// AKA HAL_NOR_Erase_Chip
	bool FMC_NOR_t::EraseChip(pureptr_t dev_addr) {
		if (State == NORState::Busy) return false;
		State = NORState::Busy;
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_FIRST), _NOR_CMD_DATA_FIRST);
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_SECOND), _NOR_CMD_DATA_SECOND);
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_THIRD), _NOR_CMD_DATA_ERASE_THIRD);
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_FOURTH), _NOR_CMD_DATA_ERASE_FOURTH);
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_FIFTH), _NOR_CMD_DATA_ERASE_FIFTH);
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_SIXTH), _NOR_CMD_DATA_CHIP_ERASE);
		State = NORState::Ready;
		return true;
	}

	// AKA HAL_NOR_Read_CFI
	bool FMC_NOR_t::ReadCFI(NORCFI& cfi) {
		if (State == NORState::Busy) return false;
		State = NORState::Busy;
		_nor_write16(devbase + shift_addr(_NOR_CMD_ADDRESS_FIRST_CFI), _NOR_CMD_DATA_CFI);
		cfi.cfi_1 = _nor_read16(devbase + shift_addr(_NOR_CFI1_ADDRESS));
		cfi.cfi_2 = _nor_read16(devbase + shift_addr(_NOR_CFI2_ADDRESS));
		cfi.cfi_3 = _nor_read16(devbase + shift_addr(_NOR_CFI3_ADDRESS));
		cfi.cfi_4 = _nor_read16(devbase + shift_addr(_NOR_CFI4_ADDRESS));
		State = NORState::Ready;
		return true;
	}

	// AKA HAL_NOR_WriteOperation_Enable/Disable
	bool FMC_NOR_t::setWriteOperation(bool ena) {
		if (State == NORState::Busy) return false;
		State = NORState::Busy;
		btcr((stduint)Init.bank << 1).setof(_SRAM_BCR_POS_WREN, ena);
		State = ena ? NORState::Ready : NORState::Protected;
		return true;
	}

	// AKA HAL_NOR_GetStatus
	NORStatus FMC_NOR_t::getStatus(pureptr_t addr, stduint timeout) {
		NORStatus status = NORStatus::Ongoing;
		uint16 tmpSR1 = 0, tmpSR2 = 0;
		uint64 tickstart = SysTick::getTick();
		while ((status != NORStatus::Success) && (status != NORStatus::Timeout)) {
			if (timeout != 0xFFFFFFFFU) { // HAL_MAX_DELAY
				if ((timeout == 0) || ((SysTick::getTick() - tickstart) > timeout)) {
					status = NORStatus::Timeout;
				}
			}
			tmpSR1 = _nor_read16((stduint)addr);
			tmpSR2 = _nor_read16((stduint)addr);
			if ((tmpSR1 & _NOR_MASK_STATUS_DQ6) == (tmpSR2 & _NOR_MASK_STATUS_DQ6)) {
				return NORStatus::Success;
			}
			if ((tmpSR1 & _NOR_MASK_STATUS_DQ5) == _NOR_MASK_STATUS_DQ5) {
				status = NORStatus::Ongoing;
			}
			tmpSR1 = _nor_read16((stduint)addr);
			tmpSR2 = _nor_read16((stduint)addr);
			if ((tmpSR1 & _NOR_MASK_STATUS_DQ6) == (tmpSR2 & _NOR_MASK_STATUS_DQ6)) {
				return NORStatus::Success;
			}
			if ((tmpSR1 & _NOR_MASK_STATUS_DQ5) == _NOR_MASK_STATUS_DQ5) {
				return NORStatus::Error;
			}
		}
		return status;
	}

	// ---- StorageTrait ----
	// NOTE: NOR is random-addressable; block == erase-block unit (Block_Size bytes).
	//       Read copies Block_Size bytes; Write erases the block then programs it.
	bool FMC_NOR_t::Read(stduint block, void* dest) {
		if (State == NORState::Busy || !Block_Size) return false;
		stduint a = devbase + block * Block_Size;
		volatile byte* p = (volatile byte*)a;
		byte* d = (byte*)dest;
		for (stduint i = 0; i < Block_Size; i++) d[i] = p[i];
		return true;
	}

	bool FMC_NOR_t::Write(stduint block, const void* src) {
		if (State == NORState::Busy || State == NORState::Protected || !Block_Size) return false;
		stduint a = devbase + block * Block_Size;
		// erase the block first, then program half-words
		if (!EraseBlock((pureptr_t)a, 0)) return false;
		const uint16* s = (const uint16*)src;
		for (stduint i = 0; i < Block_Size; i += 2) {
			if (!Program((pureptr_t)(a + i), *s++)) return false;
		}
		return true;
	}

	stduint FMC_NOR_t::getUnits() {
		if (!Init.erase_block_size) return 0;
		return Init.total_bytes / Init.erase_block_size;
	}

	int FMC_NOR_t::operator[](uint64 bytid) {
		return *(volatile byte*)(devbase + bytid);
	}

}

#endif // _MCU_STM32H7x

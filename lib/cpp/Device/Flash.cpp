// ASCII C/C++ TAB4 CRLF
// Docutitle: (Device) Flash
// Codifiers: @dosconio: ~ 20240718
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


#include "../../../inc/cpp/Device/Flash"
#include "../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"
#include "../../../inc/c/prochip/CortexM7.h"// __DSB (H7 flash program barrier)
#include "Interrupt/interrupt_flash.hpp"

namespace uni {
#if 0
#elif defined(_MCU_STM32F4x)
	Flash_t Flash;
#elif defined(_MCU_STM32H7x)

	// AKA FLASH_TIMEOUT_VALUE (50 s)
	#define _FLASH_TIMEOUT_VALUE 50000U

	Flash_t Flash;

	Reference Flash_t::operator[](FlashReg::FlashReg idx) {
		return _Flash_ADDR + _IMMx4(idx);
	}

	// per-bank register index selectors
	static FlashReg::FlashReg _FlashCR(byte bank) { return (bank == 1) ? FlashReg::CR1 : FlashReg::CR2; }
	static FlashReg::FlashReg _FlashSR(byte bank) { return (bank == 1) ? FlashReg::SR1 : FlashReg::SR2; }
	static FlashReg::FlashReg _FlashCCR(byte bank) { return (bank == 1) ? FlashReg::CCR1 : FlashReg::CCR2; }

	// AKA FLASH_WaitForLastOperation: poll BUSY|QW|WBNE until clear, then check error flags
	static bool _FlashWaitLastOperation(byte bank) {
		uint64 tickstart = SysTick::getTick();
		FlashReg::FlashReg sr = _FlashSR(bank);
		while (Flash[sr].bitof(_FLASH_SR_POS_BSY) || Flash[sr].bitof(_FLASH_SR_POS_QW) || Flash[sr].bitof(_FLASH_SR_POS_WBNE)) {
			if ((SysTick::getTick() - tickstart) > _FLASH_TIMEOUT_VALUE) return false;
		}
		if ((stduint)Flash[sr] & _FLASH_SR_ERRORS) {
			Flash[_FlashCCR(bank)] |= _FLASH_SR_ERRORS;// write-1-to-clear
			return false;
		}
		return true;
	}

	// AKA FLASH_OB_WaitForLastOperation: poll OPTSR.OPT_BUSY, then check OPTCHANGEERR
	static bool _FlashOBWait() {
		uint64 tickstart = SysTick::getTick();
		while (Flash[FlashReg::OPTSR_CUR].bitof(_FLASH_OPTSR_POS_OPT_BUSY)) {
			if ((SysTick::getTick() - tickstart) > _FLASH_TIMEOUT_VALUE) return false;
		}
		if (Flash[FlashReg::OPTSR_CUR].bitof(_FLASH_OPTSR_POS_OPTCHANGEERR)) {
			Flash[FlashReg::OPTCCR].setof(_FLASH_OPTSR_POS_OPTCHANGEERR);
			return false;
		}
		return true;
	}

	// AKA HAL_FLASH_Unlock (unlock both banks' control registers)
	bool Flash_t::Unlock() {
		if (!self[FlashReg::CR1].bitof(_FLASH_CR_POS_LOCK)) return false;
		if (!self[FlashReg::CR2].bitof(_FLASH_CR_POS_LOCK)) return false;
		self[FlashReg::KEYR1] = _FLASH_KEY1;
		self[FlashReg::KEYR1] = _FLASH_KEY2;
		self[FlashReg::KEYR2] = _FLASH_KEY1;
		self[FlashReg::KEYR2] = _FLASH_KEY2;
		return true;
	}

	// AKA HAL_FLASH_Lock (lock both banks)
	void Flash_t::Lock() {
		self[FlashReg::CR1].setof(_FLASH_CR_POS_LOCK);
		self[FlashReg::CR2].setof(_FLASH_CR_POS_LOCK);
	}

	// AKA HAL_FLASH_Program / Program_IT: program a 256-bit row (32-byte aligned addr)
	bool Flash_t::Program(stduint addr, const void* data, IOMethod method) {
		byte bank = (addr < _FLASH_BANK2_BASE) ? 1 : 2;
		if (!_FlashWaitLastOperation(bank)) return false;
		FlashReg::FlashReg cr = _FlashCR(bank);
		Flash[_FlashCCR(bank)] |= _FLASH_CCR_CLEAR;
		Flash[cr].setof(_FLASH_CR_POS_PG);
		if (method == IOMethod::Rupt) {
			Address = addr;
			Procedure = (bank == 1) ? _FLASH_PROC_PROGRAM_BANK1 : _FLASH_PROC_PROGRAM_BANK2;
			Flash[cr] |= _FLASH_CR_OPER_IT;// arm EOP + error interrupts
		}
		// write the 256-bit row as 4 x 64-bit words
		volatile uint64* dest = (volatile uint64*)addr;
		const uint64* src = (const uint64*)data;
		dest[0] = src[0];
		dest[1] = src[1];
		dest[2] = src[2];
		dest[3] = src[3];
		__DSB();
		if (method == IOMethod::Loop) {
			bool ok = _FlashWaitLastOperation(bank);
			Flash[_FlashCCR(bank)] |= _FLASH_CCR_CLEAR;
			Flash[cr].rstof(_FLASH_CR_POS_PG);
			return ok;
		}
		return true;
	}

	// AKA HAL_FLASHEx_Erase / Erase_IT: erase one sector of one bank (sector 0..7)
	bool Flash_t::Erase(byte bank, byte sector, IOMethod method) {
		if (bank < 1 || bank > 2 || sector > 7) return false;
		if (!_FlashWaitLastOperation(bank)) return false;
		FlashReg::FlashReg cr = _FlashCR(bank);
		Flash[cr].maset(_FLASH_CR_POS_PSIZE, 2, 0);
		Flash[cr].rstof(_FLASH_CR_POS_SER);
		Flash[cr].maset(_FLASH_CR_POS_SNB, 3, sector);
		Flash[cr].setof(_FLASH_CR_POS_SER);
		Flash[cr].maset(_FLASH_CR_POS_PSIZE, 2, 3);// PSIZE = double word
		if (method == IOMethod::Rupt) {
			Procedure = (bank == 1) ? _FLASH_PROC_SECTERASE_BANK1 : _FLASH_PROC_SECTERASE_BANK2;
			Flash[cr] |= _FLASH_CR_OPER_IT;
		}
		Flash[cr].setof(_FLASH_CR_POS_START);
		if (method == IOMethod::Loop) {
			bool ok = _FlashWaitLastOperation(bank);
			Flash[cr].rstof(_FLASH_CR_POS_SER);
			Flash[cr].maset(_FLASH_CR_POS_SNB, 3, 0);
			return ok;
		}
		return true;
	}

	// AKA HAL_FLASHEx_Erase / Erase_IT: mass erase one whole bank
	bool Flash_t::Erase(byte bank, IOMethod method) {
		if (bank < 1 || bank > 2) return false;
		if (!_FlashWaitLastOperation(bank)) return false;
		FlashReg::FlashReg cr = _FlashCR(bank);
		Flash[cr].maset(_FLASH_CR_POS_PSIZE, 2, 3);// PSIZE = double word
		Flash[cr].setof(_FLASH_CR_POS_BER);
		if (method == IOMethod::Rupt) {
			Procedure = (bank == 1) ? _FLASH_PROC_MASSERASE_BANK1 : _FLASH_PROC_MASSERASE_BANK2;
			Flash[cr] |= _FLASH_CR_OPER_IT;
		}
		Flash[cr].setof(_FLASH_CR_POS_START);
		if (method == IOMethod::Loop) {
			bool ok = _FlashWaitLastOperation(bank);
			Flash[cr].rstof(_FLASH_CR_POS_BER);
			return ok;
		}
		return true;
	}

	// ---- StorageTrait (32-byte row granularity) ----

	// block -> absolute address (2 banks x 1MB = 65536 rows)
	static stduint _FlashBlockAddr(stduint block) {
		return (block < _FLASH_SECTOR_TOTAL * _FLASH_SECTOR_SIZE / _FLASH_ROW_SIZE / 2) ?
			(_FLASH_BANK1_BASE + block * _FLASH_ROW_SIZE) :
			(_FLASH_BANK2_BASE + (block - (_FLASH_SECTOR_TOTAL * _FLASH_SECTOR_SIZE / _FLASH_ROW_SIZE / 2)) * _FLASH_ROW_SIZE);
	}

	bool Flash_t::Read(stduint block, void* dest, stduint Times) {
		if (!dest || !Times || block + Times > getUnits()) return false;
		for0(t, Times) {
			stduint addr = _FlashBlockAddr(block + t);
			Reference(0xE000EF5C) = addr;// DCIMVAC: invalidate the D-Cache line covering this block
			__DSB();
			MemCopyN((byte*)dest + t * _FLASH_ROW_SIZE, (void*)addr, _FLASH_ROW_SIZE);
		}
		return true;
	}

	// auto-erase the containing sector, then program the row
	bool Flash_t::Write(stduint block, const void* src, stduint Times) {
		if (!src || !Times || block + Times > getUnits()) return false;
		for0(t, Times) {
			stduint blk = block + t;
			stduint half = _FLASH_SECTOR_TOTAL * _FLASH_SECTOR_SIZE / _FLASH_ROW_SIZE / 2;// rows per bank
			byte bank = (blk < half) ? 1 : 2;
			stduint within = (blk < half) ? blk : (blk - half);
			byte sector = (byte)((within * _FLASH_ROW_SIZE) / _FLASH_SECTOR_SIZE);
			if (!Erase(bank, sector)) return false;
			if (!Program(_FlashBlockAddr(blk), (const byte*)src + t * _FLASH_ROW_SIZE)) return false;
		}
		return true;
	}

	// AKA byte access; -1 when out of range
	int Flash_t::operator[](uint64 bytid) {
		if (bytid >= (uint64)getUnits() * _FLASH_ROW_SIZE) return -1;
		return *(volatile byte*)(_FLASH_BANK1_BASE + bytid);
	}

	// ---- Option Bytes (AKA HAL_FLASH_OB_* / HAL_FLASHEx_OBProgram / OBGetConfig) ----

	// AKA HAL_FLASH_OB_Unlock (write OPT key to OPTKEYR)
	bool Flash_t::OB_Unlock() {
		if (!self[FlashReg::OPTCR].bitof(_FLASH_OPTCR_POS_OPTLOCK)) return false;
		self[FlashReg::OPTKEYR] = _FLASH_OPT_KEY1;
		self[FlashReg::OPTKEYR] = _FLASH_OPT_KEY2;
		return true;
	}

	// AKA HAL_FLASH_OB_Lock (set OPTCR.OPTLOCK)
	void Flash_t::OB_Lock() {
		self[FlashReg::OPTCR].setof(_FLASH_OPTCR_POS_OPTLOCK);
	}

	// AKA HAL_FLASH_OB_Launch (set OPTCR.OPTSTART, then wait)
	bool Flash_t::OB_Launch() {
		self[FlashReg::OPTCR].setof(_FLASH_OPTCR_POS_OPTSTART);
		return _FlashOBWait();
	}

	// AKA FLASH_OB_RDPConfig: set read protection level 0/1/2
	bool Flash_t::setReadProtection(byte level) {
		if (level > 2) return false;
		if (!_FlashOBWait()) return false;
		byte val = (level == 1) ? _FLASH_OB_RDP_LEVEL1 : (level == 2) ? _FLASH_OB_RDP_LEVEL2 : _FLASH_OB_RDP_LEVEL0;
		self[FlashReg::OPTSR_PRG].maset(_FLASH_OPTSR_POS_RDP, 8, val);
		return _FlashOBWait();
	}

	// AKA FLASH_OB_EnableWRP: protect the given sectors of one bank (WPSN is active-low)
	bool Flash_t::setWriteProtection(byte bank, stduint sector_mask) {
		if (bank < 1 || bank > 2 || (sector_mask & ~0xFFU)) return false;
		FlashReg::FlashReg wprg = (bank == 1) ? FlashReg::WPSN_PRG1 : FlashReg::WPSN_PRG2;
		if (!_FlashOBWait()) return false;
		self[wprg] = (stduint)self[wprg] & ~(sector_mask & 0xFFU);// clear bit = write-protected
		return _FlashOBWait();
	}

	// AKA FLASH_OB_BOR_LevelConfig: set BOR level 1/2/3
	bool Flash_t::setBORLevel(byte level) {
		if (level < 1 || level > 3) return false;
		if (!_FlashOBWait()) return false;
		// BOR_LEV (2b): HAL values 0x08/0x04/0x00 -> field 2/1/0
		byte field = (level == 3) ? 0 : (level == 2) ? 1 : 2;
		self[FlashReg::OPTSR_PRG].maset(_FLASH_OPTSR_POS_BOR_LEV, 2, field);
		return _FlashOBWait();
	}

	// AKA FLASH_OB_BootAddConfig: set BOOT_ADD0/ADD1 (which 0/1/2 = ADD0/ADD1/both)
	bool Flash_t::setBootAddress(byte which, stduint addr) {
		if (which > 2) return false;
		if (!_FlashOBWait()) return false;
		if (which != 1) self[FlashReg::BOOT_PRG].maset(_FLASH_BOOT_POS_ADD0, 16, (addr >> 16) & 0xFFFF);
		if (which != 0) self[FlashReg::BOOT_PRG].maset(_FLASH_BOOT_POS_ADD1, 16, addr & 0xFFFF);
		return _FlashOBWait();
	}

	// AKA FLASH_OB_PCROPConfig: proprietary code readout protection area of one bank
	bool Flash_t::setCodeReadProtection(byte bank, stduint start, stduint end) {
		if (bank < 1 || bank > 2) return false;
		FlashReg::FlashReg prpg = (bank == 1) ? FlashReg::PRAR_PRG1 : FlashReg::PRAR_PRG2;
		stduint base = (bank == 1) ? _FLASH_BANK1_BASE : _FLASH_BANK2_BASE;
		if (!_FlashOBWait()) return false;
		self[prpg] = ((start - base) >> 8) & 0xFFF;
		self[prpg] |= (((end - base) >> 8) & 0xFFF) << _FLASH_PRAR_POS_PROT_AREA_END;
		// DMEP = 0: PCROP area not erased when RDP level decreases
		return _FlashOBWait();
	}

	// AKA FLASH_OB_SecureAreaConfig: secure area of one bank
	bool Flash_t::setSecureArea(byte bank, stduint start, stduint end) {
		if (bank < 1 || bank > 2) return false;
		FlashReg::FlashReg scpg = (bank == 1) ? FlashReg::SCAR_PRG1 : FlashReg::SCAR_PRG2;
		stduint base = (bank == 1) ? _FLASH_BANK1_BASE : _FLASH_BANK2_BASE;
		if (!_FlashOBWait()) return false;
		self[scpg] = ((start - base) >> 8) & 0xFFF;
		self[scpg] |= (((end - base) >> 8) & 0xFFF) << _FLASH_SCAR_POS_SEC_AREA_END;
		// DMES = 0: secure area kept on mass erase
		return _FlashOBWait();
	}

	// AKA FLASH_OB_UserConfig: modify user option byte bits by mask/value
	bool Flash_t::setUserOption(stduint mask, stduint value) {
		if (!_FlashOBWait()) return false;
		stduint cur = (stduint)self[FlashReg::OPTSR_PRG];
		self[FlashReg::OPTSR_PRG] = (cur & ~mask) | (value & mask);
		return _FlashOBWait();
	}

	// AKA HAL_FLASHEx_OBGetConfig: read current option bytes into cfg
	bool Flash_t::getOptionBytesConfig(OptionBytesConfig_t& cfg) {
		stduint optsr = (stduint)self[FlashReg::OPTSR_CUR];
		byte rdp = (optsr >> _FLASH_OPTSR_POS_RDP) & 0xFF;
		cfg.read_protection = (rdp == _FLASH_OB_RDP_LEVEL1) ? 1 : (rdp == _FLASH_OB_RDP_LEVEL2) ? 2 : 0;
		byte bor = (optsr >> _FLASH_OPTSR_POS_BOR_LEV) & 0x3;
		cfg.bor_level = (bor == 2) ? 1 : (bor == 1) ? 2 : 3;
		cfg.write_protection1 = ~((stduint)self[FlashReg::WPSN_CUR1] & 0xFF) & 0xFF;
		cfg.write_protection2 = ~((stduint)self[FlashReg::WPSN_CUR2] & 0xFF) & 0xFF;
		stduint boot = (stduint)self[FlashReg::BOOT_CUR];
		cfg.boot_addr0 = (boot & 0xFFFF) << 16;
		cfg.boot_addr1 = (boot >> _FLASH_BOOT_POS_ADD1) & 0xFFFF;
		stduint prar = (stduint)self[FlashReg::PRAR_CUR1];
		cfg.code_read_start = ((prar & 0xFFF) << 8) + _FLASH_BANK1_BASE;
		cfg.code_read_end = (((prar >> _FLASH_PRAR_POS_PROT_AREA_END) & 0xFFF) << 8) + _FLASH_BANK1_BASE;
		stduint scar = (stduint)self[FlashReg::SCAR_CUR1];
		cfg.secure_area_start = ((scar & 0xFFF) << 8) + _FLASH_BANK1_BASE;
		cfg.secure_area_end = (((scar >> _FLASH_SCAR_POS_SEC_AREA_END) & 0xFFF) << 8) + _FLASH_BANK1_BASE;
		cfg.user_options = optsr;
		return true;
	}

	// ---- RuptTrait (NVIC + IRQ_FLASH) ----
	void Flash_t::setInterrupt(Handler_t f) const {
		FUNC_FLASH[0] = f;
	}
	void Flash_t::setInterruptPriority(byte preempt, byte sub_priority) const {
		NVIC.setPriority(IRQ_FLASH, preempt, sub_priority);
	}
	void Flash_t::enInterrupt(bool enable) const {
		NVIC.setAble(IRQ_FLASH, enable);
	}

#endif // _MCU_STM32H7x
}

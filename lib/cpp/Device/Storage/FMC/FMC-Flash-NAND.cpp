// ASCII CPP TAB4 CRLF
// Docutitle: [Driver] External NAND Flash over FMC NAND controller
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

#include "../../../../../inc/cpp/Device/Storage/FMC/FMC-Flash-NAND.hpp"
#include "../../../../../inc/cpp/Device/RCC/RCC"
#include "../../../../../inc/cpp/Device/SysTick"
#include "../../../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"
#ifdef _MCU_STM32H7x
#include "../../../../../inc/c/prochip/CortexM7.h"// __DSB (FMC command/address latch barrier)
#endif

#ifdef _MCU_STM32H7x

namespace uni {

	// ---- AKA NAND_Private_Constants (stm32h7xx_hal_nand.h) ----
	static constexpr stduint _NAND_DEVICE        = 0x80000000U; // NAND external memory window
	static constexpr stduint _NAND_WRITE_TIMEOUT = 0x01000000U;
	static constexpr stduint _NAND_CMD_AREA      = 0x10000U;    // A16 = CLE
	static constexpr stduint _NAND_ADDR_AREA     = 0x20000U;    // A17 = ALE

	// command bytes (AKA NAND_CMD_*)
	static constexpr uint8 _NAND_CMD_AREA_A      = 0x00U;
	static constexpr uint8 _NAND_CMD_AREA_C      = 0x50U;
	static constexpr uint8 _NAND_CMD_AREA_TRUE1  = 0x30U;
	static constexpr uint8 _NAND_CMD_WRITE0      = 0x80U;
	static constexpr uint8 _NAND_CMD_WRITE_TRUE1 = 0x10U;
	static constexpr uint8 _NAND_CMD_ERASE0      = 0x60U;
	static constexpr uint8 _NAND_CMD_ERASE1      = 0xD0U;
	static constexpr uint8 _NAND_CMD_READID      = 0x90U;
	static constexpr uint8 _NAND_CMD_STATUS      = 0x70U;

	// status bits (AKA NAND_BUSY/ERROR/READY)
	static constexpr uint8 _NAND_BUSY            = 0x00U;
	static constexpr uint8 _NAND_ERROR           = 0x01U;
	static constexpr uint8 _NAND_READY           = 0x40U;

	// FMC NAND PCR bit positions (AKA FMC_PCR_*_Pos, stm32h743xx.h)
	static constexpr stduint _PCR_PWAITEN = 1;
	static constexpr stduint _PCR_PBKEN   = 2;
	static constexpr stduint _PCR_PTYP    = 2; // 2 bits, NAND = 2 (0x08)
	static constexpr stduint _PCR_PWID    = 4; // 2 bits
	static constexpr stduint _PCR_ECCEN   = 6;
	static constexpr stduint _PCR_TCLR    = 9; // 4 bits
	static constexpr stduint _PCR_TAR     = 13; // 4 bits
	static constexpr stduint _PCR_ECCPS   = 17; // 3 bits
	static constexpr stduint _SR_FEMPT    = 6;

	// command/address/data window accessors (memory-mapped byte lanes, like HAL; __DSB latches each)
	static inline void _nand_cmd(stduint base, uint8 c)   { *(volatile uint8_t*)(base + _NAND_CMD_AREA) = c; __DSB(); }
	static inline void _nand_addr(stduint base, uint8 a)  { *(volatile uint8_t*)(base + _NAND_ADDR_AREA) = a; __DSB(); }
	static inline uint8  _nand_read8(stduint base)        { return *(volatile uint8_t*)base; }
	static inline uint16 _nand_read16(stduint base)       { return *(volatile uint16_t*)base; }
	static inline void _nand_write8(stduint base, uint8 d)  { *(volatile uint8_t*)base = d; __DSB(); }
	static inline void _nand_write16(stduint base, uint16 d){ *(volatile uint16_t*)base = d; __DSB(); }

	// AKA ARRAY_ADDRESS: row address (page index) from logical Page/Plane/Block
	static inline stduint _nand_array_address(const NANDAddress& a, const NANDConfig& c) {
		return a.page + ((a.block + (stduint)a.plane * c.plane_size) * c.block_size);
	}
	// AKA COLUMN_ADDRESS: spare-area column start (main area size); 16-bit mode doubles
	static inline stduint _nand_column_address(const NANDConfig& c, NANDBus bus) {
		return c.page_size * ((bus == NANDBus::Bits16) ? 2 : 1);
	}
	// AKA ADDR_1ST/2ND/3RD_CYCLE: row address cycles (2 or 3 bytes by total page count)
	static inline void _nand_send_row(stduint base, stduint nand_address, stduint total_pages) {
		_nand_addr(base, (uint8)nand_address);
		_nand_addr(base, (uint8)(nand_address >> 8));
		if (total_pages > 65535) _nand_addr(base, (uint8)(nand_address >> 16));
	}

	FMC_NAND_t FMC_NAND(FMC_R_BASE + 0x80, _NAND_DEVICE);

	// AKA HAL_NAND_Init + HAL_NAND_ConfigDevice + FMC_NAND_Init + CommonSpace/AttributeSpace_Timing_Init + __FMC_NAND_ENABLE + __FMC_ENABLE
	bool FMC_NAND_t::setMode(const NANDConfig& cfg) {
		Config = cfg;
		// HAL_NAND_MspInit: enable FMC clock (D1 AHB3)
		RCC[RCCReg::AHB3ENR].setof(12); // FMCEN
		State = NANDState::Busy;

		// ---- FMC_NAND_Init (PCR) ----
		Reference pcr = (*this)[NandReg::PCR];
		// clear config fields (AKA FMC_NAND_Init clear mask)
		pcr.rstof(_PCR_PWAITEN);
		pcr.rstof(_PCR_PBKEN);
		pcr.maset(_PCR_PWID, 2, 0);
		pcr.rstof(_PCR_ECCEN);
		pcr.maset(_PCR_TCLR, 4, 0);
		pcr.maset(_PCR_TAR, 4, 0);
		pcr.maset(_PCR_ECCPS, 3, 0);
		// HAL relies on PCR reset value 0x18 for PTYP; unisym sets it explicitly
		pcr.maset(_PCR_PTYP, 2, 2); // FMC_MEMORY_TYPE_NAND
		// set fields
		pcr.setof(_PCR_PWAITEN, Config.wait_feature);
		pcr.maset(_PCR_PWID, 2, (stduint)Config.data_bus);
		pcr.setof(_PCR_ECCEN, Config.ecc_computation);
		pcr.maset(_PCR_TCLR, 4, Config.tclr_setup_time);
		pcr.maset(_PCR_TAR, 4, Config.tar_setup_time);
		pcr.maset(_PCR_ECCPS, 3, Config.ecc_page_size >> 17);

		// ---- CommonSpace / AttributeSpace timing (PMEM / PATT) ----
		(*this)[NandReg::PMEM] =
			(stduint)Config.common_space.setup_time |
			((stduint)Config.common_space.wait_setup_time << 8) |
			((stduint)Config.common_space.hold_setup_time << 16) |
			((stduint)Config.common_space.hiz_setup_time << 24);
		(*this)[NandReg::PATT] =
			(stduint)Config.attribute_space.setup_time |
			((stduint)Config.attribute_space.wait_setup_time << 8) |
			((stduint)Config.attribute_space.hold_setup_time << 16) |
			((stduint)Config.attribute_space.hiz_setup_time << 24);

		// ---- __FMC_NAND_ENABLE + __FMC_ENABLE ----
		pcr.setof(_PCR_PBKEN, true);
		Reference(FMC_R_BASE).setof(_FMC_BCR1_FMCEN, true);

		// StorageTrait: 1 block = 1 page main area (bytes)
		Block_Size = Config.page_size * ((Config.data_bus == NANDBus::Bits16) ? 2 : 1);

		State = NANDState::Ready;
		return true;
	}

	// AKA HAL_NAND_DeInit + FMC_NAND_DeInit
	bool FMC_NAND_t::canMode() {
		// FMC_NAND_DeInit: disable bank + reset registers
		(*this)[NandReg::PCR].rstof(_PCR_PBKEN);
		(*this)[NandReg::PCR]  = 0x00000018U;
		(*this)[NandReg::SR]   = 0x00000040U;
		(*this)[NandReg::PMEM] = 0xFCFCFCFCU;
		(*this)[NandReg::PATT] = 0xFCFCFCFCU;
		RCC[RCCReg::AHB3ENR].setof(12, false); // FMCEN off
		State = NANDState::Reset;
		return true;
	}

	// AKA HAL_NAND_Read_ID
	bool FMC_NAND_t::ReadID(NANDID& id) {
		if (State == NANDState::Busy) return false;
		State = NANDState::Busy;
		_nand_cmd(data_base, _NAND_CMD_READID);
		_nand_addr(data_base, 0x00);
		if (Config.data_bus == NANDBus::Bits8) {
			uint32 data = *(volatile uint32_t*)data_base;
			id.maker_id  = (uint8)data;
			id.device_id = (uint8)(data >> 8);
			id.third_id  = (uint8)(data >> 16);
			id.fourth_id = (uint8)(data >> 24);
		} else {
			uint32 data  = *(volatile uint32_t*)data_base;
			uint32 data1 = *(volatile uint32_t*)(data_base + 16);
			id.maker_id  = (uint8)data;
			id.device_id = (uint8)(data >> 16);
			id.third_id  = (uint8)data1;
			id.fourth_id = (uint8)(data1 >> 16);
		}
		State = NANDState::Ready;
		return true;
	}

	// AKA HAL_NAND_Reset (HAL writes literal 0xFF)
	bool FMC_NAND_t::Reset() {
		if (State == NANDState::Busy) return false;
		State = NANDState::Busy;
		_nand_cmd(data_base, 0xFF);
		State = NANDState::Ready;
		return true;
	}

	// AKA HAL_NAND_Read_Page_8b/16b + Read_SpareArea_8b/16b (converged)
	bool FMC_NAND_t::Read(const NANDAddress& a, void* buf, stduint count, NANDArea area, NANDBus bus) {
		if (State == NANDState::Busy) return false;
		State = NANDState::Busy;
		stduint nand_address = _nand_array_address(a, Config);
		stduint total_pages  = Config.block_size * Config.block_nbr;
		stduint per = (area == NANDArea::Main) ? Config.page_size : Config.spare_area_size;
		byte* d8 = (byte*)buf;
		uint16* d16 = (uint16*)buf;

		while (count && nand_address < total_pages) {
			if (area == NANDArea::Main) {
				// AKA read page command + column (0x00) + row
				_nand_cmd(data_base, _NAND_CMD_AREA_A);
				if (Config.page_size <= 512) _nand_addr(data_base, 0x00);
				else { _nand_addr(data_base, 0x00); _nand_addr(data_base, 0x00); }
				_nand_send_row(data_base, nand_address, total_pages);
				_nand_cmd(data_base, _NAND_CMD_AREA_TRUE1);
			} else {
				// AKA read spare area
				if (Config.page_size <= 512) {
					_nand_cmd(data_base, _NAND_CMD_AREA_C);
					_nand_addr(data_base, 0x00);
				} else {
					_nand_cmd(data_base, _NAND_CMD_AREA_A);
					stduint col = _nand_column_address(Config, bus);
					_nand_addr(data_base, (uint8)col);
					_nand_addr(data_base, (uint8)(col >> 8));
				}
				_nand_send_row(data_base, nand_address, total_pages);
				_nand_cmd(data_base, _NAND_CMD_AREA_TRUE1);
			}

			// wait tR (page read latency ~25us): the FMC does NOT wait for R/B without
			// PWAITEN, and enabling PWAITEN can hang the bus if R/B is not wired.
			// A short fixed delay covers tR before reading data (AKA NAND_WaitRB in the reference).
			uint64 tr_t0 = SysTick::getTick();
			while ((SysTick::getTick() - tr_t0) < 1) {}

			// AKA ExtraCommandEnable: poll status then back to read mode
			if (Config.extra_command_enable) {
				uint64 t0 = SysTick::getTick();
				while (ReadStatus() != _NAND_READY) {
					if ((SysTick::getTick() - t0) > _NAND_WRITE_TIMEOUT) { State = NANDState::Ready; return false; }
				}
				_nand_cmd(data_base, 0x00);
			}

			// read data
			if (bus == NANDBus::Bits8)  { for (stduint i = 0; i < per; i++) *d8++  = _nand_read8(data_base); }
			else                        { for (stduint i = 0; i < per; i++) *d16++ = _nand_read16(data_base); }

			count--;
			nand_address++;
		}
		State = NANDState::Ready;
		return true;
	}

	// AKA HAL_NAND_Write_Page_8b/16b + Write_SpareArea_8b/16b (converged)
	bool FMC_NAND_t::Write(const NANDAddress& a, const void* buf, stduint count, NANDArea area, NANDBus bus) {
		if (State == NANDState::Busy) return false;
		State = NANDState::Busy;
		stduint nand_address = _nand_array_address(a, Config);
		stduint total_pages  = Config.block_size * Config.block_nbr;
		stduint per = (area == NANDArea::Main) ? Config.page_size : Config.spare_area_size;
		const byte* s8 = (const byte*)buf;
		const uint16* s16 = (const uint16*)buf;

		while (count && nand_address < total_pages) {
			// write command sequence (command + column; row after)
			if (area == NANDArea::Main) {
				_nand_cmd(data_base, _NAND_CMD_AREA_A);
				_nand_cmd(data_base, _NAND_CMD_WRITE0);
				if (Config.page_size <= 512) _nand_addr(data_base, 0x00);
				else { _nand_addr(data_base, 0x00); _nand_addr(data_base, 0x00); }
			} else {
				if (Config.page_size <= 512) {
					_nand_cmd(data_base, _NAND_CMD_AREA_C);
					_nand_cmd(data_base, _NAND_CMD_WRITE0);
					_nand_addr(data_base, 0x00);
				} else {
					_nand_cmd(data_base, _NAND_CMD_AREA_A);
					_nand_cmd(data_base, _NAND_CMD_WRITE0);
					stduint col = _nand_column_address(Config, bus);
					_nand_addr(data_base, (uint8)col);
					_nand_addr(data_base, (uint8)(col >> 8));
				}
			}
			_nand_send_row(data_base, nand_address, total_pages);

			// write data
			if (bus == NANDBus::Bits8)  { for (stduint i = 0; i < per; i++) _nand_write8(data_base, *s8++); }
			else                        { for (stduint i = 0; i < per; i++) _nand_write16(data_base, *s16++); }

			// confirm + poll status (timeout corrected vs HAL's per-iteration tickstart)
			_nand_cmd(data_base, _NAND_CMD_WRITE_TRUE1);
			uint64 t0 = SysTick::getTick();
			while (ReadStatus() != _NAND_READY) {
				if ((SysTick::getTick() - t0) > _NAND_WRITE_TIMEOUT) { State = NANDState::Ready; return false; }
			}

			count--;
			nand_address++;
		}
		State = NANDState::Ready;
		return true;
	}

	// AKA HAL_NAND_Erase_Block (unisym adds write-status poll, HAL does not)
	bool FMC_NAND_t::EraseBlock(const NANDAddress& a) {
		if (State == NANDState::Busy) return false;
		State = NANDState::Busy;
		stduint nand_address = _nand_array_address(a, Config);
		_nand_cmd(data_base, _NAND_CMD_ERASE0);
		_nand_addr(data_base, (uint8)nand_address);
		_nand_addr(data_base, (uint8)(nand_address >> 8));
		_nand_addr(data_base, (uint8)(nand_address >> 16));
		_nand_cmd(data_base, _NAND_CMD_ERASE1);
		uint64 t0 = SysTick::getTick();
		while (ReadStatus() != _NAND_READY) {
			if ((SysTick::getTick() - t0) > _NAND_WRITE_TIMEOUT) { State = NANDState::Ready; return false; }
		}
		State = NANDState::Ready;
		return true;
	}

	// AKA HAL_NAND_Read_Status
	stduint FMC_NAND_t::ReadStatus() {
		_nand_cmd(data_base, _NAND_CMD_STATUS);
		uint8 data = _nand_read8(data_base);
		if ((data & _NAND_ERROR) == _NAND_ERROR) return _NAND_ERROR;
		else if ((data & _NAND_READY) == _NAND_READY) return _NAND_READY;
		return _NAND_BUSY;
	}

	// AKA HAL_NAND_ECC_Enable/Disable
	bool FMC_NAND_t::setECC(bool ena) {
		if (State == NANDState::Busy) return false;
		State = NANDState::Busy;
		(*this)[NandReg::PCR].setof(_PCR_ECCEN, ena);
		State = NANDState::Ready;
		return true;
	}

	// AKA HAL_NAND_GetECC (wait SR.FEMPT then read ECCR)
	bool FMC_NAND_t::getECC(stduint& ecc, stduint timeout) {
		if (State == NANDState::Busy) return false;
		State = NANDState::Busy;
		uint64 t0 = SysTick::getTick();
		while (!(*this)[NandReg::SR].bitof(_SR_FEMPT)) {
			if (timeout != 0xFFFFFFFFU) {
				if ((timeout == 0) || ((SysTick::getTick() - t0) > timeout)) { State = NANDState::Ready; return false; }
			}
		}
		ecc = (*this)[NandReg::ECCR];
		State = NANDState::Ready;
		return true;
	}

	// ---- StorageTrait (raw NAND; 1 block = 1 page main area) ----
	// linear page index -> logical NANDAddress (inverse of ARRAY_ADDRESS)
	static NANDAddress _linear_to_address(stduint block, const NANDConfig& c) {
		NANDAddress a{};
		if (c.plane_size && c.block_size) {
			a.plane = (uint16)(block / ((stduint)c.plane_size * c.block_size));
			stduint rem = block % ((stduint)c.plane_size * c.block_size);
			a.block = (uint16)(rem / c.block_size);
			a.page  = (uint16)(rem % c.block_size);
		} else {
			a.page = (uint16)block;
		}
		return a;
	}

	bool FMC_NAND_t::Read(stduint block, void* dest) {
		return Read(_linear_to_address(block, Config), dest, 1, NANDArea::Main, Config.data_bus);
	}

	bool FMC_NAND_t::Write(stduint block, const void* src) {
		return Write(_linear_to_address(block, Config), src, 1, NANDArea::Main, Config.data_bus);
	}

	stduint FMC_NAND_t::getUnits() {
		return Config.block_nbr * Config.block_size;
	}

	int FMC_NAND_t::operator[](uint64 bytid) {
		// TODO: raw NAND is page-granular; byte access requires page read + offset
		return 0;
	}

}

#endif // _MCU_STM32H7x

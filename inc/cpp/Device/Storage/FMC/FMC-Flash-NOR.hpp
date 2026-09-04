// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: [Device] External NOR Flash over FMC NORSRAM controller
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Dependens: RCC, StorageTrait
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

#ifndef _INC_Device_FMC_Flash_NOR
#define _INC_Device_FMC_Flash_NOR

#include "../../../unisym"
#include "../../../reference"
#include "../../../trait/StorageTrait.hpp"
#include "../../../../c/driver/_predefine/predef.fmc.hpp"

namespace uni {

#if defined(_MCU_STM32H7x)

// ---- AKA HAL_NOR_StateTypeDef ----
	enum class NORState : byte { Reset, Ready, Busy, Error, Protected };

	// ---- AKA HAL_NOR_StatusTypeDef ----
	enum class NORStatus : byte { Success, Ongoing, Error, Timeout };

	// ---- AKA FMC_Memory_Type (MTYP): NOR only; SRAM/PSRAM live in SRAMMemory ----
	enum class NORMemory : byte { Nor = 2 }; // MTYP field value 2 -> 0x08 (FMC_MEMORY_TYPE_NOR)

	// ---- AKA NOR_IDTypeDef ----
	struct NORID {
		uint16 manufacturer_code; // AKA Manufacturer_Code
		uint16 device_code1;      // AKA Device_Code1
		uint16 device_code2;      // AKA Device_Code2
		uint16 device_code3;      // AKA Device_Code3
	};

	// ---- AKA NOR_CFITypeDef ----
	struct NORCFI {
		uint16 cfi_1; // AKA CFI_1
		uint16 cfi_2; // AKA CFI_2
		uint16 cfi_3; // AKA CFI_3
		uint16 cfi_4; // AKA CFI_4
	};

	// ---- AKA FMC_NORSRAM_InitTypeDef (trimmed to async NOR flash fields) + device density ----
	struct NORInit {
		SRAMBank bank = SRAMBank::Bank1;     // AKA FMC_NORSRAM_BANK1..4 (NSBank)
		bool data_address_mux = false;       // MUXEN
		NORMemory memory = NORMemory::Nor;   // MTYP (FACCEN auto-set for NOR)
		SRAMDataWidth data_width = SRAMDataWidth::W16; // MWID (NOR supports 8/16 only)
		bool write_operation = true;         // WREN
		bool extended_mode = false;          // EXTMOD (ExtendedMode)
		// device density (no HAL counterpart; user supplies from datasheet/CFI)
		stduint total_bytes = 0;             // total device capacity in bytes
		stduint erase_block_size = 0;        // erase-block unit in bytes (StorageTrait Block_Size)
	};

	// ---- AKA FMC_NORSRAM_TimingTypeDef (trimmed to async NOR flash fields) ----
	struct NORTiming {
		byte address_setup = 0;      // ADDSET
		byte address_hold = 1;       // ADDHLD
		byte data_setup = 1;         // DATAST
		byte bus_turn_around = 0;    // BUSTURN
		SRAMAccess access_mode = SRAMAccess::ModeA; // ACCMOD
	};

	// ---- AKA HAL_NOR_HandleTypeDef + StorageTrait (block device over the NOR memory window) ----
	class FMC_NOR_t : public StorageTrait {
	protected:
		stduint baseaddr;                    // FMC_Bank1_R_BASE (BTCR[0..7])
		stduint extaddr;                     // FMC_Bank1E_R_BASE (BWTR[0..6])
		stduint devbase;                     // NOR bank memory base (0x60000000 + bank*0x4000000)
		NORInit Init;                        // AKA hnor->Init
		NORState State = NORState::Reset;    // AKA hnor->State
		byte data_width = 0;                 // AKA static uwNORMemoryDataWidth (0=8b,1=16b); instance member

		inline Reference btcr(stduint word) const { return Reference(baseaddr + (word << 2)); }
		inline Reference bwtr(stduint word) const { return Reference(extaddr + (word << 2)); }

		// NOR bank memory window helpers (command/data are memory-mapped stores to devbase)
		inline Reference mem(stduint off) const { return Reference(devbase + off); }
		// AKA NOR_ADDR_SHIFT: shift logical word address by device data width (16b -> x2)
		stduint shift_addr(stduint a) const { return data_width ? (a << 1) : a; }

	public:
		FMC_NOR_t(stduint base, stduint ext, stduint dev) : baseaddr(base), extaddr(ext), devbase(dev) {}

		// AKA HAL_NOR_Init (+ MspInit FMCEN inline). Sets Block_Size = Init.erase_block_size.
		bool setMode(const NORInit& init, const NORTiming& timing, const NORTiming& ext_timing);
		// AKA HAL_NOR_DeInit (+ MspDeInit)
		bool canMode();

		// data operations (AKA HAL_NOR_Read + ReadBuffer converged)
		bool Read(pureptr_t addr, void* buf, stduint size);
		// AKA HAL_NOR_Program (single word program)
		bool Program(pureptr_t addr, uint16 data);
		// AKA HAL_NOR_ProgramBuffer (S29GL128P device-specific)
		bool ProgramBuffer(pureptr_t addr, const uint16* data, stduint size);
		// AKA HAL_NOR_Erase_Block / Erase_Chip
		bool EraseBlock(pureptr_t block_addr, pureptr_t dev_addr);
		bool EraseChip(pureptr_t dev_addr);

		// management (AKA HAL_NOR_Read_ID / Read_CFI / ReturnToReadMode)
		bool ReadID(NORID& id);
		bool ReadCFI(NORCFI& cfi);
		bool ReturnToReadMode();

		// control / state
		bool setWriteOperation(bool ena);               // AKA HAL_NOR_WriteOperation_Enable/Disable
		NORState getState() const { return State; }     // AKA HAL_NOR_GetState
		NORStatus getStatus(pureptr_t addr, stduint timeout); // AKA HAL_NOR_GetStatus

		// ---- StorageTrait (block device over the linear NOR window) ----
		// NOTE: NOR is random-addressable; block granularity follows the erase-block unit.
		//       Block_Size = Init.erase_block_size, getUnits = total_bytes / erase_block_size.
		virtual bool Read(stduint block, void* dest, stduint Times = 1) override;
		virtual bool Write(stduint block, const void* src, stduint Times = 1) override;
		virtual stduint getUnits() override;
		virtual int operator[](uint64 bytid) override;
	};

	extern FMC_NOR_t FMC_NOR;

#endif // _MCU_STM32H7x

}

#endif // _INC_Device_FMC_Flash_NOR

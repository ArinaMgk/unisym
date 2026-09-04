// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: [Device] External NAND Flash over FMC NAND controller
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Dependens: RCC, StorageTrait, SysTick
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

#ifndef _INC_Device_FMC_Flash_NAND
#define _INC_Device_FMC_Flash_NAND

#include "../../../unisym"
#include "../../../reference"
#include "../../../trait/StorageTrait.hpp"
#include "../../../../c/driver/_predefine/predef.fmc.hpp"

namespace uni {

#if defined(_MCU_STM32H7x)

// FMC NAND controller register block (AKA FMC_Bank3_TypeDef).
// Word offsets relative to FMC_Bank3_R_BASE (PCR at +0x80, ECCR at +0x94).
enum class NandReg : byte {
	PCR = 0, SR, PMEM, PATT,          // +0x80/0x84/0x88/0x8C
	ECCR = 0x14 / 4                   // +0x94 (RESERVED at 0x90)
};

// ---- AKA HAL_NAND_StateTypeDef ----
enum class NANDState : byte { Reset, Ready, Busy, Error };

// NAND external bus data width (AKA FMC_NAND_Data_Width, PWID)
enum class NANDBus : byte { Bits8 = 0, Bits16 = 1 };

// memory area selector: main page data vs spare area
enum class NANDArea : byte { Main, Spare };

// ---- AKA NAND_IDTypeDef ----
struct NANDID {
	byte maker_id;   // AKA Maker_Id
	byte device_id;  // AKA Device_Id
	byte third_id;   // AKA Third_Id
	byte fourth_id;  // AKA Fourth_Id
};

// ---- AKA NAND_AddressTypeDef (logical row/plane/block addressing) ----
struct NANDAddress {
	uint16 page;     // AKA Page
	uint16 plane;    // AKA Plane
	uint16 block;    // AKA Block
};

// ---- AKA FMC_NAND_PCC_TimingTypeDef (common/attribute space timing) ----
struct NANDPCC {
	byte setup_time = 0;        // AKA SetupTime
	byte wait_setup_time = 0;   // AKA WaitSetupTime
	byte hold_setup_time = 0;   // AKA HoldSetupTime
	byte hiz_setup_time = 0;    // AKA HiZSetupTime
};

// ---- AKA FMC_NAND_InitTypeDef + NAND_DeviceConfigTypeDef (controller + device geometry) ----
struct NANDConfig {
	// ---- controller (AKA FMC_NAND_InitTypeDef) ----
	bool wait_feature = false;      // PWAITEN
	NANDBus data_bus = NANDBus::Bits8; // PWID
	bool ecc_computation = false;   // ECCEN
	stduint ecc_page_size = 0;      // ECCPS (AKA FMC_NAND_ECC_PAGE_SIZE_*)
	byte tclr_setup_time = 0;       // TCLR
	byte tar_setup_time = 0;        // TAR
	NANDPCC common_space;           // AKA ComSpace_Timing
	NANDPCC attribute_space;        // AKA AttSpace_Timing
	// ---- device geometry (AKA NAND_DeviceConfigTypeDef) ----
	stduint page_size = 0;          // main area bytes (or words when 16-bit addressing)
	stduint spare_area_size = 0;    // spare area bytes
	stduint block_size = 0;         // pages per block
	stduint block_nbr = 0;          // total blocks
	stduint plane_nbr = 0;          // number of planes
	stduint plane_size = 0;         // blocks per plane
	bool extra_command_enable = false; // read-mode extra command (some NAND parts)
};

// ---- AKA HAL_NAND_HandleTypeDef + StorageTrait (page-granular raw NAND device) ----
class FMC_NAND_t : public StorageTrait {
protected:
	stduint ctrl_base;                     // FMC_Bank3_R_BASE (PCR/SR/PMEM/PATT/ECCR)
	stduint data_base;                     // NAND data window (NAND_DEVICE)
	NANDConfig Config;                     // AKA hnand->Init + Config
	NANDState State = NANDState::Reset;    // AKA hnand->State

	// AKA FMC_NAND_*_GetFlag/CLEAR_FLAG internal state of the write-status poll
	// command/address/data memory windows (CLE at +CMD, ALE at +ADDR, data at +0)
	inline Reference cmd()  const { return Reference(data_base + 0x10000); } // A16=CLE
	inline Reference addr()  const { return Reference(data_base + 0x20000); } // A17=ALE
	inline Reference data()  const { return Reference(data_base); }

	inline Reference operator[](NandReg idx) const { return Reference(ctrl_base + ((stduint)idx << 2)); }
	// AKA ARRAY_ADDRESS + COLUMN_ADDRESS + ADDR_x_CYCLE helpers are folded in cpp.

public:
	FMC_NAND_t(stduint ctrl, stduint data) : ctrl_base(ctrl), data_base(data) {}

	// AKA HAL_NAND_Init + HAL_NAND_ConfigDevice (converged). Sets Block_Size = Config.page_size.
	bool setMode(const NANDConfig& cfg);
	// AKA HAL_NAND_DeInit
	bool canMode();

	// data operations (4-in-1 convergence of Read/Write Page & SpareArea, 8b/16b)
	bool Read(const NANDAddress& a, void* buf, stduint count, NANDArea area = NANDArea::Main, NANDBus bus = NANDBus::Bits8);
	bool Write(const NANDAddress& a, const void* buf, stduint count, NANDArea area = NANDArea::Main, NANDBus bus = NANDBus::Bits8);
	// AKA HAL_NAND_Erase_Block (plus write-status poll)
	bool EraseBlock(const NANDAddress& a);
	// AKA HAL_NAND_Read_ID
	bool ReadID(NANDID& id);
	// AKA HAL_NAND_Reset
	bool Reset();
	// AKA HAL_NAND_Read_Status (issuing NAND_CMD_STATUS)
	stduint ReadStatus();

	// ECC (AKA HAL_NAND_ECC_Enable/Disable converged, HAL_NAND_GetECC)
	bool setECC(bool ena);
	bool getECC(stduint& ecc, stduint timeout = 0);

	// state (AKA HAL_NAND_GetState)
	NANDState getState() const { return State; }

	// ---- StorageTrait (raw NAND; 1 block = 1 page main area) ----
	// Block_Size = Config.page_size, getUnits = block_nbr * block_size.
	// Spare area is NOT part of the block device; access it via Read/Write(area=Spare).
	virtual bool Read(stduint block, void* dest, stduint Times = 1) override;
	virtual bool Write(stduint block, const void* src, stduint Times = 1) override;
	virtual stduint getUnits() override;
	virtual int operator[](uint64 bytid) override;
};

extern FMC_NAND_t FMC_NAND;

#endif // _MCU_STM32H7x

}

#endif // _INC_Device_FMC_Flash_NAND

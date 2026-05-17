// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Stroage) Floppy
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

#ifndef _INCPP_Storage_Floppy
#define _INCPP_Storage_Floppy


#include "../stdinc.h"


// FDC Commands
#define FDC_CMD_READ_TRACK     0x02
#define FDC_CMD_SPECIFY        0x03
#define FDC_CMD_SENSE_STAT     0x04
#define FDC_CMD_WRITE_DATA     0xC5 // MT=1, MFM=1 (Multitrack, MFM mode)
#define FDC_CMD_READ_DATA      0xE6 // MT=1, MFM=1, SK=0
#define FDC_CMD_RECALIBRATE    0x07
#define FDC_CMD_SENSE_INT      0x08
#define FDC_CMD_FORMAT_TRACK   0x4D
#define FDC_CMD_SEEK           0x0F

#ifdef _INC_CPP

#include "../../cpp/trait/StorageTrait.hpp"
#include "../../cpp/interrupt"

namespace uni {
	#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)

	// Standard types read from CMOS 0x10 register
	enum class FloppyDriveType : byte {
		None = 0x00,
		Drive_360KB_5_25 = 0x01,
		Drive_1_2MB_5_25 = 0x02,
		Drive_720KB_3_5  = 0x03,
		Drive_1_44MB_3_5 = 0x04,
		Drive_2_88MB_3_5 = 0x05
	};

	class FloppyDisk : public StorageTrait, public RuptTrait
	{
	public:
		enum class ReactType {
			None, Loop, Rupt,
		};

		byte id; // 0 for Drive A, 1 for Drive B
		
	protected:
		word io_base; 
		bool motor_state;

		// Dynamic geometry and physical parameters
		FloppyDriveType drive_type;
		byte CYLINDERS;
		byte HEADS;
		byte SECTORS_PER_TRACK;
		byte GAP3_LENGTH; // Gap length for formatting and R/W
		byte DATA_RATE;   // Data transfer rate configuration

	public:
		ReactType react_type = ReactType::Loop;
		bool (*fn_cmd_wait)(FloppyDisk* fd) = 0;
		void (*fn_int_wait)() = 0;
		void (*fn_feedback)() = 0;
		stdsint units = -1;

	public:
		FloppyDisk(byte id = 0, FloppyDriveType type = FloppyDriveType::Drive_1_44MB_3_5);

		virtual bool Read(stduint BlockIden, void* Dest) override;
		virtual bool Write(stduint BlockIden, const void* Sors) override;
		virtual stduint getUnits() override;
		virtual int operator[](uint64 bytid) override { return _TODO 0; }
		
		virtual void setInterrupt(Handler_t _func) const override;
		virtual void setInterruptPriority(byte preempt, byte sub_priority) const override;
		virtual void enInterrupt(bool enable = true) const override;

		virtual PartitionSlice getSlice(stduint dev) override;

		byte getID() const { return id; }
		FloppyDriveType getType() const { return drive_type; }

		// FDC Specific Methods
		void Reset();
		void Motor(bool on);
		void LBA2CHS(stduint lba, byte& cyl, byte& head, byte& sec);
		bool IsMediaPresent(); 
		
	protected:
		void WaitRQM();
		void WriteCmd(byte cmd);
		byte ReadData();
		void SenseInt(byte& st0, byte& cyl);
		void Recalibrate();
		void SetGeometry(); // Populate parameters based on drive_type
	};

	
	#endif
} // namespace uni

#endif
#endif

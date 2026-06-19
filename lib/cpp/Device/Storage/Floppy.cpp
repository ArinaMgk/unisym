// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Storage) Floppy Disk Implementation
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/c/storage/floppy.h"
#include "../../../../inc/c/driver/i8259A.h"

#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
#include "../../../../inc/c/board/IBM.h"

#pragma GCC optimize("O0")
namespace uni {

	static bool flp_result_ready() {
		return (innpb(PORT_FDC_MSR) & 0xC0) == 0xC0;
	}

	static bool flp_result_ready_retry() {
		for (int retry = 0; retry < 5; retry++) {
			for (volatile int i = 0; i < 100000; i++) _TEMP;
			if (flp_result_ready()) return true;
		}
		return false;
	}

	FloppyDisk::FloppyDisk(byte id, FloppyDriveType type) : id(id), drive_type(type) {
		Block_buffer = nullptr;
		Block_Size = 512; // Standard Floppy Sector Size
		io_base = 0x3F0;  // Standard primary FDC base port
		motor_state = false;
		SetGeometry();    // Initialize geometry parameters dynamically
	}

	void FloppyDisk::SetGeometry() {
		HEADS = 2; // Almost all modern floppy disks are double-sided
		switch (drive_type) {
			case FloppyDriveType::Drive_360KB_5_25:
				CYLINDERS = 40; SECTORS_PER_TRACK = 9;  GAP3_LENGTH = 0x2A; DATA_RATE = 0x02; // 250 Kbps
				break;
			case FloppyDriveType::Drive_1_2MB_5_25:
				CYLINDERS = 80; SECTORS_PER_TRACK = 15; GAP3_LENGTH = 0x1B; DATA_RATE = 0x00; // 500 Kbps
				break;
			case FloppyDriveType::Drive_720KB_3_5:
				CYLINDERS = 80; SECTORS_PER_TRACK = 9;  GAP3_LENGTH = 0x2A; DATA_RATE = 0x02; // 250 Kbps
				break;
			case FloppyDriveType::Drive_2_88MB_3_5:
				CYLINDERS = 80; SECTORS_PER_TRACK = 36; GAP3_LENGTH = 0x38; DATA_RATE = 0x03; // 1 Mbps
				break;
			case FloppyDriveType::Drive_1_44MB_3_5:
			default:
				CYLINDERS = 80; SECTORS_PER_TRACK = 18; GAP3_LENGTH = 0x1B; DATA_RATE = 0x00; // 500 Kbps
				break;
		}
	}

	void FloppyDisk::LBA2CHS(stduint lba, byte& cyl, byte& head, byte& sec) {
		sec  = (lba % SECTORS_PER_TRACK) + 1;
		head = (lba / SECTORS_PER_TRACK) % HEADS;
		cyl  = (lba / (SECTORS_PER_TRACK * HEADS)); 
	}

	void FloppyDisk::setInterruptPriority(byte preempt, byte sub_priority) const {
		// EMPTY
	}

	void FloppyDisk::enInterrupt(bool enable) const {
		// EMPTY
	}

	void FloppyDisk::setInterrupt(Handler_t unused_func) const {
		if (id >= 2) return; 
		i8259Master_Enable(6); // Floppy uses IRQ 6 on standard x86
	}

	void FloppyDisk::WaitRQM() {
		stduint timeout = 100000;
		while (timeout--) {
			if (innpb(PORT_FDC_MSR) & 0x80) { // Bit 7: RQM (Request for Master)
				return;
			}
		}
	}

	void FloppyDisk::WriteCmd(byte cmd) {
		WaitRQM();
		outpb(PORT_FDC_FIFO, cmd);
	}

	byte FloppyDisk::ReadData() {
		WaitRQM();
		return innpb(PORT_FDC_FIFO);
	}

	void FloppyDisk::Motor(bool on) {
		if (motor_state == on) return;
		
		// DOR Register: 
		// Bit 7: Motor D, Bit 6: Motor C, Bit 5: Motor B, Bit 4: Motor A
		// Bit 3: DMA/INT Enable, Bit 2: RESET#, Bit 1-0: Drive Select
		byte dor = 0x0C; // DMA/INT enabled, FDC not in reset
		if (on) {
			dor |= (1 << (4 + id)); // Turn on specific motor
			dor |= id;              // Select drive
		}
		
		outpb(PORT_FDC_DOR, dor);
		motor_state = on;

		// Delay for motor spin-up (~300ms)
		for (volatile int i = 0; i < 1000000; i++) _TEMP;
	}

	void FloppyDisk::SenseInt(byte& st0, byte& cyl) {
		WriteCmd(FDC_CMD_SENSE_INT);
		st0 = ReadData();
		cyl = ReadData();
	}

	void FloppyDisk::Recalibrate() {
		Motor(true);
		asserv(fn_feedback)();
		WriteCmd(FDC_CMD_RECALIBRATE);
		WriteCmd(id);
		
		if (fn_int_wait) fn_int_wait(); 
		
		byte st0, cyl;
		SenseInt(st0, cyl);
		Motor(false);
	}

	bool FloppyDisk::IsMediaPresent() {
		Motor(true);

		// Bit 7 of DIR register indicates Disk Change status
		if ((innpb(PORT_FDC_DIR) & 0x80) == 0) {
			return true; // Disk is present and unaltered
		}

		// Force seek/recalibrate to clear potential false positive from recent disk swap
		asserv(fn_feedback)();
		WriteCmd(FDC_CMD_RECALIBRATE);
		WriteCmd(id);
		
		if (fn_int_wait) fn_int_wait(); 

		byte st0, cyl;
		SenseInt(st0, cyl); 

		byte dir = innpb(PORT_FDC_DIR);
		Motor(false); 

		if (dir & 0x80) {
			return false; // Still set, meaning no media present
		}

		return true;
	}

	_WEAK bool FloppyDisk::Read(stduint BlockIden, void* Dest) {
		if (BlockIden >= getUnits()) return false;

		byte cyl, head, sec;
		LBA2CHS(BlockIden, cyl, head, sec);

		Motor(true);
		asserv(fn_feedback)();

		// Configure data transfer rate dynamically based on drive type
		outpb(PORT_FDC_CCR, DATA_RATE);

		// Note: Requires ISA DMA Channel 2 setup here (e.g., DMA_Setup_Read)

		WriteCmd(FDC_CMD_READ_DATA);
		WriteCmd((head << 2) | id); 
		WriteCmd(cyl);
		WriteCmd(head);
		WriteCmd(sec);
		WriteCmd(0x02); // 512 Bytes/Sector
		WriteCmd(SECTORS_PER_TRACK);
		WriteCmd(GAP3_LENGTH);
		WriteCmd(0xFF); // DTL

		if (react_type == ReactType::Rupt && fn_int_wait) {
			if (!fn_int_wait() && !flp_result_ready_retry()) return false;
		} else {
			for (volatile int i = 0; i < 100000; i++) _TEMP;
		}

		// Read 7 Status Bytes after operation completion
		byte st0 = ReadData();
		for (int i = 0; i < 6; i++) ReadData(); 

		Motor(false);

		// Check for errors in ST0
		if ((st0 & 0xC0) != 0x00) return false;
		return true;
	}

	_WEAK bool FloppyDisk::Write(stduint BlockIden, const void* Sors) {
		if (BlockIden >= getUnits()) return false;

		byte cyl, head, sec;
		LBA2CHS(BlockIden, cyl, head, sec);

		Motor(true);
		asserv(fn_feedback)();

		// Configure data transfer rate dynamically
		outpb(PORT_FDC_CCR, DATA_RATE);

		// Note: Requires ISA DMA Channel 2 setup here (e.g., DMA_Setup_Write)

		WriteCmd(FDC_CMD_WRITE_DATA);
		WriteCmd((head << 2) | id);
		WriteCmd(cyl);
		WriteCmd(head);
		WriteCmd(sec);
		WriteCmd(0x02); 
		WriteCmd(SECTORS_PER_TRACK);
		WriteCmd(GAP3_LENGTH);
		WriteCmd(0xFF); 

		if (react_type == ReactType::Rupt && fn_int_wait) {
			if (!fn_int_wait() && !flp_result_ready_retry()) return false;
		}

		// Read 7 Status Bytes
		byte st0 = ReadData();
		for (int i = 0; i < 6; i++) ReadData(); 

		Motor(false);
		
		if ((st0 & 0xC0) != 0x00) return false;
		return true;
	}

	_WEAK stduint FloppyDisk::getUnits() {
		return (CYLINDERS * HEADS * SECTORS_PER_TRACK); 
	}

	_WEAK PartitionSlice FloppyDisk::getSlice(stduint dev) {
		PartitionSlice slice;
		slice.address = 0;
		slice.length = getUnits();
		return slice;
	}
}
#endif

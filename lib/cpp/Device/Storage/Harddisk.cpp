// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Stroage) Harddisk
// Codifiers: @dosconio: 20250107
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

#include "../../../../inc/c/storage/harddisk.h"
#include "../../../../inc/c/driver/i8259A.h"

#if defined(_DEV_GCC) && defined(_MCCA) && _MCCA == 0x8632
#include "../../../../inc/cpp/Device/Storage/HD-DEPEND.h"
namespace uni {

	bool Harddisk_PATA::Hdisk_OUT(HdiskCommand* hd_cmd, bool (*hd_cmd_wait)()) {
		if (!hd_cmd_wait()) return false;
		// Interrupt Enable (nIEN)
		outpb(REG_DEV_CTRL, 0);
		// Load required parameters in the Command Block Registers
		outpb(REG_FEATURES, hd_cmd->feature);
		outpb(REG_NSECTOR, hd_cmd->count);
		for0(i, 3) outpb(REG_LBA_LOW + i, hd_cmd->LBA[i]);
		outpb(REG_DEVICE, hd_cmd->device);
		// Write the command code to the Command Register
		outpb(REG_CMD, hd_cmd->command);
		return true;
	}
	
	void Harddisk_PATA::setInterruptPriority(byte preempt, byte sub_priority) const {
		// EMPTY
	}
	void Harddisk_PATA::enInterrupt(bool enable) const {
		// EMPTY
	}

	void Harddisk_PATA::setInterrupt(Handler_t unused_func) const {
		if (id >= 2) return;
		i8259Slaver_Enable(_IMM(IRQ_ATA_DISK0) + id - _i8259A_SLV_IDSTART);
		i8259Master_Enable(2);
	}



	_WEAK bool Harddisk_PATA::Read(stduint BlockIden, void* Dest) {
		stduint C, B;
		__asm volatile("mov %%ecx, %0" : "=r" (C));// will break GNU stack judge: __asm ("push %ecx");
		__asm volatile("mov %%ebx, %0" : "=r" (B));// will break GNU stack judge: __asm ("push %ebx");
		__asm volatile("mov %0, %%ebx" : : "r" _IMM(Dest));// gcc use mov %eax->%ebx to assign
		__asm volatile("mov %0, %%eax": : "r" (BlockIden));
		__asm volatile("mov $1, %ecx");
		__asm volatile("call HdiskLBA28Load");
		__asm volatile("mov %0, %%ebx" : : "r" (B));// rather __asm ("pop %ebx");
		__asm volatile("mov %0, %%ecx" : : "r" (C));// rather __asm ("pop %ecx");
		return true;
	}

	_WEAK bool Harddisk_PATA::Write(stduint BlockIden, const void* Sors) {
		return _TODO false;
	}





	

}
#endif

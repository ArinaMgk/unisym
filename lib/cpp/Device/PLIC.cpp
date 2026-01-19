// ASCII CPP TAB4 CRLF
// Docutitle: (Device) RISCV PLIC
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

#include "../../../inc/cpp/interrupt"

using namespace uni;

#if defined(_MCCA) && (_MCCA & 0xFF00) == 0x1000
#include "../../../inc/c/proctrl/RISCV/riscv.h"
#include "../../../inc/c/board/QemuVirt-Riscv.h"


#define VIRT_PLIC_HART_CONFIG    "MS"
#define VIRT_PLIC_NUM_SOURCES    127
#define VIRT_PLIC_NUM_PRIORITIES 7
#define VIRT_PLIC_PRIORITY_BASE  0x04
#define VIRT_PLIC_PENDING_BASE   0x1000
#define VIRT_PLIC_ENABLE_BASE    0x2000
#define VIRT_PLIC_ENABLE_STRIDE  0x80
#define VIRT_PLIC_CONTEXT_BASE   0x200000
#define VIRT_PLIC_CONTEXT_STRIDE 0x1000
#define VIRT_PLIC_SIZE(__num_context) \
	(VIRT_PLIC_CONTEXT_BASE + (__num_context) * VIRT_PLIC_CONTEXT_STRIDE)

/* 
* Each PLIC interrupt source can be assigned a priority by writing 
* to its 32-bit memory-mapped priority register.
* The QEMU-virt (the same as FU540-C000) supports 7 levels of priority. 
* A priority value of 0 is reserved to mean "never interrupt" and 
* effectively disables the interrupt. 
* Priority 1 is the lowest active priority, and priority 7 is the highest. 
* Ties between global interrupts of the same priority are broken by 
* the Interrupt ID; interrupts with the lowest ID have the highest 
* effective priority.
*/
void InterruptControl::setPriority(Request_t req, uint32 priority)
{
	uint32* p = (uint32*)(ADDR_VIRT_PLIC + _IMM(req) * 4);
	*p = priority;
}

/*
* PLIC will mask all interrupts of a priority less than or equal to threshold.
* Maximum threshold is 7.
* For example, a threshold value of zero permits all interrupts with
* non-zero priority, whereas a value of 7 masks all interrupts.
* Notice, the threshold is global for PLIC, not for each interrupt source.
*/
void InterruptControl::setPriorityThreshold(uint32 core, uint32 threshold)
{
	uint32* p = (uint32*)(ADDR_VIRT_PLIC + VIRT_PLIC_SIZE(core) + 0);
	*p = threshold;
}

void InterruptControl::setAble(Request_t req, bool ena)
{
	uint32* p = (uint32*)(ADDR_VIRT_PLIC + VIRT_PLIC_ENABLE_BASE + _IMM(req) / bitsof(uint32) * byteof(uint32));
	if (ena) *p |= _IMM1S(_IMM(req) % bitsof(uint32));
	else {
		_TODO
	}
}

/* 
*	Query the PLIC what interrupt we should serve.
*	Perform an interrupt claim by reading the claim register, which
*	returns the ID of the highest-priority pending interrupt or zero if there is no pending interrupt. 
*	A successful claim also atomically clears the corresponding pending bit on the interrupt source.
* RETURN VALUE:
*	the ID of the highest-priority pending interrupt or zero if there is no pending interrupt.
*/
Request_t InterruptControl::getLastRequest()
{
	int hart = getTP();
	uint32* pMCLAIM = (uint32*)(ADDR_VIRT_PLIC + VIRT_PLIC_SIZE(hart) + 4);
	return (Request_t)*pMCLAIM;
}


/* 
*	Writing the interrupt ID it received from the claim (irq) to the 
*	complete register would signal the PLIC we've served this IRQ. 
*	The PLIC does not check whether the completion ID is the same as the 
*	last claim ID for that target. If the completion ID does not match an 
*	interrupt source that is currently enabled for the target, the completion
*	is silently ignored.
*/
void InterruptControl::setLastRequest(Request_t req)
{
	int hart = getTP();
	uint32* pMCOMPLETE = (uint32*)(ADDR_VIRT_PLIC + VIRT_PLIC_SIZE(hart) + 4);
	*pMCOMPLETE = _IMM(req);
}

#endif

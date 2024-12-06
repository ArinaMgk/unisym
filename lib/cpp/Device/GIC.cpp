// ASCII CPP TAB4 CRLF
// Docutitle: (Device) GIC
// Codifiers: @dosconio: 20241129
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

#include "../../../inc/c/driver/interrupt/GIC.h"

#ifdef _MPU_STM32MP13

//{TEMP}
extern "C" {
	void TMP_GIC_Enable(void);
	extern Handler_t IRQ_Vector_Table[MAX_IRQ_n];
}

namespace uni {
	GIC_t GIC;

	stduint GIC_t::IRQ_ID0;
	bool GIC_t::enable;

	
	void GIC_t::enAble(bool ena) const {
		if (ena) {
			TMP_GIC_Enable();
		}
	}

	void GIC_t::setHandler(Request_t id, Handler_t handler) const {
		if (id < 0 || id >= IRQ_GIC_LINE_COUNT) return;
		IRQ_Vector_Table[id] = handler;
	}

	Handler_t GIC_t::getHandler(Request_t id) const {
		stduint irq = _IMM(id) & 0x3FF;
		if (irq >= IRQ_GIC_LINE_COUNT) return nullptr;
		return IRQ_Vector_Table[irq];
	}

	// AKA GIC_EnableIRQ + GIC_DisableIRQ
	void GIC_t::enInterrupt(Request_t id, bool ena) const {
		if (id < 0 || id >= IRQ_GIC_LINE_COUNT) return;
		uint32* addr = (uint32*)_IMM(&self
			[ena ? GICDistributor::ISENABLER : GICDistributor::ICENABLER]);
		stduint IRQn = _IMM(id);
		addr[IRQn / 32U] = _IMM1S(IRQn % 32U);
	}

	// AKA GIC_GetEnableIRQ
	bool GIC_t::ifInterrupt(Request_t id) const {
		if (id < 0 || id >= IRQ_GIC_LINE_COUNT) return false;
		uint32* addr = (uint32*)_IMM(&self[GICDistributor::ISENABLER]);
		stduint IRQn = _IMM(id);
		return (addr[IRQn / 32U] >> (IRQn % 32U)) & _IMM1;
	}

	stduint GIC_t::getDistributorInfo() const {
		return self[GICDistributor::TYPER];
	}

	void GIC_t::setConfig(Request_t IRQn, uint32 int_config) const {
		// 32 = 16 * 2
		uint32* addr = (uint32*)_IMM(&self[GICDistributor::ICFGR]);
		addr += _IMM(IRQn) / 16;
		uint32 shift = _IMMx2(IRQn % 16U);
		Reference(addr).maset(shift, 2, int_config);
	}
	uint32 GIC_t::getConfig(Request_t IRQn) const {
		uint32* addr = (uint32*)_IMM(&self[GICDistributor::ICFGR]);
		addr += _IMM(IRQn) / 16;
		uint32 shift = _IMMx2(IRQn % 16U);
		return Reference(addr).masof(shift, 2);
		// ST's code may make a mistake
		// return (GICDistributor->ICFGR[IRQn / 16U] >> ((IRQn % 16U) >> 1U));
	}

	void GIC_t::setTarget(Request_t IRQn, uint32 target) const {
		uint32* addr = (uint32*)_IMM(&self[GICDistributor::ITARGETSR]);
		addr += _IMM(IRQn) / 4;
		uint32 shift = (IRQn % 4U) * 8U;
		Reference(addr).maset(shift, 8, target);
	}
	uint32 GIC_t::getTarget(Request_t IRQn) const {
		uint32* addr = (uint32*)_IMM(&self[GICDistributor::ITARGETSR]);
		addr += _IMM(IRQn) / 4;
		uint32 shift = (IRQn % 4U) * 8U;
		return Reference(addr).masof(shift, 8);
	}

	void GIC_t::setGroup(Request_t IRQn, uint32 group) const {
		uint32* addr = (uint32*)_IMM(&self[GICDistributor::IGROUPR]);
		addr += _IMM(IRQn) / 32U;
		uint32 shift = (IRQn % 32U);
		Reference(addr).maset(shift, 1, group);
	}

	void GIC_t::setPriority(Request_t IRQn, stduint priority) const {
		uint32* addr = (uint32*)_IMM(&self[GICDistributor::IPRIORITYR]);
		addr += _IMM(IRQn) / 4U;
		uint32 shift = (IRQn % 4U) * 8U;
		Reference(addr).maset(shift, 8, priority);
	}
	stduint GIC_t::getPriority(Request_t IRQn) const {
		uint32* addr = (uint32*)_IMM(&self[GICDistributor::IPRIORITYR]);
		addr += _IMM(IRQn) / 4U;
		uint32 shift = (IRQn % 4U) * 8U;
		return Reference(addr).masof(shift, 8);
	}

	// ---- ---- ---- ---- ---- ---- ---- ----

	stduint RuptRequest::getStatus() const {
		stduint IRQn = _IMM(this);
		uint32 shift = (IRQn % 32U);
		//
		uint32* addr = (uint32*)_IMM(&GIC[GICDistributor::ISACTIVER]);
		addr += _IMM(IRQn) / 32U;
		stduint active = Reference(addr).masof(shift, 1) << 1;
		//
		addr = (uint32*)_IMM(&GIC[GICDistributor::ISPENDR]);
		addr += _IMM(IRQn) / 32U;
		stduint pending = Reference(addr).masof(shift, 1);
		return (active | pending);
	}

	void RuptRequest::setPending(bool to_pend) const {
		stduint IRQn = _IMM(this);
		if (IRQn >= 16U) {
			uint32* addr = (uint32*)_IMM(&GIC
				[to_pend ? GICDistributor::ISPENDR : GICDistributor::ICPENDR]);
			addr[_IMM(IRQn) / 32U] = _IMM1S(IRQn % 32U);
		}
		// INTID 0-15 Software Generated Interrupt
		else if (to_pend) {
			// Forward the interrupt to the CPU interface that requested it
			GIC[GICDistributor::SGIR] = IRQn | 0x02000000U;
		}
		else {
			uint32* addr = (uint32*)_IMM(&GIC[GICDistributor::CPENDSGIR]);
			addr[IRQn / 4U] = _IMM1S((IRQn % 4U) * 8U);
		}
	}
	bool RuptRequest::getPending() const {
		bool pend;
		stduint IRQn = _IMM(this);
		if (IRQn >= 16U) {
			uint32* addr = (uint32*)_IMM(&GIC[GICDistributor::ISPENDR]);
			addr += _IMM(IRQn) / 32U;
			uint32 shift = IRQn % 32U;
			pend = Reference(addr).bitof(shift);
		}
		else {
			// INTID 0-15 Software Generated Interrupt
			uint32* addr = (uint32*)_IMM(&GIC[GICDistributor::SPENDSGIR]);
			addr += IRQn / 4U;
			uint32 shift = (IRQn % 4U) * 8U;
			pend = Reference(addr).masof(shift, 8);
		}
		return pend;
	}

}

stdsint IRQ_Initialize(void) {
	uni::GIC.canMode();
	uni::GIC.enAble();
	return 0;
}

int32_t IRQ_SetHandler(Request_t id, Handler_t handler) {
	if (id < 0 || id >= IRQ_GIC_LINE_COUNT) return -1;
	uni::GIC.setHandler(id, handler);
	return 0;
}

Handler_t IRQ_GetHandler(Request_t irqn) {
	return uni::GIC.getHandler(irqn);
}

int32_t IRQ_Enable(Request_t id) {
	if (id < 0 || id >= IRQ_GIC_LINE_COUNT) return -1;
	uni::GIC.enInterrupt(id, true);
	return 0;
}
int32_t IRQ_Disable(Request_t irqn) {
	if (irqn < 0 || irqn >= IRQ_GIC_LINE_COUNT) return -1;
	uni::GIC.enInterrupt(irqn, false);
	return 0;
}

/// Get interrupt enable state.
uint32_t IRQ_GetEnableState(Request_t irqn) {
	if (irqn < 0 || irqn >= IRQ_GIC_LINE_COUNT) return 0;
	return uni::GIC.ifInterrupt(irqn);
}

// ---------------------

/// Configure interrupt request mode.
int32_t IRQ_SetMode(Request_t irqn, uint32_t mode) {
	using namespace uni;
	uint32_t val;
	uint8_t cfg;
	uint8_t secure;
	uint8_t cpu;
	int32_t status = 0;
	if (irqn < 0 || irqn >= IRQ_GIC_LINE_COUNT) return status;
	//
	// Check triggering mode
	val = (mode & IRQ_MODE_TRIG_Msk);
	if (val == IRQ_MODE_TRIG_LEVEL) {
		cfg = 0x00U;
	}
	else if (val == IRQ_MODE_TRIG_EDGE) {
		cfg = 0x02U;
	}
	else {
		cfg = 0x00U;
		status = -1;
	}
	val = (mode & IRQ_MODE_MODEL_Msk);
	if (val == IRQ_MODE_MODEL_1N) {
		cfg |= 1;   // 1-N model
	}
	// Check interrupt type
	val = mode & IRQ_MODE_TYPE_Msk;
	if (val != IRQ_MODE_TYPE_IRQ) {
		status = -1;
	}
	// Check interrupt domain
	val = mode & IRQ_MODE_DOMAIN_Msk;
	if (val == IRQ_MODE_DOMAIN_NONSECURE) {
		secure = 0U;
	}
	else {
	// Check security extensions support
		val = GIC.getDistributorInfo() & (1UL << 10U);

		if (val != 0U) {
			// Security extensions are supported
			secure = 1U;
		}
		else {
			secure = 0U;
			status = -1;
		}
	}
	// Check interrupt CPU targets
	val = mode & IRQ_MODE_CPU_Msk;
	if (val == IRQ_MODE_CPU_ALL) {
		cpu = 0xFFU;
	}
	else {
		cpu = (uint8_t)(val >> IRQ_MODE_CPU_Pos);
	}
	// Apply configuration if no mode error
	if (status == 0) {
		GIC.setConfig(irqn, cfg);
		GIC.setTarget(irqn, cpu);
		if (secure != 0U) {
			GIC.setGroup(irqn, secure);
		}
	}
	return (status);
}

/// Get interrupt mode configuration.
uint32_t IRQ_GetMode(Request_t irqn) {
	using namespace uni;
	uint32_t mode;
	uint32_t val;
	if (irqn < 0 || irqn >= IRQ_GIC_LINE_COUNT) return mode = IRQ_MODE_ERROR;
	//
	mode = IRQ_MODE_TYPE_IRQ;
	// Get trigger mode
	val = GIC.getConfig(irqn);
	if ((val & 2U) != 0U) {
		// Corresponding interrupt is edge triggered
		mode |= IRQ_MODE_TRIG_EDGE;
	}
	else {
	// Corresponding interrupt is level triggered
		mode |= IRQ_MODE_TRIG_LEVEL;
	}
	if (val & 1U) {
		mode |= IRQ_MODE_MODEL_1N;
	}
	// Get interrupt CPU targets
	mode |= GIC.getTarget(irqn) << IRQ_MODE_CPU_Pos;
	return (mode);
}


/// Get ID number of current interrupt request (IRQ).
Request_t IRQ_GetActiveIRQ(void) {
	using namespace uni;
	Request_t irqn;
	uint32_t prio;
	GIC.getHighPendingRequest();// Dummy read to avoid GIC 390 errata 801120
	irqn = GIC.AcknowledgePending();
	_ASM volatile ("dsb 0xF":::"memory"); //__DSB();
	/* Workaround GIC 390 errata 733075 (GIC-390_Errata_Notice_v6.pdf, 09-Jul-2014)  */
	/* The following workaround code is for a single-core system.  It would be       */
	/* different in a multi-core system.                                             */
	/* If the ID is 0 or 0x3FE or 0x3FF, then the GIC CPU interface may be locked-up */
	/* so unlock it, otherwise service the interrupt as normal.                      */
	/* Special IDs 1020=0x3FC and 1021=0x3FD are reserved values in GICv1 and GICv2  */
	/* so will not occur here.                                                       */

	if ((irqn == 0) || (irqn >= 0x3FE)) {
	  /* Unlock the CPU interface with a dummy write to Interrupt Priority Register */
		prio = GIC.getPriority(Request_None);
		uni::GIC.setPriority(Request_None, prio);
		_ASM volatile ("dsb 0xF":::"memory"); //__DSB();
		if ((irqn == 0U) && ((GIC[irqn].getStatus() & 1U) != 0U) && (GIC_t::IRQ_ID0 == 0U)) {
		  /* If the ID is 0, is active and has not been seen before */
			GIC_t::IRQ_ID0 = 1U;
		}
		/* End of Workaround GIC 390 errata 733075 */
	}

	return (irqn);
}

/// Get ID number of current fast interrupt request (FIQ).
Request_t IRQ_GetActiveFIQ(void) {
	return (Request_t)-1;
}

/// Signal end of interrupt processing.
int32_t IRQ_EndOfInterrupt(Request_t irqn) {
	using namespace uni;
	if (irqn < 0 || irqn >= IRQ_GIC_LINE_COUNT) return -1;
	GIC.EndOfInterrupt(irqn);
	if (irqn == 0) GIC_t::IRQ_ID0 = 0U;
	return 0;
}


/// Set interrupt pending flag.
int32_t IRQ_SetPending(Request_t irqn) {
	if (irqn < 0 || irqn >= IRQ_GIC_LINE_COUNT) return -1;
	uni::GIC[irqn].setPending();
	return 0;
}

/// Get interrupt pending flag.
uint32_t IRQ_GetPending(Request_t irqn) {
	if (irqn < 0 || irqn >= IRQ_GIC_LINE_COUNT) return 0;
	else return uni::GIC[irqn].getPending();
}


/// Clear interrupt pending flag.
int32_t IRQ_ClearPending(Request_t irqn) {
	if (irqn < 0 || irqn >= IRQ_GIC_LINE_COUNT) return -1;
	uni::GIC[irqn].setPending(false);
	return 0;
}


/// Set interrupt priority value.
int32_t IRQ_SetPriority(Request_t irqn, uint32_t priority) {
	if (irqn < 0 || irqn >= IRQ_GIC_LINE_COUNT) return -1;
	uni::GIC.setPriority(irqn, priority);
	return 0;
}

/// Get interrupt priority.
uint32_t IRQ_GetPriority(Request_t irqn) {
	if (irqn < 0 || irqn >= IRQ_GIC_LINE_COUNT) return IRQ_PRIORITY_ERROR;
	return uni::GIC.getPriority(irqn);
}

/// Set priority masking threshold.
int32_t IRQ_SetPriorityMask(uint32_t priority) {
	uni::GIC.setInterfacePriorityMask(priority);
	return (0);
}


/// Get priority masking threshold
uint32_t IRQ_GetPriorityMask(void) {
	return uni::GIC.getInterfacePriorityMask();
}


/// Set priority grouping field split point
int32_t IRQ_SetPriorityGroupBits(uint32_t bits) {
	if (bits == IRQ_PRIORITY_Msk) {
		bits = 7U;
	}
	if (bits < 8U) {
		uni::GIC.setBinaryPoint(7U - bits);
		return 0;
	}
	else return -1;
}


/// Get priority grouping field split point
uint32_t IRQ_GetPriorityGroupBits(void) {
	uint32_t bp;
	bp = uni::GIC.getBinaryPoint() & 0x07U;
	return (7U - bp);
}


#endif

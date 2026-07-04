// ASCII CPP TAB4 CRLF
// Docutitle: Interrupt
// Codifiers: @dosconio: 20240527
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

#include "../../inc/cpp/interrupt"

namespace uni {
// Occupiers

	void RuptTrait::setInterrupt(Handler_t f) const { (void)f; }
	void RuptTrait::setInterruptPriority(byte preempt, byte sub_priority) const { (void)preempt; (void)sub_priority; }
	void RuptTrait::enInterrupt(bool enable) const { (void)enable; }
}

#if defined(_MCCA)
#if (_MCCA & 0xFF00) == 0x8600
#include "../../inc/c/driver/i8259A.h"
#include "../../inc/c/proctrl/IAx86_64.msr.h"
#elif (_MCCA & 0xFF00) == 0x1000
#include "../../inc/c/proctrl/RISCV/riscv.h"
#endif

//{TODO} Implement in Magice/AASM, because GCC compile this may be bad for different version or optimization level.
// __attribute__((interrupt)) is useless

#if (_MCCA & 0xFF00) == 0x8600

static stduint ERQ_Handlers[0x20]{
	_IMM(Divide_By_Zero_ERQHandler),
	_IMM(Step_ERQHandler),
	_IMM(NMI_ERQHandler),// x86 x64
	_IMM(Breakpoint_ERQHandler),
	_IMM(Overflow_ERQHandler),
	_IMM(Bound_ERQHandler),
	_IMM(Invalid_Opcode_ERQHandler),// x86
	_IMM(Coprocessor_Not_Available_ERQHandler),// x86
	_IMM(Double_Fault_ERQHandler),
	_IMM(Coprocessor_Segment_Overrun_ERQHandler),
	_IMM(Invalid_TSS_ERQHandler),
	_IMM(x0B_ERQHandler),
	_IMM(x0C_ERQHandler),
	_IMM(x0D_ERQHandler),
	_IMM(Page_Fault_ERQHandler),
	_IMM(x0F_ERQHandler),
	// 0x10
	_IMM(X87_FPU_Floating_Point_Error_ERQHandler),
	_IMM(Alignment_Check_ERQHandler),// 0x11
	_IMM(Machine_Check_ERQHandler),// 0x12
	_IMM(SIMD_Floating_Point_Exception_ERQHandler),// 0x13
	_IMM(Virtualization_Exception_ERQHandler),// 0x14
	_IMM(Else_ERQHandler),// 0x15
	_IMM(Else_ERQHandler),// 0x16
	_IMM(Else_ERQHandler),// 0x17
	_IMM(Else_ERQHandler),// 0x18
	_IMM(Else_ERQHandler),// 0x19
	_IMM(Else_ERQHandler),// 0x1A
	_IMM(Else_ERQHandler),// 0x1B
	_IMM(Else_ERQHandler),// 0x1C
	_IMM(Else_ERQHandler),// 0x1D
	_IMM(Else_ERQHandler),// 0x1E
	_IMM(Else_ERQHandler),// 0x1F
};

#endif

// General_IRQHandler
#if _MCCA == 0x8632
_ESYM_C __attribute__((interrupt, target("general-regs-only")))
void General_IRQHandler(void* frame) {
	outpb(PORT_i8259A_SLV_A, BYTE_EOI);
	outpb(PORT_i8259A_MAS_A, BYTE_EOI);
}
#elif _MCCA == 0x8664
extern uni::InterruptControl IC;
_ESYM_C __attribute__((interrupt, target("general-regs-only")))
void General_IRQHandler(InterruptFrame* frame) {
	IC.SendEOI();
}
#else

#endif

#if (_MCCA & 0xFF00) == 0x8600
#define IA32_APIC_BASE__APIC_GLOBAL_ENABLE     (1ULL << 11)   // EN
#define IA32_APIC_BASE__X2APIC_ENABLE          (1ULL << 10)   // EXTD
static bool pic_set = false;//{} lock should
static bool ioapic_set = false;//{} lock should
static void _IC_INIT_PIC() {
	_8259A_init_t Mas = { 0 };
	Mas.port = PORT_i8259A_MAS_A;
	Mas.ICW1.ICW4_USED = 1;
	Mas.ICW1.ENA = 1;
	Mas.ICW2.IntNo = 0x20;
	Mas.ICW3.CasPortMap = 0b00000100;
	Mas.ICW4.Not8b = 1;
	_8259A_init_t Slv = { 0 };
	Slv.port = PORT_i8259A_SLV_A;
	Slv.ICW1.ICW4_USED = 1;
	Slv.ICW1.ENA = 1;
	Slv.ICW2.IntNo = 0x70;
	Slv.ICW3.CasPortIdn = 2;
	Slv.ICW4.Not8b = 1;
	i8259A_init(&Mas);
	i8259A_init(&Slv);
}
bool uni::InterruptControl::Initialize(byte typ) {
	this->typ = typ;
	#if defined(_UEFI)        && _MCCA == 0x8664
	// return true;
	// [Diag] Read actual IA32_APIC_BASE MSR to verify HW mode
	{
		uint64_t apic_base_diag = getMSR(x86MSR::APIC_BASE);
		bool diag_en   = (apic_base_diag >> 11) & 1; // bit11: APIC Global Enable
		bool diag_extd = (apic_base_diag >> 10) & 1; // bit10: x2APIC Enable
		ploginfo("[IC-Diag] IA32_APIC_BASE = 0x%llX  EN=%d  EXTD(x2APIC)=%d",
			(unsigned long long)apic_base_diag, (int)diag_en, (int)diag_extd);
	}
	// [Diag] Read IOAPIC Redirection Entries 0..3 (high DWORDs = APIC ID field)
	// RTE high DWORD: bits[63:56] = destination field
	//   xAPIC  physical: 8-bit APIC ID in bits[59:56]
	//   x2APIC physical: 32-bit x2APIC ID in bits[63:32]
	if (typ) {
		for (byte _diag_i = 0; _diag_i < 4; _diag_i++) {
			byte _rte_hi_idx = (byte)(0x11 + _diag_i * 2); // high DWORD of RTE n
			uint32_t _rte_hi = IO_Read32(_rte_hi_idx);
			uint8_t  _dest8  = (uint8_t)(_rte_hi >> 24);   // xAPIC dest [59:56] → bits[31:24] of high DW
			bool     _x2mode = (_rte_hi & ~0xFF000000u) != 0; // any other bits set → x2APIC 32-bit ID
			ploginfo("[IC-Diag] IOAPIC RTE[%u] hi=0x%08X  dest8=0x%02X  %s",
				(unsigned)_diag_i, (unsigned)_rte_hi,
				(unsigned)_dest8,
				_x2mode ? "x2APIC(32b dest?)" : "xAPIC(8b dest)");
		}
	}
	#endif
	if (typ) { // LAPIC
		if (!pic_set) {
			_IC_INIT_PIC();
			outpb(PORT_i8259A_MAS_B, 0xFF);
			outpb(PORT_i8259A_SLV_B, 0xFF);
			pic_set = true;
		}
		if (!ioapic_set) {
			for (stduint i = 0x10; i < 0x40; i += 2)
				IO_Writ64(i, 0x10020 + ((i - 0x10) >> 1));
			ioapic_set = true;
			// enable IMCR
			outpb(0x22, 0x70);
			outpb(0x23, 0x01);
		}
		// Enable APIC/x2APIC in MSR
		uint64_t apic_base = getMSR(x86MSR::APIC_BASE);
		uint64_t old_base = apic_base;
		if (typ >= 1) apic_base |= IA32_APIC_BASE__APIC_GLOBAL_ENABLE;
		if (typ >= 2) apic_base |= IA32_APIC_BASE__X2APIC_ENABLE;
		
		if (apic_base != old_base) {
			setMSR(x86MSR::APIC_BASE, apic_base);
		}

		// Disable Legacy PIC
		outpb(PORT_i8259A_MAS_B, 0xFF);
		outpb(PORT_i8259A_SLV_B, 0xFF);

		// Setup SVR (Spurious Vector Register)
		const uint32_t vector_spurious = 0xFF;
		const uint32_t svr_val = vector_spurious | 0x100; // Bit 8: Software Enable
		WriteLAPIC(0xF0, svr_val);

		// Mask LVT (Set basic registers only)
		const uint32_t mask = 0x10000; // Bit 16: Masked
		WriteLAPIC(0x320, mask); // LVT Timer
		WriteLAPIC(0x350, mask); // LVT LINT0
		WriteLAPIC(0x360, mask); // LVT LINT1
		WriteLAPIC(0x370, mask); // LVT Error
		
		// ploginfo("[IC] APIC Mode %d Initialized", typ);
	}
	else if (!pic_set) {
		_IC_INIT_PIC();
		pic_set = true;
	}
	return true;
}

uint32 uni::InterruptControl::ReadLAPIC(uint32 offset) {
	if (typ == 2) return (uint32)getMSR(static_cast<x86MSR>(0x800 + (offset >> 4)));
	return *(volatile uint32*)_IMM(0xFEE00000u + offset);
}

void uni::InterruptControl::WriteLAPIC(uint32 offset, uint32 val) {
	if (typ == 2) setMSR(static_cast<x86MSR>(0x800 + (offset >> 4)), val);
	else *(volatile uint32*)_IMM(0xFEE00000u + offset) = val;
}

_ESYM_C void sendEOI();
void uni::InterruptControl::SendEOI(byte irq) {
	if (typ == 2) {
		// x2APIC mode
		setMSR(x86MSR::APIC_EOI, 0);
	} else if (typ == 1) {
		// Standard APIC mode
		*(volatile uint32_t*)(0xFEE000B0) = 0;
	} else {
		// Legacy PIC mode
		// Support both IRQ index (8-15) and Vector (0x70-0x77)
		if ((irq >= 8 && irq <= 15) || (irq >= 0x70 && irq <= 0x77)) {
			outpb(0xA0, 0x20); // Slave EOI
		}
		outpb(0x20, 0x20); // Master EOI
	}
}

#define mfence() _ASM volatile ("mfence":::"memory")
volatile uint32_t* const ioapic_idx = reinterpret_cast<volatile uint32_t*>(0xFEC00000);
volatile uint32_t* const ioapic_dat = reinterpret_cast<volatile uint32_t*>(0xFEC00010);
__attribute__((optimize("O0")))
void uni::InterruptControl::IO_Writ32(byte idx, uint32 val) {
	*ioapic_idx = idx;
	mfence();
	*ioapic_dat = val;
	mfence();
}
__attribute__((optimize("O0")))
uint32 uni::InterruptControl::IO_Read32(byte idx) {
	uint32 ret;
	*ioapic_idx = idx;		
	mfence();
	ret = *ioapic_dat;
	mfence();
	return ret;
}
#endif

// InterruptControl::enInterrupt
#if (_MCCA & 0xFF00) == 0x8600
void uni::InterruptControl::enInterrupt(bool enable) {
	::enInterrupt(enable);
}
#elif (_MCCA & 0xFF00) == 0x1000// RV
void uni::InterruptControl::enInterrupt(bool enable) {
	if (enable) {
		auto hart = getTP();
		setPriorityThreshold(hart, 0);
	}
	// enable machine-mode external interrupts.
	if (enable) setMIE(getMIE() | _MIE_MEIE);
	else setMIE(getMIE() & ~_MIE_MEIE);
	// machine-mode global interrupts.
	if (enable) setMSTATUS(getMSTATUS() | _MSTATUS_MIE);
	else setMSTATUS(getMSTATUS() & ~_MSTATUS_MIE);
}
/*
void EnableLocalAPIC() {
	// SVR
	volatile uint32_t* svr = reinterpret_cast<volatile uint32_t*>(0xFEE000F0);
	uint32_t value = *svr;
	// set Bit 8 (Enable) and Bits 0-7 (Spurious Vector = 0xFF)
	value |= 0x100; // Bit 8: Software Enable
	value |= 0xFF;  // Vector 0xFF for spurious interrupts
	*svr = value;
	// ploginfo("Local APIC Software Enabled via SVR.");
}
*/
#endif

bool uni::InterruptControl::TryMaskInterrupt() {
	#if (_MCCA & 0xFF00) == 0x8600
	stduint flags = getFlags();
	bool was_enabled = (byte)cast<REG_FLAG_t>(flags).IF != 0;
	if (was_enabled) {
		enInterrupt(false);
	}
	return was_enabled;
	#elif (_MCCA & 0xFF00) == 0x1000
	bool was_enabled = getInterrupt() != 0;
	if (was_enabled) {
		enInterrupt(false);
	}
	return was_enabled;
	#else
	return false;
	#endif
}

// InterruptControl::Reset
#if (_MCCA & 0xFF00) == 0x8600
// Use Interrupt or Trap Gate? RPL = 0 or 3? 
void uni::InterruptControl::Reset(word SegCode, stduint Offset) {
	for0a(i, ERQ_Handlers) {
		self[i].gate_t::setModeRupt(ERQ_Handlers[i] + Offset, SegCode);
	}
	for (stduint i = 0x20; i < 256; i++) {
		self[i].gate_t::setModeRupt(_IMM(General_IRQHandler) + Offset, SegCode);
	}
	loadIDT(_IMM(IVT_SEL_ADDR), 256 * sizeof(gate_t) - 1);
}

#elif (_MCCA & 0xFF00) == 0x1000
void uni::InterruptControl::Reset() {
	setMTVEC(_IMM(IVT_SEL_ADDR));
}
#endif

#elif defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)

namespace uni {

Reference EXTI::MaskInterrupt(_EXTI_ADDR + 0x00);// aka EXTI_IMR, default 0x00000000

Reference EXTI::MaskEvent(_EXTI_ADDR + 0x04);// aka EXTI_EMR, default 0x00000000

Reference EXTI::TriggerRising(_EXTI_ADDR + 0x08);// aka EXTI_RTSR, default 0x00000000

Reference EXTI::TriggerFalling(_EXTI_ADDR + 0x0C);// aka EXTI_FTSR, default 0x00000000

Reference EXTI::Softrupt(_EXTI_ADDR + 0x10);// aka EXTI_SWIER, default 0x00000000

Reference EXTI::Pending(_EXTI_ADDR + 0x14);// aka EXTI_PR, default undefined

}
	
#else
// may for osdev?
#endif

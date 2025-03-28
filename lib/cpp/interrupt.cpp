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

#if defined(_MCCA) && _MCCA == 0x8632
#include "../../inc/c/driver/i8259A.h"

//{TODO}
void Divide_By_Zero_ERQHandler() {}
void Step_ERQHandler() {}
void NMI_ERQHandler() {}
void Breakpoint_ERQHandler() {}
void Overflow_ERQHandler() {}
void Bound_ERQHandler_() {
	__asm("Bound_ERQHandler:");
	_TODO;
}
void Invalid_Opcode_ERQHandler_(void) { // 0x06
	// __attribute__((interrupt)) is useless
	__asm("Invalid_Opcode_ERQHandler:");
	__asm("push       %ebp");
	__asm("mov  %esp, %ebp");
	ERQ_Handler(~_IMM(0x06));
	__asm("mov  %ebp, %esp");// leave
	__asm("pop  %ebp      ");
	__asm("pop  %eax      ");// eip
	__asm("add  $2  , %eax");// skip UD2, {0x0F,0x0B}
	__asm("push %eax      ");
	__asm("iret");
}
void Coprocessor_Not_Available_ERQHandler() {}
void Double_Fault_ERQHandler(dword ErrorCode) {}
void Coprocessor_Segment_Overrun_ERQHandler() {}
void Invalid_TSS_ERQHandler(dword ErrorCode) {}
void x0B_ERQHandler(dword ErrorCode) {}
void x0C_ERQHandler(dword ErrorCode) {}
void x0D_ERQHandler(dword ErrorCode) {}
void Page_Fault_ERQHandler(dword ErrorCode) { // 0x0E
	ERQ_Handler(0x0E, ErrorCode);
	//{} IRET
}
void x0F_ERQHandler() {}
void X87_FPU_Floating_Point_Error_ERQHandler() { // 0x10
	ERQ_Handler(~_IMM(0x10));
	//{} IRET
}
void Else_ERQHandler() { // 0x20
	ERQ_Handler(~_IMM(0x20));
	//{} IRET
}

static stduint ERQ_Handlers[0x20]{
	_IMM(Divide_By_Zero_ERQHandler),
	_IMM(Step_ERQHandler),
	_IMM(NMI_ERQHandler),
	_IMM(Breakpoint_ERQHandler),
	_IMM(Overflow_ERQHandler),
	_IMM(Bound_ERQHandler),
	_IMM(Invalid_Opcode_ERQHandler),
	_IMM(Coprocessor_Not_Available_ERQHandler),
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
	_IMM(Else_ERQHandler),// 0x11
	_IMM(Else_ERQHandler),// 0x12
	_IMM(Else_ERQHandler),// 0x13
	_IMM(Else_ERQHandler),// 0x14
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


extern "C" void General_IRQHandler() {
	__asm("push %eax");
	__asm("mov $0x20, %al");
	__asm("out %al, $0xA0");// outpb(_i8259A_SLV, BYTE_EOI);
	__asm("out %al, $0x20");// outpb(_i8259A_SLV, BYTE_EOI);
	__asm("pop  %eax");
	// __asm("sti");
	__asm("iret");
}

void uni::InterruptControl::enAble(bool enable) {
	if (enable) {
		_8259A_init_t Mas = { 0 };
		Mas.port = _i8259A_MAS;
		Mas.ICW1.ICW4_USED = 1;
		Mas.ICW1.ENA = 1;
		Mas.ICW2.IntNo = 0x20;
		Mas.ICW3.CasPortMap = 0b00000100;
		Mas.ICW4.Not8b = 1;
		_8259A_init_t Slv = { 0 };
		Slv.port = _i8259A_SLV;
		Slv.ICW1.ICW4_USED = 1;
		Slv.ICW1.ENA = 1;
		Slv.ICW2.IntNo = 0x70;
		Slv.ICW3.CasPortIdn = 2;
		Slv.ICW4.Not8b = 1;
		InterruptEnable();
		i8259A_init(&Mas);
		i8259A_init(&Slv);
	}
	else
		InterruptDisable();
}

void uni::InterruptControl::Reset(word SegCode) {
	_PACKED(struct)  { uint16 u_16fore; uint32 u_32back; } tmp48_le;
	for0a(i, ERQ_Handlers) {
		GateStructInterruptR0(&self[i], ERQ_Handlers[i], SegCode, 0);
	}
	tmp48_le = { 256 * sizeof(gate_t) - 1, _IMM(IVT_SEL_ADDR) };
	for (stduint i = 0x20; i < 256; i++) {
		GateStructInterruptR0(&self[i], _IMM(General_IRQHandler), SegCode, 0);
	}
	InterruptDTabLoad(&tmp48_le);// ("lidt %0" :: "m" (tmp48_le));
}
	
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

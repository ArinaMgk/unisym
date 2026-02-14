// ASCII CPL TAB4 CRLF
// Docutitle: (Device) Programmable Interval Timer
// Codifiers: @dosconio: 20240529
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
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

#include "../../../inc/c/driver/i8259A.h"
#include "../../../inc/c/driver/PIT.h"
#include "../../../inc/c/driver/timer.h"
#include "../../../inc/cpp/Device/ACPI.hpp"

#if defined(_MCCA)
#if (_MCCA==0x8616||_MCCA==0x8632)

#define RATE_GENERATOR 0x34 // 00-11-010-0 : {Counter0 - LSB then MSB - rate generator - binary}
#define TIMER_FREQ     1193182L // clock frequency for timer in PC and AT */
#define kHZ             1000  // clock freq (software settable on IBM-PC) */

#define BEEP_HZ 440
#define BEEP_COUNTER (TIMER_FREQ / BEEP_HZ)

void PIT_Init()
{
	// 1kHz
	outpb(PORT_TIMER_MODE, RATE_GENERATOR);
	outpb(PORT_PIT_TIMER0, (byte)(TIMER_FREQ / kHZ));
	outpb(PORT_PIT_TIMER0, (byte)((TIMER_FREQ / kHZ) >> 8));
	// Chan 2: for PC Speaker
	outpb(PORT_TIMER_MODE, 0b10110110);
	outpb(PORT_PIT_TIMER2, (byte)(TIMER_FREQ / BEEP_COUNTER));
	outpb(PORT_PIT_TIMER2, (byte)((TIMER_FREQ / BEEP_COUNTER) >> 8));
	//
	i8259Master_Enable(DEV_MAS_PIT);
}

#elif _MCCA == 0x8664
using namespace uni;
LocalAPICTimer lapic_timer;

namespace {
	const uint32 CountMax = 0xFFFFFFFFu;
	volatile uint32* lvt_timer = (uint32*)(0xFEE00320);
	volatile uint32* initial_count = (uint32*)(0xFEE00380);
	volatile uint32* current_count = (uint32*)(0xFEE00390);
	volatile uint32* divide_config = (uint32*)(0xFEE003E0);
}

void LocalAPICTimer::Reset() {
	Reset(0);
	Ento(CountMax);
	ACPI::Delay_ms(100);// 0.1s
	auto elapsed = CountMax - Read();
	Endo();
	Frequency = elapsed * 10;
}
void LocalAPICTimer::Reset(stduint init_count) {
	*divide_config = 0b1011; // divide 1:1
	if (!init_count) *lvt_timer = _IMM(0b001 << 16) | 32; // masked, one-shot
	else {
		*lvt_timer = _IMM(0b010 << 16) | IRQ_LAPICTimer; // !masked, periodic
		*initial_count = init_count;
	}
}
void LocalAPICTimer::Ento(stduint count) {
	*initial_count = count;
}
void LocalAPICTimer::Endo() {
	*initial_count = 0;
}
stduint LocalAPICTimer::Read() {
	return *current_count; // CountMax - *current_count;
}

#elif (_MCCA & 0xFF00) == 0x1000
#include "../../../inc/c/proctrl/RISCV/riscv.h"
#include "../../../inc/c/board/QemuVirt-Riscv.h"
using namespace uni;
Timer clint;
void Timer::enInterrupt(bool enable) const {
	if (enable) setMIE(getMIE() | _MIE_MTIE);
	else setMIE(getMIE() & ~_MIE_MTIE);
}
uint64 Timer::Read() {
	return treat<uint64>ADDR_CLINT_MTIME;
}
void Timer::MSIP(stduint mhartid, MSIP_Type type) {
	treat<uint32>ADDR_CLINT_MSIP(mhartid) = (uint32)type;
}
void Timer::Load(stduint mhartid, uint64 timepoint) {
	treat<uint64>ADDR_CLINT_MTIMECMP(mhartid) = timepoint;
}

#endif
#endif

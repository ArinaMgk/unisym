// ASCII C&CPP TAB4 CRLF
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
#ifndef _INC_Interrupt_GIC
#define _INC_Interrupt_GIC
#include "../../stdinc.h"
#if defined(_INC_CPP) && defined(_MPU_STM32MP13)
#include "../../prochip/CortexA.h"
#include "../../../cpp/interrupt"
#include "../../../cpp/reference"

// Interrupt mode bit-masks
#define IRQ_MODE_TRIG_Pos           (0U)
#define IRQ_MODE_TRIG_Msk           (0x07UL /*<< IRQ_MODE_TRIG_Pos*/)
#define IRQ_MODE_TRIG_LEVEL         (0x00UL /*<< IRQ_MODE_TRIG_Pos*/) ///< Trigger: level triggered interrupt
#define IRQ_MODE_TRIG_LEVEL_LOW     (0x01UL /*<< IRQ_MODE_TRIG_Pos*/) ///< Trigger: low level triggered interrupt
#define IRQ_MODE_TRIG_LEVEL_HIGH    (0x02UL /*<< IRQ_MODE_TRIG_Pos*/) ///< Trigger: high level triggered interrupt
#define IRQ_MODE_TRIG_EDGE          (0x04UL /*<< IRQ_MODE_TRIG_Pos*/) ///< Trigger: edge triggered interrupt
#define IRQ_MODE_TRIG_EDGE_RISING   (0x05UL /*<< IRQ_MODE_TRIG_Pos*/) ///< Trigger: rising edge triggered interrupt
#define IRQ_MODE_TRIG_EDGE_FALLING  (0x06UL /*<< IRQ_MODE_TRIG_Pos*/) ///< Trigger: falling edge triggered interrupt
#define IRQ_MODE_TRIG_EDGE_BOTH     (0x07UL /*<< IRQ_MODE_TRIG_Pos*/) ///< Trigger: rising and falling edge triggered interrupt

#define IRQ_MODE_TYPE_Pos           (3U)
#define IRQ_MODE_TYPE_Msk           (0x01UL << IRQ_MODE_TYPE_Pos)
#define IRQ_MODE_TYPE_IRQ           (0x00UL << IRQ_MODE_TYPE_Pos)     ///< Type: interrupt source triggers CPU IRQ line
#define IRQ_MODE_TYPE_FIQ           (0x01UL << IRQ_MODE_TYPE_Pos)     ///< Type: interrupt source triggers CPU FIQ line

#define IRQ_MODE_DOMAIN_Pos         (4U)
#define IRQ_MODE_DOMAIN_Msk         (0x01UL << IRQ_MODE_DOMAIN_Pos)
#define IRQ_MODE_DOMAIN_NONSECURE   (0x00UL << IRQ_MODE_DOMAIN_Pos)   ///< Domain: interrupt is targeting non-secure domain
#define IRQ_MODE_DOMAIN_SECURE      (0x01UL << IRQ_MODE_DOMAIN_Pos)   ///< Domain: interrupt is targeting secure domain

#define IRQ_MODE_CPU_Pos            (5U)
#define IRQ_MODE_CPU_Msk            (0xFFUL << IRQ_MODE_CPU_Pos)
#define IRQ_MODE_CPU_ALL            (0x00UL << IRQ_MODE_CPU_Pos)      ///< CPU: interrupt targets all CPUs
#define IRQ_MODE_CPU_0              (0x01UL << IRQ_MODE_CPU_Pos)      ///< CPU: interrupt targets CPU 0
#define IRQ_MODE_CPU_1              (0x02UL << IRQ_MODE_CPU_Pos)      ///< CPU: interrupt targets CPU 1
#define IRQ_MODE_CPU_2              (0x04UL << IRQ_MODE_CPU_Pos)      ///< CPU: interrupt targets CPU 2
#define IRQ_MODE_CPU_3              (0x08UL << IRQ_MODE_CPU_Pos)      ///< CPU: interrupt targets CPU 3
#define IRQ_MODE_CPU_4              (0x10UL << IRQ_MODE_CPU_Pos)      ///< CPU: interrupt targets CPU 4
#define IRQ_MODE_CPU_5              (0x20UL << IRQ_MODE_CPU_Pos)      ///< CPU: interrupt targets CPU 5
#define IRQ_MODE_CPU_6              (0x40UL << IRQ_MODE_CPU_Pos)      ///< CPU: interrupt targets CPU 6
#define IRQ_MODE_CPU_7              (0x80UL << IRQ_MODE_CPU_Pos)      ///< CPU: interrupt targets CPU 7

// Encoding in some early GIC implementations
#define IRQ_MODE_MODEL_Pos          (13U)
#define IRQ_MODE_MODEL_Msk          (0x1UL << IRQ_MODE_MODEL_Pos)
#define IRQ_MODE_MODEL_NN           (0x0UL << IRQ_MODE_MODEL_Pos)     ///< Corresponding interrupt is handled using the N-N model
#define IRQ_MODE_MODEL_1N           (0x1UL << IRQ_MODE_MODEL_Pos)     ///< Corresponding interrupt is handled using the 1-N model

#define IRQ_MODE_ERROR              (0x80000000UL)                    ///< Bit indicating mode value error

/* Interrupt priority bit-masks */
#define IRQ_PRIORITY_Msk            (0x0000FFFFUL)                    ///< Interrupt priority value bit-mask
#define IRQ_PRIORITY_ERROR          (0x80000000UL)                    ///< Bit indicating priority value error

_ESYM_C{
stdsint IRQ_Initialize(void);
int32_t IRQ_SetHandler(Request_t irqn, Handler_t handler);
Handler_t IRQ_GetHandler(Request_t irqn);
int32_t IRQ_Enable(Request_t irqn);
int32_t IRQ_Disable(Request_t irqn);
uint32_t IRQ_GetEnableState(Request_t irqn);
//
int32_t IRQ_SetMode(Request_t irqn, uint32_t mode);
uint32_t IRQ_GetMode(Request_t irqn);
Request_t IRQ_GetActiveIRQ(void);
Request_t IRQ_GetActiveFIQ(void);
int32_t IRQ_EndOfInterrupt(Request_t irqn);
int32_t IRQ_SetPending(Request_t irqn);
uint32_t IRQ_GetPending(Request_t irqn);
int32_t IRQ_ClearPending(Request_t irqn);
int32_t IRQ_SetPriority(Request_t irqn, uint32_t priority);
uint32_t IRQ_GetPriority(Request_t irqn);
int32_t IRQ_SetPriorityMask(uint32_t priority);
uint32_t IRQ_GetPriorityMask(void);
int32_t IRQ_SetPriorityGroupBits(uint32_t bits);
uint32_t IRQ_GetPriorityGroupBits(void);
}

#define IRQ_GIC_LINE_COUNT 1020
#define _GIC_ADDR 0xA0021000

namespace uni {

	class /*virtual*/ RuptRequest {
	public:
		// AKA GIC_GetIRQStatus
		// return 0 - not pending/active, 1 - pending, 2 - active, 3 - pending and active
		stduint getStatus() const;

		// AKA GIC_SetPendingIRQ+GIC_ClearPendingIRQ
		void setPending(bool pending = true) const;
		// AKA GIC_GetPendingIRQ
		bool getPending() const;

	};
	
	class GIC_t {
		static bool enable;
	

	public:
		// ADDRESS ADAPT:
		// - 135D 135F

		static stduint IRQ_ID0;

		inline const RuptRequest& operator[](Request_t id) const { return treat<const RuptRequest>(this); }
		inline Reference operator[](GICReg trt) const { return _GIC_ADDR + _IMMx4(trt); }
		inline Reference operator[](GICInterface trt) const { return _GIC_ADDR + 0x1000 + _IMMx4(trt); }

		void canMode() const {
			// for0a(i, IRQTable) IRQTable[i] = nullptr;
		}
		void enAble(bool ena = true) const;
		void setHandler(Request_t id, Handler_t handler) const;
		Handler_t getHandler(Request_t id) const;
		void enInterrupt(Request_t id, bool ena = true) const;
		bool ifInterrupt(Request_t id) const;

		// AKA GIC_DistributorInfo
		stduint getDistributorInfo() const;

		// AKA GIC_SetConfiguration
		// int_config Int_config field value. 
		// - Bit 0: Reserved(0 - N - N model, 1 - 1 - N model for some GIC before v1)
		// - Bit 1: 0 - level sensitive, 1 - edge triggered
		void setConfig(Request_t IRQn, uint32 int_config) const;
		// AKA GIC_GetConfiguration
		uint32 getConfig(Request_t IRQn) const;
		

		// AKA GIC_SetTarget
		// cpu_target: CPU interfaces to assign this interrupt to.
		void setTarget(Request_t IRQn, uint32 target) const;
		// AKA GIC_GetTarget
		uint32 getTarget(Request_t IRQn) const;


		// AKA GIC_SetGroup
		void setGroup(Request_t IRQn, uint32 group) const;


		// AKA GIC_SetPriority
		void setPriority(Request_t IRQn, stduint priority) const;
		// AKA GIC_GetPriority
		stduint getPriority(Request_t IRQn) const;

		// AKA GIC_AcknowledgePending
		Request_t AcknowledgePending() const { return (Request_t)_IMM(self[GICInterface::IAR]); }
		// AKA GIC_GetHighPendingIRQ
		stduint getHighPendingRequest() const { return _IMM(self[GICInterface::HPPIR]); }

		// AKA GIC_EndInterrupt
		void EndOfInterrupt(Request_t IRQn) const { self[GICInterface::EOIR] = _IMM(IRQn); }

		// AKA GIC_G/SetInterfacePriorityMask
		uint32_t getInterfacePriorityMask(void) const { return _IMM(self[GICInterface::PMR]); }
		void setInterfacePriorityMask(stduint priority) const { self[GICInterface::PMR] = _IMM(priority) & 0xFF; }

		// AKA GIC_SetBinaryPoint
		void setBinaryPoint(stduint binaryPoint) const { self[GICInterface::BPR] = _IMM(binaryPoint) & 0x7; }
		// AKA GIC_GetBinaryPoint
		stduint getBinaryPoint(void) const { return _IMM(self[GICInterface::BPR]); }

		

	};
	extern GIC_t GIC;
	typedef GIC_t Interrupt;



}




#endif
#endif

// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Device.USB) Host Controller Driver, HCD
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

#ifndef _INC_DEVICE_USB_HCD
#define _INC_DEVICE_USB_HCD

#include "../../../c/stdinc.h"
#include "../../reference"
#include "../../interrupt"
#include "../RCC/RCCAddress"
#include "OTG.hpp"

#if defined(_MCU_STM32H7x)

// global ISR entries (extern "C"; defined in interrupt_usb.hpp), friended below
extern "C" void OTG_HS_IRQHandler();
extern "C" void OTG_FS_IRQHandler();
// user callback arrays owned by interrupt_usb.hpp (shared with PCD)
extern "C" Handler_t FUNC_OTG_HS[1];
extern "C" Handler_t FUNC_OTG_FS[1];

namespace uni {

	// AKA HCD_StateTypeDef
	enum class HCDState : unsigned {
		Reset = 0, Ready = 1, Error = 2, Busy = 3, Timeout = 4
	};

	// AKA USB_OTG_URBStateTypeDef (per-channel URB result)
	enum class URBState : unsigned {
		Idle = 0, Done = 1, NotReady = 2, NYET = 3, Error = 4, Stall = 5
	};

	// AKA USB_OTG_HCStateTypeDef (per-channel hardware state)
	enum class HostChannelState : unsigned {
		Idle = 0, XFRC = 1, Halted = 2, NAK = 3, NYET = 4, Stall = 5,
		Xacterr = 6, BBLerr = 7, DataTglErr = 8
	};

	// Host-side USB OTG controller (H7 only; AKA HCD_HandleTypeDef).
	// HCD1 = OTG1_HS (0x40040000), HCD2 = OTG2_FS (0x40080000).
	class HCD : public RuptTrait {
	public:
		// ---- init configuration (AKA HCD_InitTypeDef / USB_OTG_CfgTypeDef) ----
		byte speed = 0;                // HCD_SPEED_HIGH(0) / LOW(2) / FULL(3)
		byte host_channels = 15;
		byte phy_itface = 2;           // HCD_PHY_ULPI(1) / HCD_PHY_EMBEDDED(2)
		bool dma_enable = false;
		bool use_external_vbus = false;

		// ---- controller state ----
		stduint base = 0;              // register base address of this instance
		OTGHC hc[15];                  // host channel parameters
		HCDState State = HCDState::Reset;

		// ---- callbacks (AKA HAL_HCD_*Callback) ----
		Handler_t SOFHandler = 0;
		Handler_t ConnectHandler = 0;
		Handler_t DisconnectHandler = 0;
		_tocall_ft NotifyURBChangeHandler = 0;// (chnum, URBState)

		// AKA HAL_HCD_Init / HAL_HCD_DeInit
		bool setMode();
		bool canMode();
		// RCC AHB1ENR.OTG1HSEN / OTG2FSEN (AKA HAL_HCD_MspInit clock part)
		void enClock(bool ena = true);
		// AKA HAL_HCD_Start / HAL_HCD_Stop
		bool Start();
		bool Stop();
		// AKA HAL_HCD_ResetPort
		bool ResetPort();
		// AKA HAL_HCD_HC_Init / HAL_HCD_HC_Halt
		bool InitializeHostChannel(byte ch_num, byte epnum, byte dev_address, byte speed, byte ep_type, uint16 mps);
		bool HaltHostChannel(byte ch_num);
		// AKA HAL_HCD_HC_SubmitRequest
		bool SubmitRequest(byte ch_num, byte direction, byte ep_type, byte token, byte* pbuff, uint16 length, byte do_ping);
		// AKA HAL_HCD_HC_GetURBState / HAL_HCD_HC_GetXferCount / HAL_HCD_HC_GetState
		URBState getURBState(byte ch_num);
		stduint getXferCount(byte ch_num);
		HostChannelState getHostChannelState(byte ch_num);
		// AKA HAL_HCD_GetCurrentFrame / HAL_HCD_GetCurrentSpeed
		stduint getCurrentFrame();
		stduint getCurrentSpeed();
		// AKA HAL_HCD_GetState
		HCDState getState() const { return State; }

		// AKA HAL_HCD_IRQHandler; dispatched by the ISR (interrupt_usb.hpp)
		void HandleIRQ();

		// register access helpers (byte offsets)
		Reference GlobalReg(OTGGlobalReg reg) const { return Reference(base + _IMM(reg)); }
		Reference HostReg(OTGHostReg reg) const { return Reference(base + _IMM(reg)); }
		Reference ChannelReg(byte ch_num, stduint offset) const {
			return Reference(base + USB_OTG_HOST_CHANNEL_BASE + ch_num * USB_OTG_HOST_CHANNEL_SIZE + offset);
		}

		// RuptTrait (NVIC + IRQ_OTG_HS / IRQ_OTG_FS)
		_COM_DEF_Interrupt_Interface();

	protected:
		// ISR entries (global, extern "C", defined in interrupt_usb.hpp)
		friend void ::OTG_HS_IRQHandler();
		friend void ::OTG_FS_IRQHandler();
	};

	extern HCD HCD1; // OTG1_HS
	extern HCD HCD2; // OTG2_FS

}

#endif // _MCU_STM32H7x

#endif // _INC_DEVICE_USB_HCD

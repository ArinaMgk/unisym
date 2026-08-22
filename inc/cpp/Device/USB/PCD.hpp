// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Device.USB) Peripheral Controller Driver, PCD
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

#ifndef _INC_DEVICE_USB_PCD
#define _INC_DEVICE_USB_PCD

#include "../../../c/stdinc.h"
#include "../../reference"
#include "../../interrupt"
#include "../RCC/RCCAddress"
#include "OTG.hpp"

#if defined(_MCU_STM32H7x)

// global ISR entries (extern "C"; defined in interrupt_usb.hpp), friended below
extern "C" void OTG_HS_IRQHandler();
extern "C" void OTG_FS_IRQHandler();
// user callback arrays owned by interrupt_usb.hpp (shared with HCD)
extern "C" Handler_t FUNC_OTG_HS[1];
extern "C" Handler_t FUNC_OTG_FS[1];

namespace uni {

	// AKA PCD_StateTypeDef
	enum class PCDState : unsigned {
		Reset = 0, Ready = 1, Error = 2, Busy = 3, Timeout = 4
	};

	// AKA PCD_LPM_StateTypeDef
	enum class PCDLPMState : unsigned {
		L0 = 0, // on
		L1 = 1, // LPM L1 sleep
		L2 = 2, // suspend
		L3 = 3, // off
	};

	// AKA PCD_LPM_MsgTypeDef / PCD_BCD_MsgTypeDef (values sent to the handlers)
	enum class PCDLPMMsg : unsigned {
		L0Active = 0, L1Active = 1
	};
	enum class PCDBCDMsg : unsigned {
		Error = 0xFF,
		ContactDetection = 0xFE,
		StdDownstreamPort = 0xFD,
		ChargingDownstreamPort = 0xFC,
		DedicatedChargingPort = 0xFB,
		DiscoveryCompleted = 0x00,
	};

	// Device-side USB OTG controller (H7 only; AKA PCD_HandleTypeDef).
	// PCD1 = OTG1_HS (0x40040000), PCD2 = OTG2_FS (0x40080000).
	class PCD : public RuptTrait {
	public:
		// ---- init configuration (AKA PCD_InitTypeDef / USB_OTG_CfgTypeDef) ----
		byte speed = 0;                // PCD_SPEED_HIGH(0) / HIGH_IN_FULL(1) / FULL(2)
		uint16 ep0_mps = 512;          // USB_OTG_HS_MAX_PACKET_SIZE (512); 64 for FS
		byte dev_endpoints = 15;
		byte phy_itface = 2;           // PCD_PHY_ULPI(1) / PCD_PHY_EMBEDDED(2)
		bool dma_enable = false;
		bool vbus_sensing_enable = true;
		bool use_external_vbus = false;
		bool sof_enable = false;
		bool lpm_enable = false;
		bool battery_charging_enable = false;
		bool use_dedicated_ep1 = false;

		// ---- controller state ----
		stduint base = 0;              // register base address of this instance
		OTGEP IN_ep[16];
		OTGEP OUT_ep[16];
		PCDState State = PCDState::Reset;
		stduint Setup[12];             // setup packet buffer (48 bytes)
		PCDLPMState LPM_State = PCDLPMState::L0;
		stduint BESL = 0;
		bool lpm_active = false;
		bool battery_charging_active = false;

		// ---- callbacks (AKA HAL_PCD_*Callback; epnum/msg are passed through pureptr_t) ----
		Handler_t SetupStageHandler = 0;
		Handler_t SOFHandler = 0;
		Handler_t ResetHandler = 0;
		Handler_t SuspendHandler = 0;
		Handler_t ResumeHandler = 0;
		Handler_t ConnectHandler = 0;
		Handler_t DisconnectHandler = 0;
		_tocall_ft DataOutStageHandler = 0;      // (epnum)
		_tocall_ft DataInStageHandler = 0;       // (epnum)
		_tocall_ft ISOOUTIncompleteHandler = 0;  // (epnum)
		_tocall_ft ISOINIncompleteHandler = 0;   // (epnum)
		_tocall_ft LPMHandler = 0;               // (PCDLPMMsg)
		_tocall_ft BCDHandler = 0;               // (PCDBCDMsg)

		// AKA HAL_PCD_Init / HAL_PCD_DeInit
		bool setMode();
		bool canMode();
		// RCC AHB1ENR.OTG1HSEN / OTG2FSEN (AKA HAL_PCD_MspInit clock part)
		void enClock(bool ena = true);
		// AKA HAL_PCD_Start / HAL_PCD_Stop
		bool Start();
		bool Stop();
		// AKA HAL_PCD_DevConnect / HAL_PCD_DevDisconnect
		bool Connect();
		bool Disconnect();
		// AKA HAL_PCD_SetAddress
		bool setAddress(byte address);
		// AKA HAL_PCD_EP_Open / HAL_PCD_EP_Close
		bool OpenEndpoint(byte ep_addr, uint16 ep_mps, byte ep_type);
		bool CloseEndpoint(byte ep_addr);
		// AKA HAL_PCD_EP_Receive / HAL_PCD_EP_Transmit
		bool ReceiveEndpoint(byte ep_addr, byte* pBuf, stduint len);
		bool TransmitEndpoint(byte ep_addr, byte* pBuf, stduint len);
		// AKA HAL_PCD_EP_GetRxCount
		uint16 getRxCount(byte ep_addr);
		// AKA HAL_PCD_EP_SetStall / HAL_PCD_EP_ClrStall (merged)
		bool ConfigStall(byte ep_addr, bool set_or_reset);
		// AKA HAL_PCD_EP_Flush
		bool FlushEndpoint(byte ep_addr);
		// AKA HAL_PCD_ActivateRemoteWakeup / HAL_PCD_DeActivateRemoteWakeup
		bool ActivateRemoteWakeup();
		bool DeActivateRemoteWakeup();
		// AKA HAL_PCD_GetState
		PCDState getState() const { return State; }

		// ---- extended (AKA HAL_PCDEx_*) ----
		bool setTxFiFo(byte fifo, uint16 size);
		bool setRxFiFo(uint16 size);
		bool ActivateLPM();
		bool DeActivateLPM();
		bool ActivateBCD();
		bool DeActivateBCD();
		void DetectVBUS();               // AKA HAL_PCDEx_BCD_VBUSDetect

		// AKA HAL_PCD_IRQHandler; dispatched by the ISR (interrupt_usb.hpp)
		void HandleIRQ();

		// register access helpers (byte offsets)
		Reference GlobalReg(OTGGlobalReg reg) const { return Reference(base + _IMM(reg)); }
		Reference DeviceReg(OTGDeviceReg reg) const { return Reference(base + _IMM(reg)); }
		Reference InEndpointReg(byte epnum, OTGInEPReg reg) const {
			return Reference(base + USB_OTG_IN_ENDPOINT_BASE + epnum * USB_OTG_EP_REG_SIZE + _IMM(reg));
		}
		Reference OutEndpointReg(byte epnum, OTGOutEPReg reg) const {
			return Reference(base + USB_OTG_OUT_ENDPOINT_BASE + epnum * USB_OTG_EP_REG_SIZE + _IMM(reg));
		}

		// RuptTrait (NVIC + IRQ_OTG_HS / IRQ_OTG_FS)
		_COM_DEF_Interrupt_Interface();

	protected:
		// ISR entries (global, extern "C", defined in interrupt_usb.hpp)
		friend void ::OTG_HS_IRQHandler();
		friend void ::OTG_FS_IRQHandler();
	};

	extern PCD PCD1; // OTG1_HS
	extern PCD PCD2; // OTG2_FS

}

#endif // _MCU_STM32H7x

#endif // _INC_DEVICE_USB_PCD

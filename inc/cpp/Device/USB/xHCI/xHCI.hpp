// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) USB xHCI
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Copyright: uchan-nos/mikanos, under Apache License 2.0
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

/*
┌─────────────────────────────┐
│  Applications / OS Services │
├─────────────────────────────┤
│  USB Class Drivers          │  ← Device-specific functionality
├─────────────────────────────┤
│  USB Core Framework         │  ← Protocol abstraction layer
├─────────────────────────────┤
│  xHCI Driver                │  ← USB 3.0+ Host Controller Driver
├─────────────────────────────┤
│  PCI/PCIe Driver            │  ← Bus enumeration and communication (inc\cpp\Device\Bus\PCI.hpp)
├─────────────────────────────┤
│  xHCI Hardware Controller   │  ← Physical hardware
└─────────────────────────────┘
*/

#ifndef _INCPP_Device_USB_xHCI
#define _INCPP_Device_USB_xHCI

// x86_64 part of USB Driver is borrowed from uchan-nos/mikanos
// - https://github.com/uchan-nos/mikanos
// cited sincerely.
#if (defined(_MCCA) && ((_MCCA & 0xFF00)==0x8600))
#include <setjmp.h>
#include <iterator>
#include <vector>
#include "../../../../c/arith.h"
#include "../USB-Header.hpp"
#include "./xHCI-registers.hpp"
#include "./xHCI-Message.hpp"
#include "./xHCI-Ring.hpp"

#define CLEAR_STATUS_BIT(bitname) \
	[this](){ \
		uni::device::SpaceUSB3::PORTSC_t portsc = port_reg_set_.PORTSC.Read(); \
		portsc.data[0] &= 0x0E01C3E0u; \
		portsc.bits.bitname = 1; \
		port_reg_set_.PORTSC.Write(portsc); \
	}()


#include "./xHCI-Device.hpp"

// ---- ---- ---- ---- . ---- ---- ---- ---- //

namespace uni::device::SpaceUSB3 {

	class Port {
	public:
		Port(uint8 port_num, PortRegisterSet& port_reg_set)
			: port_num_{ port_num }, port_reg_set_{ port_reg_set }
		{
		}

		uint8 Number() const;
		bool IsConnected() const;
		bool IsEnabled() const;
		bool IsConnectStatusChanged() const;
		bool IsPortResetChanged() const;
		int Speed() const;
		Error Reset();
		DeviceUSB3* Initialize();

		void ClearConnectStatusChanged() const {
			CLEAR_STATUS_BIT(connect_status_change);
		}
		void ClearPortResetChange() const {
			CLEAR_STATUS_BIT(port_reset_change);
		}

	private:
		const uint8 port_num_;
		PortRegisterSet& port_reg_set_;
	};

	class HostController {
	public:
		HostController(uintptr_t mmio_base);
		Error Initialize();
		Error Run();
		Ring* CommandRing() { return &cr_; }
		EventRing* PrimaryEventRing() { return &er_; }
		DoorbellRegister* DoorbellRegisterAt(uint8 index);
		Port PortAt(uint8 port_num) {
			return Port{ port_num, PortRegisterSets()[port_num - 1] };
		}
		uint8 MaxPorts() const { return max_ports_; }
		DeviceManager* GetDeviceManager() { return &devmgr_; }
	public:
		Error ProcessEvents();
	private:
		static const size_t kDeviceSize = _TEMP 8;

		const uintptr_t mmio_base_;
		CapabilityRegisters* const cap_;
		OperationalRegisters* const op_;
		const uint8 max_ports_;

		class DeviceManager devmgr_;
		Ring cr_;
		EventRing er_;

		InterrupterRegisterSetArray InterrupterRegisterSets() const {
			return { mmio_base_ + cap_->RTSOFF.Read().Offset() + 0x20u, 1024 };
		}

		PortRegisterSetArray PortRegisterSets() const {
			return { reinterpret_cast<uintptr_t>(op_) + 0x400u, max_ports_ };
		}

		DoorbellRegisterArray DoorbellRegisters() const {
			return { mmio_base_ + cap_->DBOFF.Read().Offset(), 256 };
		}
		//

	public:
		Error ConfigurePort(Port& port);
		Error ConfigureEndpoints(DeviceUSB3& dev);

		/** @brief イベントリングに登録されたイベントを高々1つ処理する．
		 *
		 * xhc のプライマリイベントリングの先頭のイベントを処理する．
		 * イベントが無ければ即座に Error::kSuccess を返す．
		 *
		 * @return イベントを正常に処理できたら Error::kSuccess
		 */
		Error ProcessEvent();

	};

}


template <class T>
T* AllocArray(size_t num_obj, unsigned int alignment, unsigned int boundary) {
	auto ret = reinterpret_cast<T*>(
		uni_hostenv_allocator->allocate(sizeof(T) * num_obj,
			intlog2_iexpo(alignment), intlog2_iexpo(boundary)));
	if (ret) MemSet(ret, 0, sizeof(T) * num_obj);
	return ret;
}

#endif

#endif

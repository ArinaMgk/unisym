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
#include "../../../ISO_IEC_STD/iterator"
#include "../../../../c/arith.h"
#include "../../../../c/bitmap.h"
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
	struct HostControllerResourceConfig {
		// Zero selects a controller-scaled default for command/event rings.
		size_t command_ring_trbs = 0;
		size_t event_ring_trbs = 0;
		size_t control_transfer_ring_trbs = 64;
		size_t transfer_ring_trbs = 64;
		size_t bulk_transfer_ring_trbs = 1024;
		// Zero enables every slot reported by HCSPARAMS1.MaxSlots.
		uint8 max_slots = 0;
	};

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
		USBHostDevice_v3* Initialize();

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
		HostController(uintptr_t mmio_base,
			const HostControllerResourceConfig& resources = HostControllerResourceConfig{});
		Error Initialize();
		Error Run();
		Ring* CommandRing() { return &cr_; }
		EventRing* PrimaryEventRing() { return &er_; }
		DoorbellRegister* DoorbellRegisterAt(uint8 index);
		Port PortAt(uint8 port_num) {
			return Port{ port_num, PortRegisterSets()[port_num - 1] };
		}
		uint8 MaxPorts() const { return max_ports_; }
		uint8 MaxSlots() const { return max_slots_; }
		uint8 ContextSize() const { return context_size_; }
		size_t PageSize() const { return page_size_; }
		size_t ControlTransferRingSize() const { return resource_config_.control_transfer_ring_trbs; }
		size_t TransferRingSize() const { return resource_config_.transfer_ring_trbs; }
		size_t BulkTransferRingSize() const { return resource_config_.bulk_transfer_ring_trbs; }
		uint8 SpeedClass(uint8 root_hub_port_num, uint8 speed_id) const;
		uint8 SpeedIDForClass(uint8 root_hub_port_num, uint8 speed_class) const;
		DeviceManager* GetDeviceManager() { return &devmgr_; }
		bool IsSlotRemovalPending(uint8 slot_id) const;
		Error QueueSlotRemoval(uint8 slot_id, bool notify_disconnected);
		Error OnDisableSlotCompleted(uint8 slot_id, int completion_code);
	public:
		Error ProcessEvents();
	private:
		const uintptr_t mmio_base_;
		CapabilityRegisters* const cap_;
		OperationalRegisters* const op_;
		HostControllerResourceConfig resource_config_;
		const uint8 max_ports_;
		const uint8 max_slots_;
		const uint8 context_size_;
		size_t page_size_ = 4096;
		uint8 port_protocol_major_[256]{};
		uint8 port_speed_classes_[256][16]{};
		byte slot_removal_pending_storage_[32]{};
		byte slot_disconnect_notified_storage_[32]{};
		uni::Bitmap slot_removal_pending_;
		uni::Bitmap slot_disconnect_notified_;

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

		void InitializeSupportedProtocols();
		//

	public:
		Error ConfigurePort(Port& port);
		Error ConfigureEndpoints(USBHostDevice_v3& dev);
		Error OnHubPortStatusChanged(USBHostDevice_v3& hub_dev, uint8 downstream_port,
			uint16 status, uint16 change, uint8 speed_id);

		/** @brief Process at most one event registered in the event ring.
			 *
			 * Processes the front event of the xHC's primary event ring.
			 * If there is no event, returns Error::kSuccess immediately.
			 *
			 * @return Error::kSuccess if the event was processed successfully.
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
	else {
		plogerro("AllocArray failed for %u objs of size %u", num_obj, sizeof(T));
	}
	return ret;
}

#endif

#endif

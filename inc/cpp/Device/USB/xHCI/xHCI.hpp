// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) USB xHCI
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

#ifndef _INCPP_Device_USB_xHCI
#define _INCPP_Device_USB_xHCI

#if (defined(_MCCA) && ((_MCCA & 0xFF00)==0x8600))
#include <setjmp.h>
#include "../../../ISO_IEC_STD/iterator"
#include "../../../vector"
#include "../../../../c/arith.h"
#include "../../../../c/bitmap.h"
#include "../USB-Options.hpp"
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
	enum class ProtocolSpeedType : uint8 {
		Symmetric = 0,
		Reserved = 1,
		AsymmetricReceive = 2,
		AsymmetricTransmit = 3,
	};

	struct ProtocolSpeedInfo {
		uint8 root_hub_port_num = 0;
		uint8 speed_id = 0;
		uint32 rx_lane_mbps = 0;
		uint32 tx_lane_mbps = 0;
		uint8 speed_class = 0;
		uint8 rx_link_protocol = 0;
		uint8 tx_link_protocol = 0;
		bool full_duplex = false;

		bool operator==(const ProtocolSpeedInfo& other) const {
			return root_hub_port_num == other.root_hub_port_num &&
				speed_id == other.speed_id &&
				rx_lane_mbps == other.rx_lane_mbps &&
				tx_lane_mbps == other.tx_lane_mbps &&
				speed_class == other.speed_class &&
				rx_link_protocol == other.rx_link_protocol &&
				tx_link_protocol == other.tx_link_protocol &&
				full_duplex == other.full_duplex;
		}
	};

	using PortLinkInfo = ::uni::device::SpaceUSB::USBHostLinkInfo;

	struct HostControllerResourceConfig {
		// Zero selects a controller-scaled default for command/event rings.
		size_t command_ring_trbs = 0;
		size_t event_ring_trbs = 0;
		uint8 event_ring_segments = 1;
		uint16 max_interrupters = 1;
		size_t control_transfer_ring_trbs = 64;
		size_t transfer_ring_trbs = 64;
		size_t bulk_transfer_ring_trbs = 1024;
		size_t stream_transfer_ring_trbs = 64;
		// This is the xHCI MaxPStreams exponent, not a stream count.
		uint8 max_primary_streams = _OPT_BUS_USB_xHCI_Streams ? 4 : 0;
		uint8 u1_timeout = _OPT_BUS_USB_xHCI_LinkPower ? 10 : 0;
		uint8 u2_timeout = _OPT_BUS_USB_xHCI_LinkPower ? 32 : 0;
		// Raw 12-bit xHCI BELT value. Zero leaves LTM disabled.
		uint16 best_effort_latency_tolerance = 0;
		bool enable_link_power_management = _OPT_BUS_USB_xHCI_LinkPower != 0;
		bool enable_u3 = _OPT_BUS_USB_xHCI_LinkPower != 0;
		// Zero enables every slot reported by HCSPARAMS1.MaxSlots.
		uint8 max_slots = 0;
	};

	enum class ConfigPhase {
		NotConnected,
		WaitingAddressed,
		ResettingPort,
		EnablingSlot,
		AddressingDevice,
		InitializingDevice,
		ConfiguringEndpoints,
		Configured,
	};

	struct PendingHubChildAddress {
		uint8 root_hub_port_num;
		uint32 route_string;
		uint8 hub_slot_id;
		uint8 downstream_port;
		uint8 speed;

		bool operator==(const PendingHubChildAddress& rhs) const {
			return root_hub_port_num == rhs.root_hub_port_num &&
				route_string == rhs.route_string &&
				hub_slot_id == rhs.hub_slot_id &&
				downstream_port == rhs.downstream_port &&
				speed == rhs.speed;
		}
	};

	struct HostControllerEnumerationState {
		uni::Vector<PendingHubChildAddress> pending_hub_children{};
		PendingHubChildAddress active_hub_child{};
		bool active_hub_child_valid = false;
		PendingHubChildAddress resetting_hub_port{};
		bool resetting_hub_port_valid = false;
		uint16 resetting_hub_port_ticks = 0;
		uni::Array<ConfigPhase, 256> port_config_phase{};
		uint8 addressing_port = 0;
		bool enable_slot_command_pending = false;
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
		bool IsPortLinkStateChanged() const;
		uint8 LinkState() const;
		int Speed() const;
		Error Reset();
		Error SetLinkState(uint8 link_state);
		void ConfigureLinkPower(uint8 u1_timeout, uint8 u2_timeout);
		USBHostDevice_v3* Initialize();

		void ClearConnectStatusChanged() const {
			CLEAR_STATUS_BIT(connect_status_change);
		}
		void ClearPortResetChange() const {
			CLEAR_STATUS_BIT(port_reset_change);
		}
		void ClearPortLinkStateChange() const {
			CLEAR_STATUS_BIT(port_link_state_change);
		}

	private:
		const uint8 port_num_;
		PortRegisterSet& port_reg_set_;
	};

	class HostController {
	public:
		HostController(uintptr_t mmio_base,
			const HostControllerResourceConfig& resources = HostControllerResourceConfig{});
		~HostController();
		Error Initialize();
		Error Run();
		Ring* CommandRing() { return &cr_; }
		EventRing* PrimaryEventRing() { return EventRingAt(0); }
		EventRing* EventRingAt(uint16 interrupter_index);
		const EventRing* EventRingAt(uint16 interrupter_index) const;
		uint16 InterrupterCount() const { return max_interrupters_; }
		bool HasPendingEvent(uint16 interrupter_index) const;
		bool HasPendingEvent() const;
		bool AcknowledgeInterrupter(uint16 interrupter_index);
		uint16 InterrupterTargetForSlot(uint8 slot_id) const {
			return max_interrupters_ > 1 ? uint16(slot_id % max_interrupters_) : 0;
		}
		DoorbellRegister* DoorbellRegisterAt(uint8 index);
		Port PortAt(uint8 port_num) {
			return Port{ port_num, PortRegisterSets()[port_num - 1] };
		}
		uint8 MaxPorts() const { return max_ports_; }
		uint8 MaxSlots() const { return max_slots_; }
		uint8 ContextSize() const { return context_size_; }
		uint16 HCIVersion() const { return cap_->HCIVERSION.Read(); }
		uint16 CurrentMicroframeIndex() const {
			return runtime_->MFINDEX.Read().bits.microframe_index;
		}
		uint8 IsochronousSchedulingThreshold() const;
		bool HasContiguousFrameIDCapability() const {
			return cap_->HCCPARAMS1.Read().bits.contiguous_frame_id_capability;
		}
		bool HasBandwidthNegotiationCapability() const {
			return cap_->HCCPARAMS1.Read().bits.bw_negotiation_capability;
		}
		bool HasExtendedTBCCapability() const {
			const auto hccparams2 = cap_->HCCPARAMS2.Read();
			return HCIVersion() >= 0x0110u &&
				hccparams2.bits.large_esit_payload_capability &&
				hccparams2.bits.extended_tbc_capability;
		}
		uint8 MaximumPrimaryStreamArraySize() const {
			return cap_->HCCPARAMS1.Read().bits.maximum_primary_stream_array_size;
		}
		bool HasLatencyToleranceMessagingCapability() const {
			return cap_->HCCPARAMS1.Read().bits.latency_tolerance_messaging_capability;
		}
		bool HasU3EntryCapability() const {
			return HCIVersion() >= 0x0110u &&
				cap_->HCCPARAMS2.Read().bits.u3_entry_capability;
		}
		bool IsControllerFailed() const { return controller_failed_; }
		size_t PageSize() const { return page_size_; }
		size_t ControlTransferRingSize() const { return resource_config_.control_transfer_ring_trbs; }
		size_t TransferRingSize() const { return resource_config_.transfer_ring_trbs; }
		size_t BulkTransferRingSize() const { return resource_config_.bulk_transfer_ring_trbs; }
		size_t StreamTransferRingSize() const { return resource_config_.stream_transfer_ring_trbs; }
		uint8 ConfiguredMaxPrimaryStreams() const { return resource_config_.max_primary_streams; }
		uint8 U1Timeout() const { return resource_config_.u1_timeout; }
		uint8 U2Timeout() const { return resource_config_.u2_timeout; }
		uint16 BestEffortLatencyTolerance() const {
			return resource_config_.best_effort_latency_tolerance;
		}
		bool LinkPowerManagementEnabled() const {
			return resource_config_.enable_link_power_management;
		}
		bool U3Enabled() const { return resource_config_.enable_u3; }
		uint8 SpeedClass(uint8 root_hub_port_num, uint8 speed_id) const;
		uint8 SpeedIDForClass(uint8 root_hub_port_num, uint8 speed_class) const;
		PortLinkInfo PortLink(uint8 root_hub_port_num) const;
		DeviceManager* GetDeviceManager() { return &devmgr_; }
		HostControllerEnumerationState& EnumerationState() { return enumeration_state_; }
		bool IsSlotRemovalPending(uint8 slot_id) const;
		Error QueueSlotRemoval(uint8 slot_id, bool notify_disconnected);
		Error OnDisableSlotCompleted(uint8 slot_id, int completion_code);
		Error OnHostControllerEvent(int completion_code);
		Error SetLatencyTolerance(uint16 value);
		Error SetPortLinkState(uint8 port_num, uint8 link_state);
		void OnMFINDEXWrapEvent() { ++mfindex_wrap_count_; }
		uint64 MFINDEXWrapCount() const { return mfindex_wrap_count_; }
	public:
		Error ProcessEvents();
		Error ProcessDelayed();
	private:
		const uintptr_t mmio_base_;
		CapabilityRegisters* const cap_;
		OperationalRegisters* const op_;
		RuntimeRegisters* const runtime_;
		HostControllerResourceConfig resource_config_;
		const uint8 max_ports_;
		const uint8 max_slots_;
		const uint8 context_size_;
		const uint16 max_interrupters_;
		size_t page_size_ = 4096;
		uni::Vector<uint8> port_protocol_major_{};
		uni::Vector<ProtocolSpeedInfo> protocol_speeds_{};
		byte slot_removal_pending_storage_[32]{};
		byte slot_disconnect_notified_storage_[32]{};
		uni::Bitmap slot_removal_pending_;
		uni::Bitmap slot_disconnect_notified_;
		bool controller_failed_ = false;
		bool controller_recovery_requested_ = false;
		bool controller_recovery_in_progress_ = false;
		uint8 controller_event_completion_code_ = 0;
		uint64 mfindex_wrap_count_ = 0;
		HostControllerEnumerationState enumeration_state_{};

		class DeviceManager devmgr_;
		Ring cr_;
		EventRing er_;
		uni::Vector<EventRing*> event_rings_{};

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
		ProtocolSpeedInfo* ProtocolSpeed(uint8 root_hub_port_num, uint8 speed_id);
		const ProtocolSpeedInfo* ProtocolSpeed(uint8 root_hub_port_num,
			uint8 speed_id) const;
		Error RecoverController();
		Error InitializeRuntimeRings();
		//

	public:
		Error ConfigurePort(Port& port);
		Error ConfigureEndpoints(USBHostDevice_v3& dev);
		Error OnHubPortStatusChanged(
			USBHostDevice_v3& hub_dev, uint8 downstream_port, uint16 status, uint16 change, uint8 speed_id);
		bool ClaimHubPortReset(USBHostDevice_v3& hub_dev, uint8 downstream_port);
		void ReleaseHubPortReset(USBHostDevice_v3& hub_dev, uint8 downstream_port);

		/** @brief Process at most one event registered in the event ring.
		 *
		 * Processes the front event of the xHC's primary event ring.
		 * If there is no event, returns Error::Success immediately.
		 *
		 * @return Error::Success if the event was processed successfully.
		 */
		Error ProcessEvent(uint16 interrupter_index = 0);

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

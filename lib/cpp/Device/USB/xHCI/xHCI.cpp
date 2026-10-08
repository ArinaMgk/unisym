#if defined(_MCCA) && _MCCA == 0x8664
#include "../../../../../inc/c/msgface.h"
#include "../../../../../inc/cpp/Device/USB/USBHost-Hub.hpp"
#include "../../../../../inc/cpp/Device/USB/xHCI/xHCI-registers.hpp"
#include "../../../../../inc/cpp/Device/USB/xHCI/xHCI.hpp"
#include "../../../../../inc/cpp/ISO_IEC_STD/algorithm"



using namespace uni::device::SpaceUSB;
using namespace uni::device::SpaceUSB3;


// ---- ---- ---- ---- device.cpp ---- ---- ---- ---- //

namespace {

	SetupStageTRB MakeSetupStageTRB(SetupData setup_data, int transfer_type) {
		SetupStageTRB setup{};
		setup.bits.request_type = setup_data.request_type.data;
		setup.bits.request = setup_data.request;
		setup.bits.value = setup_data.value;
		setup.bits.index = setup_data.index;
		setup.bits.length = setup_data.length;
		setup.bits.transfer_type = transfer_type;
		return setup;
	}

	DataStageTRB MakeDataStageTRB(const void* buf, int len, bool dir_in) {
		DataStageTRB data{};
		data.SetPointer(buf);
		data.bits.trb_transfer_length = len;
		data.bits.td_size = 0;
		data.bits.direction = dir_in;
		return data;
	}

	void Log(LogLevel level, const DataStageTRB& trb) {
		Log(level,
			"DataStageTRB: len %d, buf 0x%08lx, dir %d, attr 0x%02x\n",
			trb.bits.trb_transfer_length,
			trb.bits.data_buffer_pointer,
			trb.bits.direction,
			trb.data[3] & 0x7fu);
	}

	void Log(LogLevel level, const SetupStageTRB& trb) {
		Log(level,
			"  SetupStage TRB: req_type %02x, req %02x, val %02x, ind %02x, len %02x\n",
			trb.bits.request_type,
			trb.bits.request,
			trb.bits.value,
			trb.bits.index,
			trb.bits.length);
	}

	void Log(LogLevel level, const TransferEventTRB& trb) {
		if (trb.bits.event_data) {
			// Log(level,
			// 	"Transfer (value %08lx) completed: %s, residual length %d, slot %d, ep addr %d",
			// 	reinterpret_cast<uint64_t>(trb.Pointer()),
			// 	kTRBCompletionCodeToName[trb.bits.completion_code],
			// 	trb.bits.trb_transfer_length,
			// 	trb.bits.slot_id,
			// 	trb.GetEndpointID().Address());
			return;
		}

		TRB* issuer_trb = trb.Pointer();
		// Log(level,
		// 	"%s completed: %s, residual length %d, slot %d, ep addr %d",
		// 	kTRBTypeToName[issuer_trb->bits.trb_type],
		// 	kTRBCompletionCodeToName[trb.bits.completion_code],
		// 	trb.bits.trb_transfer_length,
		// 	trb.bits.slot_id,
		// 	trb.GetEndpointID().Address());
		if (auto data_trb = TRBDynamicCast<DataStageTRB>(issuer_trb)) {
			Log(level, "  ");
			Log(level, *data_trb);
		}
		else if (auto setup_trb = TRBDynamicCast<SetupStageTRB>(issuer_trb)) {
			Log(level, "  ");
			Log(level, *setup_trb);
		}
	}
}

namespace uni::device::SpaceUSB3 {
	namespace {
		bool MatchesPortAndRoute(const USBHostDevice_v3& dev, uint8 port_num, uint32 route_string) {
			return dev.RootHubPortNum() == port_num && dev.RouteString() == route_string;
		}

		stduint RouteDepth(uint32 route_string) {
			stduint depth = 0;
			for (stduint shift = 0; shift < 20; shift += 4) {
				if (((route_string >> shift) & 0x0fu) == 0) break;
				++depth;
			}
			return depth;
		}

		bool RouteHasPrefix(uint32 route_string, uint32 prefix_route_string) {
			for (stduint shift = 0; shift < 20; shift += 4) {
				const auto prefix = (prefix_route_string >> shift) & 0x0fu;
				if (prefix == 0) return true;
				if (((route_string >> shift) & 0x0fu) != prefix) return false;
			}
			return true;
		}

		USBHostControllerIdentity ControllerIdentity(HostController& xhc) {
			return {"xhci", &xhc, 0x03u};
		}

		USBHostDeviceLocation DeviceLocation(HostController& xhc, USBHostDevice_v3& dev) {
			USBHostDevice* parent_hub = nullptr;
			if (dev.ParentHubSlotID() != 0) {
				parent_hub = xhc.GetDeviceManager()->FindBySlot(dev.ParentHubSlotID());
			}
			return {
				parent_hub,
				dev.SlotID(),
				dev.RootHubPortNum(),
				parent_hub ? dev.UpstreamPortNum() : dev.RootHubPortNum()
			};
		}

		uint32 AppendRouteString(uint32 parent_route_string, uint8 downstream_port) {
			uint32 route_string = parent_route_string;
			stduint shift = 0;
			while (shift < 20 && ((route_string >> shift) & 0x0fu) != 0) {
				shift += 4;
			}
			if (shift >= 20) return route_string;
			return route_string | (uint32(downstream_port & 0x0fu) << shift);
		}

		uint8 DetermineHubChildSpeedClass(const USBHostDevice_v3& hub, uint16 port_status) {
			if (hub.DeviceProtocol() == static_cast<uint8>(HubProtocol::SuperSpeed)) {
				return (port_status & kSuperSpeedHubPortStatusSpeed) == 0
					? kSuperSpeed : kSuperSpeedPlus;
			}
			if ((port_status & kHubPortStatusHighSpeed) != 0) return kHighSpeed;
			if ((port_status & kHubPortStatusLowSpeed) != 0) return kLowSpeed;
			return kFullSpeed;
		}

		bool QueueHubChildAddress(HostController& xhc, PendingHubChildAddress ctx) {
			auto& state = xhc.EnumerationState();
			for (stduint i = 0; i < state.pending_hub_children.Count(); ++i) {
				const auto& pending = state.pending_hub_children[i];
				if (pending.root_hub_port_num == ctx.root_hub_port_num &&
					pending.route_string == ctx.route_string) {
					return true;
				}
			}
			if (state.active_hub_child_valid &&
				state.active_hub_child.root_hub_port_num == ctx.root_hub_port_num &&
				state.active_hub_child.route_string == ctx.route_string) {
				return true;
			}
			return state.pending_hub_children.Append(ctx);
		}

		bool PopHubChildAddress(HostController& xhc, PendingHubChildAddress& ctx) {
			auto& state = xhc.EnumerationState();
			if (state.pending_hub_children.Count() == 0) return false;
			ctx = state.pending_hub_children[0];
			return state.pending_hub_children.Remove(0);
		}

		void RemovePendingHubChildren(HostController& xhc, uint8 root_hub_port_num,
			uint32 route_string_prefix, bool all_routes) {
			auto& state = xhc.EnumerationState();
			for (stduint index = state.pending_hub_children.Count(); index > 0; --index) {
				const auto& pending = state.pending_hub_children[index - 1];
				const bool matches = pending.root_hub_port_num == root_hub_port_num &&
					(all_routes || RouteHasPrefix(pending.route_string, route_string_prefix));
				if (matches) state.pending_hub_children.Remove(index - 1);
			}
			if (state.active_hub_child_valid &&
				state.active_hub_child.root_hub_port_num == root_hub_port_num &&
				(all_routes || RouteHasPrefix(state.active_hub_child.route_string, route_string_prefix))) {
				state.active_hub_child_valid = false;
			}
		}

		void RemoveDeviceSubtree(HostController& xhc, uint8 root_hub_port_num, uint32 route_string_prefix, bool all_routes) {
			uni::Array<uint8, 256> slots{};
			stduint slot_count = 0;
			for (stduint slot_id = 1; slot_id <= xhc.GetDeviceManager()->MaxSlots() && slot_id < slots.size(); ++slot_id) {
				auto* dev = xhc.GetDeviceManager()->FindBySlot(uint8(slot_id));
				if (!dev) continue;
				if (dev->RootHubPortNum() != root_hub_port_num) continue;
				if (!all_routes && !RouteHasPrefix(dev->RouteString(), route_string_prefix)) continue;
				slots[slot_count++] = uint8(slot_id);
			}
			for (stduint i = 0; i < slot_count; ++i) {
				for (stduint j = i + 1; j < slot_count; ++j) {
					auto* lhs = xhc.GetDeviceManager()->FindBySlot(slots[i]);
					auto* rhs = xhc.GetDeviceManager()->FindBySlot(slots[j]);
					const auto lhs_depth = lhs ? RouteDepth(lhs->RouteString()) : 0;
					const auto rhs_depth = rhs ? RouteDepth(rhs->RouteString()) : 0;
					if (rhs_depth > lhs_depth) {
						auto tmp = slots[i];
						slots[i] = slots[j];
						slots[j] = tmp;
					}
				}
			}
			for (stduint i = 0; i < slot_count; ++i) {
				if (auto err = xhc.QueueSlotRemoval(slots[i], true)) {
					plogerro("xHCI failed to queue Disable Slot for slot %u: %s",
						slots[i], err.Name());
				}
			}
			RemovePendingHubChildren(xhc, root_hub_port_num, route_string_prefix, all_routes);
		}
	}

	USBHostDevice_v3::USBHostDevice_v3(uint8 slot_id, DoorbellRegister* dbreg,
		HostController* host, uint8 context_size)
		: slot_id_{ slot_id }, dbreg_{ dbreg }, host_{ host }, context_size_{ context_size },
		isochronous_schedule_valid_{ isochronous_schedule_valid_storage_,
			sizeof(isochronous_schedule_valid_storage_) },
		isochronous_stream_started_{ isochronous_stream_started_storage_,
			sizeof(isochronous_stream_started_storage_) } {
	}

	USBHostDevice_v3::~USBHostDevice_v3() {
		for (auto* ring : transfer_rings_) {
			if (ring == nullptr) continue;
			ring->~Ring();
			uni_hostenv_allocator->deallocate(ring);
		}
		if (input_ctx_) uni_hostenv_allocator->deallocate(input_ctx_);
		if (ctx_) uni_hostenv_allocator->deallocate(ctx_);
	}

	Error USBHostDevice_v3::Initialize() {
		ctx_ = AllocArray<DeviceContext>(1, 64, 4096);
		if (!ctx_) return MAKE_ERROR(Error::kNoEnoughMemory);
		input_ctx_ = AllocArray<InputContext>(1, 64, 4096);
		if (!input_ctx_) {
			uni_hostenv_allocator->deallocate(ctx_);
			ctx_ = nullptr;
			return MAKE_ERROR(Error::kNoEnoughMemory);
		}
		state_ = State::Blank;
		for (size_t i = 0; i < 31; ++i) {
			const DeviceContextIndex dci(i + 1);
			//on_transferred_callbacks_[i] = nullptr;
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	void USBHostDevice_v3::SelectForSlotAssignment() {
		state_ = State::SlotAssigning;
	}

	Ring* USBHostDevice_v3::AllocTransferRing(DeviceContextIndex index, size_t buf_size) {
		int i = index.value - 1;
		if (i < 0 || i >= static_cast<int>(transfer_rings_.size()) ||
			transfer_rings_[i] != nullptr) {
			return nullptr;
		}
		auto tr = AllocArray<Ring>(1, 64, 4096);
		if (tr) {
			new(tr) Ring();
			if (auto err = tr->Initialize(buf_size)) {
				tr->~Ring();
				uni_hostenv_allocator->deallocate(tr);
				return nullptr;
			}
		}
		transfer_rings_[i] = tr;
		return tr;
	}

	Error USBHostDevice_v3::ControlIn(EndpointID ep_id, SetupData setup_data,
		void* buf, int len, ClassDriver* issuer) {
		Log(kDebug, "Device::ControlIn: ep addr %d, buf 0x%08x, len %d\n",
			ep_id.Address(), buf, len);
		if (ep_id.Number() < 0 || 15 < ep_id.Number()) {
			return MAKE_ERROR(Error::kInvalidEndpointNumber);
		}

		// control endpoint must be dir_in=true
		const DeviceContextIndex dci{ ep_id };

		Ring* tr = transfer_rings_[dci.value - 1];

		if (tr == nullptr) {
			return MAKE_ERROR(Error::kTransferRingNotSet);
		}
		if (setup_stage_map_.IsFull()) return MAKE_ERROR(Error::kFull);
		if (auto err = USBHostDevice::ControlIn(ep_id, setup_data, buf, len, issuer)) {
			return err;
		}

		auto status = StatusStageTRB{};

		if (buf) {
			auto setup_trb_position = TRBDynamicCast<SetupStageTRB>(tr->Push(
				MakeSetupStageTRB(setup_data, SetupStageTRB::kInDataStage)));
			auto data = MakeDataStageTRB(buf, len, true);
			data.bits.interrupt_on_completion = true;
			auto data_trb_position = tr->Push(data);
			tr->Push(status);

			if (!setup_stage_map_.Put(data_trb_position, setup_trb_position)) {
				return MAKE_ERROR(Error::kFull);
			}
		}
		else {
			auto setup_trb_position = TRBDynamicCast<SetupStageTRB>(tr->Push(
				MakeSetupStageTRB(setup_data, SetupStageTRB::kNoDataStage)));
			status.bits.direction = true;
			status.bits.interrupt_on_completion = true;
			auto status_trb_position = tr->Push(status);

			if (!setup_stage_map_.Put(status_trb_position, setup_trb_position)) {
				return MAKE_ERROR(Error::kFull);
			}
		}

		dbreg_->Ring(dci.value);

		return MAKE_ERROR(Error::kSuccess);
	}

	Error USBHostDevice_v3::ControlOut(EndpointID ep_id, SetupData setup_data,
		const void* buf, int len, ClassDriver* issuer) {
		if (ep_id.Number() < 0 || 15 < ep_id.Number()) {
			return MAKE_ERROR(Error::kInvalidEndpointNumber);
		}
		const DeviceContextIndex dci{ ep_id };
		if (transfer_rings_[dci.value - 1] == nullptr) {
			return MAKE_ERROR(Error::kTransferRingNotSet);
		}
		if (setup_stage_map_.IsFull()) return MAKE_ERROR(Error::kFull);
		if (auto err = USBHostDevice::ControlOut(ep_id, setup_data, buf, len, issuer)) {
			return err;
		}
		return QueueControlOut(ep_id, setup_data, buf, len);
	}

	Error USBHostDevice_v3::QueueControlOut(EndpointID ep_id, SetupData setup_data,
		const void* buf, int len) {

		// Log(kDebug, "Device::ControlOut: ep addr %d, buf 0x%08x, len %d", ep_id.Address(), buf, len);
		if (ep_id.Number() < 0 || 15 < ep_id.Number()) {
			return MAKE_ERROR(Error::kInvalidEndpointNumber);
		}

		// control endpoint must be dir_in=true
		const DeviceContextIndex dci{ ep_id };

		Ring* tr = transfer_rings_[dci.value - 1];

		if (tr == nullptr) {
			return MAKE_ERROR(Error::kTransferRingNotSet);
		}
		if (setup_stage_map_.IsFull()) return MAKE_ERROR(Error::kFull);

		auto status = StatusStageTRB{};
		status.bits.direction = true;

		if (buf) {
			auto setup_trb_position = TRBDynamicCast<SetupStageTRB>(tr->Push(
				MakeSetupStageTRB(setup_data, SetupStageTRB::kOutDataStage)));
			auto data = MakeDataStageTRB(buf, len, false);
			data.bits.interrupt_on_completion = true;
			auto data_trb_position = tr->Push(data);
			tr->Push(status);

			if (!setup_stage_map_.Put(data_trb_position, setup_trb_position)) {
				return MAKE_ERROR(Error::kFull);
			}
		}
		else {
			auto setup_trb_position = TRBDynamicCast<SetupStageTRB>(tr->Push(
				MakeSetupStageTRB(setup_data, SetupStageTRB::kNoDataStage)));
			status.bits.interrupt_on_completion = true;
			auto status_trb_position = tr->Push(status);

			if (!setup_stage_map_.Put(status_trb_position, setup_trb_position)) {
				return MAKE_ERROR(Error::kFull);
			}
		}

		dbreg_->Ring(dci.value);

		return MAKE_ERROR(Error::kSuccess);
	}

	Error USBHostDevice_v3::InterruptIn(EndpointID ep_id, void* buf, int len) {
		if (auto err = USBHostDevice::InterruptIn(ep_id, buf, len)) {
			return err;
		}

		const DeviceContextIndex dci{ ep_id };

		Ring* tr = transfer_rings_[dci.value - 1];

		if (tr == nullptr) {
			return MAKE_ERROR(Error::kTransferRingNotSet);
		}

		NormalTRB normal{};
		normal.SetPointer(buf);
		normal.bits.trb_transfer_length = len;
		normal.bits.interrupt_on_short_packet = true;
		normal.bits.interrupt_on_completion = true;

		tr->Push(normal);
		dbreg_->Ring(dci.value);
		return MAKE_ERROR(Error::kSuccess);
	}

	Error USBHostDevice_v3::InterruptOut(EndpointID ep_id, void* buf, int len) {
		if (auto err = USBHostDevice::InterruptOut(ep_id, buf, len)) {
			return err;
		}
		if (ep_id.Number() <= 0 || ep_id.Number() > 15 || ep_id.IsIn() ||
			buf == nullptr || len <= 0) {
			return MAKE_ERROR(Error::kInvalidEndpointNumber);
		}
		const auto* config = EndpointConfigOf(ep_id);
		if (config == nullptr || config->ep_type != EndpointType::kInterrupt) {
			return MAKE_ERROR(Error::kInvalidEndpointNumber);
		}

		const DeviceContextIndex dci{ ep_id };
		Ring* tr = transfer_rings_[dci.value - 1];
		if (tr == nullptr) {
			return MAKE_ERROR(Error::kTransferRingNotSet);
		}

		NormalTRB normal{};
		normal.SetPointer(buf);
		normal.bits.trb_transfer_length = len;
		normal.bits.interrupt_on_completion = true;
		tr->Push(normal);
		dbreg_->Ring(dci.value);
		return MAKE_ERROR(Error::kSuccess);
	}

	const EndpointConfig* USBHostDevice_v3::EndpointConfigOf(EndpointID ep_id) {
		for (int i = 0; i < NumEndpointConfigs(); ++i) {
			const auto& config = EndpointConfigs()[i];
			if (config.ep_id.Address() == ep_id.Address()) return &config;
		}
		return nullptr;
	}

	Error USBHostDevice_v3::BulkTransfer(EndpointID ep_id, bool dir_in, void* buf, int len) {
		if (ep_id.Number() <= 0 || ep_id.Number() > 15 || ep_id.IsIn() != dir_in ||
			buf == nullptr || len <= 0) {
			return MAKE_ERROR(Error::kInvalidEndpointNumber);
		}
		const auto* config = EndpointConfigOf(ep_id);
		if (config == nullptr || config->ep_type != EndpointType::kBulk) {
			return MAKE_ERROR(Error::kInvalidEndpointNumber);
		}

		const DeviceContextIndex dci{ ep_id };
		Ring* tr = transfer_rings_[dci.value - 1];
		if (tr == nullptr) {
			return MAKE_ERROR(Error::kTransferRingNotSet);
		}
		auto& pending = pending_bulk_transfers_[dci.value - 1];
		if (pending.active) {
			return MAKE_ERROR(Error::kFull);
		}
		if (config->max_packet_size <= 0) {
			return MAKE_ERROR(Error::kInvalidEndpointNumber);
		}

		constexpr uintptr_t kBufferBoundary = 64u * 1024u;
		auto* next = static_cast<uint8*>(buf);
		int remaining = len;
		size_t trb_count = 0;
		while (remaining > 0) {
			const uintptr_t bytes_to_boundary = kBufferBoundary -
				(reinterpret_cast<uintptr_t>(next) & (kBufferBoundary - 1));
			const int chunk = remaining < static_cast<int>(bytes_to_boundary)
				? remaining : static_cast<int>(bytes_to_boundary);
			remaining -= chunk;
			next += chunk;
			if (++trb_count > tr->UsableSize()) {
				return MAKE_ERROR(Error::kFull);
			}
		}

		pending = PendingBulkTransfer{};
		pending.buffer = buf;
		pending.length = len;
		pending.trb_count = trb_count;
		pending.active = true;

		const uint64 max_packet_size = static_cast<uint64>(config->max_packet_size);
		const uint64 td_packet_count =
			(static_cast<uint64>(len) + max_packet_size - 1) / max_packet_size;
		next = static_cast<uint8*>(buf);
		remaining = len;
		uint64 enqueued_length = 0;
		for (size_t index = 0; index < trb_count; ++index) {
			const uintptr_t bytes_to_boundary = kBufferBoundary -
				(reinterpret_cast<uintptr_t>(next) & (kBufferBoundary - 1));
			const int chunk = remaining < static_cast<int>(bytes_to_boundary)
				? remaining : static_cast<int>(bytes_to_boundary);
			const bool last = index + 1 == trb_count;
			enqueued_length += chunk;

			NormalTRB normal{};
			normal.SetPointer(next);
			normal.bits.trb_transfer_length = chunk;
			if (!last) {
				const uint64 packets_transferred = enqueued_length / max_packet_size;
				const uint64 packets_remaining = td_packet_count - packets_transferred;
				normal.bits.td_size = static_cast<uint32>(
					packets_remaining > 31 ? 31 : packets_remaining);
			}
			normal.bits.interrupt_on_short_packet = dir_in;
			normal.bits.chain_bit = !last;
			normal.bits.interrupt_on_completion = last;
			TRB* position = tr->Push(normal);
			if (index == 0) pending.first_trb = position;
			if (last) pending.last_trb = position;

			remaining -= chunk;
			next += chunk;
		}
		pending.next_trb = tr->EnqueuePointer();
		pending.next_cycle_state = tr->ProducerCycleState();
		dbreg_->Ring(dci.value);
		return MAKE_ERROR(Error::kSuccess);
	}

	size_t USBHostDevice_v3::ActiveIsochronousTRBs(EndpointID ep_id) const {
		size_t count = 0;
		for (const auto& pending : pending_isochronous_transfers_) {
			if (pending.ep_id.Address() == ep_id.Address()) {
				count += pending.trb_count;
			}
		}
		return count;
	}

	Error USBHostDevice_v3::IsochronousTransfer(EndpointID ep_id, void* buf, int len,
		const IsochronousTransferOptions& options) {
		if (host_ == nullptr || host_->IsControllerFailed()) {
			return MAKE_ERROR(Error::kTransferFailed);
		}
		if (ep_id.Number() <= 0 || ep_id.Number() > 15 || len < 0 ||
			(buf == nullptr && len != 0)) {
			return MAKE_ERROR(Error::kInvalidEndpointNumber);
		}
		const auto* config = EndpointConfigOf(ep_id);
		if (config == nullptr || config->ep_type != EndpointType::kIsochronous ||
			config->max_packet_size <= 0) {
			return MAKE_ERROR(Error::kInvalidEndpointNumber);
		}
		uint32 max_esit_payload = config->bytes_per_interval;
		if (max_esit_payload == 0) {
			max_esit_payload = uint32(config->max_packet_size) *
				(uint32(config->max_burst) + 1u) * (uint32(config->mult) + 1u);
		}
		if (uint32(len) > max_esit_payload) {
			return MAKE_ERROR(Error::kBufferTooSmall);
		}

		const DeviceContextIndex dci{ ep_id };
		Ring* tr = transfer_rings_[dci.value - 1];
		if (tr == nullptr) return MAKE_ERROR(Error::kTransferRingNotSet);

		constexpr uintptr_t kBufferBoundary = 64u * 1024u;
		auto* next = static_cast<uint8*>(buf);
		int remaining = len;
		size_t trb_count = 0;
		do {
			const uintptr_t bytes_to_boundary = next == nullptr ? kBufferBoundary :
				kBufferBoundary - (reinterpret_cast<uintptr_t>(next) & (kBufferBoundary - 1));
			const int chunk = remaining < static_cast<int>(bytes_to_boundary)
				? remaining : static_cast<int>(bytes_to_boundary);
			remaining -= chunk;
			if (next != nullptr) next += chunk;
			++trb_count;
		} while (remaining > 0);
		if (ActiveIsochronousTRBs(ep_id) + trb_count > tr->UsableSize()) {
			return MAKE_ERROR(Error::kFull);
		}

		const int speed_class = host_->SpeedClass(RootHubPortNum(),
			GetSlotContext()->bits.speed);
		if (speed_class == 0) return MAKE_ERROR(Error::kUnknownXHCISpeedID);
		const int interval_exponent = speed_class == kFullSpeed || speed_class == kLowSpeed
			? config->interval + 2 : config->interval - 1;
		if (interval_exponent < 0 || interval_exponent > 18) {
			return MAKE_ERROR(Error::kInvalidDescriptor);
		}
		uint32 interval_uframes = 1u << interval_exponent;
		if (interval_uframes > 16384u) interval_uframes = 16384u;

		bool schedule_immediately = options.schedule_immediately;
		uint16 frame_id = options.frame_id;
		if (!schedule_immediately) {
			if (options.automatic_frame_id) {
				const uint32 current_uframe = host_->CurrentMicroframeIndex();
				const uint32 scheduling_lead =
					host_->IsochronousSchedulingThreshold() + 10u;
				uint32 start_uframe = next_isochronous_uframe_[dci.value - 1];
				const uint32 scheduled_delta = (start_uframe - current_uframe) & 0x3fffu;
				if (!isochronous_schedule_valid_[dci.value - 1] ||
					scheduled_delta < scheduling_lead || scheduled_delta > 0x2000u) {
					start_uframe = current_uframe + scheduling_lead;
					start_uframe = (start_uframe + 7u) & ~7u;
					start_uframe = ((start_uframe + interval_uframes - 1u) /
						interval_uframes) * interval_uframes;
					start_uframe %= 16384u;
				}
				frame_id = static_cast<uint16>((start_uframe >> 3) & 0x7ffu);
				next_isochronous_uframe_[dci.value - 1] = static_cast<uint16>(
					(start_uframe + interval_uframes) % 16384u);
				isochronous_schedule_valid_.setof(dci.value - 1);
			}
			else if (frame_id > 0x7ffu) {
				return MAKE_ERROR(Error::kInvalidPhase);
			}
			else {
				next_isochronous_uframe_[dci.value - 1] = static_cast<uint16>(
					(uint32(frame_id) * 8u + interval_uframes) % 16384u);
				isochronous_schedule_valid_.setof(dci.value - 1);
			}
			if (!host_->HasContiguousFrameIDCapability() &&
				isochronous_stream_started_[dci.value - 1]) {
				schedule_immediately = true;
			}
		}

		const uint32 max_packet_size = static_cast<uint32>(config->max_packet_size);
		uint32 packet_count = (uint32(len) + max_packet_size - 1u) / max_packet_size;
		if (packet_count == 0) packet_count = 1;
		uint32 burst_count = 0;
		uint32 last_burst_packet_count = 0;
		if (host_->HCIVersion() >= 0x0100u && speed_class >= kSuperSpeed) {
			const uint32 packets_per_burst = uint32(config->max_burst) + 1u;
			burst_count = (packet_count + packets_per_burst - 1u) / packets_per_burst - 1u;
			const uint32 residue = packet_count % packets_per_burst;
			last_burst_packet_count = residue == 0 ? config->max_burst : residue - 1u;
		}
		else if (host_->HCIVersion() >= 0x0100u) {
			last_burst_packet_count = packet_count - 1u;
		}
		const bool extended_tbc = host_->HasExtendedTBCCapability();
		const uint32 max_burst_count = extended_tbc ? 31u : 3u;
		if (burst_count > max_burst_count || last_burst_packet_count > 15u) {
			return MAKE_ERROR(Error::kInvalidDescriptor);
		}

		PendingIsochronousTransfer pending{};
		pending.ep_id = ep_id;
		pending.buffer = buf;
		pending.length = len;
		pending.trb_count = trb_count;
		pending.frame_id = frame_id;
		pending.schedule_immediately = schedule_immediately;

		next = static_cast<uint8*>(buf);
		remaining = len;
		uint32 enqueued_length = 0;
		const bool first_cycle_state = tr->ProducerCycleState();
		for (size_t index = 0; index < trb_count; ++index) {
			const uintptr_t bytes_to_boundary = next == nullptr ? kBufferBoundary :
				kBufferBoundary - (reinterpret_cast<uintptr_t>(next) & (kBufferBoundary - 1));
			const int chunk = remaining < static_cast<int>(bytes_to_boundary)
				? remaining : static_cast<int>(bytes_to_boundary);
			const bool last = index + 1 == trb_count;
			enqueued_length += chunk;
			const uint32 packets_transferred = enqueued_length / max_packet_size;
			const uint32 remaining_packets = packet_count - packets_transferred;

			TRB* position;
			if (index == 0) {
				IsochronousTRB isoch{};
				isoch.SetPointer(next);
				isoch.bits.trb_transfer_length = chunk;
				isoch.bits.td_size = extended_tbc ? burst_count :
					(remaining_packets > 31u ? 31u : remaining_packets);
				isoch.bits.interrupt_on_short_packet = ep_id.IsIn();
				isoch.bits.chain_bit = !last;
				isoch.bits.interrupt_on_completion = last;
				isoch.bits.transfer_burst_count = extended_tbc ? 0u : burst_count;
				isoch.bits.transfer_last_burst_packet_count = last_burst_packet_count;
				isoch.bits.frame_id = frame_id;
				isoch.bits.schedule_immediately = schedule_immediately;
				position = tr->PushDeferred(isoch);
				pending.first_trb = position;
			}
			else {
				NormalTRB normal{};
				normal.SetPointer(next);
				normal.bits.trb_transfer_length = chunk;
				normal.bits.td_size = remaining_packets > 31u ? 31u : remaining_packets;
				normal.bits.interrupt_on_short_packet = ep_id.IsIn();
				normal.bits.chain_bit = !last;
				normal.bits.interrupt_on_completion = last;
				position = tr->Push(normal);
			}
			if (last) pending.last_trb = position;
			remaining -= chunk;
			if (next != nullptr) next += chunk;
		}
		pending_isochronous_transfers_.Append(pending);
		tr->Commit(pending.first_trb, first_cycle_state);
		isochronous_stream_started_.setof(dci.value - 1);
		dbreg_->Ring(dci.value);
		return MAKE_ERROR(Error::kSuccess);
	}

	USBHostDevice_v3::PendingIsochronousTransfer*
	USBHostDevice_v3::FindIsochronousTransfer(EndpointID ep_id,
		const TRB* issuer_trb) {
		if (issuer_trb == nullptr) return nullptr;
		const DeviceContextIndex dci{ ep_id };
		Ring* tr = transfer_rings_[dci.value - 1];
		if (tr == nullptr) return nullptr;
		for (auto& pending : pending_isochronous_transfers_) {
			if (pending.ep_id.Address() != ep_id.Address()) continue;
			const TRB* cursor = pending.first_trb;
			for (size_t index = 0; index < pending.trb_count; ++index) {
				if (cursor == issuer_trb) return &pending;
				cursor = tr->NextTransferTRB(cursor);
				if (cursor == nullptr) break;
			}
		}
		return nullptr;
	}

	int USBHostDevice_v3::IsochronousTransferredLength(
		const PendingIsochronousTransfer& pending, const TRB* issuer_trb,
		int residual_length) const {
		const DeviceContextIndex dci{ pending.ep_id };
		Ring* tr = transfer_rings_[dci.value - 1];
		if (tr == nullptr || issuer_trb == nullptr || residual_length < 0) return -1;
		const TRB* cursor = pending.first_trb;
		int transferred = 0;
		for (size_t index = 0; index < pending.trb_count; ++index) {
			int trb_length;
			if (index == 0) {
				auto* isoch = TRBDynamicCast<IsochronousTRB>(const_cast<TRB*>(cursor));
				if (isoch == nullptr) return -1;
				trb_length = isoch->bits.trb_transfer_length;
			}
			else {
				auto* normal = TRBDynamicCast<NormalTRB>(const_cast<TRB*>(cursor));
				if (normal == nullptr) return -1;
				trb_length = normal->bits.trb_transfer_length;
			}
			if (cursor == issuer_trb) {
				if (residual_length > trb_length) return -1;
				return transferred + trb_length - residual_length;
			}
			transferred += trb_length;
			cursor = tr->NextTransferTRB(cursor);
			if (cursor == nullptr) return -1;
		}
		return -1;
	}

	int USBHostDevice_v3::BulkTransferredLength(EndpointID ep_id, const TRB* issuer_trb,
		int residual_length) const {
		const DeviceContextIndex dci{ ep_id };
		const auto& pending = pending_bulk_transfers_[dci.value - 1];
		Ring* tr = transfer_rings_[dci.value - 1];
		if (!pending.active || tr == nullptr || residual_length < 0) return -1;

		const TRB* cursor = pending.first_trb;
		int transferred = 0;
		for (size_t index = 0; index < pending.trb_count; ++index) {
			auto* normal = TRBDynamicCast<NormalTRB>(const_cast<TRB*>(cursor));
			if (normal == nullptr) return -1;
			const int trb_length = normal->bits.trb_transfer_length;
			if (cursor == issuer_trb) {
				if (residual_length > trb_length) return -1;
				return transferred + trb_length - residual_length;
			}
			transferred += trb_length;
			cursor = tr->NextTransferTRB(cursor);
			if (cursor == nullptr) return -1;
		}
		return -1;
	}

	Error USBHostDevice_v3::BeginBulkRecovery(EndpointID ep_id) {
		const DeviceContextIndex dci{ ep_id };
		auto& pending = pending_bulk_transfers_[dci.value - 1];
		if (!pending.active || pending.recovery_phase != BulkRecoveryPhase::None ||
			host_ == nullptr) {
			return MAKE_ERROR(Error::kInvalidPhase);
		}
		pending.recovery_phase = BulkRecoveryPhase::ResetEndpoint;
		ResetEndpointCommandTRB reset{ ep_id, slot_id_, false };
		host_->CommandRing()->Push(reset);
		host_->DoorbellRegisterAt(0)->Ring(0);
		return MAKE_ERROR(Error::kSuccess);
	}

	Error USBHostDevice_v3::CompleteBulkFailure(EndpointID ep_id) {
		const DeviceContextIndex dci{ ep_id };
		auto& pending = pending_bulk_transfers_[dci.value - 1];
		pending = PendingBulkTransfer{};
		return this->OnBulkCompleted(ep_id, nullptr, 0);
	}

	Error USBHostDevice_v3::OnEndpointResetCompleted(EndpointID ep_id, int completion_code) {
		const DeviceContextIndex dci{ ep_id };
		auto& pending = pending_bulk_transfers_[dci.value - 1];
		if (!pending.active || pending.recovery_phase != BulkRecoveryPhase::ResetEndpoint) {
			return MAKE_ERROR(Error::kInvalidPhase);
		}
		if (completion_code != 1 || host_ == nullptr) {
			return CompleteBulkFailure(ep_id);
		}

		pending.recovery_phase = BulkRecoveryPhase::ClearEndpointHalt;
		if (auto err = this->OnBulkCompleted(ep_id, nullptr, 0)) {
			pending = PendingBulkTransfer{};
			return err;
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error USBHostDevice_v3::QueueBulkDequeuePointer(EndpointID ep_id) {
		const DeviceContextIndex dci{ ep_id };
		auto& pending = pending_bulk_transfers_[dci.value - 1];
		if (!pending.active || host_ == nullptr ||
			(pending.recovery_phase != BulkRecoveryPhase::ClearEndpointHalt &&
			pending.recovery_phase != BulkRecoveryPhase::ClearTTBuffer)) {
			return MAKE_ERROR(Error::kInvalidPhase);
		}

		pending.recovery_phase = BulkRecoveryPhase::SetDequeuePointer;
		SetTRDequeuePointerCommandTRB set_dequeue{
			pending.next_trb, pending.next_cycle_state, ep_id, slot_id_ };
		host_->CommandRing()->Push(set_dequeue);
		host_->DoorbellRegisterAt(0)->Ring(0);
		return MAKE_ERROR(Error::kSuccess);
	}

	Error USBHostDevice_v3::BeginTTBufferClear(EndpointID ep_id) {
		const DeviceContextIndex dci{ ep_id };
		auto& pending = pending_bulk_transfers_[dci.value - 1];
		if (!pending.active ||
			pending.recovery_phase != BulkRecoveryPhase::ClearEndpointHalt ||
			host_ == nullptr) {
			return MAKE_ERROR(Error::kInvalidPhase);
		}

		const auto* slot_ctx = GetSlotContext();
		if (slot_ctx->bits.tt_hub_slot_id == 0) {
			return QueueBulkDequeuePointer(ep_id);
		}
		auto* tt_hub = host_->GetDeviceManager()->FindBySlot(
			slot_ctx->bits.tt_hub_slot_id);
		if (tt_hub == nullptr) {
			pending = PendingBulkTransfer{};
			return MAKE_ERROR(Error::kTransferFailed);
		}

		pending.recovery_phase = BulkRecoveryPhase::ClearTTBuffer;
		if (auto err = tt_hub->SubmitTTBufferClear(*this, ep_id)) {
			pending = PendingBulkTransfer{};
			return err;
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error USBHostDevice_v3::SubmitTTBufferClear(USBHostDevice_v3& child,
		EndpointID ep_id) {
		const auto* child_slot_ctx = child.GetSlotContext();
		if (child_slot_ctx->bits.tt_hub_slot_id != slot_id_ ||
			child_slot_ctx->bits.usb_device_address == 0 ||
			ep_id.Number() <= 0 || ep_id.Number() > 15) {
			return MAKE_ERROR(Error::kInvalidPhase);
		}

		SetupData setup_data{};
		setup_data.request_type.bits.direction = request_type::kOut;
		setup_data.request_type.bits.type = request_type::kClass;
		setup_data.request_type.bits.recipient = request_type::kOther;
		setup_data.request = static_cast<uint8>(HubRequest::ClearTTBuffer);
		setup_data.value = static_cast<uint16>(ep_id.Number()) |
			(static_cast<uint16>(child_slot_ctx->bits.usb_device_address) << 4) |
			(static_cast<uint16>(EndpointType::kBulk) << 11) |
			(ep_id.IsIn() ? 0x8000u : 0u);
		setup_data.index = GetSlotContext()->bits.mtt
			? child_slot_ctx->bits.tt_port_num : 1;
		setup_data.length = 0;
		return QueueControlOut(kDefaultControlPipeID, setup_data, nullptr, 0);
	}

	Error USBHostDevice_v3::OnTTBufferClearCompleted(EndpointID ep_id,
		int completion_code) {
		const DeviceContextIndex dci{ ep_id };
		auto& pending = pending_bulk_transfers_[dci.value - 1];
		if (!pending.active ||
			pending.recovery_phase != BulkRecoveryPhase::ClearTTBuffer) {
			return MAKE_ERROR(Error::kInvalidPhase);
		}
		if (completion_code != 1) {
			pending = PendingBulkTransfer{};
			return MAKE_ERROR(Error::kTransferFailed);
		}
		return QueueBulkDequeuePointer(ep_id);
	}

	Error USBHostDevice_v3::OnTransferRingDequeueSet(EndpointID ep_id, int completion_code) {
		const DeviceContextIndex dci{ ep_id };
		auto& pending = pending_bulk_transfers_[dci.value - 1];
		if (!pending.active ||
			pending.recovery_phase != BulkRecoveryPhase::SetDequeuePointer) {
			return MAKE_ERROR(Error::kInvalidPhase);
		}
		pending = PendingBulkTransfer{};
		return completion_code == 1
			? MAKE_ERROR(Error::kSuccess) : MAKE_ERROR(Error::kTransferFailed);
	}

	Error USBHostDevice_v3::OnHubPortStatusReceived(uint8 port_num, uint16 status,
		uint16 change, uint8 speed_id) {
		if (!host_) return MAKE_ERROR(Error::kNotImplemented);
		return host_->OnHubPortStatusChanged(*this, port_num, status, change, speed_id);
	}

	Error USBHostDevice_v3::ConfigureHub(uint8 num_ports, uint16 characteristics) {
		if (num_ports == 0 || hub_context_update_pending_) {
			return MAKE_ERROR(Error::kInvalidPhase);
		}
		const uint8 speed_class = host_->SpeedClass(
			RootHubPortNum(), GetSlotContext()->bits.speed);
		if (speed_class == 0) return MAKE_ERROR(Error::kUnknownXHCISpeedID);

		MemSet(GetInputControlContext(), 0, ContextSize());
		MemCopyN(GetInputSlotContext(), GetSlotContext(), sizeof(SlotContext));
		auto* slot_ctx = EnableInputSlotContext();
		slot_ctx->bits.hub = 1;
		slot_ctx->bits.num_ports = num_ports;
		if (speed_class == kHighSpeed) {
			slot_ctx->bits.mtt = DeviceProtocol() ==
				static_cast<uint8>(HubProtocol::HighSpeedMultipleTT);
			slot_ctx->bits.ttt = (characteristics >> 5) & 0x03u;
		}
		else {
			slot_ctx->bits.mtt = 0;
			slot_ctx->bits.ttt = 0;
		}

		hub_context_update_pending_ = true;
		ConfigureEndpointCommandTRB cmd{ InputContextBuffer(), SlotID() };
		host_->CommandRing()->Push(cmd);
		host_->DoorbellRegisterAt(0)->Ring(0);
		return MAKE_ERROR(Error::kSuccess);
	}

	uint8 USBHostDevice_v3::HubDepth() const {
		const stduint depth = RouteDepth(RouteString());
		return static_cast<uint8>(depth > 4 ? 4 : depth);
	}

	Error USBHostDevice_v3::OnTransferEventReceived(const TransferEventTRB& trb) {
		const auto residual_length = trb.bits.trb_transfer_length;
		const bool transfer_succeeded = trb.bits.completion_code == 1 /* Success */ ||
			trb.bits.completion_code == 13 /* Short Packet */;
		Log(kDebug, trb);

		TRB* issuer_trb = trb.Pointer();
		{
			const auto ep_id = trb.GetEndpointID();
			const auto* config = EndpointConfigOf(ep_id);
			if (config != nullptr && config->ep_type == EndpointType::kIsochronous) {
				if (issuer_trb == nullptr) {
					if (trb.bits.completion_code == 14 || trb.bits.completion_code == 15) {
						return this->OnIsochronousCompleted(ep_id, nullptr, 0, 0, true,
							trb.bits.completion_code);
					}
					return MAKE_ERROR(Error::kTransferFailed);
				}
				auto* pending = FindIsochronousTransfer(ep_id, issuer_trb);
				if (pending == nullptr) return MAKE_ERROR(Error::kInvalidPhase);
				const stduint pending_index = static_cast<stduint>(
					pending - pending_isochronous_transfers_.begin());
				int transfer_length = transfer_succeeded
					? IsochronousTransferredLength(*pending, issuer_trb,
						static_cast<int>(residual_length)) : -1;
				if (transfer_succeeded && transfer_length < 0) {
					transfer_length = -1;
				}
				const PendingIsochronousTransfer completed = *pending;
				pending_isochronous_transfers_.Remove(pending_index);
				return this->OnIsochronousCompleted(ep_id, completed.buffer,
					transfer_length, completed.frame_id,
					completed.schedule_immediately, trb.bits.completion_code);
			}
		}
		if (issuer_trb == nullptr) return MAKE_ERROR(Error::kTransferFailed);
		if (auto normal_trb = TRBDynamicCast<NormalTRB>(issuer_trb)) {
			const auto ep_id = trb.GetEndpointID();
			const auto* config = EndpointConfigOf(ep_id);
			if (config == nullptr) {
				return MAKE_ERROR(Error::kInvalidEndpointNumber);
			}
			if (config->ep_type == EndpointType::kBulk) {
				const DeviceContextIndex dci{ ep_id };
				auto& pending = pending_bulk_transfers_[dci.value - 1];
				if (!pending.active) {
					return MAKE_ERROR(Error::kInvalidPhase);
				}
				if (!transfer_succeeded) {
					if (pending.recovery_phase != BulkRecoveryPhase::None) {
						return MAKE_ERROR(Error::kSuccess);
					}
					return BeginBulkRecovery(ep_id);
				}

				int transfer_length = BulkTransferredLength(
					ep_id, issuer_trb, static_cast<int>(residual_length));
				if (transfer_length < 0) {
					return CompleteBulkFailure(ep_id);
				}
				if (trb.bits.completion_code == 13 /* Short Packet */ &&
					issuer_trb != pending.last_trb) {
					if (!pending.short_event_seen) {
						pending.short_transfer_length = transfer_length;
						pending.short_event_seen = true;
					}
					return MAKE_ERROR(Error::kSuccess);
				}
				if (issuer_trb != pending.last_trb) {
					return MAKE_ERROR(Error::kInvalidPhase);
				}
				if (pending.short_event_seen) {
					transfer_length = pending.short_transfer_length;
				}
				void* buffer = pending.buffer;
				pending = PendingBulkTransfer{};
				return this->OnBulkCompleted(ep_id, buffer, transfer_length);
			}
			if (!transfer_succeeded ||
				residual_length > normal_trb->bits.trb_transfer_length) {
				if (config->ep_type == EndpointType::kInterrupt) {
					return this->OnInterruptCompleted(ep_id, nullptr, 0);
				}
				return MAKE_ERROR(Error::kTransferFailed);
			}
			const auto transfer_length =
				normal_trb->bits.trb_transfer_length - residual_length;
			if (config->ep_type == EndpointType::kInterrupt) {
				return this->OnInterruptCompleted(
					ep_id, normal_trb->Pointer(), transfer_length);
			}
			return MAKE_ERROR(Error::kNotImplemented);
		}

		auto opt_setup_stage_trb = setup_stage_map_.Get(issuer_trb);
		if (!opt_setup_stage_trb) {
			if (!transfer_succeeded) {
				return MAKE_ERROR(Error::kTransferFailed);
			}
			Log(kDebug, "No Corresponding Setup Stage for issuer %s\n",
				kTRBTypeToName[issuer_trb->bits.trb_type]);
			if (auto data_trb = TRBDynamicCast<DataStageTRB>(issuer_trb)) {
				Log(kDebug, *data_trb);
			}
			return MAKE_ERROR(Error::kNoCorrespondingSetupStage);
		}
		setup_stage_map_.Delete(issuer_trb);

		auto setup_stage_trb = opt_setup_stage_trb.value();
		SetupData setup_data{};
		setup_data.request_type.data = setup_stage_trb->bits.request_type;
		setup_data.request = setup_stage_trb->bits.request;
		setup_data.value = setup_stage_trb->bits.value;
		setup_data.index = setup_stage_trb->bits.index;
		setup_data.length = setup_stage_trb->bits.length;

		void* data_stage_buffer{ nullptr };
		int transfer_length{ 0 };
		if (auto data_stage_trb = TRBDynamicCast<DataStageTRB>(issuer_trb)) {
			data_stage_buffer = data_stage_trb->Pointer();
			transfer_length =
				data_stage_trb->bits.trb_transfer_length - residual_length;
		}
		else if (auto status_stage_trb = TRBDynamicCast<StatusStageTRB>(issuer_trb)) {
	   // pass
		}
		else {
			return MAKE_ERROR(Error::kNotImplemented);
		}

		const bool is_clear_tt_buffer =
			setup_data.request_type.bits.direction == request_type::kOut &&
			setup_data.request_type.bits.type == request_type::kClass &&
			setup_data.request_type.bits.recipient == request_type::kOther &&
			setup_data.request == static_cast<uint8>(HubRequest::ClearTTBuffer) &&
			setup_data.length == 0;
		if (is_clear_tt_buffer && host_ != nullptr) {
			for (stduint slot_id = 1;
				slot_id <= host_->GetDeviceManager()->MaxSlots(); ++slot_id) {
				auto* child = host_->GetDeviceManager()->FindBySlot(
					static_cast<uint8>(slot_id));
				if (child == nullptr || child == this) continue;
				const auto* child_slot_ctx = child->GetSlotContext();
				if (child_slot_ctx->bits.tt_hub_slot_id != slot_id_) continue;
				for (size_t index = 0;
					index < child->pending_bulk_transfers_.size(); ++index) {
					auto& pending = child->pending_bulk_transfers_[index];
					if (!pending.active ||
						pending.recovery_phase != BulkRecoveryPhase::ClearTTBuffer) {
						continue;
					}
					const EndpointID recovery_ep{ static_cast<int>(index + 1) };
					const uint16 expected_value =
						static_cast<uint16>(recovery_ep.Number()) |
						(static_cast<uint16>(child_slot_ctx->bits.usb_device_address) << 4) |
						(static_cast<uint16>(EndpointType::kBulk) << 11) |
						(recovery_ep.IsIn() ? 0x8000u : 0u);
					const uint16 expected_tt_port = GetSlotContext()->bits.mtt
						? child_slot_ctx->bits.tt_port_num : 1;
					if (setup_data.value != expected_value ||
						setup_data.index != expected_tt_port) {
						continue;
					}
					return child->OnTTBufferClearCompleted(recovery_ep,
						trb.bits.completion_code);
				}
			}
			return MAKE_ERROR(Error::kSuccess);
		}

		for (size_t index = 0; index < pending_bulk_transfers_.size(); ++index) {
			auto& pending = pending_bulk_transfers_[index];
			if (!pending.active ||
				pending.recovery_phase != BulkRecoveryPhase::ClearEndpointHalt) {
				continue;
			}
			const EndpointID recovery_ep{ static_cast<int>(index + 1) };
			const uint16 descriptor_address = static_cast<uint16>(
				recovery_ep.Number() | (recovery_ep.IsIn() ? 0x80 : 0x00));
			const bool is_clear_halt =
				setup_data.request_type.bits.direction == request_type::kOut &&
				setup_data.request_type.bits.type == request_type::kStandard &&
				setup_data.request_type.bits.recipient == request_type::kEndpoint &&
				setup_data.request == static_cast<uint8>(StandardRequest::ClearFeature) && setup_data.value == 0 &&
				setup_data.index == descriptor_address && setup_data.length == 0;
			if (!is_clear_halt) continue;

			const Error callback_error = this->OnControlCompleted(
				trb.GetEndpointID(), setup_data, data_stage_buffer,
				transfer_succeeded ? transfer_length : -1);
			if (!transfer_succeeded || callback_error) {
				pending = PendingBulkTransfer{};
				return transfer_succeeded
					? callback_error : MAKE_ERROR(Error::kTransferFailed);
			}

			return BeginTTBufferClear(recovery_ep);
		}

		return this->OnControlCompleted(
			trb.GetEndpointID(), setup_data, data_stage_buffer,
			transfer_succeeded ? transfer_length : -1);
	}
}

// ---- ---- ---- ---- devmgr.cpp ---- ---- ---- ---- //

namespace uni::device::SpaceUSB3 {
	Error DeviceManager::Initialize(size_t max_slots) {
		max_slots_ = max_slots;

		devices_ = AllocArray<USBHostDevice_v3*>(max_slots_ + 1, 0, 0);
		if (devices_ == nullptr) {
			return MAKE_ERROR(Error::kNoEnoughMemory);
		}

		device_context_pointers_ = AllocArray<void*>(max_slots_ + 1, 64, 4096);
		if (device_context_pointers_ == nullptr) {
			uni_hostenv_allocator->deallocate(devices_);
			return MAKE_ERROR(Error::kNoEnoughMemory);
		}

		for (size_t i = 0; i <= max_slots_; ++i) {
			devices_[i] = nullptr;
			device_context_pointers_[i] = nullptr;
		}

		return MAKE_ERROR(Error::kSuccess);
	}

	void** DeviceManager::DeviceContexts() const {
		return device_context_pointers_;
	}

	USBHostDevice_v3* DeviceManager::FindByPort(uint8 port_num, uint32_t route_string) const {
		for (size_t i = 1; i <= max_slots_; ++i) {
			auto dev = devices_[i];
			if (dev == nullptr) continue;
			if (MatchesPortAndRoute(*dev, port_num, route_string)) {
				return dev;
			}
		}
		return nullptr;
	}

	USBHostDevice_v3* DeviceManager::FindByState(enum USBHostDevice_v3::State state) const {
		for (size_t i = 1; i <= max_slots_; ++i) {
			auto dev = devices_[i];
			if (dev == nullptr) continue;
			if (dev->State() == state) {
				return dev;
			}
		}
		return nullptr;
	}

	USBHostDevice_v3* DeviceManager::FindBySlot(uint8 slot_id) const {
		if (slot_id > max_slots_) {
			return nullptr;
		}
		return devices_[slot_id];
	}

	/*
	WithError<Device*> DeviceManager::Get(uint8 device_id) const {
	  if (device_id >= num_devices_) {
		return {nullptr, Error::kInvalidDeviceId};
	  }
	  return {&devices_[device_id], Error::kSuccess};
	}
	*/

	Error DeviceManager::AllocDevice(uint8 slot_id, DoorbellRegister* dbreg, HostController* host) {
		if (slot_id > max_slots_) {
			return MAKE_ERROR(Error::kInvalidSlotID);
		}

		if (devices_[slot_id] != nullptr) {
			return MAKE_ERROR(Error::kAlreadyAllocated);
		}

		devices_[slot_id] = AllocArray<USBHostDevice_v3>(1, 64, 0);
		if (!devices_[slot_id]) return MAKE_ERROR(Error::kNoEnoughMemory);
		new(devices_[slot_id]) USBHostDevice_v3(slot_id, dbreg, host, host->ContextSize());
		if (auto err = devices_[slot_id]->Initialize()) {
			devices_[slot_id]->~USBHostDevice_v3();
			uni_hostenv_allocator->deallocate(devices_[slot_id]);
			devices_[slot_id] = nullptr;
			return err;
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error DeviceManager::LoadDCBAA(uint8 slot_id) {
		if (slot_id > max_slots_) {
			return MAKE_ERROR(Error::kInvalidSlotID);
		}

		auto dev = devices_[slot_id];
		device_context_pointers_[slot_id] = dev->DeviceContextBuffer();
		return MAKE_ERROR(Error::kSuccess);
	}

	Error DeviceManager::Remove(uint8 slot_id) {
		if (slot_id == 0 || slot_id > max_slots_ || devices_[slot_id] == nullptr) {
			return MAKE_ERROR(Error::kInvalidSlotID);
		}
		device_context_pointers_[slot_id] = nullptr;
		devices_[slot_id]->~USBHostDevice_v3();
		uni_hostenv_allocator->deallocate(devices_[slot_id]);
		devices_[slot_id] = nullptr;
		return MAKE_ERROR(Error::kSuccess);
	}

	void DeviceManager::Reset() {
		if (devices_ == nullptr || device_context_pointers_ == nullptr) return;
		for (size_t slot_id = 1; slot_id <= max_slots_; ++slot_id) {
			if (devices_[slot_id] == nullptr) continue;
			device_context_pointers_[slot_id] = nullptr;
			devices_[slot_id]->~USBHostDevice_v3();
			uni_hostenv_allocator->deallocate(devices_[slot_id]);
			devices_[slot_id] = nullptr;
		}
	}

	bool HostController::IsSlotRemovalPending(uint8 slot_id) const {
		return slot_id != 0 && slot_id <= max_slots_ && slot_removal_pending_.bitof(slot_id);
	}

	Error HostController::QueueSlotRemoval(uint8 slot_id, bool notify_disconnected) {
		if (slot_id == 0 || slot_id > max_slots_) {
			return MAKE_ERROR(Error::kInvalidSlotID);
		}
		auto* dev = devmgr_.FindBySlot(slot_id);
		if (notify_disconnected && dev != nullptr && !slot_disconnect_notified_[slot_id]) {
			if (g_host_device_disconnected_hook) {
				const auto controller = ControllerIdentity(*this);
				const auto location = DeviceLocation(*this, *dev);
				g_host_device_disconnected_hook(controller, location, *dev);
			}
			slot_disconnect_notified_.setof(slot_id);
		}
		if (slot_removal_pending_[slot_id]) {
			return MAKE_ERROR(Error::kSuccess);
		}

		slot_removal_pending_.setof(slot_id);
		DisableSlotCommandTRB disable{ slot_id };
		cr_.Push(disable);
		DoorbellRegisterAt(0)->Ring(0);
		return MAKE_ERROR(Error::kSuccess);
	}

	Error HostController::OnDisableSlotCompleted(uint8 slot_id, int completion_code) {
		if (slot_id == 0 || slot_id > max_slots_ || !slot_removal_pending_.bitof(slot_id)) {
			return MAKE_ERROR(Error::kInvalidSlotID);
		}
		// Slot Not Enabled means the controller has already reached the requested state.
		if (completion_code != 1 && completion_code != 11) {
			plogerro("xHCI Disable Slot failed: slot=%u completion=%d",
				slot_id, completion_code);
			return MAKE_ERROR(Error::kTransferFailed);
		}
		if (devmgr_.FindBySlot(slot_id) != nullptr) {
			if (auto err = devmgr_.Remove(slot_id)) return err;
		}
		slot_removal_pending_.setof(slot_id, false);
		slot_disconnect_notified_.setof(slot_id, false);
		return MAKE_ERROR(Error::kSuccess);
	}
}

// ---- ---- ---- ---- port.cpp ---- ---- ---- ---- //

namespace uni::device::SpaceUSB3 {
	uint8 Port::Number() const {
		return port_num_;
	}

	bool Port::IsConnected() const {
		return port_reg_set_.PORTSC.Read().bits.current_connect_status;
	}

	bool Port::IsEnabled() const {
		return port_reg_set_.PORTSC.Read().bits.port_enabled_disabled;
	}

	bool Port::IsConnectStatusChanged() const {
		return port_reg_set_.PORTSC.Read().bits.connect_status_change;
	}

	bool Port::IsPortResetChanged() const {
		return port_reg_set_.PORTSC.Read().bits.port_reset_change;
	}

	int Port::Speed() const {
		return port_reg_set_.PORTSC.Read().bits.port_speed;
	}

	Error Port::Reset() {
		auto portsc = port_reg_set_.PORTSC.Read();
		portsc.data[0] &= 0x0e00c3e0u;
		portsc.data[0] |= 0x00020010u; // Write 1 to PR and CSC
		port_reg_set_.PORTSC.Write(portsc);
		while (port_reg_set_.PORTSC.Read().bits.port_reset);
		return MAKE_ERROR(Error::kSuccess);
	}

	USBHostDevice_v3* Port::Initialize() {
		return nullptr;
	}
}

// ---- ---- ---- ---- registers.cpp ---- ---- ---- ---- //

namespace {
	template <class Ptr, class Disp>
	Ptr AddOrNull(Ptr p, Disp d) {
		return d == 0 ? nullptr : p + d;
	}
}

namespace uni::device::SpaceUSB3 {
	ExtendedRegisterList::Iterator& ExtendedRegisterList::Iterator::operator++() {
		if (reg_) {
			reg_ = AddOrNull(reg_, reg_->Read().bits.next_pointer);
			static_assert(sizeof(*reg_) == 4);
		}
		return *this;
	}

	ExtendedRegisterList::ExtendedRegisterList(uint64_t mmio_base,
		HCCPARAMS1_t hccp)
		: first_{ AddOrNull(reinterpret_cast<ValueType*>(mmio_base), hccp.bits.xhci_extended_capabilities_pointer) } {
	}
}

// ---- ---- ---- ---- trb.cpp ---- ---- ---- ---- //

namespace uni::device::SpaceUSB3 {
	const uni::Array<const char*, 37> kTRBCompletionCodeToName{
		"Invalid",
		"Success",
		"Data Buffer Error",
		"Babble Detected Error",
		"USB Transaction Error",
		"TRB Error",
		"Stall Error",
		"Resource Error",
		"Bandwidth Error",
		"No Slots Available Error",
		"Invalid Stream Type Error",
		"Slot Not Enabled Error",
		"Endpoint Not Enabled Error",
		"Short Packet",
		"Ring Underrun",
		"Ring Overrun",
		"VF Event Ring Full Error",
		"Parameter Error",
		"Bandwidth Overrun Error",
		"Context State Error",
		"No ping Response Error",
		"Event Ring Full Error",
		"Incompatible Device Error",
		"Missed Service Error",
		"Command Ring Stopped",
		"Command Aborted",
		"Stopped",
		"Stopped - Length Invalid",
		"Stopped - Short Packet",
		"Max Exit Latency Too Large Error",
		"Reserved",
		"Isoch Buffer Overrun",
		"Event Lost Error",
		"Undefined Error",
		"Invalid Stream ID Error",
		"Secondary Bandwidth Error",
		"Split Transaction Error",
	};

	const uni::Array<const char*, 64> kTRBTypeToName{
		"Reserved",                             // 0
		"Normal",
		"Setup Stage",
		"Data Stage",
		"Status Stage",
		"Isoch",
		"Link",
		"EventData",
		"No-Op",                                // 8
		"Enable Slot Command",
		"Disable Slot Command",
		"Address Device Command",
		"Configure Endpoint Command",
		"Evaluate Context Command",
		"Reset Endpoint Command",
		"Stop Endpoint Command",
		"Set TR Dequeue Pointer Command",       // 16
		"Reset Device Command",
		"Force Event Command",
		"Negotiate Bandwidth Command",
		"Set Latency Tolerance Value Command",
		"Get Port Bandwidth Command",
		"Force Header Command",
		"No Op Command",
		"Reserved",                             // 24
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",
		"Transfer Event",                       // 32
		"Command Completion Event",
		"Port Status Change Event",
		"Bandwidth Request Event",
		"Doorbell Event",
		"Host Controller Event",
		"Device Notification Event",
		"MFINDEX Wrap Event",
		"Reserved",                             // 40
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",
		"Reserved",
		"Vendor Defined",                       // 48
		"Vendor Defined",
		"Vendor Defined",
		"Vendor Defined",
		"Vendor Defined",
		"Vendor Defined",
		"Vendor Defined",
		"Vendor Defined",
		"Vendor Defined",                       // 56
		"Vendor Defined",
		"Vendor Defined",
		"Vendor Defined",
		"Vendor Defined",
		"Vendor Defined",
		"Vendor Defined",
		"Vendor Defined",
	};
}

// ---- ---- ---- ---- xHCI.cpp ---- ---- ---- ---- //

namespace {



	/* Between resetting a root hub port and assigning an address,
		 * no other processing must be interleaved; only that port's processing is allowed.
		 * WaitingAddressed is the state waiting for the sequence from reset
		 * (ResettingPort) to address assignment (AddressingDevice) to complete.
		 */

	void InitializeSlotContext(SlotContext& ctx, uint8 root_hub_port_num, uint32 route_string,
		uint8 speed, const USBHostDevice_v3* parent_hub, uint8 downstream_port) {
		ctx.bits.route_string = route_string;
		ctx.bits.root_hub_port_num = root_hub_port_num;
		ctx.bits.context_entries = 1;
		ctx.bits.speed = speed;
		ctx.bits.tt_hub_slot_id = 0;
		ctx.bits.tt_port_num = 0;
		uint8 parent_speed_class = 0;
		if (parent_hub && parent_hub->Controller()) {
			const uint8 parent_speed_id = parent_hub->GetSlotContext()->bits.speed;
			parent_speed_class = parent_hub->Controller()->SpeedClass(root_hub_port_num, parent_speed_id);
		}
		if (parent_speed_class == kHighSpeed && speed < kHighSpeed) {
			ctx.bits.tt_hub_slot_id = parent_hub->SlotID();
			ctx.bits.tt_port_num = downstream_port;
		}
	}

	void InitializeSlotContext(SlotContext& ctx, Port& port) {
		InitializeSlotContext(ctx, port.Number(), 0, port.Speed(), nullptr, 0);
	}

	Error TryAddressNextHubChild(HostController& xhc) {
		auto& state = xhc.EnumerationState();
		if (state.enable_slot_command_pending || state.active_hub_child_valid ||
			state.addressing_port != 0) {
			return MAKE_ERROR(Error::kSuccess);
		}
		PendingHubChildAddress ctx{};
		if (!PopHubChildAddress(xhc, ctx)) {
			return MAKE_ERROR(Error::kSuccess);
		}
		state.active_hub_child = ctx;
		state.active_hub_child_valid = true;
		EnableSlotCommandTRB cmd{};
		xhc.CommandRing()->Push(cmd);
		state.enable_slot_command_pending = true;
		xhc.DoorbellRegisterAt(0)->Ring(0);
		return MAKE_ERROR(Error::kSuccess);
	}

	unsigned int DetermineMaxPacketSizeForControlPipe(unsigned int speed_class) {
		switch (speed_class) {
		case 5: // Super Speed Plus
		case 4: // Super Speed
			return 512;
		case 3: // High Speed
			return 64;
		default:
			return 8;
		}
	}

	int MostSignificantBit(uint32_t value) {
		if (value == 0) {
			return -1;
		}

		int msb_index;
		_ASM("bsr %1, %0"
			: "=r"(msb_index) : "m"(value));
		return msb_index;
	}

	void InitializeEP0Context(EndpointContext& ctx,
		Ring* transfer_ring,
		unsigned int max_packet_size) {
		ctx.bits.ep_type = 4; // Control Endpoint. Bidirectional.
		ctx.bits.max_packet_size = max_packet_size;
		ctx.bits.max_burst_size = 0;
		ctx.SetTransferRingBuffer(transfer_ring->Buffer());
		ctx.bits.dequeue_cycle_state = 1;
		ctx.bits.interval = 0;
		ctx.bits.max_primary_streams = 0;
		ctx.bits.mult = 0;
		ctx.bits.error_count = 3;
	}

	Error ResetPort(HostController& xhc, Port& port) {
		auto& state = xhc.EnumerationState();
		const bool is_connected = port.IsConnected();
		Log(kDebug, "ResetPort: port.IsConnected() = %s\n",
			is_connected ? "true" : "false");

		if (!is_connected) {
			return MAKE_ERROR(Error::kSuccess);
		}

		if (state.enable_slot_command_pending || state.addressing_port != 0 ||
			state.active_hub_child_valid) {
			state.port_config_phase[port.Number()] = ConfigPhase::WaitingAddressed;
		}
		else {
			const auto port_phase = state.port_config_phase[port.Number()];
			if (port_phase != ConfigPhase::NotConnected &&
				port_phase != ConfigPhase::WaitingAddressed) {
				return MAKE_ERROR(Error::kInvalidPhase);
			}
			state.addressing_port = port.Number();
			state.port_config_phase[port.Number()] = ConfigPhase::ResettingPort;
			port.Reset();
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error ResumePendingEnumeration(HostController& xhc) {
		auto& state = xhc.EnumerationState();
		if (state.enable_slot_command_pending || state.addressing_port != 0 ||
			state.active_hub_child_valid) {
			return MAKE_ERROR(Error::kSuccess);
		}
		for (size_t i = 1; i < state.port_config_phase.size(); ++i) {
			if (state.port_config_phase[i] != ConfigPhase::WaitingAddressed) continue;
			auto port = xhc.PortAt(static_cast<uint8>(i));
			return ResetPort(xhc, port);
		}
		return TryAddressNextHubChild(xhc);
	}

	void ReleaseEnumerationOwner(HostController& xhc, USBHostDevice_v3* dev) {
		if (dev == nullptr) return;
		auto& state = xhc.EnumerationState();
		if (dev->RouteString() == 0) {
			const uint8 port_id = dev->RootHubPortNum();
			state.port_config_phase[port_id] = ConfigPhase::NotConnected;
			if (state.addressing_port == port_id) state.addressing_port = 0;
		}
		else if (state.active_hub_child_valid &&
			state.active_hub_child.root_hub_port_num == dev->RootHubPortNum() &&
			state.active_hub_child.route_string == dev->RouteString()) {
			state.active_hub_child_valid = false;
		}
	}

	void ReleaseActiveEnableSlotOwner(HostController& xhc) {
		auto& state = xhc.EnumerationState();
		if (state.active_hub_child_valid) {
			state.active_hub_child_valid = false;
			return;
		}
		if (state.addressing_port != 0) {
			state.port_config_phase[state.addressing_port] = ConfigPhase::NotConnected;
			state.addressing_port = 0;
		}
	}

	Error RollbackEnabledSlot(HostController& xhc, uint8 slot_id) {
		ReleaseActiveEnableSlotOwner(xhc);
		if (slot_id != 0) {
			if (auto err = xhc.QueueSlotRemoval(slot_id, false)) return err;
		}
		if (auto err = ResumePendingEnumeration(xhc)) return err;
		return MAKE_ERROR(Error::kTransferFailed);
	}

	Error RollbackEnumeration(HostController& xhc, USBHostDevice_v3& dev) {
		const uint8 slot_id = dev.SlotID();
		ReleaseEnumerationOwner(xhc, &dev);
		if (auto err = xhc.QueueSlotRemoval(slot_id, false)) return err;
		if (auto err = ResumePendingEnumeration(xhc)) return err;
		return MAKE_ERROR(Error::kTransferFailed);
	}

	Error EnableSlot(HostController& xhc, Port& port) {
		auto& state = xhc.EnumerationState();
		const bool is_enabled = port.IsEnabled();
		const bool reset_completed = port.IsPortResetChanged();
		Log(kDebug, "EnableSlot: port.IsEnabled() = %s, port.IsPortResetChanged() = %s\n",
			is_enabled ? "true" : "false",
			reset_completed ? "true" : "false");

		if (is_enabled && reset_completed) {
			port.ClearPortResetChange();

			state.port_config_phase[port.Number()] = ConfigPhase::EnablingSlot;

			EnableSlotCommandTRB cmd{};
			xhc.CommandRing()->Push(cmd);
			state.enable_slot_command_pending = true;
			xhc.DoorbellRegisterAt(0)->Ring(0);
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error AddressDevice(HostController& xhc, uint8 port_id, uint8 slot_id) {
		auto& state = xhc.EnumerationState();
		Log(kDebug, "AddressDevice: port_id = %d, slot_id = %d\n", port_id, slot_id);
		auto port = xhc.PortAt(port_id);
		const uint8 speed_id = port.Speed();
		const uint8 speed_class = xhc.SpeedClass(port.Number(), speed_id);
		if (speed_class == 0) {
			return MAKE_ERROR(Error::kUnknownXHCISpeedID);
		}

		if (auto err = xhc.GetDeviceManager()->AllocDevice(
			slot_id, xhc.DoorbellRegisterAt(slot_id), &xhc)) return err;

		USBHostDevice_v3* dev = xhc.GetDeviceManager()->FindBySlot(slot_id);
		if (dev == nullptr) {
			return MAKE_ERROR(Error::kInvalidSlotID);
		}

		MemSet(dev->GetInputControlContext(), 0, dev->ContextSize());

		const auto ep0_dci = DeviceContextIndex(0, false);
		auto slot_ctx = dev->EnableInputSlotContext();
		auto ep0_ctx = dev->EnableInputEndpoint(ep0_dci);

		InitializeSlotContext(*slot_ctx, port);
		dev->SetParentHubInfo(0, 0);

		auto* ep0_ring = dev->AllocTransferRing(ep0_dci, xhc.ControlTransferRingSize());
		if (ep0_ring == nullptr) return MAKE_ERROR(Error::kNoEnoughMemory);
		InitializeEP0Context(*ep0_ctx, ep0_ring,
			DetermineMaxPacketSizeForControlPipe(speed_class));

		xhc.GetDeviceManager()->LoadDCBAA(slot_id);

		state.port_config_phase[port_id] = ConfigPhase::AddressingDevice;

		AddressDeviceCommandTRB addr_dev_cmd{ dev->InputContextBuffer(), slot_id };
		xhc.CommandRing()->Push(addr_dev_cmd);
		xhc.DoorbellRegisterAt(0)->Ring(0);

		return MAKE_ERROR(Error::kSuccess);
	}

	Error AddressDevice(HostController& xhc, const PendingHubChildAddress& child_ctx, uint8 slot_id) {
		if (auto err = xhc.GetDeviceManager()->AllocDevice(
			slot_id, xhc.DoorbellRegisterAt(slot_id), &xhc)) return err;

		USBHostDevice_v3* dev = xhc.GetDeviceManager()->FindBySlot(slot_id);
		if (dev == nullptr) {
			return MAKE_ERROR(Error::kInvalidSlotID);
		}

		auto* parent_hub = xhc.GetDeviceManager()->FindBySlot(child_ctx.hub_slot_id);
		if (parent_hub == nullptr) {
			return MAKE_ERROR(Error::kInvalidSlotID);
		}

		MemSet(dev->GetInputControlContext(), 0, dev->ContextSize());

		const auto ep0_dci = DeviceContextIndex(0, false);
		auto slot_ctx = dev->EnableInputSlotContext();
		auto ep0_ctx = dev->EnableInputEndpoint(ep0_dci);
		InitializeSlotContext(*slot_ctx, child_ctx.root_hub_port_num, child_ctx.route_string,
			child_ctx.speed, parent_hub, child_ctx.downstream_port);
		dev->SetParentHubInfo(child_ctx.hub_slot_id, child_ctx.downstream_port);

		auto* ep0_ring = dev->AllocTransferRing(ep0_dci, xhc.ControlTransferRingSize());
		if (ep0_ring == nullptr) return MAKE_ERROR(Error::kNoEnoughMemory);
		const uint8 speed_class = xhc.SpeedClass(
			child_ctx.root_hub_port_num, child_ctx.speed);
		if (speed_class == 0) return MAKE_ERROR(Error::kUnknownXHCISpeedID);
		InitializeEP0Context(*ep0_ctx, ep0_ring,
			DetermineMaxPacketSizeForControlPipe(speed_class));

		xhc.GetDeviceManager()->LoadDCBAA(slot_id);

		AddressDeviceCommandTRB addr_dev_cmd{ dev->InputContextBuffer(), slot_id };
		xhc.CommandRing()->Push(addr_dev_cmd);
		xhc.DoorbellRegisterAt(0)->Ring(0);
		return MAKE_ERROR(Error::kSuccess);
	}

	Error InitializeDevice(HostController& xhc, uint8 port_id, uint8 slot_id) {
		// Log(kDebug, "InitializeDevice: port_id = %d, slot_id = %d", port_id, slot_id);

		auto dev = xhc.GetDeviceManager()->FindBySlot(slot_id);
		if (dev == nullptr) {
			return MAKE_ERROR(Error::kInvalidSlotID);
		}

		if (dev->RouteString() == 0) {
			xhc.EnumerationState().port_config_phase[port_id] = ConfigPhase::InitializingDevice;
		}
		return dev->StartInitialize();
	}

	Error CompleteConfiguration(HostController& xhc, uint8 port_id, uint8 slot_id) {
		// Log(kDebug, "CompleteConfiguration: port_id = %d, slot_id = %d\n", port_id, slot_id);

		auto dev = xhc.GetDeviceManager()->FindBySlot(slot_id);
		if (dev == nullptr) {
			return MAKE_ERROR(Error::kInvalidSlotID);
		}

		if (auto err = dev->OnEndpointsConfigured()) return err;
		if (g_host_device_configured_hook) {
			const auto controller = ControllerIdentity(xhc);
			const auto location = DeviceLocation(xhc, *dev);
			g_host_device_configured_hook(controller, location, *dev);
		}

		if (dev->RouteString() == 0) {
			xhc.EnumerationState().port_config_phase[port_id] = ConfigPhase::Configured;
		}
		auto& state = xhc.EnumerationState();
		if (state.active_hub_child_valid &&
			state.active_hub_child.root_hub_port_num == dev->RootHubPortNum() &&
			state.active_hub_child.route_string == dev->RouteString()) {
			state.active_hub_child_valid = false;
			return TryAddressNextHubChild(xhc);
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error OnEvent(HostController& xhc, PortStatusChangeEventTRB& trb) {
		// Log(kDebug, "PortStatusChangeEvent: port_id = %d", trb.bits.port_id);
		auto port_id = trb.bits.port_id;
		auto port = xhc.PortAt(port_id);

		auto& state = xhc.EnumerationState();
		switch (state.port_config_phase[port_id]) {
		case ConfigPhase::NotConnected:
			return ResetPort(xhc, port);
		case ConfigPhase::ResettingPort:
			return EnableSlot(xhc, port);
		case ConfigPhase::WaitingAddressed:
		case ConfigPhase::EnablingSlot:
			// Before the Enable Slot command is completed, the controller/virtual machine may continue to report port status changes.
			if (!port.IsConnected()) {
				// restore state
				state.port_config_phase[port_id] = ConfigPhase::NotConnected;
				if (state.addressing_port == port_id) state.addressing_port = 0;
				ploginfo("Port %u disconnected while enabling slot, reset state", port_id);
			} else {
				// ploginfo("Port %u: PSC arrived during EnablingSlot -- ignoring", port_id);// ignore
			}
			return MAKE_ERROR(Error::kSuccess);
		case ConfigPhase::AddressingDevice:
		case ConfigPhase::InitializingDevice:
		case ConfigPhase::ConfiguringEndpoints:
		case ConfigPhase::Configured:
			if (!port.IsConnected()) {
				state.port_config_phase[port_id] = ConfigPhase::NotConnected;
				if (state.addressing_port == port_id) state.addressing_port = 0;
				RemoveDeviceSubtree(xhc, port_id, 0, true);
				ploginfo("Port %u disconnected, subtree disable queued", port_id);
				return ResumePendingEnumeration(xhc);
			}
			return MAKE_ERROR(Error::kSuccess);
		default:
			plogerro("OnEvent kInvalidPhase %u", state.port_config_phase[port_id]);
			return MAKE_ERROR(Error::kInvalidPhase);
		}
	}

	Error OnEvent(HostController& xhc, TransferEventTRB& trb) {
		const uint8 slot_id = trb.bits.slot_id;
		auto dev = xhc.GetDeviceManager()->FindBySlot(slot_id);
		if (dev == nullptr) {
			return MAKE_ERROR(Error::kInvalidSlotID);
		}
		if (xhc.IsSlotRemovalPending(slot_id)) {
			return MAKE_ERROR(Error::kSuccess);
		}
		if (auto err = dev->OnTransferEventReceived(trb)) {
			if (!dev->IsInitialized()) {
				plogerro("xHCI enumeration transfer failed: slot=%u error=%s",
					slot_id, err.Name());
				return RollbackEnumeration(xhc, *dev);
			}
			return err;
		}

		const auto port_id = dev->GetSlotContext()->bits.root_hub_port_num;
		auto& state = xhc.EnumerationState();
		if (dev->IsInitialized() &&
			((dev->RouteString() == 0 && state.port_config_phase[port_id] == ConfigPhase::InitializingDevice) ||
			 (state.active_hub_child_valid &&
			  state.active_hub_child.root_hub_port_num == dev->RootHubPortNum() &&
			  state.active_hub_child.route_string == dev->RouteString()))) {
			if (auto err = xhc.ConfigureEndpoints(*dev)) {
				plogerro("xHCI endpoint configuration setup failed: slot=%u error=%s",
					slot_id, err.Name());
				return RollbackEnumeration(xhc, *dev);
			}
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error OnEvent(HostController& xhc, BandwidthRequestEventTRB& trb) {
		if (trb.bits.completion_code != 1) {
			return MAKE_ERROR(Error::kTransferFailed);
		}
		if (trb.bits.slot_id == 0 ||
			xhc.GetDeviceManager()->FindBySlot(trb.bits.slot_id) == nullptr) {
			return MAKE_ERROR(Error::kInvalidSlotID);
		}
		auto* dev = xhc.GetDeviceManager()->FindBySlot(trb.bits.slot_id);
		if (!xhc.HasBandwidthNegotiationCapability()) {
			plogwarn("xHCI unsolicited bandwidth request without BNC: slot=%u",
				trb.bits.slot_id);
		}
		// The owner must release bandwidth through a lower-bandwidth alternate
		// setting; issuing Negotiate Bandwidth here may generate this event again.
		if (g_host_bandwidth_request_hook) {
			const auto controller = ControllerIdentity(xhc);
			const auto location = DeviceLocation(xhc, *dev);
			g_host_bandwidth_request_hook(controller, location, *dev);
		}
		else {
			plogwarn("xHCI bandwidth request has no policy handler: slot=%u",
				trb.bits.slot_id);
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error OnEvent(HostController& xhc, DoorbellEventTRB& trb) {
		(void)xhc;
		plogwarn("xHCI Doorbell Event: reason=%u vf=%u slot=%u completion=%u",
			trb.bits.doorbell_reason, trb.bits.vf_id, trb.bits.slot_id,
			trb.bits.completion_code);
		return MAKE_ERROR(trb.bits.completion_code == 1
			? Error::kSuccess : Error::kTransferFailed);
	}

	Error OnEvent(HostController& xhc, HostControllerEventTRB& trb) {
		return xhc.OnHostControllerEvent(trb.bits.completion_code);
	}

	Error OnEvent(HostController& xhc, DeviceNotificationEventTRB& trb) {
		if (trb.bits.completion_code != 1 || trb.bits.slot_id == 0) {
			return MAKE_ERROR(Error::kTransferFailed);
		}
		auto* dev = xhc.GetDeviceManager()->FindBySlot(trb.bits.slot_id);
		if (dev == nullptr) return MAKE_ERROR(Error::kInvalidSlotID);
		// DNCTRL enables only Function Wake (N1); reject unexpected firmware events.
		if (trb.bits.notification_type != 1) {
			plogwarn("xHCI unsupported device notification: type=%u slot=%u",
				trb.bits.notification_type, trb.bits.slot_id);
			return MAKE_ERROR(Error::kSuccess);
		}
		if (g_host_device_notification_hook) {
			const auto controller = ControllerIdentity(xhc);
			const auto location = DeviceLocation(xhc, *dev);
			g_host_device_notification_hook(controller, location, *dev,
				trb.bits.notification_type, trb.NotificationData());
		}
		else {
			plogwarn("xHCI Function Wake has no notification handler: slot=%u",
				trb.bits.slot_id);
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error OnEvent(HostController& xhc, MFINDEXWrapEventTRB& trb) {
		if (trb.bits.completion_code != 1) {
			return MAKE_ERROR(Error::kTransferFailed);
		}
		xhc.OnMFINDEXWrapEvent();
		return MAKE_ERROR(Error::kSuccess);
	}

	Error OnEvent(HostController& xhc, CommandCompletionEventTRB& trb) {
		auto& state = xhc.EnumerationState();
		if (trb.Pointer() == nullptr) return MAKE_ERROR(Error::kInvalidPhase);
		const auto issuer_type = trb.Pointer()->bits.trb_type;
		const auto slot_id = trb.bits.slot_id;
		// Log(kDebug, "CommandCompletionEvent: slot_id = %d, issuer = %s",
		// 	trb.bits.slot_id, kTRBTypeToName[issuer_type]);

		if (issuer_type == DisableSlotCommandTRB::Type) {
			return xhc.OnDisableSlotCompleted(slot_id, trb.bits.completion_code);
		}
		if (slot_id != 0 && xhc.IsSlotRemovalPending(slot_id)) {
			return MAKE_ERROR(Error::kSuccess);
		}

		if (issuer_type == EnableSlotCommandTRB::Type) {
			state.enable_slot_command_pending = false;
			if (trb.bits.completion_code != 1) {
				plogerro("xHCI Enable Slot failed: completion=%u",
					trb.bits.completion_code);
				ReleaseActiveEnableSlotOwner(xhc);
				if (auto err = ResumePendingEnumeration(xhc)) return err;
				return MAKE_ERROR(Error::kTransferFailed);
			}
			Error err = MAKE_ERROR(Error::kInvalidPhase);
			if (state.active_hub_child_valid) {
				err = AddressDevice(xhc, state.active_hub_child, slot_id);
			}
			else if (state.addressing_port != 0 &&
				state.port_config_phase[state.addressing_port] == ConfigPhase::EnablingSlot) {
				err = AddressDevice(xhc, state.addressing_port, slot_id);
			}
			if (err) {
				plogerro("xHCI failed to prepare Address Device: slot=%u error=%s",
					slot_id, err.Name());
				return RollbackEnabledSlot(xhc, slot_id);
			}
			return MAKE_ERROR(Error::kSuccess);
		}
		else if (issuer_type == AddressDeviceCommandTRB::Type) {
			auto dev = xhc.GetDeviceManager()->FindBySlot(slot_id);
			if (dev == nullptr) {
				return MAKE_ERROR(Error::kInvalidSlotID);
			}
			if (trb.bits.completion_code != 1) {
				plogerro("xHCI Address Device failed: slot=%u completion=%u",
					slot_id, trb.bits.completion_code);
				return RollbackEnumeration(xhc, *dev);
			}

			auto port_id = dev->GetSlotContext()->bits.root_hub_port_num;
			if (dev->RouteString() == 0) {
				if (port_id != state.addressing_port) {
					return MAKE_ERROR(Error::kInvalidPhase);
				}
				if (state.port_config_phase[port_id] != ConfigPhase::AddressingDevice) {
					return MAKE_ERROR(Error::kInvalidPhase);
				}

				state.addressing_port = 0;
				if (auto err = ResumePendingEnumeration(xhc)) return err;
			}

			if (auto err = InitializeDevice(xhc, port_id, slot_id)) {
				plogerro("xHCI device initialization failed: slot=%u error=%s",
					slot_id, err.Name());
				return RollbackEnumeration(xhc, *dev);
			}
			return MAKE_ERROR(Error::kSuccess);
		}
		else if (issuer_type == ResetEndpointCommandTRB::Type) {
			auto dev = xhc.GetDeviceManager()->FindBySlot(slot_id);
			if (dev == nullptr) {
				return MAKE_ERROR(Error::kInvalidSlotID);
			}
			auto* command = TRBDynamicCast<ResetEndpointCommandTRB>(trb.Pointer());
			if (command == nullptr) {
				return MAKE_ERROR(Error::kInvalidPhase);
			}
			return dev->OnEndpointResetCompleted(
				command->GetEndpointID(), trb.bits.completion_code);
		}
		else if (issuer_type == SetTRDequeuePointerCommandTRB::Type) {
			auto dev = xhc.GetDeviceManager()->FindBySlot(slot_id);
			if (dev == nullptr) {
				return MAKE_ERROR(Error::kInvalidSlotID);
			}
			auto* command = TRBDynamicCast<SetTRDequeuePointerCommandTRB>(trb.Pointer());
			if (command == nullptr) {
				return MAKE_ERROR(Error::kInvalidPhase);
			}
			return dev->OnTransferRingDequeueSet(
				command->GetEndpointID(), trb.bits.completion_code);
		}
		else if (issuer_type == ConfigureEndpointCommandTRB::Type) {
			auto dev = xhc.GetDeviceManager()->FindBySlot(slot_id);
			if (dev == nullptr) {
				return MAKE_ERROR(Error::kInvalidSlotID);
			}
			const bool hub_context_update = dev->IsHubContextUpdatePending();
			if (hub_context_update) dev->CompleteHubContextUpdate();
			if (trb.bits.completion_code != 1) {
				plogerro("xHCI Configure Endpoint failed: slot=%u completion=%u",
					slot_id, trb.bits.completion_code);
				return RollbackEnumeration(xhc, *dev);
			}
			if (hub_context_update) return MAKE_ERROR(Error::kSuccess);

			auto port_id = dev->GetSlotContext()->bits.root_hub_port_num;
			if (dev->RouteString() == 0 &&
				state.port_config_phase[port_id] != ConfigPhase::ConfiguringEndpoints) {
				return MAKE_ERROR(Error::kInvalidPhase);
			}

			if (auto err = CompleteConfiguration(xhc, port_id, slot_id)) {
				plogerro("xHCI class endpoint startup failed: slot=%u error=%s",
					slot_id, err.Name());
				return RollbackEnumeration(xhc, *dev);
			}
			return MAKE_ERROR(Error::kSuccess);
		}

		if (trb.bits.completion_code != 1) {
			plogerro("xHCI command failed: type=%u slot=%u completion=%u",
				issuer_type, slot_id, trb.bits.completion_code);
			return MAKE_ERROR(Error::kTransferFailed);
		}
		return MAKE_ERROR(Error::kInvalidPhase);
	}


}

namespace uni::device::SpaceUSB3 {





	Error HostController::Run() {
		if (controller_failed_) return MAKE_ERROR(Error::kTransferFailed);
	  // Run the controller
		auto usbcmd = op_->USBCMD.Read();
		usbcmd.bits.run_stop = true;
		op_->USBCMD.Write(usbcmd);
		op_->USBCMD.Read();

		while (op_->USBSTS.Read().bits.host_controller_halted);

		return MAKE_ERROR(Error::kSuccess);
	}

	DoorbellRegister* HostController::DoorbellRegisterAt(uint8 index) {
		return &DoorbellRegisters()[index];
	}

	Error HostController::ProcessEvents() {
		if (controller_failed_) return MAKE_ERROR(Error::kTransferFailed);
		const auto status = op_->USBSTS.Read();
		if (status.bits.host_system_error || status.bits.host_controller_error) {
			plogwarn("xHCI fatal status: HSE=%u HCE=%u",
				status.bits.host_system_error, status.bits.host_controller_error);
			controller_event_completion_code_ = 0;
			controller_recovery_requested_ = true;
			return RecoverController();
		}
		Error first_error = MAKE_ERROR(Error::kSuccess);
		while (this->PrimaryEventRing()->HasFront()) {
			if (auto err = ProcessEvent()) {
				if (!first_error) first_error = err;
			}
		}
		return first_error;
	}






}


Error HostController::ConfigurePort(Port& port) {
	if (enumeration_state_.port_config_phase[port.Number()] == ConfigPhase::NotConnected) {
		return ResetPort(self, port);
	}
	return MAKE_ERROR(Error::kSuccess);
}

Error HostController::OnHubPortStatusChanged(USBHostDevice_v3& hub_dev, uint8 downstream_port,
	uint16 status, uint16 change, uint8 speed_id) {
	(void)change;
	const auto root_hub_port_num = hub_dev.RootHubPortNum();
	const auto route_string = AppendRouteString(hub_dev.RouteString(), downstream_port);
	auto* existing = GetDeviceManager()->FindByPort(root_hub_port_num, route_string);

	if ((status & 0x0001u) == 0) {
		if (existing) {
			RemoveDeviceSubtree(self, root_hub_port_num, route_string, false);
		} else {
			RemovePendingHubChildren(self, root_hub_port_num, route_string, false);
		}
		return ResumePendingEnumeration(self);
	}

	if (existing != nullptr) {
		return MAKE_ERROR(Error::kSuccess);
	}

	PendingHubChildAddress child_ctx{};
	child_ctx.root_hub_port_num = root_hub_port_num;
	child_ctx.route_string = route_string;
	child_ctx.hub_slot_id = hub_dev.SlotID();
	child_ctx.downstream_port = downstream_port;
	if (speed_id != 0 && SpeedClass(root_hub_port_num, speed_id) != 0) {
		child_ctx.speed = speed_id;
	}
	else {
		const uint8 speed_class = DetermineHubChildSpeedClass(hub_dev, status);
		child_ctx.speed = SpeedIDForClass(root_hub_port_num, speed_class);
	}
	if (child_ctx.speed == 0) {
		return MAKE_ERROR(Error::kUnknownXHCISpeedID);
	}
	if (!QueueHubChildAddress(self, child_ctx)) {
		return MAKE_ERROR(Error::kNoEnoughMemory);
	}
	return TryAddressNextHubChild(self);
}

Error HostController::ConfigureEndpoints(USBHostDevice_v3& dev) {
	auto& xhc = self;
	const auto configs = dev.EndpointConfigs();
	const auto len = dev.NumEndpointConfigs();

	MemSet(dev.GetInputControlContext(), 0, dev.ContextSize());
	MemCopyN(dev.GetInputSlotContext(), dev.GetSlotContext(), sizeof(SlotContext));

	auto slot_ctx = dev.EnableInputSlotContext();
	slot_ctx->bits.context_entries = 1;
	const auto port_id{ dev.GetSlotContext()->bits.root_hub_port_num };
	const int speed_id{ dev.GetSlotContext()->bits.speed };
	const int port_speed{ xhc.SpeedClass(port_id, speed_id) };
	if (port_speed == 0 || port_speed > kSuperSpeedPlus) {
		return MAKE_ERROR(Error::kUnknownXHCISpeedID);
	}

	auto convert_interval{
	  (port_speed == kFullSpeed || port_speed == kLowSpeed)
	  ? [](EndpointType type, int interval) { // for FS, LS
		if (type == EndpointType::kIsochronous) return interval + 2;
		else return MostSignificantBit(interval) + 3;
	  }
	  : [](EndpointType type, int interval) { // for HS, SS, SSP
		return interval - 1;
	  } };

	for (int i = 0; i < len; ++i) {
		const DeviceContextIndex ep_dci{ configs[i].ep_id };
		if (configs[i].max_packet_size <= 0 || configs[i].max_burst > 15 ||
			(configs[i].ep_type == EndpointType::kIsochronous && configs[i].mult > 2)) {
			return MAKE_ERROR(Error::kInvalidDescriptor);
		}
		if (ep_dci.value > static_cast<int>(slot_ctx->bits.context_entries)) {
			slot_ctx->bits.context_entries = ep_dci.value;
		}
		auto ep_ctx = dev.EnableInputEndpoint(ep_dci);
		switch (configs[i].ep_type) {
		case EndpointType::kControl:
			ep_ctx->bits.ep_type = 4;
			break;
		case EndpointType::kIsochronous:
			ep_ctx->bits.ep_type = configs[i].ep_id.IsIn() ? 5 : 1;
			break;
		case EndpointType::kBulk:
			ep_ctx->bits.ep_type = configs[i].ep_id.IsIn() ? 6 : 2;
			break;
		case EndpointType::kInterrupt:
			ep_ctx->bits.ep_type = configs[i].ep_id.IsIn() ? 7 : 3;
			break;
		}
		ep_ctx->bits.max_packet_size = configs[i].max_packet_size;
		ep_ctx->bits.interval = convert_interval(configs[i].ep_type, configs[i].interval);
		ep_ctx->bits.max_burst_size = configs[i].max_burst;
		ep_ctx->bits.mult = port_speed >= kSuperSpeed &&
			configs[i].ep_type == EndpointType::kIsochronous
			? configs[i].mult : 0;
		uint32 max_esit_payload = configs[i].bytes_per_interval;
		if (max_esit_payload == 0 &&
			(configs[i].ep_type == EndpointType::kIsochronous ||
			 configs[i].ep_type == EndpointType::kInterrupt)) {
			max_esit_payload = uint32(configs[i].max_packet_size) *
				(uint32(configs[i].max_burst) + 1u) *
				(uint32(ep_ctx->bits.mult) + 1u);
		}
		if (max_esit_payload > 0x00ffffffu ||
			(max_esit_payload > 0xffffu &&
			 !cap_->HCCPARAMS2.Read().bits.large_esit_payload_capability)) {
			return MAKE_ERROR(Error::kInvalidDescriptor);
		}
		ep_ctx->bits.max_esit_payload_lo = max_esit_payload & 0xffffu;
		ep_ctx->bits.max_esit_payload_hi = (max_esit_payload >> 16) & 0xffu;
		uint32 average_trb_length = max_esit_payload != 0
			? max_esit_payload
			: uint32(configs[i].max_packet_size) * (uint32(configs[i].max_burst) + 1u);
		if (average_trb_length == 0) average_trb_length = 1;
		if (average_trb_length > 0xffffu) average_trb_length = 0xffffu;
		ep_ctx->bits.average_trb_length = average_trb_length;

		const size_t transfer_ring_size = configs[i].ep_type == EndpointType::kBulk
			? xhc.BulkTransferRingSize() : xhc.TransferRingSize();
		auto tr = dev.AllocTransferRing(ep_dci, transfer_ring_size);
		if (tr == nullptr) return MAKE_ERROR(Error::kNoEnoughMemory);
		ep_ctx->SetTransferRingBuffer(tr->Buffer());

		ep_ctx->bits.dequeue_cycle_state = 1;
		ep_ctx->bits.max_primary_streams = 0;
		ep_ctx->bits.error_count = 3;
	}

	if (dev.RouteString() == 0) {
		xhc.EnumerationState().port_config_phase[port_id] = ConfigPhase::ConfiguringEndpoints;
	}

	ConfigureEndpointCommandTRB cmd{ dev.InputContextBuffer(), dev.SlotID() };
	xhc.CommandRing()->Push(cmd);
	xhc.DoorbellRegisterAt(0)->Ring(0);

	return MAKE_ERROR(Error::kSuccess);
}

Error HostController::ProcessEvent() {
	auto& xhc = self;
	if (!xhc.PrimaryEventRing()->HasFront()) {
		return MAKE_ERROR(Error::kSuccess);
	}
	Error err = MAKE_ERROR(Error::kNotImplemented);
	auto event_trb = xhc.PrimaryEventRing()->Front();
	if (auto trb = TRBDynamicCast<TransferEventTRB>(event_trb)) {
		err = OnEvent(xhc, *trb);
	}
	else if (auto trb = TRBDynamicCast<PortStatusChangeEventTRB>(event_trb)) {
		err = OnEvent(xhc, *trb);
	}
	else if (auto trb = TRBDynamicCast<CommandCompletionEventTRB>(event_trb)) {
		err = OnEvent(xhc, *trb);
	}
	else if (auto trb = TRBDynamicCast<BandwidthRequestEventTRB>(event_trb)) {
		err = OnEvent(xhc, *trb);
	}
	else if (auto trb = TRBDynamicCast<DoorbellEventTRB>(event_trb)) {
		err = OnEvent(xhc, *trb);
	}
	else if (auto trb = TRBDynamicCast<HostControllerEventTRB>(event_trb)) {
		err = OnEvent(xhc, *trb);
	}
	else if (auto trb = TRBDynamicCast<DeviceNotificationEventTRB>(event_trb)) {
		err = OnEvent(xhc, *trb);
	}
	else if (auto trb = TRBDynamicCast<MFINDEXWrapEventTRB>(event_trb)) {
		err = OnEvent(xhc, *trb);
	}
	else {
		plogwarn("xHCI unhandled vendor/reserved event type=%u",
			event_trb->bits.trb_type);
		err = MAKE_ERROR(Error::kSuccess);
	}
	xhc.PrimaryEventRing()->Pop();
	if (xhc.controller_recovery_requested_) return xhc.RecoverController();
	return err;
}


// 1. locate Operational Registers

HostController::HostController(uintptr_t mmio_base,
	const HostControllerResourceConfig& resources)
	: mmio_base_{ mmio_base },
	cap_{ reinterpret_cast<CapabilityRegisters*>(mmio_base) },
	op_{ reinterpret_cast<OperationalRegisters*>(mmio_base + cap_->CAPLENGTH.Read()) },
	runtime_{ reinterpret_cast<RuntimeRegisters*>(mmio_base + cap_->RTSOFF.Read().Offset()) },
	resource_config_{ resources },
	max_ports_{ static_cast<uint8>(cap_->HCSPARAMS1.Read().bits.max_ports) },
	max_slots_{ static_cast<uint8>(resources.max_slots != 0 &&
		resources.max_slots < cap_->HCSPARAMS1.Read().bits.max_device_slots
		? resources.max_slots : cap_->HCSPARAMS1.Read().bits.max_device_slots) },
	context_size_{ static_cast<uint8>(cap_->HCCPARAMS1.Read().bits.context_size ? 64 : 32) },
	slot_removal_pending_{ slot_removal_pending_storage_, sizeof(slot_removal_pending_storage_) },
	slot_disconnect_notified_{ slot_disconnect_notified_storage_, sizeof(slot_disconnect_notified_storage_) }
{
	if (resource_config_.command_ring_trbs == 0) {
		resource_config_.command_ring_trbs = max_slots_ * 4u + 1u;
		if (resource_config_.command_ring_trbs < 64) resource_config_.command_ring_trbs = 64;
		if (resource_config_.command_ring_trbs > 4096) resource_config_.command_ring_trbs = 4096;
	}
	if (resource_config_.event_ring_trbs == 0) {
		resource_config_.event_ring_trbs = max_slots_ * 8u + 1u;
		if (resource_config_.event_ring_trbs < 256) resource_config_.event_ring_trbs = 256;
		if (resource_config_.event_ring_trbs > 4096) resource_config_.event_ring_trbs = 4096;
	}
}

uint8 HostController::IsochronousSchedulingThreshold() const {
	const uint8 raw = cap_->HCSPARAMS2.Read().bits.isochronous_scheduling_threshold;
	return static_cast<uint8>((raw & 0x07u) * ((raw & 0x08u) ? 8u : 1u));
}

Error HostController::OnHostControllerEvent(int completion_code) {
	plogwarn("xHCI Host Controller Event: completion=%d", completion_code);
	controller_event_completion_code_ = static_cast<uint8>(completion_code);
	controller_recovery_requested_ = true;
	return MAKE_ERROR(Error::kSuccess);
}

namespace {
	constexpr uint32 kSupportedProtocolCapabilityID = 2;
	constexpr uint32 kUSBProtocolName = 0x20425355u;

	uint32 ProtocolSpeedMbps(ProtocolSpeedID_t speed) {
		switch (speed.bits.speed_id_exponent) {
		case 0:
			return speed.bits.speed_id_mantissa / 1000000u;
		case 1:
			return speed.bits.speed_id_mantissa / 1000u;
		case 2:
			return speed.bits.speed_id_mantissa;
		case 3:
			return uint32(speed.bits.speed_id_mantissa) * 1000u;
		default:
			return 0;
		}
	}

	uint8 ClassifyProtocolSpeed(uint8 protocol_major, ProtocolSpeedID_t speed) {
		const uint32 megabits_per_second = ProtocolSpeedMbps(speed);
		if (megabits_per_second == 0) return 0;
		if (protocol_major >= 3) {
			return speed.bits.link_protocol == 1 || megabits_per_second > 5000u
				? kSuperSpeedPlus : kSuperSpeed;
		}
		if (megabits_per_second >= 480u) return kHighSpeed;
		if (megabits_per_second >= 12u) return kFullSpeed;
		return kLowSpeed;
	}
}

void HostController::InitializeSupportedProtocols() {
	ExtendedRegisterList extregs{ mmio_base_, cap_->HCCPARAMS1.Read() };
	for (auto& extreg : extregs) {
		if (extreg.Read().bits.capability_id != kSupportedProtocolCapabilityID) continue;
		auto* protocol = reinterpret_cast<SupportedProtocolCapability*>(&extreg);
		const auto revision = protocol->revision.Read();
		if (uint32(protocol->name_string.Read()) != kUSBProtocolName) continue;
		if (revision.bits.major_revision != 2 && revision.bits.major_revision != 3) continue;
		const auto ports = protocol->ports.Read();
		const uint8 first_port = ports.bits.compatible_port_offset;
		const uint8 port_count = ports.bits.compatible_port_count;
		if (first_port == 0 || port_count == 0 || first_port > max_ports_) continue;
		uint16 last_port = uint16(first_port) + port_count - 1;
		if (last_port > max_ports_) last_port = max_ports_;

		for (uint16 port = first_port; port <= last_port; ++port) {
			port_protocol_major_[port] = revision.bits.major_revision;
		}

		const uint8 speed_count = ports.bits.protocol_speed_id_count;
		if (speed_count == 0) {
			for (uint16 port = first_port; port <= last_port; ++port) {
				if (revision.bits.major_revision >= 3) {
					port_speed_classes_[port][kSuperSpeed] = kSuperSpeed;
				} else {
					port_speed_classes_[port][kFullSpeed] = kFullSpeed;
					port_speed_classes_[port][kLowSpeed] = kLowSpeed;
					port_speed_classes_[port][kHighSpeed] = kHighSpeed;
				}
			}
			continue;
		}

		auto* speed_regs = reinterpret_cast<MemMapRegister<ProtocolSpeedID_t>*>(
			reinterpret_cast<uintptr_t>(protocol) + sizeof(SupportedProtocolCapability));
		for (uint8 index = 0; index < speed_count; ++index) {
			const auto speed = speed_regs[index].Read();
			if (speed.bits.speed_id_value == 0) continue;
			const uint8 speed_class = ClassifyProtocolSpeed(revision.bits.major_revision, speed);
			for (uint16 port = first_port; port <= last_port; ++port) {
				port_speed_classes_[port][speed.bits.speed_id_value] = speed_class;
			}
		}
	}
}

uint8 HostController::SpeedClass(uint8 root_hub_port_num, uint8 speed_id) const {
	if (root_hub_port_num != 0 && speed_id < 16) {
		if (const uint8 speed_class = port_speed_classes_[root_hub_port_num][speed_id]) {
			return speed_class;
		}
		if (port_protocol_major_[root_hub_port_num] != 0) return 0;
	}
	if (speed_id >= kFullSpeed && speed_id <= kSuperSpeedPlus) return speed_id;
	return 0;
}

uint8 HostController::SpeedIDForClass(uint8 root_hub_port_num, uint8 speed_class) const {
	if (root_hub_port_num == 0 || root_hub_port_num > max_ports_) return 0;
	for (uint8 speed_id = 1; speed_id < 16; ++speed_id) {
		if (SpeedClass(root_hub_port_num, speed_id) == speed_class) return speed_id;
	}
	return 0;
}

// 2. OS Ownership Handoff

static void RequestHCOwnership(uintptr_t mmio_base, HCCPARAMS1_t hccp) {
	ExtendedRegisterList extregs{ mmio_base, hccp };
	auto ext_usblegsup = std::find_if(
		extregs.begin(), extregs.end(),
		[](auto& reg) { return reg.Read().bits.capability_id == 1; });

	if (ext_usblegsup == extregs.end()) {
		return;
	}
	auto& reg = reinterpret_cast<MemMapRegister<USBLEGSUP_t>&>(*ext_usblegsup);
	auto r = reg.Read();
	if (r.bits.hc_os_owned_semaphore) {
		return;
	}
	r.bits.hc_os_owned_semaphore = 1;
	// Log(kDebug, "waiting until OS owns xHC...\n");
	reg.Write(r);
	do {
		r = reg.Read();
	} while (r.bits.hc_bios_owned_semaphore ||
		!r.bits.hc_os_owned_semaphore);
	// Log(kDebug, "OS has owned xHC\n");
}

// 3. Halt & Reset

static Error RegisterCommandRing(Ring* ring, MemMapRegister<CRCR_t>* crcr) {
	CRCR_t value = crcr->Read();
	value.bits.ring_cycle_state = true;
	value.bits.command_stop = false;
	value.bits.command_abort = false;
	value.SetPointer(reinterpret_cast<uint64_t>(ring->Buffer()));
	crcr->Write(value);
	return MAKE_ERROR(Error::kSuccess);
}

Error HostController::InitializeRuntimeRings() {
	auto* primary_interrupter = &InterrupterRegisterSets()[0];
	if (auto err = cr_.Initialize(resource_config_.command_ring_trbs)) return err;
	if (auto err = RegisterCommandRing(&cr_, &op_->CRCR)) return err;
	if (auto err = er_.Initialize(resource_config_.event_ring_trbs,
		primary_interrupter)) return err;

	auto iman = primary_interrupter->IMAN.Read();
	iman.bits.interrupt_pending = true;
	iman.bits.interrupt_enable = true;
	primary_interrupter->IMAN.Write(iman);

	auto dnctrl = op_->DNCTRL.Read();
	dnctrl.data[0] = 1u << 1;
	op_->DNCTRL.Write(dnctrl);

	auto usbcmd = op_->USBCMD.Read();
	usbcmd.bits.interrupter_enable = true;
	usbcmd.bits.host_system_error_enable = true;
	usbcmd.bits.enable_wrap_event = HCIVersion() >= 0x0100u;
	usbcmd.bits.extended_tbc_enable = HasExtendedTBCCapability();
	op_->USBCMD.Write(usbcmd);
	return MAKE_ERROR(Error::kSuccess);
}

Error HostController::RecoverController() {
	if (!controller_recovery_requested_ || controller_recovery_in_progress_) {
		return MAKE_ERROR(Error::kTransferFailed);
	}
	controller_recovery_in_progress_ = true;
	controller_failed_ = true;
	plogwarn("xHCI controller recovery started: completion=%u",
		controller_event_completion_code_);

	auto usbcmd = op_->USBCMD.Read();
	usbcmd.bits.run_stop = false;
	usbcmd.bits.interrupter_enable = false;
	usbcmd.bits.host_system_error_enable = false;
	usbcmd.bits.enable_wrap_event = false;
	op_->USBCMD.Write(usbcmd);
	stduint wait_count = 10000000u;
	while (!op_->USBSTS.Read().bits.host_controller_halted && wait_count != 0) {
		--wait_count;
	}
	if (wait_count == 0) {
		controller_recovery_in_progress_ = false;
		return MAKE_ERROR(Error::kHostControllerNotHalted);
	}

	for (stduint slot_id = 1; slot_id <= devmgr_.MaxSlots(); ++slot_id) {
		auto* dev = devmgr_.FindBySlot(static_cast<uint8>(slot_id));
		if (dev == nullptr || slot_disconnect_notified_[slot_id]) continue;
		if (g_host_device_disconnected_hook) {
			const auto controller = ControllerIdentity(*this);
			const auto location = DeviceLocation(*this, *dev);
			g_host_device_disconnected_hook(controller, location, *dev);
		}
		slot_disconnect_notified_.setof(slot_id);
	}
	devmgr_.Reset();
	MemSet(slot_removal_pending_storage_, 0, sizeof(slot_removal_pending_storage_));
	MemSet(slot_disconnect_notified_storage_, 0, sizeof(slot_disconnect_notified_storage_));

	enumeration_state_.pending_hub_children.Clear();
	enumeration_state_.active_hub_child_valid = false;
	enumeration_state_.addressing_port = 0;
	enumeration_state_.enable_slot_command_pending = false;
	for (size_t index = 0; index < enumeration_state_.port_config_phase.size(); ++index) {
		enumeration_state_.port_config_phase[index] = ConfigPhase::NotConnected;
	}

	usbcmd = op_->USBCMD.Read();
	usbcmd.bits.host_controller_reset = true;
	op_->USBCMD.Write(usbcmd);
	wait_count = 10000000u;
	while ((op_->USBCMD.Read().bits.host_controller_reset ||
		op_->USBSTS.Read().bits.controller_not_ready) && wait_count != 0) {
		--wait_count;
	}
	if (wait_count == 0) {
		controller_recovery_in_progress_ = false;
		return MAKE_ERROR(Error::kTransferFailed);
	}

	auto config = op_->CONFIG.Read();
	config.bits.max_device_slots_enabled = max_slots_;
	op_->CONFIG.Write(config);
	DCBAAP_t dcbaap{};
	dcbaap.SetPointer(reinterpret_cast<uint64_t>(devmgr_.DeviceContexts()));
	op_->DCBAAP.Write(dcbaap);
	if (auto err = InitializeRuntimeRings()) {
		controller_recovery_in_progress_ = false;
		return err;
	}

	usbcmd = op_->USBCMD.Read();
	usbcmd.bits.run_stop = true;
	op_->USBCMD.Write(usbcmd);
	wait_count = 10000000u;
	while (op_->USBSTS.Read().bits.host_controller_halted && wait_count != 0) {
		--wait_count;
	}
	if (wait_count == 0) {
		controller_recovery_in_progress_ = false;
		return MAKE_ERROR(Error::kTransferFailed);
	}

	controller_recovery_requested_ = false;
	controller_recovery_in_progress_ = false;
	controller_failed_ = false;
	controller_event_completion_code_ = 0;

	Error first_error = MAKE_ERROR(Error::kSuccess);
	for (uint16 port_id = 1; port_id <= max_ports_; ++port_id) {
		auto port = PortAt(static_cast<uint8>(port_id));
		if (!port.IsConnected()) continue;
		if (auto err = ConfigurePort(port)) {
			if (!first_error) first_error = err;
		}
	}
	plogwarn("xHCI controller recovery completed");
	return first_error;
}

Error HostController::Initialize() {
	const uint16 hci_version = cap_->HCIVERSION.Read();
	if (hci_version < 0x0100u) {
		plogwarn("xHCI version 0x%04X is older than 1.0; continuing",
			static_cast<unsigned>(hci_version));
	}
	const auto hcsparams1 = cap_->HCSPARAMS1.Read();
	if (max_ports_ == 0 || max_slots_ == 0 || hcsparams1.bits.max_interrupters == 0) {
		return MAKE_ERROR(Error::kInvalidControllerCapability);
	}
	auto valid_ring_size = [](size_t size) { return size >= 16 && size <= 4096; };
	if (!valid_ring_size(resource_config_.command_ring_trbs) ||
		!valid_ring_size(resource_config_.event_ring_trbs) ||
		!valid_ring_size(resource_config_.control_transfer_ring_trbs) ||
		!valid_ring_size(resource_config_.transfer_ring_trbs) ||
		!valid_ring_size(resource_config_.bulk_transfer_ring_trbs)) {
		return MAKE_ERROR(Error::kInvalidControllerCapability);
	}
	InitializeSupportedProtocols();
	RequestHCOwnership(mmio_base_, cap_->HCCPARAMS1.Read());
	// Stop Controller
	auto usbcmd = op_->USBCMD.Read();
	usbcmd.bits.interrupter_enable = false;
	usbcmd.bits.host_system_error_enable = false;
	usbcmd.bits.enable_wrap_event = false;
	// Host controller must be halted before resetting it.
	if (!op_->USBSTS.Read().bits.host_controller_halted) {
		usbcmd.bits.run_stop = false;  // stop
	}
	// Loop wait for HCHalted set
	op_->USBCMD.Write(usbcmd);
	while (!op_->USBSTS.Read().bits.host_controller_halted);
	// Reset controller
	usbcmd = op_->USBCMD.Read();
	usbcmd.bits.host_controller_reset = true;
	op_->USBCMD.Write(usbcmd);
	// Loop wait for HCHalted set
	while (op_->USBCMD.Read().bits.host_controller_reset);
	// Loop wait for Controller Not Ready clear
	while (op_->USBSTS.Read().bits.controller_not_ready);
	const uint32 page_size_bitmap = uint32(op_->PAGESIZE.Read());
	if (page_size_bitmap == 0) {
		return MAKE_ERROR(Error::kUnsupportedPageSize);
	}
	uint8 page_size_shift = 0;
	while (page_size_shift < 16 && (page_size_bitmap & (1u << page_size_shift)) == 0) {
		++page_size_shift;
	}
	if (page_size_shift == 16) return MAKE_ERROR(Error::kUnsupportedPageSize);
	page_size_ = size_t(4096) << page_size_shift;
	if (auto err = devmgr_.Initialize(max_slots_)) {
		return err;
	}

	// Log(kDebug, "MaxSlots: %u", cap_->HCSPARAMS1.Read().bits.max_device_slots);
	// Set "Max Slots Enabled" field in CONFIG.
	auto config = op_->CONFIG.Read();
	config.bits.max_device_slots_enabled = max_slots_;
	op_->CONFIG.Write(config);

	// Aloc Scratchpad Buffers) , set DCBAAP "Address Book"
	auto hcsparams2 = cap_->HCSPARAMS2.Read();
	// Combine to get scratchpad buffer count (high 5 bits and low 5 bits)
	const uint16_t max_scratchpad_buffers =
		hcsparams2.bits.max_scratchpad_buffers_low
		| (hcsparams2.bits.max_scratchpad_buffers_high << 5);
	if (max_scratchpad_buffers > 0) {
		auto scratchpad_buf_arr = AllocArray<void*>(max_scratchpad_buffers, 64, 0);
		if (scratchpad_buf_arr == nullptr) return MAKE_ERROR(Error::kNoEnoughMemory);
		for (int i = 0; i < max_scratchpad_buffers; ++i) {
			scratchpad_buf_arr[i] = uni_hostenv_allocator->allocate(
				page_size_, 12 + page_size_shift, 12 + page_size_shift);
			if (scratchpad_buf_arr[i] == nullptr) {
				for (int j = 0; j < i; ++j) {
					uni_hostenv_allocator->deallocate(scratchpad_buf_arr[j]);
				}
				uni_hostenv_allocator->deallocate(scratchpad_buf_arr);
				return MAKE_ERROR(Error::kNoEnoughMemory);
			}
			Log(kDebug, "scratchpad buffer array %d = %p\n",
				i, scratchpad_buf_arr[i]);
		}
		// Store the physical address of the pointer array into DCBAAP array entry 0
		devmgr_.DeviceContexts()[0] = scratchpad_buf_arr;
		Log(kInfo, "wrote scratchpad buffer array %p to dev ctx array 0\n",
			scratchpad_buf_arr);
	}

	DCBAAP_t dcbaap{};
	// Fill the base address of the device context base address array into the 64-bit DCBAAP structure
	dcbaap.SetPointer(reinterpret_cast<uint64_t>(devmgr_.DeviceContexts()));
	// Write to operational register
	op_->DCBAAP.Write(dcbaap);

	controller_failed_ = false;
	controller_recovery_requested_ = false;
	controller_recovery_in_progress_ = false;
	controller_event_completion_code_ = 0;
	return InitializeRuntimeRings();
}


#endif

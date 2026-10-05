#if defined(_MCCA) && _MCCA == 0x8664
#include "../../../../../inc/c/msgface.h"
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
		struct PendingHubChildAddress {
			uint8 root_hub_port_num;
			uint32 route_string;
			uint8 hub_slot_id;
			uint8 downstream_port;
			uint8 speed;
		};

		uni::Array<PendingHubChildAddress, 16> pending_hub_children{};
		stduint pending_hub_child_count = 0;
		PendingHubChildAddress active_hub_child{};
		bool active_hub_child_valid = false;

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

		uint8 DetermineHubChildSpeed(uint16 port_status) {
			if ((port_status & (1u << 10)) != 0) return 3; // High Speed
			if ((port_status & (1u << 9)) != 0) return 2;  // Low Speed
			return 1; // Full Speed
		}

		bool QueueHubChildAddress(PendingHubChildAddress ctx) {
			if (pending_hub_child_count >= pending_hub_children.size()) {
				return false;
			}
			for (stduint i = 0; i < pending_hub_child_count; ++i) {
				const auto& pending = pending_hub_children[i];
				if (pending.root_hub_port_num == ctx.root_hub_port_num &&
					pending.route_string == ctx.route_string) {
					return true;
				}
			}
			if (active_hub_child_valid &&
				active_hub_child.root_hub_port_num == ctx.root_hub_port_num &&
				active_hub_child.route_string == ctx.route_string) {
				return true;
			}
			pending_hub_children[pending_hub_child_count++] = ctx;
			return true;
		}

		bool PopHubChildAddress(PendingHubChildAddress& ctx) {
			if (pending_hub_child_count == 0) return false;
			ctx = pending_hub_children[0];
			for (stduint i = 1; i < pending_hub_child_count; ++i) {
				pending_hub_children[i - 1] = pending_hub_children[i];
			}
			--pending_hub_child_count;
			return true;
		}

		void RemovePendingHubChildren(uint8 root_hub_port_num, uint32 route_string_prefix, bool all_routes) {
			stduint dst = 0;
			for (stduint src = 0; src < pending_hub_child_count; ++src) {
				const auto& pending = pending_hub_children[src];
				const bool matches = pending.root_hub_port_num == root_hub_port_num &&
					(all_routes || RouteHasPrefix(pending.route_string, route_string_prefix));
				if (!matches) {
					pending_hub_children[dst++] = pending;
				}
			}
			pending_hub_child_count = dst;
			if (active_hub_child_valid &&
				active_hub_child.root_hub_port_num == root_hub_port_num &&
				(all_routes || RouteHasPrefix(active_hub_child.route_string, route_string_prefix))) {
				active_hub_child_valid = false;
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
				auto* dev = xhc.GetDeviceManager()->FindBySlot(slots[i]);
				if (!dev) continue;
				if (g_host_device_disconnected_hook) {
					const auto controller = ControllerIdentity(xhc);
					const auto location = DeviceLocation(xhc, *dev);
					g_host_device_disconnected_hook(controller, location, *dev);
				}
				xhc.GetDeviceManager()->Remove(slots[i]);
			}
			RemovePendingHubChildren(root_hub_port_num, route_string_prefix, all_routes);
		}
	}

	USBHostDevice_v3::USBHostDevice_v3(uint8 slot_id, DoorbellRegister* dbreg,
		HostController* host, uint8 context_size)
		: slot_id_{ slot_id }, dbreg_{ dbreg }, host_{ host }, context_size_{ context_size } {
	}

	USBHostDevice_v3::~USBHostDevice_v3() {
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
		state_ = State::kBlank;
		for (size_t i = 0; i < 31; ++i) {
			const DeviceContextIndex dci(i + 1);
			//on_transferred_callbacks_[i] = nullptr;
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	void USBHostDevice_v3::SelectForSlotAssignment() {
		state_ = State::kSlotAssigning;
	}

	Ring* USBHostDevice_v3::AllocTransferRing(DeviceContextIndex index, size_t buf_size) {
		int i = index.value - 1;
		auto tr = AllocArray<Ring>(1, 64, 4096);
		if (tr) {
			// Ring* tr = new(tr) Ring();
			tr->Initialize(buf_size);
		}
		transfer_rings_[i] = tr;
		return tr;
	}

	Error USBHostDevice_v3::ControlIn(EndpointID ep_id, SetupData setup_data,
		void* buf, int len, ClassDriver* issuer) {
		if (auto err = USBHostDevice::ControlIn(ep_id, setup_data, buf, len, issuer)) {
			return err;
		}

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

		auto status = StatusStageTRB{};

		if (buf) {
			auto setup_trb_position = TRBDynamicCast<SetupStageTRB>(tr->Push(
				MakeSetupStageTRB(setup_data, SetupStageTRB::kInDataStage)));
			auto data = MakeDataStageTRB(buf, len, true);
			data.bits.interrupt_on_completion = true;
			auto data_trb_position = tr->Push(data);
			tr->Push(status);

			setup_stage_map_.Put(data_trb_position, setup_trb_position);
		}
		else {
			auto setup_trb_position = TRBDynamicCast<SetupStageTRB>(tr->Push(
				MakeSetupStageTRB(setup_data, SetupStageTRB::kNoDataStage)));
			status.bits.direction = true;
			status.bits.interrupt_on_completion = true;
			auto status_trb_position = tr->Push(status);

			setup_stage_map_.Put(status_trb_position, setup_trb_position);
		}

		dbreg_->Ring(dci.value);

		return MAKE_ERROR(Error::kSuccess);
	}

	Error USBHostDevice_v3::ControlOut(EndpointID ep_id, SetupData setup_data,
		const void* buf, int len, ClassDriver* issuer) {
		if (auto err = USBHostDevice::ControlOut(ep_id, setup_data, buf, len, issuer)) {
			return err;
		}

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

		auto status = StatusStageTRB{};
		status.bits.direction = true;

		if (buf) {
			auto setup_trb_position = TRBDynamicCast<SetupStageTRB>(tr->Push(
				MakeSetupStageTRB(setup_data, SetupStageTRB::kOutDataStage)));
			auto data = MakeDataStageTRB(buf, len, false);
			data.bits.interrupt_on_completion = true;
			auto data_trb_position = tr->Push(data);
			tr->Push(status);

			setup_stage_map_.Put(data_trb_position, setup_trb_position);
		}
		else {
			auto setup_trb_position = TRBDynamicCast<SetupStageTRB>(tr->Push(
				MakeSetupStageTRB(setup_data, SetupStageTRB::kNoDataStage)));
			status.bits.interrupt_on_completion = true;
			auto status_trb_position = tr->Push(status);

			setup_stage_map_.Put(status_trb_position, setup_trb_position);
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

		Log(kDebug, "Device::InterrutpOut: ep addr %d, buf %08lx, len %d, dev %08lx\n",
			ep_id.Address(), buf, len, this);
		return MAKE_ERROR(Error::kNotImplemented);
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
		if (!pending.active || pending.recovery_phase != BulkRecoveryPhase::kNone ||
			host_ == nullptr) {
			return MAKE_ERROR(Error::kInvalidPhase);
		}
		pending.recovery_phase = BulkRecoveryPhase::kResetEndpoint;
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
		if (!pending.active || pending.recovery_phase != BulkRecoveryPhase::kResetEndpoint) {
			return MAKE_ERROR(Error::kInvalidPhase);
		}
		if (completion_code != 1 || host_ == nullptr) {
			return CompleteBulkFailure(ep_id);
		}

		pending.recovery_phase = BulkRecoveryPhase::kClearEndpointHalt;
		if (auto err = this->OnBulkCompleted(ep_id, nullptr, 0)) {
			pending = PendingBulkTransfer{};
			return err;
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error USBHostDevice_v3::OnTransferRingDequeueSet(EndpointID ep_id, int completion_code) {
		const DeviceContextIndex dci{ ep_id };
		auto& pending = pending_bulk_transfers_[dci.value - 1];
		if (!pending.active ||
			pending.recovery_phase != BulkRecoveryPhase::kSetDequeuePointer) {
			return MAKE_ERROR(Error::kInvalidPhase);
		}
		pending = PendingBulkTransfer{};
		return completion_code == 1
			? MAKE_ERROR(Error::kSuccess) : MAKE_ERROR(Error::kTransferFailed);
	}

	Error USBHostDevice_v3::OnHubPortStatusReceived(uint8 port_num, uint16 status, uint16 change) {
		if (!host_) return MAKE_ERROR(Error::kNotImplemented);
		return host_->OnHubPortStatusChanged(*this, port_num, status, change);
	}

	Error USBHostDevice_v3::OnTransferEventReceived(const TransferEventTRB& trb) {
		const auto residual_length = trb.bits.trb_transfer_length;
		const bool transfer_succeeded = trb.bits.completion_code == 1 /* Success */ ||
			trb.bits.completion_code == 13 /* Short Packet */;
		Log(kDebug, trb);

		TRB* issuer_trb = trb.Pointer();
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
					if (pending.recovery_phase != BulkRecoveryPhase::kNone) {
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

		for (size_t index = 0; index < pending_bulk_transfers_.size(); ++index) {
			auto& pending = pending_bulk_transfers_[index];
			if (!pending.active ||
				pending.recovery_phase != BulkRecoveryPhase::kClearEndpointHalt) {
				continue;
			}
			const EndpointID recovery_ep{ static_cast<int>(index + 1) };
			const uint16 descriptor_address = static_cast<uint16>(
				recovery_ep.Number() | (recovery_ep.IsIn() ? 0x80 : 0x00));
			const bool is_clear_halt =
				setup_data.request_type.bits.direction == request_type::kOut &&
				setup_data.request_type.bits.type == request_type::kStandard &&
				setup_data.request_type.bits.recipient == request_type::kEndpoint &&
				setup_data.request == request::kClearFeature && setup_data.value == 0 &&
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

			pending.recovery_phase = BulkRecoveryPhase::kSetDequeuePointer;
			SetTRDequeuePointerCommandTRB set_dequeue{
				pending.next_trb, pending.next_cycle_state, recovery_ep, slot_id_ };
			host_->CommandRing()->Push(set_dequeue);
			host_->DoorbellRegisterAt(0)->Ring(0);
			return MAKE_ERROR(Error::kSuccess);
		}

		if (!transfer_succeeded) {
			return MAKE_ERROR(Error::kTransferFailed);
		}
		return this->OnControlCompleted(
			trb.GetEndpointID(), setup_data, data_stage_buffer, transfer_length);
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

		devices_[slot_id] = AllocArray<USBHostDevice_v3>(1, 64, 4096);
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
		device_context_pointers_[slot_id] = nullptr;
		devices_[slot_id]->~USBHostDevice_v3();
		uni_hostenv_allocator->deallocate(devices_[slot_id]);
		devices_[slot_id] = nullptr;
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



	enum class ConfigPhase {
		kNotConnected,
		kWaitingAddressed,
		kResettingPort,
		kEnablingSlot,
		kAddressingDevice,
		kInitializingDevice,
		kConfiguringEndpoints,
		kConfigured,
	};
	/* Between resetting a root hub port and assigning an address,
		 * no other processing must be interleaved; only that port's processing is allowed.
		 * kWaitingAddressed is the state waiting for the sequence from reset
		 * (kResettingPort) to address assignment (kAddressingDevice) to complete.
		 */

	uni::Array<volatile ConfigPhase, 256> port_config_phase{};  // index: port number

	/** Port number currently processing from kResettingPort to kAddressingDevice.
		 * 0 indicates no port is in that state.
		 */
	uint8 addressing_port{ 0 };

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
		if (active_hub_child_valid || addressing_port != 0) {
			return MAKE_ERROR(Error::kSuccess);
		}
		PendingHubChildAddress ctx{};
		if (!PopHubChildAddress(ctx)) {
			return MAKE_ERROR(Error::kSuccess);
		}
		active_hub_child = ctx;
		active_hub_child_valid = true;
		EnableSlotCommandTRB cmd{};
		xhc.CommandRing()->Push(cmd);
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
		const bool is_connected = port.IsConnected();
		Log(kDebug, "ResetPort: port.IsConnected() = %s\n",
			is_connected ? "true" : "false");

		if (!is_connected) {
			return MAKE_ERROR(Error::kSuccess);
		}

		if (addressing_port != 0 || active_hub_child_valid) {
			port_config_phase[port.Number()] = ConfigPhase::kWaitingAddressed;
		}
		else {
			const auto port_phase = port_config_phase[port.Number()];
			if (port_phase != ConfigPhase::kNotConnected &&
				port_phase != ConfigPhase::kWaitingAddressed) {
				return MAKE_ERROR(Error::kInvalidPhase);
			}
			addressing_port = port.Number();
			port_config_phase[port.Number()] = ConfigPhase::kResettingPort;
			port.Reset();
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error EnableSlot(HostController& xhc, Port& port) {
		const bool is_enabled = port.IsEnabled();
		const bool reset_completed = port.IsPortResetChanged();
		Log(kDebug, "EnableSlot: port.IsEnabled() = %s, port.IsPortResetChanged() = %s\n",
			is_enabled ? "true" : "false",
			reset_completed ? "true" : "false");

		if (is_enabled && reset_completed) {
			port.ClearPortResetChange();

			port_config_phase[port.Number()] = ConfigPhase::kEnablingSlot;

			EnableSlotCommandTRB cmd{};
			xhc.CommandRing()->Push(cmd);
			xhc.DoorbellRegisterAt(0)->Ring(0);
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error AddressDevice(HostController& xhc, uint8 port_id, uint8 slot_id) {
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

		InitializeEP0Context(
			*ep0_ctx, dev->AllocTransferRing(ep0_dci, 32),
			DetermineMaxPacketSizeForControlPipe(speed_class));

		xhc.GetDeviceManager()->LoadDCBAA(slot_id);

		port_config_phase[port_id] = ConfigPhase::kAddressingDevice;

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

		InitializeEP0Context(
			*ep0_ctx, dev->AllocTransferRing(ep0_dci, 32),
			DetermineMaxPacketSizeForControlPipe(slot_ctx->bits.speed));

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
			port_config_phase[port_id] = ConfigPhase::kInitializingDevice;
		}
		dev->StartInitialize();

		return MAKE_ERROR(Error::kSuccess);
	}

	Error CompleteConfiguration(HostController& xhc, uint8 port_id, uint8 slot_id) {
		// Log(kDebug, "CompleteConfiguration: port_id = %d, slot_id = %d\n", port_id, slot_id);

		auto dev = xhc.GetDeviceManager()->FindBySlot(slot_id);
		if (dev == nullptr) {
			return MAKE_ERROR(Error::kInvalidSlotID);
		}

		dev->OnEndpointsConfigured();
		if (g_host_device_configured_hook) {
			const auto controller = ControllerIdentity(xhc);
			const auto location = DeviceLocation(xhc, *dev);
			g_host_device_configured_hook(controller, location, *dev);
		}

		if (dev->RouteString() == 0) {
			port_config_phase[port_id] = ConfigPhase::kConfigured;
		}
		if (active_hub_child_valid &&
			active_hub_child.root_hub_port_num == dev->RootHubPortNum() &&
			active_hub_child.route_string == dev->RouteString()) {
			active_hub_child_valid = false;
			return TryAddressNextHubChild(xhc);
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error OnEvent(HostController& xhc, PortStatusChangeEventTRB& trb) {
		// Log(kDebug, "PortStatusChangeEvent: port_id = %d", trb.bits.port_id);
		auto port_id = trb.bits.port_id;
		auto port = xhc.PortAt(port_id);

		switch (port_config_phase[port_id]) {
		case ConfigPhase::kNotConnected:
			return ResetPort(xhc, port);
		case ConfigPhase::kResettingPort:
			return EnableSlot(xhc, port);
		case ConfigPhase::kWaitingAddressed:
		case ConfigPhase::kEnablingSlot:
			// Before the Enable Slot command is completed, the controller/virtual machine may continue to report port status changes.
			if (!port.IsConnected()) {
				// restore state
				port_config_phase[port_id] = ConfigPhase::kNotConnected;
				if (addressing_port == port_id) addressing_port = 0;
				ploginfo("Port %u disconnected while enabling slot, reset state", port_id);
			} else {
				// ploginfo("Port %u: PSC arrived during kEnablingSlot -- ignoring", port_id);// ignore
			}
			return MAKE_ERROR(Error::kSuccess);
		case ConfigPhase::kAddressingDevice:
		case ConfigPhase::kInitializingDevice:
		case ConfigPhase::kConfiguringEndpoints:
		case ConfigPhase::kConfigured:
			if (!port.IsConnected()) {
				port_config_phase[port_id] = ConfigPhase::kNotConnected;
				if (addressing_port == port_id) addressing_port = 0;
				RemoveDeviceSubtree(xhc, port_id, 0, true);
				ploginfo("Port %u disconnected, subtree removed", port_id);
				return MAKE_ERROR(Error::kSuccess);
			}
			return MAKE_ERROR(Error::kSuccess);
		default:
			plogerro("OnEvent kInvalidPhase %u", port_config_phase[port_id]);
			return MAKE_ERROR(Error::kInvalidPhase);
		}
	}

	Error OnEvent(HostController& xhc, TransferEventTRB& trb) {
		const uint8 slot_id = trb.bits.slot_id;
		auto dev = xhc.GetDeviceManager()->FindBySlot(slot_id);
		if (dev == nullptr) {
			return MAKE_ERROR(Error::kInvalidSlotID);
		}
		if (auto err = dev->OnTransferEventReceived(trb)) {
			return err;
		}

		const auto port_id = dev->GetSlotContext()->bits.root_hub_port_num;
		if (dev->IsInitialized() &&
			((dev->RouteString() == 0 && port_config_phase[port_id] == ConfigPhase::kInitializingDevice) ||
			 (active_hub_child_valid &&
			  active_hub_child.root_hub_port_num == dev->RootHubPortNum() &&
			  active_hub_child.route_string == dev->RouteString()))) {
			return xhc.ConfigureEndpoints(*dev);
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error OnEvent(HostController& xhc, CommandCompletionEventTRB& trb) {
		const auto issuer_type = trb.Pointer()->bits.trb_type;
		const auto slot_id = trb.bits.slot_id;
		// Log(kDebug, "CommandCompletionEvent: slot_id = %d, issuer = %s",
		// 	trb.bits.slot_id, kTRBTypeToName[issuer_type]);

		if (issuer_type == EnableSlotCommandTRB::Type) {
			if (active_hub_child_valid) {
				return AddressDevice(xhc, active_hub_child, slot_id);
			}
			if (port_config_phase[addressing_port] != ConfigPhase::kEnablingSlot) {
				return MAKE_ERROR(Error::kInvalidPhase);
			}
			return AddressDevice(xhc, addressing_port, slot_id);
		}
		else if (issuer_type == AddressDeviceCommandTRB::Type) {
			auto dev = xhc.GetDeviceManager()->FindBySlot(slot_id);
			if (dev == nullptr) {
				return MAKE_ERROR(Error::kInvalidSlotID);
			}

			auto port_id = dev->GetSlotContext()->bits.root_hub_port_num;
			if (dev->RouteString() == 0) {
				if (port_id != addressing_port) {
					return MAKE_ERROR(Error::kInvalidPhase);
				}
				if (port_config_phase[port_id] != ConfigPhase::kAddressingDevice) {
					return MAKE_ERROR(Error::kInvalidPhase);
				}

				addressing_port = 0;
				for (size_t i = 0; i < port_config_phase.size(); ++i) {
					if (port_config_phase[i] == ConfigPhase::kWaitingAddressed) {
						auto port = xhc.PortAt(i);
						if (auto err = ResetPort(xhc, port); err) {
							return err;
						}
						break;
					}
				}
			}

			return InitializeDevice(xhc, port_id, slot_id);
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

			auto port_id = dev->GetSlotContext()->bits.root_hub_port_num;
			if (dev->RouteString() == 0 && port_config_phase[port_id] != ConfigPhase::kConfiguringEndpoints) {
				return MAKE_ERROR(Error::kInvalidPhase);
			}

			return CompleteConfiguration(xhc, port_id, slot_id);
		}

		return MAKE_ERROR(Error::kInvalidPhase);
	}


}

namespace uni::device::SpaceUSB3 {





	Error HostController::Run() {
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
	if (port_config_phase[port.Number()] == ConfigPhase::kNotConnected) {
		return ResetPort(self, port);
	}
	return MAKE_ERROR(Error::kSuccess);
}

Error HostController::OnHubPortStatusChanged(USBHostDevice_v3& hub_dev, uint8 downstream_port, uint16 status, uint16 change) {
	(void)change;
	const auto root_hub_port_num = hub_dev.RootHubPortNum();
	const auto route_string = AppendRouteString(hub_dev.RouteString(), downstream_port);
	auto* existing = GetDeviceManager()->FindByPort(root_hub_port_num, route_string);

	if ((status & 0x0001u) == 0) {
		if (existing) {
			RemoveDeviceSubtree(self, root_hub_port_num, route_string, false);
		} else {
			RemovePendingHubChildren(root_hub_port_num, route_string, false);
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	if (existing != nullptr) {
		return MAKE_ERROR(Error::kSuccess);
	}

	PendingHubChildAddress child_ctx{};
	child_ctx.root_hub_port_num = root_hub_port_num;
	child_ctx.route_string = route_string;
	child_ctx.hub_slot_id = hub_dev.SlotID();
	child_ctx.downstream_port = downstream_port;
	child_ctx.speed = DetermineHubChildSpeed(status);
	if (!QueueHubChildAddress(child_ctx)) {
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
	slot_ctx->bits.context_entries = 31;
	const auto port_id{ dev.GetSlotContext()->bits.root_hub_port_num };
	const int speed_id{ dev.GetSlotContext()->bits.speed };
	const int port_speed{ dev.RouteString() == 0
		? xhc.SpeedClass(port_id, speed_id)
		: speed_id };
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
		ep_ctx->bits.average_trb_length = 1;

		constexpr size_t kDefaultTransferRingSize = 32;
		constexpr size_t kBulkTransferRingSize = 1024;
		const size_t transfer_ring_size = configs[i].ep_type == EndpointType::kBulk
			? kBulkTransferRingSize : kDefaultTransferRingSize;
		auto tr = dev.AllocTransferRing(ep_dci, transfer_ring_size);
		ep_ctx->SetTransferRingBuffer(tr->Buffer());

		ep_ctx->bits.dequeue_cycle_state = 1;
		ep_ctx->bits.max_primary_streams = 0;
		ep_ctx->bits.mult = 0;
		ep_ctx->bits.error_count = 3;
	}

	if (dev.RouteString() == 0) {
		port_config_phase[port_id] = ConfigPhase::kConfiguringEndpoints;
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
	xhc.PrimaryEventRing()->Pop();
	return err;
}


// 1. locate Operational Registers

HostController::HostController(uintptr_t mmio_base)
	: mmio_base_{ mmio_base },
	cap_{ reinterpret_cast<CapabilityRegisters*>(mmio_base) },
	op_{ reinterpret_cast<OperationalRegisters*>(mmio_base + cap_->CAPLENGTH.Read()) },
	max_ports_{ static_cast<uint8>(cap_->HCSPARAMS1.Read().bits.max_ports) },
	max_slots_{ static_cast<uint8>(cap_->HCSPARAMS1.Read().bits.max_device_slots < kDeviceSize
		? cap_->HCSPARAMS1.Read().bits.max_device_slots : kDeviceSize) },
	context_size_{ static_cast<uint8>(cap_->HCCPARAMS1.Read().bits.context_size ? 64 : 32) }
{
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
	if ((uint32(op_->PAGESIZE.Read()) & 1u) == 0) {
		return MAKE_ERROR(Error::kUnsupportedPageSize);
	}
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
		auto scratchpad_buf_arr = AllocArray<void*>(max_scratchpad_buffers, 64, 4096);
		for (int i = 0; i < max_scratchpad_buffers; ++i) {
			scratchpad_buf_arr[i] = uni_hostenv_allocator->allocate(0x1000, 12, 12);
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

	// Hardware is now ready; below is the setup of dynamic interaction channels

	// Register 32 Command Rings
	auto primary_interrupter = &InterrupterRegisterSets()[0];
	if (auto err = cr_.Initialize(32)) {
		return err;
	}
	if (auto err = RegisterCommandRing(&cr_, &op_->CRCR)) {
		return err;
	}
	// init ring and bind it to the primary interrupter
	if (auto err = er_.Initialize(32, primary_interrupter)) {
		return err;
	}

	// Enable interrupt for the primary interrupter
	auto iman = primary_interrupter->IMAN.Read();
	iman.bits.interrupt_pending = true;
	iman.bits.interrupt_enable = true;
	primary_interrupter->IMAN.Write(iman);

	// Enable interrupt for the controller
	usbcmd = op_->USBCMD.Read();
	usbcmd.bits.interrupter_enable = true;
	op_->USBCMD.Write(usbcmd);

	return MAKE_ERROR(Error::kSuccess);
}


#endif

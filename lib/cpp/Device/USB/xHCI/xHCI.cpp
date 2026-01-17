#if defined(_MCCA) && _MCCA == 0x8664
#include "../../../../../inc/c/msgface.h"
#include "../../../../../inc/cpp/Device/USB/xHCI/xHCI.hpp"
#include <algorithm>

template <class T>
T* AllocArray(size_t num_obj, unsigned int alignment, unsigned int boundary) {
	return reinterpret_cast<T*>(
		AllocMem(sizeof(T) * num_obj, alignment, boundary));
}

#define FreeMem(x)

// ---- ---- ---- ---- device.cpp ---- ---- ---- ---- //

namespace {
	using namespace usb::xhci;

	SetupStageTRB MakeSetupStageTRB(usb::SetupData setup_data, int transfer_type) {
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

namespace usb::xhci {
	Device::Device(uint8_t slot_id, DoorbellRegister* dbreg)
		: slot_id_{ slot_id }, dbreg_{ dbreg } {
	}

	Error Device::Initialize() {
		state_ = State::kBlank;
		for (size_t i = 0; i < 31; ++i) {
			const DeviceContextIndex dci(i + 1);
			//on_transferred_callbacks_[i] = nullptr;
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	void Device::SelectForSlotAssignment() {
		state_ = State::kSlotAssigning;
	}

	Ring* Device::AllocTransferRing(DeviceContextIndex index, size_t buf_size) {
		int i = index.value - 1;
		auto tr = AllocArray<Ring>(1, 64, 4096);
		if (tr) {
			tr->Initialize(buf_size);
		}
		transfer_rings_[i] = tr;
		return tr;
	}

	Error Device::ControlIn(EndpointID ep_id, SetupData setup_data,
		void* buf, int len, ClassDriver* issuer) {
		if (auto err = usb::Device::ControlIn(ep_id, setup_data, buf, len, issuer)) {
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

	Error Device::ControlOut(EndpointID ep_id, SetupData setup_data,
		const void* buf, int len, ClassDriver* issuer) {
		if (auto err = usb::Device::ControlOut(ep_id, setup_data, buf, len, issuer)) {
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

	Error Device::InterruptIn(EndpointID ep_id, void* buf, int len) {
		if (auto err = usb::Device::InterruptIn(ep_id, buf, len)) {
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

	Error Device::InterruptOut(EndpointID ep_id, void* buf, int len) {
		if (auto err = usb::Device::InterruptOut(ep_id, buf, len)) {
			return err;
		}

		Log(kDebug, "Device::InterrutpOut: ep addr %d, buf %08lx, len %d, dev %08lx\n",
			ep_id.Address(), buf, len, this);
		return MAKE_ERROR(Error::kNotImplemented);
	}

	Error Device::OnTransferEventReceived(const TransferEventTRB& trb) {
		const auto residual_length = trb.bits.trb_transfer_length;

		if (trb.bits.completion_code != 1 /* Success */ &&
			trb.bits.completion_code != 13 /* Short Packet */) {
			Log(kDebug, trb);
			return MAKE_ERROR(Error::kTransferFailed);
		}
		Log(kDebug, trb);

		TRB* issuer_trb = trb.Pointer();
		if (auto normal_trb = TRBDynamicCast<NormalTRB>(issuer_trb)) {
			const auto transfer_length =
				normal_trb->bits.trb_transfer_length - residual_length;
			return this->OnInterruptCompleted(
				trb.GetEndpointID(), normal_trb->Pointer(), transfer_length);
		}

		auto opt_setup_stage_trb = setup_stage_map_.Get(issuer_trb);
		if (!opt_setup_stage_trb) {
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
		return this->OnControlCompleted(
			trb.GetEndpointID(), setup_data, data_stage_buffer, transfer_length);
	}
}

// ---- ---- ---- ---- devmgr.cpp ---- ---- ---- ---- //

namespace usb::xhci {
	Error DeviceManager::Initialize(size_t max_slots) {
		max_slots_ = max_slots;

		devices_ = AllocArray<Device*>(max_slots_ + 1, 0, 0);
		if (devices_ == nullptr) {
			return MAKE_ERROR(Error::kNoEnoughMemory);
		}

		device_context_pointers_ = AllocArray<DeviceContext*>(max_slots_ + 1, 64, 4096);
		if (device_context_pointers_ == nullptr) {
			FreeMem(devices_);
			return MAKE_ERROR(Error::kNoEnoughMemory);
		}

		for (size_t i = 0; i <= max_slots_; ++i) {
			devices_[i] = nullptr;
			device_context_pointers_[i] = nullptr;
		}

		return MAKE_ERROR(Error::kSuccess);
	}

	DeviceContext** DeviceManager::DeviceContexts() const {
		return device_context_pointers_;
	}

	Device* DeviceManager::FindByPort(uint8_t port_num, uint32_t route_string) const {
		for (size_t i = 1; i <= max_slots_; ++i) {
			auto dev = devices_[i];
			if (dev == nullptr) continue;
			if (dev->GetDeviceContext()->slot_context.bits.root_hub_port_num == port_num) {
				return dev;
			}
		}
		return nullptr;
	}

	Device* DeviceManager::FindByState(enum Device::State state) const {
		for (size_t i = 1; i <= max_slots_; ++i) {
			auto dev = devices_[i];
			if (dev == nullptr) continue;
			if (dev->State() == state) {
				return dev;
			}
		}
		return nullptr;
	}

	Device* DeviceManager::FindBySlot(uint8_t slot_id) const {
		if (slot_id > max_slots_) {
			return nullptr;
		}
		return devices_[slot_id];
	}

	/*
	WithError<Device*> DeviceManager::Get(uint8_t device_id) const {
	  if (device_id >= num_devices_) {
		return {nullptr, Error::kInvalidDeviceId};
	  }
	  return {&devices_[device_id], Error::kSuccess};
	}
	*/

	Error DeviceManager::AllocDevice(uint8_t slot_id, DoorbellRegister* dbreg) {
		if (slot_id > max_slots_) {
			return MAKE_ERROR(Error::kInvalidSlotID);
		}

		if (devices_[slot_id] != nullptr) {
			return MAKE_ERROR(Error::kAlreadyAllocated);
		}

		devices_[slot_id] = AllocArray<Device>(1, 64, 4096);
		new(devices_[slot_id]) Device(slot_id, dbreg);
		return MAKE_ERROR(Error::kSuccess);
	}

	Error DeviceManager::LoadDCBAA(uint8_t slot_id) {
		if (slot_id > max_slots_) {
			return MAKE_ERROR(Error::kInvalidSlotID);
		}

		auto dev = devices_[slot_id];
		device_context_pointers_[slot_id] = dev->GetDeviceContext();
		return MAKE_ERROR(Error::kSuccess);
	}

	Error DeviceManager::Remove(uint8_t slot_id) {
		device_context_pointers_[slot_id] = nullptr;
		FreeMem(devices_[slot_id]);
		devices_[slot_id] = nullptr;
		return MAKE_ERROR(Error::kSuccess);
	}
}

// ---- ---- ---- ---- port.cpp ---- ---- ---- ---- //

namespace usb::xhci {
	uint8_t Port::Number() const {
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

	Device* Port::Initialize() {
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

namespace usb::xhci {
	ExtendedRegisterList::Iterator& ExtendedRegisterList::Iterator::operator++() {
		if (reg_) {
			reg_ = AddOrNull(reg_, reg_->Read().bits.next_pointer);
			static_assert(sizeof(*reg_) == 4);
		}
		return *this;
	}

	ExtendedRegisterList::ExtendedRegisterList(uint64_t mmio_base,
		HCCPARAMS1_Bitmap hccp)
		: first_{ AddOrNull(reinterpret_cast<ValueType*>(mmio_base), hccp.bits.xhci_extended_capabilities_pointer) } {
	}
}


// ---- ---- ---- ---- ring.cpp ---- ---- ---- ---- //


namespace usb::xhci {
	Ring::~Ring() {
		if (buf_ != nullptr) {
			FreeMem(buf_);
		}
	}

	Error Ring::Initialize(size_t buf_size) {
		if (buf_ != nullptr) {
			FreeMem(buf_);
		}

		cycle_bit_ = true;
		write_index_ = 0;
		buf_size_ = buf_size;

		buf_ = AllocArray<TRB>(buf_size_, 64, 64 * 1024);
		if (buf_ == nullptr) {
			return MAKE_ERROR(Error::kNoEnoughMemory);
		}
		MemSet(buf_, 0, buf_size_ * sizeof(TRB));
		return MAKE_ERROR(Error::kSuccess);
	}

	void Ring::CopyToLast(const std::array<uint32_t, 4>& data) {
		for (int i = 0; i < 3; ++i) {
		  // data[0..2] must be written prior to data[3].
			buf_[write_index_].data[i] = data[i];
		}
		buf_[write_index_].data[3]
			= (data[3] & 0xfffffffeu) | static_cast<uint32_t>(cycle_bit_);
	}

	TRB* Ring::Push(const std::array<uint32_t, 4>& data) {
		auto trb_ptr = &buf_[write_index_];
		CopyToLast(data);

		++write_index_;
		if (write_index_ == buf_size_ - 1) {
			LinkTRB link{ buf_ };
			link.bits.toggle_cycle = true;
			CopyToLast(link.data);

			write_index_ = 0;
			cycle_bit_ = !cycle_bit_;
		}

		return trb_ptr;
	}

	Error EventRing::Initialize(size_t buf_size,
		InterrupterRegisterSet* interrupter) {
		if (buf_ != nullptr) {
			FreeMem(buf_);
		}

		cycle_bit_ = true;
		buf_size_ = buf_size;
		interrupter_ = interrupter;

		buf_ = AllocArray<TRB>(buf_size_, 64, 64 * 1024);
		if (buf_ == nullptr) {
			return MAKE_ERROR(Error::kNoEnoughMemory);
		}
		MemSet(buf_, 0, buf_size_ * sizeof(TRB));

		erst_ = AllocArray<EventRingSegmentTableEntry>(1, 64, 64 * 1024);
		if (erst_ == nullptr) {
			FreeMem(buf_);
			return MAKE_ERROR(Error::kNoEnoughMemory);
		}
		MemSet(erst_, 0, 1 * sizeof(EventRingSegmentTableEntry));

		erst_[0].bits.ring_segment_base_address = reinterpret_cast<uint64_t>(buf_);
		erst_[0].bits.ring_segment_size = buf_size_;

		ERSTSZ_Bitmap erstsz = interrupter_->ERSTSZ.Read();
		erstsz.SetSize(1);
		interrupter_->ERSTSZ.Write(erstsz);

		WriteDequeuePointer(&buf_[0]);

		ERSTBA_Bitmap erstba = interrupter_->ERSTBA.Read();
		erstba.SetPointer(reinterpret_cast<uint64_t>(erst_));
		interrupter_->ERSTBA.Write(erstba);

		return MAKE_ERROR(Error::kSuccess);
	}

	void EventRing::WriteDequeuePointer(TRB* p) {
		auto erdp = interrupter_->ERDP.Read();
		erdp.SetPointer(reinterpret_cast<uint64_t>(p));
		interrupter_->ERDP.Write(erdp);
	}

	void EventRing::Pop() {
		auto p = ReadDequeuePointer() + 1;

		TRB* segment_begin
			= reinterpret_cast<TRB*>(erst_[0].bits.ring_segment_base_address);
		TRB* segment_end = segment_begin + erst_[0].bits.ring_segment_size;

		if (p == segment_end) {
			p = segment_begin;
			cycle_bit_ = !cycle_bit_;
		}

		WriteDequeuePointer(p);
	}
}

// ---- ---- ---- ---- trb.cpp ---- ---- ---- ---- //

namespace usb::xhci {
	const std::array<const char*, 37> kTRBCompletionCodeToName{
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

	const std::array<const char*, 64> kTRBTypeToName{
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
	using namespace usb::xhci;

	Error RegisterCommandRing(Ring* ring, MemMapRegister<CRCR_Bitmap>* crcr) {
		CRCR_Bitmap value = crcr->Read();
		value.bits.ring_cycle_state = true;
		value.bits.command_stop = false;
		value.bits.command_abort = false;
		value.SetPointer(reinterpret_cast<uint64_t>(ring->Buffer()));
		crcr->Write(value);
		return MAKE_ERROR(Error::kSuccess);
	}

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
	/* root hub port はリセット処理をしてからアドレスを割り当てるまでは
	 * 他の処理を挟まず，そのポートについての処理だけをしなければならない．
	 * kWaitingAddressed はリセット（kResettingPort）からアドレス割り当て
	 * （kAddressingDevice）までの一連の処理の実行を待っている状態．
	 */

	std::array<volatile ConfigPhase, 256> port_config_phase{};  // index: port number

	/** kResettingPort から kAddressingDevice までの処理を実行中のポート番号．
	 * 0 ならその状態のポートがないことを示す．
	 */
	uint8_t addressing_port{ 0 };

	void InitializeSlotContext(SlotContext& ctx, Port& port) {
		ctx.bits.route_string = 0;
		ctx.bits.root_hub_port_num = port.Number();
		ctx.bits.context_entries = 1;
		ctx.bits.speed = port.Speed();
	}

	unsigned int DetermineMaxPacketSizeForControlPipe(unsigned int slot_speed) {
		switch (slot_speed) {
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

	Error ResetPort(Controller& xhc, Port& port) {
		const bool is_connected = port.IsConnected();
		Log(kDebug, "ResetPort: port.IsConnected() = %s\n",
			is_connected ? "true" : "false");

		if (!is_connected) {
			return MAKE_ERROR(Error::kSuccess);
		}

		if (addressing_port != 0) {
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

	Error EnableSlot(Controller& xhc, Port& port) {
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

	Error AddressDevice(Controller& xhc, uint8_t port_id, uint8_t slot_id) {
		Log(kDebug, "AddressDevice: port_id = %d, slot_id = %d\n", port_id, slot_id);

		xhc.GetDeviceManager()->AllocDevice(slot_id, xhc.DoorbellRegisterAt(slot_id));

		Device* dev = xhc.GetDeviceManager()->FindBySlot(slot_id);
		if (dev == nullptr) {
			return MAKE_ERROR(Error::kInvalidSlotID);
		}

		MemSet(&dev->GetInputContext()->input_control_context, 0, sizeof(InputControlContext));

		const auto ep0_dci = DeviceContextIndex(0, false);
		auto slot_ctx = dev->GetInputContext()->EnableSlotContext();
		auto ep0_ctx = dev->GetInputContext()->EnableEndpoint(ep0_dci);

		auto port = xhc.PortAt(port_id);
		InitializeSlotContext(*slot_ctx, port);

		InitializeEP0Context(
			*ep0_ctx, dev->AllocTransferRing(ep0_dci, 32),
			DetermineMaxPacketSizeForControlPipe(slot_ctx->bits.speed));

		xhc.GetDeviceManager()->LoadDCBAA(slot_id);

		port_config_phase[port_id] = ConfigPhase::kAddressingDevice;

		AddressDeviceCommandTRB addr_dev_cmd{ dev->GetInputContext(), slot_id };
		xhc.CommandRing()->Push(addr_dev_cmd);
		xhc.DoorbellRegisterAt(0)->Ring(0);

		return MAKE_ERROR(Error::kSuccess);
	}

	Error InitializeDevice(Controller& xhc, uint8_t port_id, uint8_t slot_id) {
		// Log(kDebug, "InitializeDevice: port_id = %d, slot_id = %d", port_id, slot_id);

		auto dev = xhc.GetDeviceManager()->FindBySlot(slot_id);
		if (dev == nullptr) {
			return MAKE_ERROR(Error::kInvalidSlotID);
		}

		port_config_phase[port_id] = ConfigPhase::kInitializingDevice;
		dev->StartInitialize();

		return MAKE_ERROR(Error::kSuccess);
	}

	Error CompleteConfiguration(Controller& xhc, uint8_t port_id, uint8_t slot_id) {
		// Log(kDebug, "CompleteConfiguration: port_id = %d, slot_id = %d\n", port_id, slot_id);

		auto dev = xhc.GetDeviceManager()->FindBySlot(slot_id);
		if (dev == nullptr) {
			return MAKE_ERROR(Error::kInvalidSlotID);
		}

		dev->OnEndpointsConfigured();

		port_config_phase[port_id] = ConfigPhase::kConfigured;
		return MAKE_ERROR(Error::kSuccess);
	}

	Error OnEvent(Controller& xhc, PortStatusChangeEventTRB& trb) {
		// Log(kDebug, "PortStatusChangeEvent: port_id = %d", trb.bits.port_id);
		auto port_id = trb.bits.port_id;
		auto port = xhc.PortAt(port_id);

		switch (port_config_phase[port_id]) {
		case ConfigPhase::kNotConnected:
			return ResetPort(xhc, port);
		case ConfigPhase::kResettingPort:
			return EnableSlot(xhc, port);
		default:
			plogerro("OnEvent kInvalidPhase %u", port_config_phase[port_id]);
			return MAKE_ERROR(Error::kInvalidPhase);
		}
	}

	Error OnEvent(Controller& xhc, TransferEventTRB& trb) {
		const uint8_t slot_id = trb.bits.slot_id;
		auto dev = xhc.GetDeviceManager()->FindBySlot(slot_id);
		if (dev == nullptr) {
			return MAKE_ERROR(Error::kInvalidSlotID);
		}
		if (auto err = dev->OnTransferEventReceived(trb)) {
			return err;
		}

		const auto port_id = dev->GetDeviceContext()->slot_context.bits.root_hub_port_num;
		if (dev->IsInitialized() &&
			port_config_phase[port_id] == ConfigPhase::kInitializingDevice) {
			return ConfigureEndpoints(xhc, *dev);
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error OnEvent(Controller& xhc, CommandCompletionEventTRB& trb) {
		const auto issuer_type = trb.Pointer()->bits.trb_type;
		const auto slot_id = trb.bits.slot_id;
		// Log(kDebug, "CommandCompletionEvent: slot_id = %d, issuer = %s",
		// 	trb.bits.slot_id, kTRBTypeToName[issuer_type]);

		if (issuer_type == EnableSlotCommandTRB::Type) {
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

			auto port_id = dev->GetDeviceContext()->slot_context.bits.root_hub_port_num;

			if (port_id != addressing_port) {
				return MAKE_ERROR(Error::kInvalidPhase);
			}
			if (port_config_phase[port_id] != ConfigPhase::kAddressingDevice) {
				return MAKE_ERROR(Error::kInvalidPhase);
			}

			addressing_port = 0;
			for (int i = 0; i < port_config_phase.size(); ++i) {
				if (port_config_phase[i] == ConfigPhase::kWaitingAddressed) {
					auto port = xhc.PortAt(i);
					if (auto err = ResetPort(xhc, port); err) {
						return err;
					}
					break;
				}
			}

			return InitializeDevice(xhc, port_id, slot_id);
		}
		else if (issuer_type == ConfigureEndpointCommandTRB::Type) {
			auto dev = xhc.GetDeviceManager()->FindBySlot(slot_id);
			if (dev == nullptr) {
				return MAKE_ERROR(Error::kInvalidSlotID);
			}

			auto port_id = dev->GetDeviceContext()->slot_context.bits.root_hub_port_num;
			if (port_config_phase[port_id] != ConfigPhase::kConfiguringEndpoints) {
				return MAKE_ERROR(Error::kInvalidPhase);
			}

			return CompleteConfiguration(xhc, port_id, slot_id);
		}

		return MAKE_ERROR(Error::kInvalidPhase);
	}

	void RequestHCOwnership(uintptr_t mmio_base, HCCPARAMS1_Bitmap hccp) {
		ExtendedRegisterList extregs{ mmio_base, hccp };

		auto ext_usblegsup = std::find_if(
			extregs.begin(), extregs.end(),
			[](auto& reg) { return reg.Read().bits.capability_id == 1; });

		if (ext_usblegsup == extregs.end()) {
			return;
		}

		auto& reg =
			reinterpret_cast<MemMapRegister<USBLEGSUP_Bitmap>&>(*ext_usblegsup);
		auto r = reg.Read();
		if (r.bits.hc_os_owned_semaphore) {
			return;
		}

		r.bits.hc_os_owned_semaphore = 1;
		Log(kDebug, "waiting until OS owns xHC...\n");
		reg.Write(r);

		do {
			r = reg.Read();
		} while (r.bits.hc_bios_owned_semaphore ||
			!r.bits.hc_os_owned_semaphore);
		Log(kDebug, "OS has owned xHC\n");
	}
}

namespace usb::xhci {

	Controller::Controller(uintptr_t mmio_base)
		: mmio_base_{ mmio_base },
		cap_{ reinterpret_cast<CapabilityRegisters*>(mmio_base) },
		op_{ reinterpret_cast<OperationalRegisters*>(
			mmio_base + cap_->CAPLENGTH.Read()) },
		max_ports_{ static_cast<uint8_t>(
			cap_->HCSPARAMS1.Read().bits.max_ports) } {
	}

	Error Controller::Initialize() {
		if (auto err = devmgr_.Initialize(kDeviceSize)) {
			return err;
		}

		RequestHCOwnership(mmio_base_, cap_->HCCPARAMS1.Read());

		auto usbcmd = op_->USBCMD.Read();
		usbcmd.bits.interrupter_enable = false;
		usbcmd.bits.host_system_error_enable = false;
		usbcmd.bits.enable_wrap_event = false;
		// Host controller must be halted before resetting it.
		if (!op_->USBSTS.Read().bits.host_controller_halted) {
			usbcmd.bits.run_stop = false;  // stop
		}

		op_->USBCMD.Write(usbcmd);
		while (!op_->USBSTS.Read().bits.host_controller_halted);

		// Reset controller
		usbcmd = op_->USBCMD.Read();
		usbcmd.bits.host_controller_reset = true;
		op_->USBCMD.Write(usbcmd);
		while (op_->USBCMD.Read().bits.host_controller_reset);
		while (op_->USBSTS.Read().bits.controller_not_ready);

		Log(kDebug, "MaxSlots: %u", cap_->HCSPARAMS1.Read().bits.max_device_slots);
		// Set "Max Slots Enabled" field in CONFIG.
		auto config = op_->CONFIG.Read();
		config.bits.max_device_slots_enabled = kDeviceSize;
		op_->CONFIG.Write(config);

		auto hcsparams2 = cap_->HCSPARAMS2.Read();
		const uint16_t max_scratchpad_buffers =
			hcsparams2.bits.max_scratchpad_buffers_low
			| (hcsparams2.bits.max_scratchpad_buffers_high << 5);
		if (max_scratchpad_buffers > 0) {
			auto scratchpad_buf_arr = AllocArray<void*>(max_scratchpad_buffers, 64, 4096);
			for (int i = 0; i < max_scratchpad_buffers; ++i) {
				scratchpad_buf_arr[i] = AllocMem(4096, 4096, 4096);
				Log(kDebug, "scratchpad buffer array %d = %p\n",
					i, scratchpad_buf_arr[i]);
			}
			devmgr_.DeviceContexts()[0] = reinterpret_cast<DeviceContext*>(scratchpad_buf_arr);
			Log(kInfo, "wrote scratchpad buffer array %p to dev ctx array 0\n",
				scratchpad_buf_arr);
		}

		DCBAAP_Bitmap dcbaap{};
		dcbaap.SetPointer(reinterpret_cast<uint64_t>(devmgr_.DeviceContexts()));
		op_->DCBAAP.Write(dcbaap);

		auto primary_interrupter = &InterrupterRegisterSets()[0];
		if (auto err = cr_.Initialize(32)) {
			return err;
		}
		if (auto err = RegisterCommandRing(&cr_, &op_->CRCR)) {
			return err;
		}
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

	Error Controller::Run() {
	  // Run the controller
		auto usbcmd = op_->USBCMD.Read();
		usbcmd.bits.run_stop = true;
		op_->USBCMD.Write(usbcmd);
		op_->USBCMD.Read();

		while (op_->USBSTS.Read().bits.host_controller_halted);

		return MAKE_ERROR(Error::kSuccess);
	}

	DoorbellRegister* Controller::DoorbellRegisterAt(uint8_t index) {
		return &DoorbellRegisters()[index];
	}

	Error ConfigurePort(Controller& xhc, Port& port) {
		if (port_config_phase[port.Number()] == ConfigPhase::kNotConnected) {
			return ResetPort(xhc, port);
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error ConfigureEndpoints(Controller& xhc, Device& dev) {
		const auto configs = dev.EndpointConfigs();
		const auto len = dev.NumEndpointConfigs();

		MemSet(&dev.GetInputContext()->input_control_context, 0, sizeof(InputControlContext));
		MemCopyN(&dev.GetInputContext()->slot_context, &dev.GetDeviceContext()->slot_context, sizeof(SlotContext));

		auto slot_ctx = dev.GetInputContext()->EnableSlotContext();
		slot_ctx->bits.context_entries = 31;
		const auto port_id{ dev.GetDeviceContext()->slot_context.bits.root_hub_port_num };
		const int port_speed{ xhc.PortAt(port_id).Speed() };
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
			auto ep_ctx = dev.GetInputContext()->EnableEndpoint(ep_dci);
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

			auto tr = dev.AllocTransferRing(ep_dci, 32);
			ep_ctx->SetTransferRingBuffer(tr->Buffer());

			ep_ctx->bits.dequeue_cycle_state = 1;
			ep_ctx->bits.max_primary_streams = 0;
			ep_ctx->bits.mult = 0;
			ep_ctx->bits.error_count = 3;
		}

		port_config_phase[port_id] = ConfigPhase::kConfiguringEndpoints;

		ConfigureEndpointCommandTRB cmd{ dev.GetInputContext(), dev.SlotID() };
		xhc.CommandRing()->Push(cmd);
		xhc.DoorbellRegisterAt(0)->Ring(0);

		return MAKE_ERROR(Error::kSuccess);
	}

	Error ProcessEvent(Controller& xhc) {
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
}

#endif

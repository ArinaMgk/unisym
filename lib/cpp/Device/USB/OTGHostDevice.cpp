// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [Device.USB] OTG Host Device Bridge
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

#include "../../../../inc/cpp/Device/USB/OTGHostDevice.hpp"
#if defined(_MCU_STM32H7x)
#include "../../../../inc/cpp/Device/USB/USBHost-Hub.hpp"
#endif

namespace uni::device::SpaceUSB {
#if defined(_MCU_STM32H7x)

	OTGHostDevice* OTGHostDevice::ch_owner_[16] = {};

	namespace {
		class ControlIrqGuard {
			stduint mask_;
		public:
			ControlIrqGuard() {
				_ASM volatile("mrs %0, primask\n\tcpsid i" : "=r"(mask_) : : "memory");
			}
			~ControlIrqGuard() {
				if (!mask_) _ASM volatile("cpsie i" : : : "memory");
			}
		};

		const stduint kChildSettleTicks = 100;
		// wait limit of one enumeration round, and how many rounds are tried before giving the port up
		const stduint kChildEnumTimeoutMs = 4000;
		const byte kChildEnumRetryLimit = 3;

		// the bus device list: slots never shift, so a tick never walks over a moved bridge
		OTGHostDevice* s_bridges[OTGHostDevice::kMaxBridges] = {};
		// bit n set while address n is taken; address 0 is never handed out
		stduint s_addr_used[4] = {};
		// the bus millisecond clock: getCurrentFrame() wraps every 14 bits, so timeouts count these
		stduint s_bus_tick = 0;
		uint32 s_control_submission = 0;
	}

	OTGHostDevice* OTGHostDevice::OwnerOfChannel(byte ch_num) {
		return ch_num < 16 ? ch_owner_[ch_num] : nullptr;
	}

	void OTGHostDevice::SetChannelOwner(byte ch_num, OTGHostDevice* dev) {
		if (ch_num < 16) ch_owner_[ch_num] = dev;
	}

	void OTGHostDevice::RegisterBridge(OTGHostDevice* dev) {
		if (dev == nullptr) return;
		for (int i = 0; i < kMaxBridges; i++) {
			if (s_bridges[i] == dev) return;
		}
		for (int i = 0; i < kMaxBridges; i++) {
			if (s_bridges[i] == nullptr) {
				s_bridges[i] = dev;
				return;
			}
		}
		// more devices than the bus table holds: the extra one is simply not ticked
	}

	void OTGHostDevice::UnregisterBridge(OTGHostDevice* dev) {
		for (int i = 0; i < kMaxBridges; i++) {
			if (s_bridges[i] == dev) s_bridges[i] = nullptr;
		}
	}

	OTGHostDevice* OTGHostDevice::BridgeAt(int index) {
		return (index >= 0 && index < kMaxBridges) ? s_bridges[index] : nullptr;
	}

	int OTGHostDevice::NumBridges() {
		int n = 0;
		for (int i = 0; i < kMaxBridges; i++) {
			if (s_bridges[i]) n++;
		}
		return n;
	}

	void OTGHostDevice::ReserveDeviceAddress(byte address) {
		if (address == 0 || address > 127) return;
		s_addr_used[address >> 5] |= (1u << (address & 31));
	}

	void OTGHostDevice::FreeDeviceAddress(byte address) {
		if (address == 0 || address > 127) return;
		s_addr_used[address >> 5] &= ~(1u << (address & 31));
	}

	// address 1 belongs to whatever hangs on the root port
	byte OTGHostDevice::AllocDeviceAddress() {
		for (byte a = 2; a <= 127; a++) {
			if (s_addr_used[a >> 5] & (1u << (a & 31))) continue;
			ReserveDeviceAddress(a);
			return a;
		}
		return 0;// the bus is out of addresses
	}

	void* OTGHostDevice::operator new(size_t size) {
		return uni_hostenv_allocator->allocate(size);
	}

	void OTGHostDevice::operator delete(void* ptr) noexcept {
		uni_hostenv_allocator->deallocate(ptr);
	}

	OTGHostDevice::OTGHostDevice(HCD& hcd, byte dev_address, byte speed, byte assigned_address)
		: hcd_{ hcd }, dev_address_{ dev_address }, speed_{ speed } {
		for (byte i = 0; i < 16; i++) ep_of_ch_[i] = kNoChannel;
		for (byte i = 0; i < 32; i++) ch_of_ep_[i] = kNoChannel;
		// the address this device will answer is reserved before anything is sent
		assigned_addr_ = assigned_address ? assigned_address : kDefaultDeviceAddress;
		SetAssignedAddress(assigned_addr_);
		ReserveDeviceAddress(assigned_addr_);
		last_service_tick_ = s_bus_tick;
		RegisterBridge(this);
		ctrl_ch_ = hcd_.AllocChannel();
		if (ctrl_ch_ == kNoChannel) return;// no channel left: every request below fails
		SetChannelOwner(ctrl_ch_, this);
		// EP0 starts at 8 bytes: ControlIn() re-programs it from the first reply
		hcd_.InitializeHostChannel(ctrl_ch_, 0x00, dev_address, speed, 0, 8);// EP_TYPE_CTRL
	}

	// AKA USBH_FreePipe: the channels of this device go back to the controller pool
	OTGHostDevice::~OTGHostDevice() {
		// the downstream devices go first: they are part of this very bus
		while (num_children_ > 0) {
			const int last = num_children_ - 1;
			OTGHostDevice* child = children_[last];
			children_[last] = nullptr;
			child_port_[last] = 0;
			--num_children_;
			if (child == nullptr) continue;
			// cut the parent link so the child does not ask a dying bridge for anything
			child->parent_ = nullptr;
			child->parent_port_ = 0;
			delete child;
		}
		hub_addressing_child_ = nullptr;
		hub_addressing_ = false;
		hub_addressing_port_ = 0;
		UnregisterBridge(this);
		FreeDeviceAddress(assigned_addr_);
		for (byte ch = 0; ch < 16; ch++) {
			if (ch_owner_[ch] != this) continue;
			SetChannelOwner(ch, nullptr);
			hcd_.HaltHostChannel(ch);
			hcd_.FreeChannel(ch);
		}
	}

	// The device needs the 2 ms recovery window of SET_ADDRESS before it answers
	void OTGHostDevice::OnDeviceAddressChanged(byte address) {
		const stduint start = hcd_.getCurrentFrame();
		stduint spin = 0;
		// the frame counter is the only clock available here: never spin forever on it
		while ((stduint)(hcd_.getCurrentFrame() - start) < 3 && spin++ < 200000) {}
		dev_address_ = address;
		// endpoint 0 keeps the packet size learned from the device descriptor
		hcd_.InitializeHostChannel(ctrl_ch_, 0x00, dev_address_, speed_, 0, hcd_.hc[ctrl_ch_].max_packet);
	}

	Error OTGHostDevice::ControlIn(EndpointID ep_id, SetupData setup_data,
		void* buf, int len, ClassDriver* issuer) {
		if (transfers_stopping_) return MAKE_ERROR(Error::InvalidPhase);
		if (ep_id.Number() != 0) {
			return MAKE_ERROR(Error::InvalidEndpointNumber);
		}
		if (ctrl_ch_ == kNoChannel) return MAKE_ERROR(Error::NoEnoughMemory);
		if (ctrl_stage_ != ControlStage::Idle && ctrl_stage_ != ControlStage::Done) {
			return MAKE_ERROR(Error::Full);
		}
		if (auto err = USBHostDevice::ControlIn(ep_id, setup_data, buf, len, issuer)) {
			return err;
		}
		// stage machine: SETUP(OUT) -> DATA(IN) -> STATUS(OUT)
		ctrl_ep_id_ = ep_id;
		ctrl_setup_ = setup_data;
		ctrl_buf_ = buf;
		ctrl_len_ = len;
		ctrl_len_full_ = ctrl_len_;
		ctrl_setup_.length = static_cast<uint16>(ctrl_len_);
		ctrl_xfer_count_ = 0;
		mps_probe_ = false;
		ch_nak_retry_[ctrl_ch_] = 0;
		ctrl_err_retry_ = 0;
		ctrl_submit_failures_ = 0;
		if (!mps_known_ && setup_data.request == static_cast<uint8>(StandardRequest::GetDescriptor) &&
			(setup_data.value >> 8) == DeviceDescriptor::kType && ctrl_len_ > 8) {
			// the 8-byte header fits every bMaxPacketSize0
			mps_probe_ = true;
			ctrl_len_ = 8;
			ctrl_setup_.length = 8;
		}
		// SETUP stage: 8-byte setup packet, OUT, PID SETUP (token 0)
		ctrl_retry_pending_ = false;
		ctrl_stage_ = ControlStage::Setup;
		ResubmitControlStage();
		return MAKE_ERROR(Error::Success);
	}

	Error OTGHostDevice::ControlOut(EndpointID ep_id, SetupData setup_data,
		const void* buf, int len, ClassDriver* issuer) {
		if (transfers_stopping_) return MAKE_ERROR(Error::InvalidPhase);
		if (ep_id.Number() != 0) {
			return MAKE_ERROR(Error::InvalidEndpointNumber);
		}
		if (ctrl_ch_ == kNoChannel) return MAKE_ERROR(Error::NoEnoughMemory);
		if (ctrl_stage_ != ControlStage::Idle && ctrl_stage_ != ControlStage::Done) {
			return MAKE_ERROR(Error::Full);
		}
		if (auto err = USBHostDevice::ControlOut(ep_id, setup_data, buf, len, issuer)) {
			return err;
		}
		ctrl_ep_id_ = ep_id;
		ctrl_setup_ = setup_data;
		ctrl_buf_ = const_cast<void*>(buf);
		ctrl_len_ = len;
		ch_nak_retry_[ctrl_ch_] = 0;
		ctrl_err_retry_ = 0;
		ctrl_submit_failures_ = 0;
		ctrl_retry_pending_ = false;
		ctrl_stage_ = ControlStage::Setup;
		ResubmitControlStage();
		return MAKE_ERROR(Error::Success);
	}

	// called from the main loop, far more often than a frame: the transfer goes out the moment the window opens
	void OTGHostDevice::Service() {
		if (transfers_stopping_) return;
		// a control stage waiting for the bus goes first, a periodic poll can wait for the next frame
		if (ctrl_retry_pending_) {
			if (ctrl_halt_pending_ || ctrl_completion_pending_ || s_bus_tick == ctrl_retry_tick_) return;
			ctrl_retry_pending_ = false;
			ResubmitControlStage();
			return;
		}
		ProcessDeferredInterrupts();
	}
	void OTGHostDevice::PollBus() {
		OTGHostDevice* list[kMaxBridges] = {};
		int count = 0;
		for (int i = 0; i < kMaxBridges; i++) {
			if (s_bridges[i]) list[count++] = s_bridges[i];
		}
		for (int i = 0; i < count; i++) {
			// a hub may take a downstream device away while the list is walked
			bool alive = false;
			for (int j = 0; j < kMaxBridges; j++) {
				if (s_bridges[j] == list[i]) { alive = true; break; }
			}
			if (!alive) continue;
			list[i]->Service();
		}
	}

	Error OTGHostDevice::InterruptIn(EndpointID ep_id, void* buf, int len) {
		if (transfers_stopping_) return MAKE_ERROR(Error::InvalidPhase);
		if (auto err = USBHostDevice::InterruptIn(ep_id, buf, len)) {
			return err;
		}
		const byte ch = ChannelOfEndpoint(ep_id);
		if (!ch) return MAKE_ERROR(Error::InvalidEndpointNumber);
		ch_xfer_base_[ch] = static_cast<byte*>(buf);
		ch_xfer_len_[ch] = static_cast<uint16>(len);
		if (ch_poll_interval_[ch]) {
			// bInterval counts milliseconds on LS/FS, and the frame counter of this core is not a millisecond
			// clock: pace the polls with the bus tick so a slow frame counter cannot throttle them
			const stduint now_tick = s_bus_tick;
			if ((stduint)(now_tick - ch_poll_last_[ch]) < ch_poll_interval_[ch]) {
				ch_deferred_buf_[ch] = static_cast<byte*>(buf);
				ch_deferred_len_[ch] = static_cast<uint16>(len);
				return MAKE_ERROR(Error::Success);
			}
			ch_poll_last_[ch] = now_tick;
		}
		ch_waiting_since_[ch] = s_bus_tick;
		hcd_.SubmitRequest(ch, 1, 3, 1, static_cast<byte*>(buf), static_cast<uint16>(len), 0);
		return MAKE_ERROR(Error::Success);
	}

	// Submit every interrupt poll that was held back and whose interval is over
	void OTGHostDevice::ProcessDeferredInterrupts() {
		if (transfers_stopping_) return;
		const stduint now_tick = s_bus_tick;
		for (byte ch = 0; ch < 16; ch++) {
			if (ch_deferred_buf_[ch] == nullptr) continue;
			if ((stduint)(now_tick - ch_poll_last_[ch]) < ch_poll_interval_[ch]) continue;
			byte* buf = ch_deferred_buf_[ch];
			const uint16 len = ch_deferred_len_[ch];
			ch_deferred_buf_[ch] = nullptr;
			ch_poll_last_[ch] = now_tick;
			ch_waiting_since_[ch] = s_bus_tick;
			hcd_.SubmitRequest(ch, 1, 3, 1, buf, len, 0);
		}
	}

	Error OTGHostDevice::InterruptOut(EndpointID ep_id, void* buf, int len) {
		if (transfers_stopping_) return MAKE_ERROR(Error::InvalidPhase);
		if (auto err = USBHostDevice::InterruptOut(ep_id, buf, len)) {
			return err;
		}
		const byte ch = ChannelOfEndpoint(ep_id);
		if (!ch) return MAKE_ERROR(Error::InvalidEndpointNumber);
		ch_xfer_base_[ch] = static_cast<byte*>(buf);
		ch_xfer_len_[ch] = static_cast<uint16>(len);
		ch_waiting_since_[ch] = s_bus_tick;
		hcd_.SubmitRequest(ch, 0, 3, 1, static_cast<byte*>(buf), static_cast<uint16>(len), 0);
		return MAKE_ERROR(Error::Success);
	}

	// the bulk pipe of a host class driver
	Error OTGHostDevice::BulkTransfer(EndpointID ep_id, bool dir_in, void* buf, int len) {
		if (transfers_stopping_) return MAKE_ERROR(Error::InvalidPhase);
		if (!buf || len <= 0 || stduint(len) > kMaxBulkTransferBytes ||
			stduint(len) > 0xFFFFu || ep_id.IsIn() != dir_in)
			return MAKE_ERROR(Error::IndexOutOfRange);
		const byte ch = ChannelOfEndpoint(ep_id);
		if (ch >= hcd_.host_channels || ch >= numsof(hcd_.hc) || hcd_.hc[ch].ep_type != 2)
			return MAKE_ERROR(Error::InvalidEndpointNumber);
		const stduint mps = hcd_.hc[ch].max_packet;
		// STM32H7 LL USB_HC_StartXfer limits a host request to 256 packets.
		if (!mps || stduint(len) > 256u * mps) return MAKE_ERROR(Error::IndexOutOfRange);
		ch_xfer_base_[ch] = static_cast<byte*>(buf);
		ch_xfer_len_[ch] = static_cast<uint16>(len);
		ch_nak_retry_[ch] = 0;
		// token 1 = DATA1; BULK takes the PID from the per-channel toggle
		if (!hcd_.SubmitRequest(ch, dir_in ? 1 : 0, 2, 1, static_cast<byte*>(buf),
			static_cast<uint16>(len), 0)) {
			ch_xfer_base_[ch] = nullptr;
			ch_xfer_len_[ch] = 0;
			return MAKE_ERROR(Error::TransferFailed);
		}
		return MAKE_ERROR(Error::Success);
	}

	stduint OTGHostDevice::getBulkTransferLimit() {
		stduint limit = kMaxBulkTransferBytes;
		bool have_in = false, have_out = false;
		for (int i = 0; i < NumEndpointConfigs(); ++i) {
			const auto& endpoint = EndpointConfigs()[i];
			if (endpoint.ep_type != EndpointType::kBulk) continue;
			if (endpoint.max_packet_size <= 0) return 0;
			limit = minof(limit, stduint(256) * endpoint.max_packet_size);
			if (endpoint.ep_id.IsIn()) have_in = true;
			else have_out = true;
		}
		return have_in && have_out ? limit : 0;
	}

	bool OTGHostDevice::StopTransfers() {
		ControlIrqGuard guard;
		if (!transfers_stopping_) {
			transfers_stopping_ = true;
			ctrl_retry_pending_ = ctrl_waiting_ = false;
			ctrl_completion_pending_ = ctrl_halt_pending_ = false;
			ctrl_stage_ = ControlStage::Done;
			for (byte ch = 0; ch < hcd_.host_channels && ch < numsof(hcd_.hc); ++ch) {
				if (OwnerOfChannel(ch) != this) continue;
				ch_deferred_buf_[ch] = nullptr;
				ch_waiting_since_[ch] = 0;
				if (hcd_.ChannelReg(ch, 0x000).bitof(USB_OTG_HCCHAR_CHENA_Pos))
					(void)hcd_.HaltHostChannel(ch);
			}
		}
		for (byte ch = 0; ch < hcd_.host_channels && ch < numsof(hcd_.hc); ++ch) {
			if (OwnerOfChannel(ch) == this &&
				hcd_.ChannelReg(ch, 0x000).bitof(USB_OTG_HCCHAR_CHENA_Pos)) return false;
		}
		for (byte ch = 0; ch < hcd_.host_channels && ch < numsof(hcd_.hc); ++ch) {
			if (OwnerOfChannel(ch) != this) continue;
			hcd_.ChannelReg(ch, 0x00C) = 0;
			hcd_.ChannelReg(ch, 0x008) = 0xFFFFFFFFu;
			hcd_.hc[ch].xfer_buff = nullptr;
			hcd_.hc[ch].xfer_len = hcd_.hc[ch].xfer_count = 0;
			ch_xfer_base_[ch] = nullptr;
			ch_xfer_len_[ch] = 0;
			OTG::ClearHaltPending(ch);
		}
		ctrl_buf_ = nullptr;
		return true;
	}

	// the hub class driver reports one downstream port here; only here can a child get an address
	Error OTGHostDevice::OnHubPortStatusReceived(uint8 port_num, uint16 status,
		uint16 change, uint8 speed_id) {
		(void)speed_id;
		if (port_num == 0 || port_num > 16) {
			return MAKE_ERROR(Error::InvalidEndpointNumber);
		}
		hub_port_status_[port_num - 1] = status;
		hub_port_change_[port_num - 1] |= change;
		hub_port_event_count_++;
		// A device that went away: give its channels and its USB address back
		if ((change & kHubPortStatusConnect) && !(status & kHubPortStatusConnect)) {
			if (DropChildDevice(byte(port_num))) child_down_count_++;
			return MAKE_ERROR(Error::Success);
		}
		if (!(status & kHubPortStatusConnect)) {
			return MAKE_ERROR(Error::Success);
		}
		if (ChildOfPort(byte(port_num))) {
			return MAKE_ERROR(Error::Success);// this port is already brought up
		}
		// the hub has not finished resetting: until PORT_ENABLE the device does not answer
		if (!(status & kHubPortStatusEnable)) {
			return MAKE_ERROR(Error::Success);
		}
		// a fresh device answers at address 0 and only one may sit there
		if (hub_addressing_) {
			return MAKE_ERROR(Error::Success);
		}
		return StartChildDevice(byte(port_num), status);
	}

	// A hub class request must not share a frame with a downstream device holding the bus
	bool OTGHostDevice::ChildBusy() {
		for (int i = 0; i < num_children_; i++) {
			OTGHostDevice* c = children_[i];
			if (c == nullptr) continue;
			if (c->ctrl_stage_ != ControlStage::Idle && c->ctrl_stage_ != ControlStage::Done) return true;
			if (c->ctrl_retry_pending_) return true;
			for (byte ch = 0; ch < 16; ch++) {
				if (c->hcd_.hc[ch].dev_addr != c->dev_address_) continue;
				if ((stduint)c->hcd_.ChannelReg(ch, 0x000) & USB_OTG_HCCHAR_CHENA) return true;
			}
		}
		return false;
	}

	OTGHostDevice* OTGHostDevice::ChildOfPort(byte port) const {
		for (int i = 0; i < num_children_; i++) {
			if (child_port_[i] == port) return children_[i];
		}
		return nullptr;
	}

	// which downstream port may be reset next: at most one per round, none while one is being addressed
	byte OTGHostDevice::NextHubPortToReset() {
		if (hub_addressing_) return 0;
		const byte ports = HubNumPorts();
		for (byte port = 1; port <= ports && port <= 16; port++) {
			const uint16 status = hub_port_status_[port - 1];
			if (!(status & kHubPortStatusConnect)) continue;
			if (status & kHubPortStatusEnable) continue;// already enabled: the transport took it
			if (ChildOfPort(port)) continue;
			return port;
		}
		return 0;
	}

	// build the device that just showed up on a hub port
	Error OTGHostDevice::StartChildDevice(byte port, uint16 port_status) {
		if (num_children_ >= kMaxHubChildren) {
			child_fail_count_++;
			return MAKE_ERROR(Error::NoEnoughMemory);
		}
		const byte address = AllocDeviceAddress();
		if (address == 0) {
			child_fail_count_++;
			return MAKE_ERROR(Error::NoEnoughMemory);
		}
		// PORT_LOW_SPEED is status bit 9; HCD speed codes are 1 = FS, 2 = LS
		const byte speed = (port_status & kHubPortStatusLowSpeed) ? byte(2) : byte(1);
		// the child starts at address 0: the hub forwards those tokens to the enabled port
		OTGHostDevice* child = new OTGHostDevice(hcd_, 0, speed, address);
		if (child == nullptr) {
			FreeDeviceAddress(address);
			child_fail_count_++;
			return MAKE_ERROR(Error::NoEnoughMemory);
		}
		if (child->ControlChannel() == kNoChannel) {
			delete child;// its destructor gives the address back too
			child_fail_count_++;
			return MAKE_ERROR(Error::NoEnoughMemory);
		}
		child->parent_ = this;
		child->parent_port_ = port;
		children_[num_children_] = child;
		child_port_[num_children_] = port;
		num_children_++;
		hub_addressing_ = true;
		hub_addressing_port_ = port;
		hub_addressing_child_ = child;
		hub_addressing_since_ = s_bus_tick;
		hub_addressing_retry_ = 0;
		// hold the first transaction back until the reset recovery time is over (started by TickAddressing)
		hub_child_started_ = false;
		child_up_count_++;
		return MAKE_ERROR(Error::Success);
	}

	// the downstream device is gone: its channels and its USB address go back to the bus
	bool OTGHostDevice::DropChildDevice(byte port) {
		for (int i = 0; i < num_children_; i++) {
			if (child_port_[i] != port) continue;
			OTGHostDevice* child = children_[i];
			const int last = num_children_ - 1;
			children_[i] = children_[last];
			child_port_[i] = child_port_[last];
			children_[last] = nullptr;
			child_port_[last] = 0;
			--num_children_;
			if (hub_addressing_child_ == child) {
				hub_addressing_child_ = nullptr;
				hub_addressing_ = false;
				hub_addressing_port_ = 0;
				hub_child_started_ = false;
				hub_addressing_retry_ = 0;
			}
			if (child == nullptr) return false;
			child->parent_ = nullptr;
			child->parent_port_ = 0;
			delete child;
			return true;
		}
		return false;
	}

	// the next port waits until the device on this one has an address of its own
	void OTGHostDevice::TickAddressing() {
		if (!hub_addressing_) return;
		// the reset recovery time (USB 2.0 TDRSTR = 10ms) must pass before the first transaction
		if (!hub_child_started_) {
			if ((s_bus_tick - hub_addressing_since_) < kChildSettleTicks) return;
			hub_child_started_ = true;
			hub_addressing_since_ = s_bus_tick;
			if (hub_addressing_child_) hub_addressing_child_->StartInitialize();
			return;
		}
		if (hub_addressing_child_ && hub_addressing_child_->IsInitialized()) {
			hub_addressing_ = false;
			hub_addressing_port_ = 0;
			hub_addressing_child_ = nullptr;
			hub_child_started_ = false;
			hub_addressing_retry_ = 0;
			return;
		}
		if ((s_bus_tick - hub_addressing_since_) <= kChildEnumTimeoutMs) return;
		// the enumeration did not finish: try the whole round again a few times before giving up
		hub_addressing_since_ = s_bus_tick;
		if (hub_addressing_retry_ < kChildEnumRetryLimit && hub_addressing_child_) {
			hub_addressing_retry_++;
			hub_addressing_child_->StartInitialize();
			return;
		}
		const byte port = hub_addressing_port_;
		hub_addressing_ = false;
		hub_addressing_port_ = 0;
		hub_addressing_child_ = nullptr;
		hub_child_started_ = false;
		hub_addressing_retry_ = 0;
		// the device never answered: give the port up so the hub may retry it
		child_fail_count_++;
		DropChildDevice(port);
	}

	// one millisecond of this bridge: the bus transfers are driven from PollBus instead
	void OTGHostDevice::Tick() {
		Tick(1, false);
	}

	void OTGHostDevice::Tick(uint32 elapsed_ms, bool elapsed_time) {
		if (transfers_stopping_) return;
		TickAddressing();
		if (elapsed_time) ProcessDelayed(elapsed_ms);
		else ProcessDelayed();
		// the caller may tick without ever calling PollBus: a held-back poll or a control stage waiting to be
		// re-sent must not park forever, the poll interval and the retry both live on this tick
		Service();
		// Stop a timed-out stage before reusing its channel; a queued completion wins the race.
		if (ctrl_waiting_ && ctrl_ch_ != kNoChannel
			&& (s_bus_tick - ctrl_submit_tick_) > kCtrlWatchdogTicks) {
			ControlIrqGuard guard;
			const stduint pending = stduint(hcd_.ChannelReg(ctrl_ch_, 0x008)) &
				stduint(hcd_.ChannelReg(ctrl_ch_, 0x00C));
			if (ctrl_waiting_ && !ctrl_completion_pending_ && !ctrl_halt_pending_ && !pending) {
				ctrl_halt_pending_ = true;
				if (hcd_.HaltHostChannel(ctrl_ch_)) ctrl_waiting_ = false;
				else ctrl_halt_pending_ = false;
			}
		}
		// the same silent loss parks an interrupt or bulk endpoint: re-program the left-over channel and hand the
		// layer a NAK-like completion, so the class driver's own poll re-arm runs again
		for (byte ch = 0; ch < 16; ch++) {
			if (ch == ctrl_ch_ || ch_waiting_since_[ch] == 0) continue;
			if ((s_bus_tick - ch_waiting_since_[ch]) <= kChannelWatchdogTicks) continue;
			ch_waiting_since_[ch] = 0;
			const byte ep_addr = ep_of_ch_[ch];
			const byte hc_epnum = static_cast<byte>((ep_addr >> 1) | ((ep_addr & 0x01) ? 0x80 : 0x00));
			hcd_.InitializeHostChannel(ch, hc_epnum, dev_address_, speed_,
				hcd_.hc[ch].ep_type, hcd_.hc[ch].max_packet);
			OnChannelURBCompleted(ch, URBState::NotReady);
		}
	}

	// tick every device on the bus
	void OTGHostDevice::TickAll() {
		AdvanceTime(1);
		ServiceAll(1, false);
	}

	void OTGHostDevice::AdvanceTime(uint32 elapsed_ms) {
		s_bus_tick += elapsed_ms;
	}

	void OTGHostDevice::ServiceAll(uint32 elapsed_ms) {
		ServiceAll(elapsed_ms, true);
	}

	void OTGHostDevice::ServiceAll(uint32 elapsed_ms, bool elapsed_time) {
		OTGHostDevice* list[kMaxBridges] = {};
		int count = 0;
		for (int i = 0; i < kMaxBridges; i++) {
			if (s_bridges[i]) list[count++] = s_bridges[i];
		}
		for (int i = 0; i < count; i++) {
			// a hub tick may take a downstream device away: never touch what is gone
			bool alive = false;
			for (int j = 0; j < kMaxBridges; j++) {
				if (s_bridges[j] == list[i]) { alive = true; break; }
			}
			if (!alive) continue;
			// A bridge created by a completion starts at the current bus time.
			const uint32 elapsed = uint32(s_bus_tick - list[i]->last_service_tick_);
			list[i]->last_service_tick_ = s_bus_tick;
			list[i]->Tick(elapsed_time ? elapsed : elapsed_ms, elapsed_time);
		}
	}

	// AKA USBH_AllocPipe: one host channel per endpoint address
	Error OTGHostDevice::ConfigureEndpoints() {
		if (endpoints_configured_) {
			return MAKE_ERROR(Error::Success);
		}
		for (int i = 0; i < NumEndpointConfigs(); ++i) {
			const EndpointConfig& conf = EndpointConfigs()[i];
			const byte ch = hcd_.AllocChannel();
			if (ch == kNoChannel) break;// the controller has no channel left
			const byte ep_num = static_cast<byte>(conf.ep_id.Number());
			// keyed by the EndpointID address (2 * number + direction), what class drivers pass around
			const byte ep_id_addr = static_cast<byte>(conf.ep_id.Address() & 0x1F);
			const byte ep_addr = static_cast<byte>(
				conf.ep_id.IsIn() ? (ep_num | 0x80) : ep_num);
			const byte ep_type = static_cast<byte>(conf.ep_type);// kControl=0, kIsochronous=1, kBulk=2, kInterrupt=3
			ch_of_ep_[ep_id_addr] = ch;
			ep_of_ch_[ch] = ep_id_addr;
			SetChannelOwner(ch, this);
			if (ep_type == 3) {// kInterrupt: bInterval counts frames (milliseconds) on LS/FS
				ch_poll_interval_[ch] = conf.interval ? static_cast<byte>(conf.interval) : 1;
			}
			hcd_.InitializeHostChannel(ch, ep_addr, dev_address_, speed_,
				ep_type, static_cast<uint16>(conf.max_packet_size));
		}
		endpoints_configured_ = true;
		return MAKE_ERROR(Error::Success);
	}

	// Channel of an endpoint address, programming the channels on first use.
	byte OTGHostDevice::ChannelOfEndpoint(EndpointID ep_id) {
		byte ch = ch_of_ep_[ep_id.Address() & 0x1F];
		if (ch == kNoChannel) {
			ConfigureEndpoints();
			ch = ch_of_ep_[ep_id.Address() & 0x1F];
		}
		return ch == kNoChannel ? 0 : ch;// 0 tells the caller this endpoint has no channel
	}

	// Send the stage in flight again: a NAK means the device was not ready for it
	Error OTGHostDevice::ResubmitControlStage() {
		if (ctrl_stage_ == ControlStage::Idle || ctrl_stage_ == ControlStage::Done)
			return MAKE_ERROR(Error::InvalidPhase);
		bool submitted = false;
		{
			ControlIrqGuard guard;
			const stduint active = stduint(hcd_.ChannelReg(ctrl_ch_, 0x000)) &
				(USB_OTG_HCCHAR_CHENA | USB_OTG_HCCHAR_CHDIS);
			const stduint pending = stduint(hcd_.ChannelReg(ctrl_ch_, 0x008)) &
				stduint(hcd_.ChannelReg(ctrl_ch_, 0x00C));
			if (ctrl_completion_pending_ || ctrl_halt_pending_ || active || pending) {
				ctrl_retry_pending_ = true;
				ctrl_retry_tick_ = s_bus_tick;
				return MAKE_ERROR(Error::Full);
			}
			if (ctrl_reset_pending_) {
				hcd_.InitializeHostChannel(ctrl_ch_, 0x00, dev_address_, speed_, 0, hcd_.hc[ctrl_ch_].max_packet);
				ctrl_reset_pending_ = false;
			}
			if (!++s_control_submission) ++s_control_submission;
			ctrl_submission_ = s_control_submission;
			switch (ctrl_stage_) {
			case ControlStage::Setup:
				submitted = hcd_.SubmitRequest(ctrl_ch_, 0, 0, 0, reinterpret_cast<byte*>(&ctrl_setup_), 8, 0);
				break;
			case ControlStage::DataIn:
				submitted = hcd_.SubmitRequest(ctrl_ch_, 1, 0, 1, static_cast<byte*>(ctrl_buf_),
					static_cast<uint16>(ctrl_len_), 0);
				break;
			case ControlStage::DataOut:
				submitted = hcd_.SubmitRequest(ctrl_ch_, 0, 0, 1, static_cast<byte*>(ctrl_buf_),
					static_cast<uint16>(ctrl_len_), 0);
				break;
			case ControlStage::StatusIn:
				submitted = hcd_.SubmitRequest(ctrl_ch_, 1, 0, 1, nullptr, 0, 0);
				break;
			case ControlStage::StatusOut:
				submitted = hcd_.SubmitRequest(ctrl_ch_, 0, 0, 1, nullptr, 0, 0);
				break;
			default:
				break;
			}
			ctrl_waiting_ = submitted;
			ctrl_retry_pending_ = !submitted;
			if (submitted) {
				ctrl_submit_tick_ = s_bus_tick;
				ctrl_submit_failures_ = 0;
			}
			else ctrl_retry_tick_ = s_bus_tick;
		}
		if (submitted) return MAKE_ERROR(Error::Success);
		if (++ctrl_submit_failures_ <= kCtrlErrRetryLimit) return MAKE_ERROR(Error::Full);
		ctrl_retry_pending_ = false;
		ctrl_stage_ = ControlStage::Done;
		OnControlCompleted(ctrl_ep_id_, ctrl_setup_, ctrl_buf_, -1);
		return MAKE_ERROR(Error::TransferFailed);
	}

	// Advance the control stage machine on a control-channel URB completion.
	void OTGHostDevice::OnChannelURBCompleted(byte ch_num, URBState urb_state) {
		if (transfers_stopping_ || ch_num >= hcd_.host_channels || ch_num >= numsof(hcd_.hc)) return;
		ch_waiting_since_[ch_num] = 0;
		if (ch_num != ctrl_ch_) {
			// route by the endpoint type; the class driver gets its own buffer back
			const byte ep_addr = ep_of_ch_[ch_num];
			const byte ep_type = hcd_.hc[ch_num].ep_type;
			const bool dir_in = (ep_addr & 0x01) != 0;
			const EndpointID ep_id{ static_cast<int>(ep_addr >> 1), dir_in };
			if (urb_state == URBState::Done) {
				ch_nak_retry_[ch_num] = 0;
				const int len = dir_in ? static_cast<int>(hcd_.getXferCount(ch_num))
					: static_cast<int>(ch_xfer_len_[ch_num]);
				if (ep_type == 2) OnBulkCompleted(ep_id, ch_xfer_base_[ch_num], len);
				else OnInterruptCompleted(ep_id, ch_xfer_base_[ch_num], len);
			}
			else if (urb_state == URBState::NotReady || urb_state == URBState::NYET
				|| urb_state == URBState::Idle) {
				if (ep_type == 3) {
					// a periodic NAK ends this poll with no report, the driver re-arms it later
					ch_nak_retry_[ch_num] = 0;
					OnInterruptCompleted(ep_id, nullptr, 0);
					return;
				}
				// Bulk endpoints retry the same request until the device becomes ready.
				if (ch_xfer_base_[ch_num] && ch_xfer_len_[ch_num]
					&& ch_nak_retry_[ch_num]++ < kNakRetryLimit) {
					if (hcd_.SubmitRequest(ch_num, dir_in ? 1 : 0, ep_type, 1,
						ch_xfer_base_[ch_num], ch_xfer_len_[ch_num], 0)) return;
				}
				OnBulkCompleted(ep_id, nullptr, 0);
			}
			else {
				if (ep_type == 2 && urb_state == URBState::Stall) {
					// CLEAR_FEATURE(HALT) puts the device toggle at DATA0 too
					hcd_.hc[ch_num].toggle_in = 0;
					hcd_.hc[ch_num].toggle_out = 0;
				}
				ch_nak_retry_[ch_num] = 0;
				// stall / error: report a null completion so the driver can recover
				if (ep_type == 2) OnBulkCompleted(ep_id, nullptr, 0);
				else OnInterruptCompleted(ep_id, nullptr, 0);
			}
			return;
		}
		ctrl_completion_pending_ = false;
		ctrl_waiting_ = false;
		if (ctrl_halt_pending_) {
			ctrl_halt_pending_ = false;
			if (ctrl_err_retry_ < kCtrlErrRetryLimit) {
				++ctrl_err_retry_;
				ctrl_stage_ = ControlStage::Setup;
				ctrl_reset_pending_ = true;
				ctrl_retry_pending_ = true;
				ctrl_retry_tick_ = s_bus_tick;
			}
			else {
				ctrl_stage_ = ControlStage::Done;
				OnControlCompleted(ctrl_ep_id_, ctrl_setup_, ctrl_buf_, -1);
			}
			return;
		}
		if (ctrl_stage_ == ControlStage::Idle || ctrl_stage_ == ControlStage::Done) return;
		if (urb_state != URBState::Done) {
			const bool retryable = urb_state == URBState::NotReady
				|| urb_state == URBState::NYET || urb_state == URBState::Idle;
			// the bus lost this answer, the device did not refuse it: run the whole control transfer again
			if (urb_state == URBState::Error && ctrl_err_retry_ < kCtrlErrRetryLimit) {
				ctrl_err_retry_++;
				ch_nak_retry_[ctrl_ch_] = 0;
				ctrl_stage_ = ControlStage::Setup;
				ctrl_retry_pending_ = true;
				return;
			}
			// ST USBH_HandleControl/CTRL_DATA_OUT_WAIT: a halted OUT NAK requires host resubmission.
			if (retryable && ch_nak_retry_[ctrl_ch_]++ < kNakRetryLimit) {
				// Retry from the bus tick, never from inside the channel ISR.
				ctrl_retry_pending_ = true;
				return;
			}
			ctrl_retry_pending_ = false;
			ctrl_stage_ = ControlStage::Done;
			// a negative length tells the device layer this transfer failed
			OnControlCompleted(ctrl_ep_id_, ctrl_setup_, ctrl_buf_, -1);
			return;
		}
		ctrl_retry_pending_ = false;
		ch_nak_retry_[ctrl_ch_] = 0;
		ctrl_err_retry_ = 0;
		switch (ctrl_stage_) {
		case ControlStage::Setup:
			// SETUP done -> DATA stage (direction from request)
			if (ctrl_setup_.request_type.bits.direction == request_type::kIn) {
				if (ctrl_len_ > 0) {
					ctrl_stage_ = ControlStage::DataIn;
					ResubmitControlStage();
				}
				else {
					ctrl_stage_ = ControlStage::StatusOut;
					ResubmitControlStage();
				}
			}
			else {
				if (ctrl_len_ > 0) {
					ctrl_stage_ = ControlStage::DataOut;
					ResubmitControlStage();
				}
				else {
					ctrl_stage_ = ControlStage::StatusIn;
					ResubmitControlStage();
				}
			}
			break;
		case ControlStage::DataIn:
		case ControlStage::DataOut:
			ctrl_xfer_count_ = static_cast<int>(hcd_.getXferCount(ctrl_ch_));
			if (ctrl_setup_.request_type.bits.direction == request_type::kIn) {
				ctrl_stage_ = ControlStage::StatusOut;
				ResubmitControlStage();
			}
			else {
				ctrl_stage_ = ControlStage::StatusIn;
				ResubmitControlStage();
			}
			break;
		case ControlStage::StatusIn:
		case ControlStage::StatusOut:
			// STATUS done -> control transfer complete
			ctrl_stage_ = ControlStage::Done;
			if (mps_probe_) {
				// The 8-byte reply carries bMaxPacketSize0 at descriptor offset 7.
				byte mps = static_cast<const byte*>(ctrl_buf_)[7];
				if (mps != 8 && mps != 16 && mps != 32 && mps != 64) mps = 8;
				hcd_.InitializeHostChannel(ctrl_ch_, 0x00, dev_address_, speed_, 0, mps);
				mps_known_ = true;
				mps_probe_ = false;
				// redo the request at the real packet size, as one completion
				ctrl_len_ = ctrl_len_full_;
				ctrl_setup_.length = static_cast<uint16>(ctrl_len_full_);
				ctrl_retry_pending_ = false;
				ctrl_stage_ = ControlStage::Setup;
				ResubmitControlStage();
				return;
			}
			// the stack uses this as its buffer length, so report what really moved
			OnControlCompleted(ctrl_ep_id_, ctrl_setup_, ctrl_buf_,
				(ctrl_setup_.request_type.bits.direction == request_type::kIn)
					? ctrl_xfer_count_ : ctrl_len_);
			break;
		default:
			break;
		}
	}

#endif // _MCU_STM32H7x
}

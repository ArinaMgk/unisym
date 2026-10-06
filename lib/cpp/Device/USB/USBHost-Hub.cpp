// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [Device.USB] Host Hub Class Driver
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

#include "../../../../inc/cpp/Device/USB/USBHost-Hub.hpp"
#include "../../../../inc/cpp/Device/USB/OTGHostDevice.hpp"

namespace uni::device::SpaceUSB {

	USBHubDriver::USBHubDriver(USBHostDevice* dev)
		: ClassDriver{ dev } {
	}

	void* USBHubDriver::operator new(size_t size) {
		(void)size;
		return uni_hostenv_allocator->allocate(sizeof(USBHubDriver));
	}

	void USBHubDriver::operator delete(void* ptr) noexcept {
		uni_hostenv_allocator->deallocate(ptr);
	}

	Error USBHubDriver::Initialize() {
		return MAKE_ERROR(Error::kSuccess);
	}

	Error USBHubDriver::SetEndpoint(const EndpointConfig& config) {
		if (config.ep_type == EndpointType::kInterrupt && config.ep_id.IsIn()) {
			ep_interrupt_in_ = config.ep_id;
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	// the first hub class request: the hub descriptor says how many ports exist
	Error USBHubDriver::OnEndpointsConfigured() {
		is_super_speed_ = ParentDevice()->DeviceProtocol() ==
			static_cast<uint8>(HubProtocol::SuperSpeed);
		extended_port_status_ = is_super_speed_ && ParentDevice()->USBRelease() >= 0x0310u;
		SetupData setup_data{};
		setup_data.request_type.bits.direction = request_type::kIn;
		setup_data.request_type.bits.type = request_type::kClass;
		setup_data.request_type.bits.recipient = request_type::kDevice;
		setup_data.request = static_cast<uint8>(StandardRequest::GetDescriptor);
		setup_data.value = static_cast<uint16>(is_super_speed_
			? descriptor_type::kSuperSpeedHub : descriptor_type::kHub) << 8;
		setup_data.index = 0;
		setup_data.length = is_super_speed_
			? sizeof(SuperSpeedHubDescriptor) : sizeof(HubDescriptor);
		initialize_phase_ = 1;
		return ParentDevice()->ControlIn(kDefaultControlPipeID, setup_data,
			buf_.data(), setup_data.length, this);
	}

	Error USBHubDriver::StartStatusChangePolling() {
		if (ep_interrupt_in_.Number() == 0) {
			return MAKE_ERROR(Error::kSuccess);
		}
		const stduint bytes = status_change_bytes_ ? status_change_bytes_ : 1;
		return ParentDevice()->InterruptIn(ep_interrupt_in_, interrupt_buf_.data(), int(bytes));
	}

	// SET_FEATURE on a downstream port (PORT_POWER, PORT_RESET)
	Error USBHubDriver::RequestSetPortFeature(uint8 port_num, HubPortFeature feature_selector) {
		SetupData setup_data{};
		setup_data.request_type.bits.direction = request_type::kOut;
		setup_data.request_type.bits.type = request_type::kClass;
		setup_data.request_type.bits.recipient = request_type::kOther;
		setup_data.request = static_cast<uint8>(StandardRequest::SetFeature);
		setup_data.value = static_cast<uint16>(feature_selector);
		setup_data.index = port_num;
		setup_data.length = 0;
		pending_kind_ = 1;
		return ParentDevice()->ControlOut(kDefaultControlPipeID, setup_data, nullptr, 0, this);
	}

	Error USBHubDriver::RequestSetHubDepth() {
		SetupData setup_data{};
		setup_data.request_type.bits.direction = request_type::kOut;
		setup_data.request_type.bits.type = request_type::kClass;
		setup_data.request_type.bits.recipient = request_type::kDevice;
		setup_data.request = static_cast<uint8>(HubRequest::SetHubDepth);
		setup_data.value = ParentDevice()->HubDepth();
		setup_data.index = 0;
		setup_data.length = 0;
		initialize_phase_ = 6;
		return ParentDevice()->ControlOut(kDefaultControlPipeID, setup_data, nullptr, 0, this);
	}

	// the port status record: keep it per port and hand it to the transport
	Error USBHubDriver::RecordPortStatus(uint8 port_num, const void* buf, int len) {
		if (port_num == 0 || port_num > 16) {
			return MAKE_ERROR(Error::kInvalidEndpointNumber);
		}
		if (buf == nullptr || len < int(sizeof(HubPortStatus))) {
			return MAKE_ERROR(Error::kInvalidDescriptor);
		}
		const auto port_status = reinterpret_cast<const HubPortStatus*>(buf);
		port_status_[port_num - 1] = port_status->status;
		port_change_[port_num - 1] = port_status->change;
		uint8 speed_id = 0;
		if (extended_port_status_ && len >= int(sizeof(ExtendedHubPortStatus))) {
			const auto extended = reinterpret_cast<const ExtendedHubPortStatus*>(buf);
			speed_id = extended->extended_status & 0x0fu;
		}
		ParentDevice()->OnHubPortStatusReceived(
			port_num, port_status->status, port_status->change, speed_id);
		if (g_hub_port_status_hook) {
			g_hub_port_status_hook(*ParentDevice(), port_num, port_status->status, port_status->change);
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	// the port policy: no wait may run in the interrupt, so every step is ticked here
	Error USBHubDriver::ProcessDelayed() {
		static const uint16 kRescanTicks = 250;
		static uint16 rescan_ticks = 0;
		static const uint16 kSettleTicks = 100;
		static uint16 settle_ticks = 0;
		static const uint16 kPendingTicks = 300;
		static uint16 pending_ticks = 0;
		static const uint8 kPortResetTries = 3;
		static const uint16 kChildBusyWaitTicks = 50;// at most this many ticks the policy waits for a busy child
		static uint16 child_busy_ticks = 0;
		if (num_ports_ == 0 || initialize_phase_ != 3) {
			return MAKE_ERROR(Error::kSuccess);// wait for the descriptor and the first port scan
		}
		// hold the policy back while a downstream device uses the bus, bounded so it can never starve
		if (ParentDevice()->ChildBusy() && child_busy_ticks < kChildBusyWaitTicks) {
			child_busy_ticks++;
			return MAKE_ERROR(Error::kSuccess);
		}
		child_busy_ticks = 0;
		if (wait_ticks_) {
			--wait_ticks_;
			return MAKE_ERROR(Error::kSuccess);
		}
		if (pending_kind_) {
			if (++pending_ticks > kPendingTicks) {
				pending_ticks = 0;
				pending_kind_ = 0;
			}
			else return MAKE_ERROR(Error::kSuccess);// one port request at a time
		}
		else pending_ticks = 0;
		if (delayed_step_ == 0) {
			port_cursor_ = 1;// power every port once, then wait bPwrOn2PwrGood
			delayed_step_ = 1;
		}
		if (delayed_step_ == 1) {
			if (port_cursor_ <= num_ports_) {
				return RequestSetPortFeature(port_cursor_++, HubPortFeature::Power);
			}
			wait_ticks_ = power_good_ticks_;
			delayed_step_ = 2;
		}
		if (delayed_step_ == 2) {
			port_cursor_ = 1;// after power-good: read every port status once
			delayed_step_ = 3;
		}
		if (delayed_step_ == 3) {
			if (port_cursor_ <= num_ports_) {
				pending_kind_ = 2;
				return RequestPortStatus(port_cursor_++);
			}
			delayed_step_ = 4;
		}
		if (delayed_step_ == 4) {
			if (++rescan_ticks >= kRescanTicks) {
				rescan_ticks = 0;
				delayed_step_ = 2;// back to reading every port status once
				return MAKE_ERROR(Error::kSuccess);
			}
			for (uint8 port = 1; port <= num_ports_; port++) {// a port that lost ENABLE may come up again
				if (!(port_status_[port - 1] & kHubPortStatusConnect)) {
					reset_mask_ &= uint16(~(1u << (port - 1)));
					reset_count_[port - 1] = 0;// a device plugged in again gets the tries back
				}
				if (!(ready_mask_ & uint16(1u << (port - 1)))) continue;
				if (port_status_[port - 1] & kHubPortStatusEnable) continue;
				ready_mask_ &= uint16(~(1u << (port - 1)));
			}
			const uint8 want = g_hub_reset_port_hook ? g_hub_reset_port_hook(*ParentDevice()) : 0;
			if (g_hub_reset_port_hook && want == 0) return MAKE_ERROR(Error::kSuccess);// nothing to bring up now
			// First, process any settling ports that have finally become ENABLED
			for (uint8 port = 1; port <= num_ports_; port++) {
				if ((reset_mask_ & uint16(1u << (port - 1))) && (port_status_[port - 1] & kHubPortStatusEnable)) {
					ready_mask_ |= uint16(1u << (port - 1));
					reset_mask_ &= uint16(~(1u << (port - 1)));
				}
			}

			const bool is_addressing = ParentDevice()->HubAddressingPort() != 0;
			if (!is_addressing && reset_mask_ == 0) {
				// Only pick a new port to reset if the bus is clear from address 0 assignments
				for (uint8 port = 1; port <= num_ports_; port++) {
					if (want && port != want) continue;
					if (ready_mask_ & uint16(1u << (port - 1))) continue;
					if (!(port_status_[port - 1] & kHubPortStatusConnect)) continue;
					
					port_cursor_ = port;
					reset_mask_ |= uint16(1u << (port - 1));
					delayed_step_ = 5;
					return RequestSetPortFeature(port, HubPortFeature::Reset);
				}
			}
			if (++settle_ticks > kSettleTicks) {
				settle_ticks = 0;
				for (uint8 port = 1; port <= num_ports_; port++) {
					if (!(reset_mask_ & uint16(1u << (port - 1)))) continue;
					if (port_status_[port - 1] & kHubPortStatusEnable) continue;
					if (port_status_[port - 1] & 0x0010) {// PORT_RESET: reset still in progress
						pending_kind_ = 1;
						return RequestClearPortFeature(port, HubPortFeature::Reset);
					}
					// a port that never reached ENABLE gets another reset, up to kPortResetTries
					if (reset_count_[port - 1] == 0) reset_count_[port - 1] = kPortResetTries;
					if (--reset_count_[port - 1] > 0) {
						port_cursor_ = port;
						delayed_step_ = 5;
						return RequestSetPortFeature(port, HubPortFeature::Reset);
					}
					reset_mask_ &= uint16(~(1u << (port - 1)));// the tries are used up
				}
			}
			for (uint8 port = 1; port <= num_ports_; port++) {// read a port whose reset is still settling
				if (want && port != want) continue;
				if (!(reset_mask_ & uint16(1u << (port - 1)))) continue;
				if (port_status_[port - 1] & kHubPortStatusEnable) continue;
				port_cursor_ = port;
				delayed_step_ = 5;
				break;
			}
			if (delayed_step_ == 4) return MAKE_ERROR(Error::kSuccess);
		}
		if (delayed_step_ == 5) {
			wait_ticks_ = 60;// the reset recovery window, then read that port again
			delayed_step_ = 6;
		}
		if (delayed_step_ == 6 && port_cursor_) {
			const uint8 port = port_cursor_;
			port_cursor_ = 0;
			delayed_step_ = 4;
			pending_kind_ = 2;
			return RequestPortStatus(port);
		}
		delayed_step_ = 4;
		return MAKE_ERROR(Error::kSuccess);
	}

	Error USBHubDriver::RequestPortStatus(uint8 port_num) {
		SetupData setup_data{};
		setup_data.request_type.bits.direction = request_type::kIn;
		setup_data.request_type.bits.type = request_type::kClass;
		setup_data.request_type.bits.recipient = request_type::kOther;
		setup_data.request = static_cast<uint8>(StandardRequest::GetStatus);
		setup_data.value = static_cast<uint16>(extended_port_status_
			? HubPortStatusType::Extended : HubPortStatusType::Standard);
		setup_data.index = port_num;
		setup_data.length = extended_port_status_
			? sizeof(ExtendedHubPortStatus) : sizeof(HubPortStatus);
		return ParentDevice()->ControlIn(kDefaultControlPipeID, setup_data,
			port_status_buf_.data(), setup_data.length, this);
	}

	Error USBHubDriver::RequestClearPortFeature(uint8 port_num, HubPortFeature feature_selector) {
		SetupData setup_data{};
		setup_data.request_type.bits.direction = request_type::kOut;
		setup_data.request_type.bits.type = request_type::kClass;
		setup_data.request_type.bits.recipient = request_type::kOther;
		setup_data.request = static_cast<uint8>(StandardRequest::ClearFeature);
		setup_data.value = static_cast<uint16>(feature_selector);
		setup_data.index = port_num;
		setup_data.length = 0;
		return ParentDevice()->ControlOut(kDefaultControlPipeID, setup_data, nullptr, 0, this);
	}

	Error USBHubDriver::ContinuePendingPortStatus() {
		if (pending_status_index_ < pending_status_count_) {
			const auto next_port = pending_status_ports_[pending_status_index_++];
			initialize_phase_ = 4;
			return RequestPortStatus(next_port);
		}
		initialize_phase_ = 3;
		return StartStatusChangePolling();
	}

	Error USBHubDriver::ClearNextPendingChange() {
		const uint8 bit_count = is_super_speed_ ? 8 : 5;
		while (clear_change_bit_ < bit_count) {
			const uint16 bit_mask = uint16(1u << clear_change_bit_);
			if ((pending_change_mask_ & bit_mask) != 0) {
				HubPortFeature feature_selector = static_cast<HubPortFeature>(16u + clear_change_bit_);
				if (is_super_speed_) {
					switch (clear_change_bit_) {
					case 0: feature_selector = HubPortFeature::ConnectionChange; break;
					case 3: feature_selector = HubPortFeature::OverCurrentChange; break;
					case 4: feature_selector = HubPortFeature::ResetChange; break;
					case 5: feature_selector = HubPortFeature::WarmResetChange; break;
					case 6: feature_selector = HubPortFeature::LinkStateChange; break;
					case 7: feature_selector = HubPortFeature::ConfigErrorChange; break;
					default:
						++clear_change_bit_;
						continue;
					}
				}
				++clear_change_bit_;
				initialize_phase_ = 5;
				return RequestClearPortFeature(clear_change_port_, feature_selector);
			}
			++clear_change_bit_;
		}
		pending_change_mask_ = 0;
		clear_change_port_ = 0;
		clear_change_bit_ = 0;
		return ContinuePendingPortStatus();
	}

	Error USBHubDriver::BeginPortScan() {
		if (num_ports_ == 0) {
			initialize_phase_ = 3;
			return StartStatusChangePolling();
		}
		status_change_bytes_ = uint8((num_ports_ + 8) / 8);
		scan_port_ = 1;
		initialize_phase_ = 2;
		return RequestPortStatus(scan_port_);
	}

	Error USBHubDriver::OnControlCompleted(EndpointID ep_id, SetupData setup_data,
		const void* buf, int len) {
		(void)ep_id;
		const uint8 pending_kind = pending_kind_;
		pending_kind_ = 0;
		if (pending_kind == 1) {
			return MAKE_ERROR(Error::kSuccess);// the SET_FEATURE of the port policy needs no further step
		}
		if (pending_kind == 2) {
			if (len >= int(sizeof(HubPortStatus))) {
				return RecordPortStatus(uint8(setup_data.index), buf, len);
			}
			// the port did not answer: read every port again after a short pause
			delayed_step_ = 3;
			port_cursor_ = 1;
			wait_ticks_ = 10;
			return MAKE_ERROR(Error::kSuccess);
		}
		if (initialize_phase_ == 1) {
			uint16 characteristics = 0;
			uint8 power_on_to_power_good = 0;
			if (is_super_speed_) {
				if (buf == nullptr || len < int(sizeof(SuperSpeedHubDescriptor))) {
					return MAKE_ERROR(Error::kInvalidDescriptor);
				}
				auto hub_desc = DescriptorDynamicCast<SuperSpeedHubDescriptor>(
					reinterpret_cast<const uint8*>(buf));
				if (hub_desc == nullptr) {
					return MAKE_ERROR(Error::kInvalidDescriptor);
				}
				num_ports_ = hub_desc->num_ports;
				characteristics = hub_desc->characteristics;
				power_on_to_power_good = hub_desc->power_on_to_power_good;
			}
			else {
				if (buf == nullptr || len < int(sizeof(HubDescriptor))) {
					return MAKE_ERROR(Error::kInvalidDescriptor);
				}
				auto hub_desc = DescriptorDynamicCast<HubDescriptor>(
					reinterpret_cast<const uint8*>(buf));
				if (hub_desc == nullptr) {
					return MAKE_ERROR(Error::kInvalidDescriptor);
				}
				num_ports_ = hub_desc->num_ports;
				characteristics = hub_desc->characteristics;
				power_on_to_power_good = hub_desc->power_on_to_power_good;
			}
			if (num_ports_ > 16) {
				return MAKE_ERROR(Error::kInvalidDescriptor);
			}
			ParentDevice()->SetHubNumPorts(num_ports_);
			ParentDevice()->SetHubPowerOnToPowerGood(power_on_to_power_good);
			// bPwrOn2PwrGood is in 2ms units, the port policy is ticked about once per millisecond
			const uint16 power_good_ms = uint16(power_on_to_power_good) * 2;
			power_good_ticks_ = power_good_ms == 0 ? 100 : uint8(power_good_ms > 255 ? 255 : power_good_ms);
			if (auto err = ParentDevice()->ConfigureHub(num_ports_, characteristics)) return err;
			if (g_hub_descriptor_complete_hook) {
				g_hub_descriptor_complete_hook(*ParentDevice());
			}
			if (is_super_speed_) return RequestSetHubDepth();
			return BeginPortScan();
		}
		if (initialize_phase_ == 6) {
			const bool depth_set = setup_data.request == static_cast<uint8>(HubRequest::SetHubDepth)
				&& setup_data.request_type.bits.recipient == request_type::kDevice;
			if (!depth_set || len < 0) return MAKE_ERROR(Error::kTransferFailed);
			return BeginPortScan();
		}
		if (initialize_phase_ == 2) {
			const auto port_status = reinterpret_cast<const HubPortStatus*>(buf);
			const bool got_status = setup_data.request == static_cast<uint8>(StandardRequest::GetStatus)
				&& setup_data.request_type.bits.recipient == request_type::kOther
				&& len >= int(sizeof(HubPortStatus));
			if (!got_status) {
				// a silent port is skipped so that one of them cannot stop the whole scan
				if (scan_port_ < num_ports_) {
					++scan_port_;
					return RequestPortStatus(scan_port_);
				}
				initialize_phase_ = 3;
				return StartStatusChangePolling();
			}
			if (auto err = RecordPortStatus(uint8(setup_data.index), buf, len)) return err;
			pending_change_mask_ = uint16(port_status->change & (is_super_speed_ ? 0x00f9u : 0x001fu));
			clear_change_port_ = uint8(setup_data.index);
			clear_change_bit_ = 0;
			if (pending_change_mask_ != 0) {
				return ClearNextPendingChange();
			}
			if (scan_port_ < num_ports_) {
				++scan_port_;
				return RequestPortStatus(scan_port_);
			}
			initialize_phase_ = 3;
			return StartStatusChangePolling();
		}
		if (initialize_phase_ == 4) {
			const auto port_status = reinterpret_cast<const HubPortStatus*>(buf);
			const bool got_status = setup_data.request == static_cast<uint8>(StandardRequest::GetStatus)
				&& setup_data.request_type.bits.recipient == request_type::kOther
				&& len >= int(sizeof(HubPortStatus));
			if (!got_status) {
				return ContinuePendingPortStatus();
			}
			if (auto err = RecordPortStatus(uint8(setup_data.index), buf, len)) return err;
			pending_change_mask_ = uint16(port_status->change & (is_super_speed_ ? 0x00f9u : 0x001fu));
			clear_change_port_ = uint8(setup_data.index);
			clear_change_bit_ = 0;
			if (pending_change_mask_ != 0) {
				return ClearNextPendingChange();
			}
			return ContinuePendingPortStatus();
		}
		if (initialize_phase_ == 5) {
			if (setup_data.request == static_cast<uint8>(StandardRequest::ClearFeature) &&
				setup_data.request_type.bits.recipient == request_type::kOther) {
				return ClearNextPendingChange();
			}
			return MAKE_ERROR(Error::kInvalidPhase);
		}
		return MAKE_ERROR(Error::kNotImplemented);
	}

	Error USBHubDriver::OnInterruptCompleted(EndpointID ep_id, const void* buf, int len) {
		(void)ep_id;
		if (initialize_phase_ != 3 || pending_kind_ != 0) {
			return StartStatusChangePolling();
		}
		pending_status_count_ = 0;
		pending_status_index_ = 0;
		const auto* bits = reinterpret_cast<const uint8_t*>(buf);
		const stduint bytes = stduint(len);
		for (uint8 port = 1; port <= num_ports_; ++port) {
			const stduint bit_index = port;
			const stduint byte_index = bit_index / 8;
			const uint8 bit_mask = uint8(1u << (bit_index % 8));
			if (byte_index < bytes && (bits[byte_index] & bit_mask) != 0) {
				if (pending_status_count_ < pending_status_ports_.size()) {
					pending_status_ports_[pending_status_count_++] = port;
				}
			}
		}
		if (pending_status_count_ == 0) {
			return StartStatusChangePolling();
		}
		return ContinuePendingPortStatus();
	}

}

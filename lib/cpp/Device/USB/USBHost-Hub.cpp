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

	Error USBHubDriver::OnEndpointsConfigured() {
		SetupData setup_data{};
		setup_data.request_type.bits.direction = request_type::kIn;
		setup_data.request_type.bits.type = request_type::kClass;
		setup_data.request_type.bits.recipient = request_type::kDevice;
		setup_data.request = request::kGetDescriptor;
		setup_data.value = static_cast<uint16>(descriptor_type::kHub) << 8;
		setup_data.index = 0;
		setup_data.length = sizeof(HubDescriptor);
		initialize_phase_ = 1;
		return ParentDevice()->ControlIn(kDefaultControlPipeID, setup_data,
			buf_.data(), sizeof(HubDescriptor), this);
	}

	Error USBHubDriver::StartStatusChangePolling() {
		if (ep_interrupt_in_.Number() == 0) {
			return MAKE_ERROR(Error::kSuccess);
		}
		const stduint bytes = status_change_bytes_ ? status_change_bytes_ : 1;
		return ParentDevice()->InterruptIn(ep_interrupt_in_, interrupt_buf_.data(), int(bytes));
	}

	Error USBHubDriver::RequestSetPortFeature(uint8 port_num, uint16 feature_selector) {
		SetupData setup_data{};
		setup_data.request_type.bits.direction = request_type::kOut;
		setup_data.request_type.bits.type = request_type::kClass;
		setup_data.request_type.bits.recipient = request_type::kOther;
		setup_data.request = request::kSetFeature;
		setup_data.value = feature_selector;
		setup_data.index = port_num;
		setup_data.length = 0;
		pending_kind_ = 1;
		return ParentDevice()->ControlOut(kDefaultControlPipeID, setup_data, nullptr, 0, this);
	}

	Error USBHubDriver::RecordPortStatus(uint8 port_num, const void* buf) {
		if (port_num == 0 || port_num > 16) {
			return MAKE_ERROR(Error::kInvalidEndpointNumber);
		}
		const auto port_status = reinterpret_cast<const HubPortStatus*>(buf);
		port_status_[port_num - 1] = port_status->status;
		port_change_[port_num - 1] = port_status->change;
		ParentDevice()->OnHubPortStatusReceived(port_num, port_status->status, port_status->change);
		if (g_hub_port_status_hook) {
			g_hub_port_status_hook(*ParentDevice(), port_num, port_status->status, port_status->change);
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error USBHubDriver::RequestPortStatus(uint8 port_num) {
		SetupData setup_data{};
		setup_data.request_type.bits.direction = request_type::kIn;
		setup_data.request_type.bits.type = request_type::kClass;
		setup_data.request_type.bits.recipient = request_type::kOther;
		setup_data.request = request::kGetStatus;
		setup_data.value = 0;
		setup_data.index = port_num;
		setup_data.length = sizeof(HubPortStatus);
		return ParentDevice()->ControlIn(kDefaultControlPipeID, setup_data,
			port_status_buf_.data(), sizeof(HubPortStatus), this);
	}

	Error USBHubDriver::RequestClearPortFeature(uint8 port_num, uint16 feature_selector) {
		SetupData setup_data{};
		setup_data.request_type.bits.direction = request_type::kOut;
		setup_data.request_type.bits.type = request_type::kClass;
		setup_data.request_type.bits.recipient = request_type::kOther;
		setup_data.request = request::kClearFeature;
		setup_data.value = feature_selector;
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
		while (clear_change_bit_ < 5) {
			const uint16 bit_mask = uint16(1u << clear_change_bit_);
			if ((pending_change_mask_ & bit_mask) != 0) {
				const auto feature_selector = uint16(16u + clear_change_bit_);
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

	Error USBHubDriver::OnControlCompleted(EndpointID ep_id, SetupData setup_data, const void* buf, int len) {
		(void)ep_id;
		(void)len;
		if (initialize_phase_ == 1) {
			auto hub_desc = DescriptorDynamicCast<HubDescriptor>(reinterpret_cast<const uint8*>(buf));
			if (hub_desc == nullptr) {
				return MAKE_ERROR(Error::kInvalidDescriptor);
			}
			num_ports_ = hub_desc->num_ports;
			ParentDevice()->SetHubNumPorts(num_ports_);
			if (g_hub_descriptor_complete_hook) {
				g_hub_descriptor_complete_hook(*ParentDevice());
			}
			if (num_ports_ == 0) {
				initialize_phase_ = 3;
				return StartStatusChangePolling();
			}
			status_change_bytes_ = uint8((num_ports_ + 8) / 8);
			scan_port_ = 1;
			initialize_phase_ = 2;
			return RequestPortStatus(scan_port_);
		}
		if (initialize_phase_ == 2) {
			auto port_status = reinterpret_cast<const HubPortStatus*>(buf);
			if (setup_data.request == request::kGetStatus &&
				setup_data.request_type.bits.recipient == request_type::kOther) {
				ParentDevice()->OnHubPortStatusReceived(uint8(setup_data.index), port_status->status, port_status->change);
				if (g_hub_port_status_hook) {
					g_hub_port_status_hook(*ParentDevice(), uint8(setup_data.index), port_status->status, port_status->change);
				}
			}
			pending_change_mask_ = uint16(port_status->change & 0x001fu);
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
			auto port_status = reinterpret_cast<const HubPortStatus*>(buf);
			if (setup_data.request == request::kGetStatus &&
				setup_data.request_type.bits.recipient == request_type::kOther) {
				ParentDevice()->OnHubPortStatusReceived(uint8(setup_data.index), port_status->status, port_status->change);
				if (g_hub_port_status_hook) {
					g_hub_port_status_hook(*ParentDevice(), uint8(setup_data.index), port_status->status, port_status->change);
				}
			}
			pending_change_mask_ = uint16(port_status->change & 0x001fu);
			clear_change_port_ = uint8(setup_data.index);
			clear_change_bit_ = 0;
			if (pending_change_mask_ != 0) {
				return ClearNextPendingChange();
			}
			return ContinuePendingPortStatus();
		}
		if (initialize_phase_ == 5) {
			if (setup_data.request == request::kClearFeature &&
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

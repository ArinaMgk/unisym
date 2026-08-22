// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Device.USB) OTG Host Device Bridge
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

namespace uni::device::SpaceUSB {
#if defined(_MCU_STM32H7x)

	// EP0 uses channel 0; other endpoints use their endpoint number as channel.
	// Control pipe: SETUP -> DATA -> STATUS, each stage is one SubmitRequest.

	OTGHostDevice::OTGHostDevice(HCD& hcd, byte dev_address, byte speed)
		: hcd_{ hcd }, dev_address_{ dev_address }, speed_{ speed } {
		// default control pipe on channel 0
		hcd_.InitializeHostChannel(0, 0x00, dev_address, speed, 0, 64);// EP_TYPE_CTRL, mps 64
	}

	Error OTGHostDevice::ControlIn(EndpointID ep_id, SetupData setup_data,
		void* buf, int len, ClassDriver* issuer) {
		if (auto err = DeviceUSB::ControlIn(ep_id, setup_data, buf, len, issuer)) {
			return err;
		}
		if (ep_id.Number() != 0) {
			return MAKE_ERROR(Error::kInvalidEndpointNumber);
		}
		// stage machine: SETUP(OUT) -> DATA(IN) -> STATUS(OUT)
		ctrl_ep_id_ = ep_id;
		ctrl_setup_ = setup_data;
		ctrl_buf_ = buf;
		ctrl_len_ = len;
		ctrl_stage_ = ControlStage::Setup;
		// SETUP stage: 8-byte setup packet, OUT, PID SETUP (token 0)
		hcd_.SubmitRequest(0, 0, 0, 0, reinterpret_cast<byte*>(&ctrl_setup_), 8, 0);
		return MAKE_ERROR(Error::kSuccess);
	}

	Error OTGHostDevice::ControlOut(EndpointID ep_id, SetupData setup_data,
		const void* buf, int len, ClassDriver* issuer) {
		if (auto err = DeviceUSB::ControlOut(ep_id, setup_data, buf, len, issuer)) {
			return err;
		}
		if (ep_id.Number() != 0) {
			return MAKE_ERROR(Error::kInvalidEndpointNumber);
		}
		ctrl_ep_id_ = ep_id;
		ctrl_setup_ = setup_data;
		ctrl_buf_ = const_cast<void*>(buf);
		ctrl_len_ = len;
		ctrl_stage_ = ControlStage::Setup;
		hcd_.SubmitRequest(0, 0, 0, 0, reinterpret_cast<byte*>(&ctrl_setup_), 8, 0);
		return MAKE_ERROR(Error::kSuccess);
	}

	Error OTGHostDevice::InterruptIn(EndpointID ep_id, void* buf, int len) {
		if (auto err = DeviceUSB::InterruptIn(ep_id, buf, len)) {
			return err;
		}
		const byte ch = static_cast<byte>(ep_id.Number());
		// IN interrupt transfer, DATA1 first (toggle starts at 0)
		hcd_.SubmitRequest(ch, 1, 3, 1, static_cast<byte*>(buf), static_cast<uint16>(len), 0);
		return MAKE_ERROR(Error::kSuccess);
	}

	Error OTGHostDevice::InterruptOut(EndpointID ep_id, void* buf, int len) {
		if (auto err = DeviceUSB::InterruptOut(ep_id, buf, len)) {
			return err;
		}
		const byte ch = static_cast<byte>(ep_id.Number());
		hcd_.SubmitRequest(ch, 0, 3, 1, static_cast<byte*>(buf), static_cast<uint16>(len), 0);
		return MAKE_ERROR(Error::kSuccess);
	}

	Error OTGHostDevice::OnHubPortStatusReceived(uint8 port_num, uint16 status, uint16 change) {
		// no hub behind OTG1_HS/OTG2_FS root port for now
		(void)port_num; (void)status; (void)change;
		return MAKE_ERROR(Error::kSuccess);
	}

	// Configure all endpoints recorded in ep_configs_ as host channels.
	Error OTGHostDevice::ConfigureEndpoints() {
		for (int i = 0; i < NumEndpointConfigs(); ++i) {
			const EndpointConfig& conf = EndpointConfigs()[i];
			const byte ep_num = static_cast<byte>(conf.ep_id.Number());
			const byte ep_addr = static_cast<byte>(
				(conf.ep_id.Address() & 0x01) ? (ep_num | 0x80) : ep_num);
			const byte ep_type = static_cast<byte>(conf.ep_type);// kControl=0, kIsochronous=1, kBulk=2, kInterrupt=3
			hcd_.InitializeHostChannel(ep_num, ep_addr, dev_address_, speed_,
				ep_type, static_cast<uint16>(conf.max_packet_size));
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	// Advance the control stage machine on channel-0 URB completion.
	void OTGHostDevice::OnChannelURBCompleted(byte ch_num, URBState urb_state) {
		if (ch_num != 0) {
			// non-control endpoint: interrupt/bulk completion
			if (urb_state == URBState::Done) {
				OnInterruptCompleted(EndpointID{ static_cast<int>(ch_num), true },
					nullptr, static_cast<int>(hcd_.getXferCount(ch_num)));
			}
			return;
		}
		if (urb_state != URBState::Done) {
			ctrl_stage_ = ControlStage::Done;
			OnControlCompleted(ctrl_ep_id_, ctrl_setup_, ctrl_buf_, 0);
			return;
		}
		switch (ctrl_stage_) {
		case ControlStage::Setup:
			// SETUP done -> DATA stage (direction from request)
			if (ctrl_setup_.request_type.bits.direction == request_type::kIn) {
				if (ctrl_len_ > 0) {
					ctrl_stage_ = ControlStage::DataIn;
					hcd_.SubmitRequest(0, 1, 0, 1,
						static_cast<byte*>(ctrl_buf_), static_cast<uint16>(ctrl_len_), 0);
				}
				else {
					ctrl_stage_ = ControlStage::StatusOut;
					hcd_.SubmitRequest(0, 0, 0, 1, nullptr, 0, 0);
				}
			}
			else {
				if (ctrl_len_ > 0) {
					ctrl_stage_ = ControlStage::DataOut;
					hcd_.SubmitRequest(0, 0, 0, 1,
						static_cast<byte*>(ctrl_buf_), static_cast<uint16>(ctrl_len_), 0);
				}
				else {
					ctrl_stage_ = ControlStage::StatusIn;
					hcd_.SubmitRequest(0, 1, 0, 1, nullptr, 0, 0);
				}
			}
			break;
		case ControlStage::DataIn:
		case ControlStage::DataOut:
			// DATA done -> STATUS stage (reverse direction, zero length)
			if (ctrl_setup_.request_type.bits.direction == request_type::kIn) {
				ctrl_stage_ = ControlStage::StatusOut;
				hcd_.SubmitRequest(0, 0, 0, 1, nullptr, 0, 0);
			}
			else {
				ctrl_stage_ = ControlStage::StatusIn;
				hcd_.SubmitRequest(0, 1, 0, 1, nullptr, 0, 0);
			}
			break;
		case ControlStage::StatusIn:
		case ControlStage::StatusOut:
			// STATUS done -> control transfer complete
			ctrl_stage_ = ControlStage::Done;
			OnControlCompleted(ctrl_ep_id_, ctrl_setup_, ctrl_buf_, ctrl_len_);
			break;
		default:
			break;
		}
	}

#endif // _MCU_STM32H7x
}

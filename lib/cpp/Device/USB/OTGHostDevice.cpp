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

namespace uni::device::SpaceUSB {
#if defined(_MCU_STM32H7x)

	// EP0 uses channel 0; other endpoints use their endpoint number as channel.
	// Control pipe: SETUP -> DATA -> STATUS, each stage is one SubmitRequest.

	OTGHostDevice::OTGHostDevice(HCD& hcd, byte dev_address, byte speed)
		: hcd_{ hcd }, dev_address_{ dev_address }, speed_{ speed } {
		// EP0 starts at 8 bytes: ControlIn() re-programs it from the first reply
		hcd_.InitializeHostChannel(0, 0x00, dev_address, speed, 0, 8);// EP_TYPE_CTRL
	}

	Error OTGHostDevice::ControlIn(EndpointID ep_id, SetupData setup_data,
		void* buf, int len, ClassDriver* issuer) {
		if (auto err = USBHostDevice::ControlIn(ep_id, setup_data, buf, len, issuer)) {
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
		ctrl_len_full_ = ctrl_len_;
		ctrl_setup_.length = static_cast<uint16>(ctrl_len_);
		ctrl_xfer_count_ = 0;
		mps_probe_ = false;
		if (!mps_known_ && setup_data.request == request::kGetDescriptor &&
			(setup_data.value >> 8) == DeviceDescriptor::kType && ctrl_len_ > 8) {
			// the 8-byte header fits every bMaxPacketSize0
			mps_probe_ = true;
			ctrl_len_ = 8;
			ctrl_setup_.length = 8;
		}
		// SETUP stage: 8-byte setup packet, OUT, PID SETUP (token 0)
		ctrl_stage_ = ControlStage::Setup;
		hcd_.SubmitRequest(0, 0, 0, 0, reinterpret_cast<byte*>(&ctrl_setup_), 8, 0);
		return MAKE_ERROR(Error::kSuccess);
	}

	Error OTGHostDevice::ControlOut(EndpointID ep_id, SetupData setup_data,
		const void* buf, int len, ClassDriver* issuer) {
		if (auto err = USBHostDevice::ControlOut(ep_id, setup_data, buf, len, issuer)) {
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
		if (auto err = USBHostDevice::InterruptIn(ep_id, buf, len)) {
			return err;
		}
		const byte ch = ChannelOfEndpoint(ep_id);
		if (!ch) return MAKE_ERROR(Error::kInvalidEndpointNumber);
		// IN interrupt transfer, DATA1 first (toggle starts at 0)
		ch_xfer_base_[ch] = static_cast<byte*>(buf);
		ch_xfer_len_[ch] = static_cast<uint16>(len);
		hcd_.SubmitRequest(ch, 1, 3, 1, static_cast<byte*>(buf), static_cast<uint16>(len), 0);
		return MAKE_ERROR(Error::kSuccess);
	}

	Error OTGHostDevice::InterruptOut(EndpointID ep_id, void* buf, int len) {
		if (auto err = USBHostDevice::InterruptOut(ep_id, buf, len)) {
			return err;
		}
		const byte ch = ChannelOfEndpoint(ep_id);
		if (!ch) return MAKE_ERROR(Error::kInvalidEndpointNumber);
		ch_xfer_base_[ch] = static_cast<byte*>(buf);
		ch_xfer_len_[ch] = static_cast<uint16>(len);
		hcd_.SubmitRequest(ch, 0, 3, 1, static_cast<byte*>(buf), static_cast<uint16>(len), 0);
		return MAKE_ERROR(Error::kSuccess);
	}

	// AKA the bulk pipe of a host class driver
	Error OTGHostDevice::BulkTransfer(EndpointID ep_id, bool dir_in, void* buf, int len) {
		const byte ch = ChannelOfEndpoint(ep_id);
		if (!ch || buf == nullptr || len <= 0) {
			return MAKE_ERROR(Error::kInvalidEndpointNumber);
		}
		ch_xfer_base_[ch] = static_cast<byte*>(buf);
		ch_xfer_len_[ch] = static_cast<uint16>(len);
		ch_nak_retry_[ch] = 0;
		// token 1 = DATA1; BULK takes the PID from the per-channel toggle
		hcd_.SubmitRequest(ch, dir_in ? 1 : 0, 2, 1, static_cast<byte*>(buf),
			static_cast<uint16>(len), 0);// EP_TYPE_BULK
		return MAKE_ERROR(Error::kSuccess);
	}

	Error OTGHostDevice::OnHubPortStatusReceived(uint8 port_num, uint16 status, uint16 change) {
		// no hub behind OTG1_HS/OTG2_FS root port for now
		(void)port_num; (void)status; (void)change;
		return MAKE_ERROR(Error::kSuccess);
	}

	// AKA USBH_AllocPipe: one host channel per endpoint address
	Error OTGHostDevice::ConfigureEndpoints() {
		if (endpoints_configured_) {
			return MAKE_ERROR(Error::kSuccess);
		}
		byte next_ch = 1;// channel 0 is the default control pipe
		for (int i = 0; i < NumEndpointConfigs(); ++i) {
			const EndpointConfig& conf = EndpointConfigs()[i];
			if (next_ch > 15) break;
			const byte ep_num = static_cast<byte>(conf.ep_id.Number());
			// keyed by the EndpointID address (2 * number + direction), what class drivers pass around
			const byte ep_id_addr = static_cast<byte>(conf.ep_id.Address() & 0x1F);
			const byte ep_addr = static_cast<byte>(
				conf.ep_id.IsIn() ? (ep_num | 0x80) : ep_num);
			const byte ep_type = static_cast<byte>(conf.ep_type);// kControl=0, kIsochronous=1, kBulk=2, kInterrupt=3
			ch_of_ep_[ep_id_addr] = next_ch;
			ep_of_ch_[next_ch] = ep_id_addr;
			hcd_.InitializeHostChannel(next_ch, ep_addr, dev_address_, speed_,
				ep_type, static_cast<uint16>(conf.max_packet_size));
			next_ch++;
		}
		endpoints_configured_ = true;
		return MAKE_ERROR(Error::kSuccess);
	}

	// Channel of an endpoint address, programming the channels on first use.
	byte OTGHostDevice::ChannelOfEndpoint(EndpointID ep_id) {
		byte ch = ch_of_ep_[ep_id.Address() & 0x1F];
		if (!ch) {
			ConfigureEndpoints();
			ch = ch_of_ep_[ep_id.Address() & 0x1F];
		}
		return ch;
	}

	// Advance the control stage machine on channel-0 URB completion.
	void OTGHostDevice::OnChannelURBCompleted(byte ch_num, URBState urb_state) {
		if (ch_num != 0) {
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
			else if (urb_state == URBState::NotReady || urb_state == URBState::NYET) {
				const HostChannelState ch_state = hcd_.getHostChannelState(ch_num);
				if (ch_state == HostChannelState::Xacterr
					|| ch_state == HostChannelState::DataTglErr) {
					return;// the HCD re-activated this channel by itself
				}
				// NAK is not a failure: the very same URB goes out again
				if (ch_xfer_base_[ch_num] && ch_xfer_len_[ch_num]
					&& ch_nak_retry_[ch_num]++ < kNakRetryLimit) {
					hcd_.SubmitRequest(ch_num, dir_in ? 1 : 0, ep_type, 1,
						ch_xfer_base_[ch_num], ch_xfer_len_[ch_num], 0);
					return;
				}
				if (ep_type == 2) OnBulkCompleted(ep_id, nullptr, 0);
				else OnInterruptCompleted(ep_id, nullptr, 0);
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
			// the status transfer below resets the counters, keep the real byte count
			ctrl_xfer_count_ = static_cast<int>(hcd_.getXferCount(0));
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
			if (mps_probe_) {
				// The 8-byte reply carries bMaxPacketSize0 at descriptor offset 7.
				byte mps = static_cast<const byte*>(ctrl_buf_)[7];
				if (mps != 8 && mps != 16 && mps != 32 && mps != 64) mps = 8;
				hcd_.InitializeHostChannel(0, 0x00, dev_address_, speed_, 0, mps);
				mps_known_ = true;
				mps_probe_ = false;
				// redo the request at the real packet size, as one completion
				ctrl_len_ = ctrl_len_full_;
				ctrl_setup_.length = static_cast<uint16>(ctrl_len_full_);
				ctrl_stage_ = ControlStage::Setup;
				hcd_.SubmitRequest(0, 0, 0, 0, reinterpret_cast<byte*>(&ctrl_setup_), 8, 0);
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

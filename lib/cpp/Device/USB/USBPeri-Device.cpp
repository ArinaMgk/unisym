// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Device.USB) Peripheral Device Stack
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

#include "../../../../inc/cpp/Device/USB/USBPeri-Device.hpp"
#include "../../../../inc/cpp/Device/USB/USBPeri-Class.hpp"
#include "../../../../inc/c/driver/_predefine/predef.usbperi.hpp"

namespace uni::device::SpaceUSB {
#if defined(_MCU_STM32H7x)

	// ---- lifecycle (AKA USBD_Init / DeInit / Start / Stop) ----
	bool PeripheralDevice::setMode(PCD& pcd, PeripheralDescriptor& desc) {
		this->pcd = &pcd;
		this->pdesc = &desc;
		this->dev_state = PeripheralState::Default;
		this->dev_address = 0;
		this->ep0_state = ControlState::Idle;
		return true;
	}

	bool PeripheralDevice::canMode() {
		if (pclass) pclass->canMode((byte)dev_config);
		Stop();
		pcd = nullptr;
		dev_state = PeripheralState::Default;
		return true;
	}

	bool PeripheralDevice::Start() {
		if (pcd) pcd->Start();
		return true;
	}

	bool PeripheralDevice::Stop() {
		if (pcd) pcd->Stop();
		return true;
	}

	// ---- class registration (AKA USBD_RegisterClass) ----
	void PeripheralDevice::RegisterClass(ClassPeripheral& cls) {
		pclass = &cls;
		cls.setParent(this);
	}

	// ---- events ----
	// AKA USBD_LL_SetupStage: parse SETUP, dispatch to standard request handler.
	void PeripheralDevice::HandleSetup() {
		ParseSetup();
		ep0_state = ControlState::Setup;
		ep0_data_len = request.length;

		switch (request.request_type & (_USB_REQ_RECIPIENT_MASK | _USB_REQ_TYPE_MASK)) {
		case _USB_REQ_RECIPIENT_DEVICE:
			HandleStandardDeviceRequest();
			break;
		case _USB_REQ_RECIPIENT_INTERFACE:
			HandleStandardInterfaceRequest();
			break;
		case _USB_REQ_RECIPIENT_ENDPOINT:
			HandleStandardEndpointRequest();
			break;
		default:
			StallControl();
			break;
		}
	}

	// AKA USBD_LL_DataInStage (EP0 IN data stage completed)
	void PeripheralDevice::HandleDataInn() {
		Endpoint* pep = &ep_in[0];
		if (ep0_state == ControlState::DataInn) {
			if (pep->rem_length > pep->maxpacket) {
				pep->rem_length -= pep->maxpacket;
				// continue sending remaining data (caller supplies buffer)
				ReceiveControl(nullptr, 0);   // prepare for premature end
			}
			else {
				// last packet is MPS multiple -> send ZLP
				if ((pep->total_length % pep->maxpacket == 0) &&
					(pep->total_length >= pep->maxpacket) &&
					(pep->total_length < ep0_data_len)) {
					ep0_data_len = 0;
					ReceiveControl(nullptr, 0);
				}
				else {
					if (pclass) pclass->EP0TxSent();
					ReceiveStatus();
				}
			}
		}
		if (dev_test_mode == 1) {
			dev_test_mode = 0;
		}
	}

	// AKA USBD_LL_DataOutStage (EP0 OUT data stage completed)
	void PeripheralDevice::HandleDataOut() {
		Endpoint* pep = &ep_out[0];
		if (ep0_state == ControlState::DataOut) {
			if (pep->rem_length > pep->maxpacket) {
				pep->rem_length -= pep->maxpacket;
				ReceiveControl(nullptr, pep->rem_length > pep->maxpacket ? pep->maxpacket : (uint16)pep->rem_length);
			}
			else {
				if (pclass) pclass->EP0RxReady();
				SendStatus();
			}
		}
	}

	// AKA USBD_LL_Reset
	void PeripheralDevice::HandleReset() {
		OpenEndpoint(0x00, 0, _USB_MAX_EP0_SIZE);   // EP0 OUT, CTRL
		ep_out[0].maxpacket = _USB_MAX_EP0_SIZE;
		OpenEndpoint(0x80, 0, _USB_MAX_EP0_SIZE);   // EP0 IN, CTRL
		ep_in[0].maxpacket = _USB_MAX_EP0_SIZE;
		dev_state = PeripheralState::Default;
		if (pclass) pclass->canMode((byte)dev_config);
	}

	void PeripheralDevice::HandleSuspend() {
		dev_old_state = dev_state;
		dev_state = PeripheralState::Suspended;
	}

	void PeripheralDevice::HandleResume() {
		dev_state = dev_old_state;
	}

	void PeripheralDevice::HandleSOF() {
		if (dev_state == PeripheralState::Configured && pclass) pclass->SOF();
	}

	void PeripheralDevice::HandleConnect() {}
	void PeripheralDevice::HandleDisconnect() {
		dev_state = PeripheralState::Default;
		if (pclass) pclass->canMode((byte)dev_config);
	}

	// ---- EP0 control transfer primitives ----
	void PeripheralDevice::SendControl(const byte* buf, uint16 len) {
		ep0_state = ControlState::DataInn;
		ep_in[0].total_length = len;
		ep_in[0].rem_length = len;
		TransmitEndpoint(0x80, const_cast<byte*>(buf), len);
	}

	void PeripheralDevice::ReceiveControl(byte* buf, uint16 len) {
		ep0_state = ControlState::DataOut;
		ep_out[0].total_length = len;
		ep_out[0].rem_length = len;
		ReceiveEndpoint(0x00, buf, len);
	}

	void PeripheralDevice::SendStatus() {
		ep0_state = ControlState::StatusInn;
		TransmitEndpoint(0x80, nullptr, 0);
	}

	void PeripheralDevice::ReceiveStatus() {
		ep0_state = ControlState::StatusOut;
		ReceiveEndpoint(0x00, nullptr, 0);
	}

	void PeripheralDevice::StallControl() {
		ConfigStall(0x80, true);
		ConfigStall(0x00, true);
	}

	// ---- endpoint ops (delegate to PCD) ----
	void PeripheralDevice::OpenEndpoint(byte ep_addr, byte ep_type, uint16 ep_mps) {
		if (pcd) pcd->OpenEndpoint(ep_addr, ep_mps, ep_type);
	}
	void PeripheralDevice::CloseEndpoint(byte ep_addr) {
		if (pcd) pcd->CloseEndpoint(ep_addr);
	}
	void PeripheralDevice::TransmitEndpoint(byte ep_addr, byte* buf, uint16 len) {
		if (pcd) pcd->TransmitEndpoint(ep_addr, buf, len);
	}
	void PeripheralDevice::ReceiveEndpoint(byte ep_addr, byte* buf, uint16 len) {
		if (pcd) pcd->ReceiveEndpoint(ep_addr, buf, len);
	}
	void PeripheralDevice::ConfigStall(byte ep_addr, bool set_or_reset) {
		if (pcd) pcd->ConfigStall(ep_addr, set_or_reset);
	}
	void PeripheralDevice::FlushEndpoint(byte ep_addr) {
		if (pcd) pcd->FlushEndpoint(ep_addr);
	}
	void PeripheralDevice::setAddress(byte address) {
		if (pcd) pcd->setAddress(address);
	}
	uint16 PeripheralDevice::getRxCount(byte ep_addr) {
		return pcd ? pcd->getRxCount(ep_addr) : 0;
	}

	// ---- standard request handlers ----

	// AKA USBD_StdDevReq
	void PeripheralDevice::HandleStandardDeviceRequest() {
		switch (request.request) {
		case _USB_REQ_GET_DESCRIPTOR:
			getDescriptor();
			break;
		case _USB_REQ_SET_ADDRESS:
			if (request.index == 0 && request.length == 0) {
				byte addr = (byte)(request.value) & 0x7F;
				if (dev_state == PeripheralState::Configured) {
					StallControl();
				}
				else {
					dev_address = addr;
					setAddress(addr);
					SendStatus();
					dev_state = (addr != 0) ? PeripheralState::Address : PeripheralState::Default;
				}
			}
			else {
				StallControl();
			}
			break;
		case _USB_REQ_SET_CONFIGURATION:
			// USBD_SetConfig logic folded here
			{
				byte cfgidx = (byte)(request.value);
				if (cfgidx > 1) {
					StallControl();
					break;
				}
				if (dev_state == PeripheralState::Address) {
					if (cfgidx) {
						dev_config = cfgidx;
						dev_state = PeripheralState::Configured;
						if (!SetClassConfiguration(cfgidx)) { StallControl(); break; }
						SendStatus();
					}
					else SendStatus();
				}
				else if (dev_state == PeripheralState::Configured) {
					if (cfgidx == 0) {
						dev_state = PeripheralState::Address;
						dev_config = 0;
						ClearClassConfiguration(0);
						SendStatus();
					}
					else if (cfgidx != dev_config) {
						ClearClassConfiguration((byte)dev_config);
						dev_config = cfgidx;
						if (!SetClassConfiguration(cfgidx)) { StallControl(); break; }
						SendStatus();
					}
					else SendStatus();
				}
				else StallControl();
			}
			break;
		case _USB_REQ_GET_CONFIGURATION:
			if (request.length != 1) StallControl();
			else {
				byte cfg = (dev_state == PeripheralState::Configured) ? (byte)dev_config : 0;
				SendControl(&cfg, 1);
			}
			break;
		case _USB_REQ_GET_STATUS:
			if (dev_state == PeripheralState::Address || dev_state == PeripheralState::Configured) {
				byte status[2] = { 0, 0 };
				if (dev_remote_wakeup) status[0] |= 0x02;   // USB_CONFIG_REMOTE_WAKEUP
				SendControl(status, 2);
			}
			else StallControl();
			break;
		case _USB_REQ_SET_FEATURE:
			if (request.value == _USB_FEATURE_REMOTE_WAKEUP) {
				dev_remote_wakeup = true;
				if (pclass) pclass->Setup();
				SendStatus();
			}
			break;
		case _USB_REQ_CLEAR_FEATURE:
			if (request.value == _USB_FEATURE_REMOTE_WAKEUP) {
				dev_remote_wakeup = false;
				if (pclass) pclass->Setup();
				SendStatus();
			}
			break;
		default:
			StallControl();
			break;
		}
	}

	// AKA USBD_StdItfReq
	void PeripheralDevice::HandleStandardInterfaceRequest() {
		if (dev_state == PeripheralState::Configured) {
			if ((byte)(request.index) <= 1) {
				if (pclass) pclass->Setup();
				if (request.length == 0) SendStatus();
			}
			else StallControl();
		}
		else StallControl();
	}

	// AKA USBD_StdEPReq
	void PeripheralDevice::HandleStandardEndpointRequest() {
		byte ep_addr = (byte)(request.index);
		// class request goes to the class Setup
		if ((request.request_type & _USB_REQ_TYPE_MASK) == _USB_REQ_TYPE_CLASS) {
			if (pclass) pclass->Setup();
			return;
		}
		switch (request.request) {
		case _USB_REQ_SET_FEATURE:
			if (request.value == _USB_FEATURE_EP_HALT && dev_state == PeripheralState::Configured) {
				if (ep_addr != 0x00 && ep_addr != 0x80) ConfigStall(ep_addr, true);
				if (pclass) pclass->Setup();
				SendStatus();
			}
			else StallControl();
			break;
		case _USB_REQ_CLEAR_FEATURE:
			if (request.value == _USB_FEATURE_EP_HALT && dev_state == PeripheralState::Configured) {
				if ((ep_addr & 0x7F) != 0x00) {
					ConfigStall(ep_addr, false);
					if (pclass) pclass->Setup();
				}
				SendStatus();
			}
			else StallControl();
			break;
		case _USB_REQ_GET_STATUS:
			if (dev_state == PeripheralState::Configured) {
				Endpoint* pep = (ep_addr & 0x80) ? &ep_in[ep_addr & 0x7F] : &ep_out[ep_addr & 0x7F];
				pep->status = 0;
				byte status[2] = { (byte)pep->status, (byte)(pep->status >> 8) };
				SendControl(status, 2);
			}
			else StallControl();
			break;
		default:
			break;
		}
	}

	// ---- descriptor dispatch (AKA USBD_GetDescriptor) ----
	void PeripheralDevice::getDescriptor() {
		uint16 len = 0;
		const byte* pbuf = nullptr;
		byte desc_type = (byte)(request.value >> 8);

		switch (desc_type) {
		case _USB_DESC_TYPE_DEVICE:
			if (pdesc && pdesc->GetDeviceDescriptor) pbuf = pdesc->GetDeviceDescriptor(dev_speed, len);
			break;
		case _USB_DESC_TYPE_CONFIGURATION:
			if (pclass) pbuf = pclass->getConfigurationDescriptor(len);
			break;
		case _USB_DESC_TYPE_STRING: {
			byte idx = (byte)(request.value);
			if (pdesc) {
				switch (idx) {
				case _USBD_IDX_LANGID_STR:   if (pdesc->GetLangIDStrDescriptor)       pbuf = pdesc->GetLangIDStrDescriptor(dev_speed, len); break;
				case _USBD_IDX_MFC_STR:      if (pdesc->GetManufacturerStrDescriptor) pbuf = pdesc->GetManufacturerStrDescriptor(dev_speed, len); break;
				case _USBD_IDX_PRODUCT_STR:  if (pdesc->GetProductStrDescriptor)      pbuf = pdesc->GetProductStrDescriptor(dev_speed, len); break;
				case _USBD_IDX_SERIAL_STR:   if (pdesc->GetSerialStrDescriptor)       pbuf = pdesc->GetSerialStrDescriptor(dev_speed, len); break;
				case _USBD_IDX_CONFIG_STR:   if (pdesc->GetConfigurationStrDescriptor) pbuf = pdesc->GetConfigurationStrDescriptor(dev_speed, len); break;
				case _USBD_IDX_INTERFACE_STR:if (pdesc->GetInterfaceStrDescriptor)    pbuf = pdesc->GetInterfaceStrDescriptor(dev_speed, len); break;
				default: StallControl(); return;
				}
			}
			break;
		}
		case _USB_DESC_TYPE_DEVICE_QUALIFIER:
			if (dev_speed == PeripheralSpeed::High && pclass) pbuf = pclass->getDeviceQualifierDescriptor(len);
			else { StallControl(); return; }
			break;
		case _USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION:
			if (dev_speed == PeripheralSpeed::High && pclass) pbuf = pclass->getOtherSpeedConfigurationDescriptor(len);
			else { StallControl(); return; }
			break;
		default:
			StallControl();
			return;
		}

		if (len != 0 && request.length != 0) {
			if (len > request.length) len = request.length;
			SendControl(pbuf, len);
		}
	}

	// AKA USBD_ParseSetupRequest
	void PeripheralDevice::ParseSetup() {
		const byte* p = reinterpret_cast<const byte*>(pcd->Setup);
		request.request_type = p[0];
		request.request = p[1];
		request.value = (uint16)(p[2]) | ((uint16)(p[3]) << 8);
		request.index = (uint16)(p[4]) | ((uint16)(p[5]) << 8);
		request.length = (uint16)(p[6]) | ((uint16)(p[7]) << 8);
	}

	// ---- class config ----
	bool PeripheralDevice::SetClassConfiguration(byte cfgidx) {
		return pclass ? pclass->setMode(cfgidx) : false;
	}
	bool PeripheralDevice::ClearClassConfiguration(byte cfgidx) {
		return pclass ? pclass->canMode(cfgidx) : false;
	}



#endif // _MCU_STM32H7x
}

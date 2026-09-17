// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [Device.USB] Peripheral CDC Class
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

#include "../../../../inc/cpp/Device/USB/USBPeri-CDC.hpp"
#include "../../../../inc/c/driver/_predefine/predef.usbperi.hpp"

namespace uni::device::SpaceUSB {
#if defined(_MCU_STM32H7x)

	// ---- constants ----
	#define _CDC_MAX_FS_PACKET        0x40
	#define _CDC_CMD_PACKET           0x10
	#define _USB_CDC_CONFIG_DESC_SIZE 67

	// CDC class-specific descriptor subtypes (AKA USB_CDC_DESC_TYPE_*)
	#define _CDC_DESC_HEADER          0x00
	#define _CDC_DESC_CALL_MGMT       0x01
	#define _CDC_DESC_ACM             0x02
	#define _CDC_DESC_UNION           0x06

	// bmCapabilities of the ACM functional descriptor: bit1 = Set/Get_Line_Coding
	#define _CDC_ACM_SUPPORT_LINE_CODING 0x02

	// ---- configuration descriptor (AKA USBD_CDC_CfgFSDesc) ----
	// 67 bytes, so it does NOT fit in a single 64-byte EP0 packet: the control-IN data
	// stage spans two packets and relies on the device stack's continue-send path.
	static const byte _cfg_desc_fs[_USB_CDC_CONFIG_DESC_SIZE] = {
		// ---- Configuration descriptor (9) ----
		0x09, _USB_DESC_TYPE_CONFIGURATION,
		_USB_CDC_CONFIG_DESC_SIZE, 0x00,// wTotalLength
		0x02,                           // bNumInterfaces: communication + data
		0x01,                           // bConfigurationValue
		0x00,                           // iConfiguration
		0xC0,                           // bmAttributes: self-powered
		0x32,                           // bMaxPower: 100 mA

		// ---- Interface 0: communication (9) ----
		0x09, _USB_DESC_TYPE_INTERFACE,
		0x00,                           // bInterfaceNumber
		0x00,                           // bAlternateSetting
		0x01,                           // bNumEndpoints: the notification pipe
		0x02,                           // bInterfaceClass: CDC
		0x02,                           // bInterfaceSubClass: ACM
		0x01,                           // bInterfaceProtocol: AT commands
		0x00,                           // iInterface

		// ---- Header functional descriptor (5) ----
		0x05, 0x24, _CDC_DESC_HEADER,
		0x10, 0x01,                     // bcdCDC 1.10

		// ---- Call management functional descriptor (5) ----
		0x05, 0x24, _CDC_DESC_CALL_MGMT,
		0x00,                           // bmCapabilities: no call management
		0x01,                           // bDataInterface

		// ---- ACM functional descriptor (4) ----
		0x04, 0x24, _CDC_DESC_ACM,
		_CDC_ACM_SUPPORT_LINE_CODING,

		// ---- Union functional descriptor (5) ----
		0x05, 0x24, _CDC_DESC_UNION,
		0x00,                           // bMasterInterface: communication
		0x01,                           // bSlaveInterface0: data

		// ---- Endpoint 0x82: interrupt IN notification (7) ----
		0x07, _USB_DESC_TYPE_ENDPOINT,
		0x82,                           // bEndpointAddress: IN 2
		0x03,                           // bmAttributes: interrupt
		_CDC_CMD_PACKET, 0x00,          // wMaxPacketSize
		0x10,                           // bInterval

		// ---- Interface 1: data (9) ----
		0x09, _USB_DESC_TYPE_INTERFACE,
		0x01,                           // bInterfaceNumber
		0x00,                           // bAlternateSetting
		0x02,                           // bNumEndpoints: bulk OUT + bulk IN
		0x0A,                           // bInterfaceClass: CDC data
		0x00,                           // bInterfaceSubClass
		0x00,                           // bInterfaceProtocol
		0x00,                           // iInterface

		// ---- Endpoint 0x01: bulk OUT, host -> device (7) ----
		0x07, _USB_DESC_TYPE_ENDPOINT,
		0x01,                           // bEndpointAddress: OUT 1
		0x02,                           // bmAttributes: bulk
		_CDC_MAX_FS_PACKET, 0x00,       // wMaxPacketSize
		0x00,                           // bInterval (ignored for bulk)

		// ---- Endpoint 0x81: bulk IN, device -> host (7) ----
		0x07, _USB_DESC_TYPE_ENDPOINT,
		0x81,                           // bEndpointAddress: IN 1
		0x02,                           // bmAttributes: bulk
		_CDC_MAX_FS_PACKET, 0x00,       // wMaxPacketSize
		0x00,                           // bInterval
	};

	// ---- ClassPeripheral: init (AKA USBD_CDC_Init) ----
	bool USBPeri_CDC::setMode(byte cfgidx) {
		(void)cfgidx;
		PeripheralDevice* dev = Parent();
		if (!dev) return false;
		// data interface: bulk both ways
		dev->OpenEndpoint(ep_out_addr, 2, _CDC_MAX_FS_PACKET);
		dev->OpenEndpoint(ep_in_addr, 2, _CDC_MAX_FS_PACKET);
		// communication interface: the notification pipe exists but only NAKs. Leaving
		// it unopened is what makes some hosts retry the request forever.
		dev->OpenEndpoint(ep_cmd_addr, 3, _CDC_CMD_PACKET);
		// drop whatever the FIFOs still hold from the previous session, then arm the
		// bulk OUT so a received packet has somewhere to land
		dev->FlushEndpoint(ep_out_addr);
		dev->FlushEndpoint(ep_in_addr);
		dev->ReceiveEndpoint(ep_out_addr, rx_scratch, sizeof(rx_scratch));
		// Per-session state only. The packet/byte counters are LIFETIME totals and must
		// NOT be cleared here: setMode() runs again on every re-enumeration, and zeroing
		// them would make the application's per-second deltas wrap around into a bogus
		// "4294967294 bytes/s" line.
		rx_rd = rx_wr;
		tx_rd = tx_wr;
		tx_active = false;
		line_state = 0;
		line_coding_updated = false;
		line_state_updated = false;
		return true;
	}

	bool USBPeri_CDC::canMode(byte cfgidx) {
		(void)cfgidx;
		PeripheralDevice* dev = Parent();
		if (!dev) return false;
		// bus reset / cable unplug / SET_CONFIGURATION(0): end the session cleanly, so a
		// re-plug starts from empty queues and empty FIFOs (hot-plug recovery)
		tx_active = false;// whatever was in flight is not coming back
		rx_rd = rx_wr;
		tx_rd = tx_wr;
		dev->FlushEndpoint(ep_out_addr);
		dev->FlushEndpoint(ep_in_addr);
		dev->CloseEndpoint(ep_out_addr);
		dev->CloseEndpoint(ep_in_addr);
		dev->CloseEndpoint(ep_cmd_addr);
		line_state = 0;
		line_state_updated = true;// let the application report the port went away
		return true;
	}

	// ---- ClassPeripheral: setup (AKA USBD_CDC_Setup) ----
	bool USBPeri_CDC::Setup() {
		PeripheralDevice* dev = Parent();
		if (!dev) return false;
		const SetupPacket& req = dev->request;
		byte rtype = req.request_type & _USB_REQ_TYPE_MASK;
		bool to_device = (req.request_type & 0x80) == 0;

		if (rtype == _USB_REQ_TYPE_CLASS) {
			switch (req.request) {
			case (byte)CDCRequest::SetControlLineState:
				// No data stage, so THIS function owns the whole transfer: without the
				// status stage below, opening the port on the host would never complete.
				if (!to_device || req.length != 0) { dev->StallControl(); return false; }
				line_state = (byte)(req.value & 0x03);
				line_state_updated = true;
				dev->SendStatus();// AKA USBD_CtlSendStatus
				return true;

			case (byte)CDCRequest::SetLineCoding:
				// 7 bytes OUT: hand the endpoint to the stack; EP0RxReady() runs once the
				// data has landed in line_coding (the stack sends the status then).
				if (to_device && req.length == sizeof(CDCLineCoding)) {
					dev->ReceiveControl(reinterpret_cast<byte*>(&line_coding), sizeof(CDCLineCoding));
					return true;
				}
				dev->StallControl();
				return false;

			case (byte)CDCRequest::GetLineCoding:
				// 7 bytes IN (the stack sends the status stage afterwards)
				if (!to_device && req.length >= sizeof(CDCLineCoding)) {
					dev->SendControl(reinterpret_cast<const byte*>(&line_coding), sizeof(CDCLineCoding));
					return true;
				}
				dev->StallControl();
				return false;

			default:
				dev->StallControl();
				return false;
			}
		}
		// Standard interface requests are routed here by the device stack, which sends
		// the status stage itself, so only GET_INTERFACE has to be answered here.
		if (rtype == _USB_REQ_TYPE_STANDARD && req.request == _USB_REQ_GET_INTERFACE) {
			byte alt = 0;
			dev->SendControl(&alt, 1);
			return true;
		}
		return true;
	}

	// AKA USBD_CDC_EP0_RxReady: the SET_LINE_CODING payload has arrived in line_coding.
	bool USBPeri_CDC::EP0RxReady() {
		line_coding_updated = true;
		return true;
	}

	// ---- ClassPeripheral: data plane ----
	// AKA CDC_Receive_FS: one bulk OUT packet is done and sits in rx_scratch. Queue it
	// for the application, then re-arm the endpoint right away.
	bool USBPeri_CDC::out(byte epnum) {
		if (epnum != (ep_out_addr & 0x7F)) return true;
		PeripheralDevice* dev = Parent();
		if (!dev) return true;
		uint16 n = dev->getRxCount(ep_out_addr);
		if (n) {
			if (n > (_CDC_RX_QUEUE_SIZE - (rx_wr - rx_rd))) {
				// queue full: drop this whole packet instead of tearing it apart
				rx_overflow++;
			}
			else {
				for (uint16 i = 0; i < n; i++) rx_ring[(rx_wr + i) % _CDC_RX_QUEUE_SIZE] = rx_scratch[i];
				rx_wr += n;
				rx_packets++;
				rx_bytes += n;
			}
		}
		else rx_empty++;
		dev->ReceiveEndpoint(ep_out_addr, rx_scratch, sizeof(rx_scratch));
		return true;
	}

	// AKA the application side of CDC_Receive_FS: drain the queue from the main loop.
	uint32 USBPeri_CDC::Rx(byte* dst, uint32 maxlen) {
		uint32 avail = rx_wr - rx_rd;
		uint32 take = (avail < maxlen) ? avail : maxlen;
		for (uint32 i = 0; i < take; i++) dst[i] = rx_ring[(rx_rd + i) % _CDC_RX_QUEUE_SIZE];
		rx_rd += take;
		return take;
	}

	// ---- ClassPeripheral: data plane (bulk IN) ----
	// AKA CDC_Transmit_FS: queue bytes for the host. Called from the application; the
	// interrupt side (inn) keeps feeding packets until the queue runs dry.
	bool USBPeri_CDC::Transmit(const byte* buf, uint16 len) {
		if (!len) return true;
		if (len > (_CDC_TX_QUEUE_SIZE - (tx_wr - tx_rd))) {
			// backpressure: the host is not reading, drop instead of blocking the caller
			tx_dropped++;
			return false;
		}
		for (uint16 i = 0; i < len; i++) tx_ring[(tx_wr + i) % _CDC_TX_QUEUE_SIZE] = buf[i];
		tx_wr += len;
		tx_bytes += len;
		TxPump();
		return true;
	}

	// Start the next IN packet if the queue has data and none is in flight. Only one
	// context can be pumping at a time: tx_active is set here and cleared in inn(),
	// and inn() can only run while a transfer is in flight.
	void USBPeri_CDC::TxPump() {
		if (tx_active) return;
		PeripheralDevice* dev = Parent();
		if (!dev) return;
		uint32 avail = tx_wr - tx_rd;
		if (!avail) return;
		uint32 take = (avail < sizeof(tx_packet)) ? avail : sizeof(tx_packet);
		for (uint32 i = 0; i < take; i++) tx_packet[i] = tx_ring[(tx_rd + i) % _CDC_TX_QUEUE_SIZE];
		tx_rd += take;
		tx_active = true;
		tx_packets++;
		// tx_packet must stay untouched until this transfer completes: the non-DMA IN
		// path reads it from the FIFO-empty interrupt.
		dev->TransmitEndpoint(ep_in_addr, tx_packet, (uint16)take);
	}

	// AKA CDC_Transmit_FS completion: the previous IN packet is gone, send the next.
	bool USBPeri_CDC::inn(byte epnum) {
		if (epnum != (ep_in_addr & 0x7F)) return true;
		tx_active = false;
		TxPump();
		return true;
	}

	// ---- ClassPeripheral: descriptors ----
	const byte* USBPeri_CDC::getConfigurationDescriptor(uint16& len) {
		len = sizeof(_cfg_desc_fs);
		return _cfg_desc_fs;
	}
	const byte* USBPeri_CDC::getOtherSpeedConfigurationDescriptor(uint16& len) {
		len = sizeof(_cfg_desc_fs);
		return _cfg_desc_fs;
	}
	const byte* USBPeri_CDC::getDeviceQualifierDescriptor(uint16& len) {
		static const byte dev_qualifier[10] = {
			10, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 64, 0x01, 0x00
		};
		len = sizeof(dev_qualifier);
		return dev_qualifier;
	}

#endif // _MCU_STM32H7x
}

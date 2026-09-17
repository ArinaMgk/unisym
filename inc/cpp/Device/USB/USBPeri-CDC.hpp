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

#ifndef _INC_DEVICE_USB_PERIPHERAL_CDC
#define _INC_DEVICE_USB_PERIPHERAL_CDC

#include "../../../c/stdinc.h"
#include "USBPeri-Device.hpp"
#include "USBPeri-Class.hpp"

#if defined(_MCU_STM32H7x)

// CDC = Communications Device Class; ACM = Abstract Control Model, the subclass making
// the device look like a plain serial port (AKA "USB virtual COM port", VCP).
// No vendor driver is needed: Win10/11 binds usbser.sys from the class code alone.
//
// Layout declared by this class (AKA USBD_CDC_CfgFSDesc):
//   Interface 0  communication (class 0x02 / subclass 0x02 ACM / protocol 0x01 AT)
//     + Header / CallManagement / ACM / Union class-specific descriptors
//     + EP 0x82 interrupt IN 16B   notification pipe (kept NAKing)
//   Interface 1  data (class 0x0A)
//     + EP 0x01 bulk OUT 64B       host -> device
//     + EP 0x81 bulk IN  64B       device -> host

namespace uni::device::SpaceUSB {

	// AKA USBD_CDC_REQ_*
	enum class CDCRequest : byte {
		SetLineCoding = 0x20,// host -> device, 7 bytes
		GetLineCoding = 0x21,// device -> host, 7 bytes
		SetControlLineState = 0x22,// no data stage: DTR/RTS
		SendBreak = 0x23,
	};

	// AKA USBD_CDC_LineCodingTypeDef: exactly the 7 bytes travelling on the wire.
	_PACKED(struct) CDCLineCoding {
		stduint dwDTERate;// baudrate
		byte bCharFormat;// 0:1 stop, 1:1.5 stop, 2:2 stop
		byte bParityType;// 0:none, 1:odd, 2:even, 3:mark, 4:space
		byte bDataBits;// 5..8, 16
	};

	// queue sizes: the receive queue holds what the host sent and the application has
	// not drained yet; the transmit queue holds what the application queued and the
	// controller has not sent yet.
	#define _CDC_RX_QUEUE_SIZE 1024
	#define _CDC_TX_QUEUE_SIZE 1024

	class USBPeri_CDC : public ClassPeripheral {
	public:
		// endpoint addresses (AKA CDC_IN_EP / CDC_OUT_EP / CDC_CMD_EP)
		byte ep_in_addr = 0x81;
		byte ep_out_addr = 0x01;
		byte ep_cmd_addr = 0x82;

		// ---- application-visible state (AKA the USBD_CDC_ItfTypeDef callbacks) ----
		// The class only records what the host asked for; the application polls these
		// flags from its main loop and prints/acts there. Nothing here blocks, prints or
		// allocates, because Setup()/EP0RxReady()/out() all run in interrupt context.
		CDCLineCoding line_coding{ 115200, 0, 0, 8 };
		volatile byte line_state = 0;// bit0 DTR, bit1 RTS (last SET_CONTROL_LINE_STATE)
		volatile bool line_coding_updated = false;// set when the host sent SET_LINE_CODING
		volatile bool line_state_updated = false;// set when DTR/RTS changed
		volatile uint32 rx_packets = 0;// bulk OUT packets queued
		volatile uint32 rx_empty = 0;// bulk OUT completions carrying no data
		volatile uint32 rx_bytes = 0;// lifetime total of queued bytes
		volatile uint32 rx_overflow = 0;// packets dropped because the queue was full

		// AKA "the host opened the port" (DTR): Windows asserts it when a terminal opens.
		bool isOpen() const { return (line_state & 0x01) != 0; }

		// ---- received-byte queue (AKA CDC_Receive_FS destination) ----
		// The interrupt handler queues, the application drains: single producer, single
		// consumer, so no locking and no blocking. The application polls RxCount() from
		// its main loop - never call these from Setup()/out()/inn() context.
		uint32 RxCount() const { return rx_wr - rx_rd; }
		uint32 Rx(byte* dst, uint32 maxlen);// pop up to maxlen bytes, return how many
		void RxFlush() { rx_rd = rx_wr; }

		// ---- transmit queue (AKA CDC_Transmit_FS) ----
		// Queue bytes for the host (bulk IN). The application calls this from its main
		// loop; the interrupt drains one packet per IN completion (inn). A false return
		// is the backpressure signal: the host is not reading fast enough and the queue
		// is full - the demo counts it instead of blocking.
		bool Transmit(const byte* buf, uint16 len);
		uint32 TxFree() const { return _CDC_TX_QUEUE_SIZE - (tx_wr - tx_rd); }
		bool TxBusy() const { return tx_active; }
		volatile uint32 tx_bytes = 0;// bytes accepted for transmission
		volatile uint32 tx_dropped = 0;// Transmit() calls rejected because the queue was full
		volatile uint32 tx_packets = 0;// IN packets handed to the controller

		// ---- ClassPeripheral: configuration ----
		bool setMode(byte cfgidx) override;
		bool canMode(byte cfgidx) override;

		// ---- ClassPeripheral: control plane ----
		bool Setup() override;
		bool EP0RxReady() override;

		// ---- ClassPeripheral: data plane (bulk endpoints) ----
		// A received packet lands in rx_scratch and is queued into rx_ring here; the
		// endpoint is re-armed immediately, because an unarmed OUT endpoint makes the
		// core drop packets (the RxFIFO handler would have nowhere to copy them).
		bool out(byte epnum) override;
		bool inn(byte epnum) override;

		// ---- ClassPeripheral: descriptors ----
		const byte* getConfigurationDescriptor(uint16& len) override;
		const byte* getOtherSpeedConfigurationDescriptor(uint16& len) override;
		const byte* getDeviceQualifierDescriptor(uint16& len) override;

	private:
		void TxPump();// start the next IN packet when none is in flight
		byte rx_scratch[64];// landing buffer for one bulk OUT packet (<= MPS)
		byte rx_ring[_CDC_RX_QUEUE_SIZE];
		volatile uint32 rx_wr = 0, rx_rd = 0;
		byte tx_ring[_CDC_TX_QUEUE_SIZE];
		byte tx_packet[64];// persistent source of the IN packet in flight
		volatile uint32 tx_wr = 0, tx_rd = 0;
		volatile bool tx_active = false;
	};

}

#endif // _MCU_STM32H7x

#endif // _INC_DEVICE_USB_PERIPHERAL_CDC

// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [Device.USB] Peripheral MSC Class
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

#include "../../../../inc/cpp/Device/USB/USBPeri-MSC.hpp"
#include "../../../../inc/c/driver/_predefine/predef.usbperi.hpp"

namespace uni::device::SpaceUSB {
#if defined(_MCU_STM32H7x)

	// ---- constants ----
	#define _MSC_MAX_FS_PACKET      0x40
	#define _MSC_MAX_HS_PACKET      0x200
	#define _BOT_GET_MAX_LUN        0xFE
	#define _BOT_RESET              0xFF
	#define _USB_MSC_CONFIG_DESC_SIZE 32

	#define _CBW_SIGNATURE          0x43425355
	#define _CSW_SIGNATURE          0x53425355
	#define _CBW_LENGTH             31
	#define _CSW_LENGTH             13

	#define _CSW_CMD_PASSED         0x00
	#define _CSW_CMD_FAILED         0x01
	#define _CSW_PHASE_ERROR        0x02

	#define _SENSE_LIST_DEPTH       4
	#define _REQUEST_SENSE_DATA_LEN 18
	#define _STANDARD_INQUIRY_DATA_LEN 36
	#define _LENGTH_INQUIRY_PAGE00  7
	#define _READ_CAPACITY10_DATA_LEN 8
	#define _READ_FORMAT_CAPACITY_DATA_LEN 12
	#define _MODE_SENSE6_DATA_LEN   4
	#define _MODE_SENSE10_DATA_LEN  8

	// inquiry page 00 / mode sense data
	static const byte _page00_inquiry[] = { 0x00, 0x00, 0x00, 0x07, 0x00, 0x80, 0x83 };
	static const byte _mode_sense6[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	static const byte _mode_sense10[] = { 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	// ---- configuration descriptor (AKA USBD_MSC_CfgHSDesc / CfgFSDesc) ----
	static const byte _cfg_desc_hs[] = {
		0x09, _USB_DESC_TYPE_CONFIGURATION, _USB_MSC_CONFIG_DESC_SIZE, 0x00,
		0x01, 0x01, 0x04, 0xC0, 0x32,
		// interface
		0x09, _USB_DESC_TYPE_INTERFACE, 0x00, 0x00, 0x02,
		0x08, 0x06, 0x50, 0x05,
		// EP IN (maxpacket 512)
		0x07, _USB_DESC_TYPE_ENDPOINT, 0x81, 0x02, 0x00, 0x02, 0x00,
		// EP OUT (maxpacket 512)
		0x07, _USB_DESC_TYPE_ENDPOINT, 0x01, 0x02, 0x00, 0x02, 0x00,
	};
	static const byte _cfg_desc_fs[] = {
		0x09, _USB_DESC_TYPE_CONFIGURATION, _USB_MSC_CONFIG_DESC_SIZE, 0x00,
		0x01, 0x01, 0x04, 0xC0, 0x32,
		// interface
		0x09, _USB_DESC_TYPE_INTERFACE, 0x00, 0x00, 0x02,
		0x08, 0x06, 0x50, 0x05,
		// EP IN (maxpacket 64)
		0x07, _USB_DESC_TYPE_ENDPOINT, 0x81, 0x02, 0x40, 0x00, 0x00,
		// EP OUT (maxpacket 64)
		0x07, _USB_DESC_TYPE_ENDPOINT, 0x01, 0x02, 0x40, 0x00, 0x00,
	};

	// ---- lifecycle: heap burst buffer (AKA USBD_malloc(sizeof HandleTypeDef)) ----
	USBPeri_MSC::USBPeri_MSC() {
		bot_data = new byte[media_packet];
	}
	USBPeri_MSC::~USBPeri_MSC() {
		if (bot_data) { delete[] bot_data; bot_data = nullptr; }
	}

	// bind the block device + standard inquiry data (single LUN)
	void USBPeri_MSC::Bind(PeripheralDevice& dev, StorageTrait* storage, const byte* inquiry) {
		dev.RegisterClass(*this);
		storage_ = storage;
		inquiry_ = inquiry;
	}

	// ---- ClassPeripheral: init (AKA USBD_MSC_Init) ----
	bool USBPeri_MSC::setMode(byte cfgidx) {
		(void)cfgidx;
		PeripheralDevice* dev = Parent();
		if (!dev) return false;
		byte mps = (dev->getSpeed() == PeripheralSpeed::High) ? _MSC_MAX_HS_PACKET : _MSC_MAX_FS_PACKET;
		dev->OpenEndpoint(ep_out_addr, 2, mps);   // EP_TYPE_BULK
		dev->OpenEndpoint(ep_in_addr, 2, mps);
		// init BOT
		bot_state = BotState::Idle;
		bot_status = 0;
		sense_head = 0;
		sense_tail = 0;
		dev->FlushEndpoint(ep_out_addr);
		dev->FlushEndpoint(ep_in_addr);
		dev->ReceiveEndpoint(ep_out_addr, reinterpret_cast<byte*>(&cbw), _CBW_LENGTH);
		return true;
	}

	bool USBPeri_MSC::canMode(byte cfgidx) {
		(void)cfgidx;
		PeripheralDevice* dev = Parent();
		if (!dev) return false;
		dev->CloseEndpoint(ep_out_addr);
		dev->CloseEndpoint(ep_in_addr);
		bot_state = BotState::Idle;
		return true;
	}

	// ---- ClassPeripheral: setup (AKA USBD_MSC_Setup) ----
	bool USBPeri_MSC::Setup() {
		PeripheralDevice* dev = Parent();
		if (!dev) return false;
		const SetupPacket& req = dev->request;
		byte rtype = req.request_type & _USB_REQ_TYPE_MASK;

		if (rtype == _USB_REQ_TYPE_CLASS) {
			switch (req.request) {
			case _BOT_GET_MAX_LUN:
				if (req.value == 0 && req.length == 1 && (req.request_type & 0x80) == 0x80) {
					byte mlun = 0;   // single LUN -> max_lun = 0
					dev->SendControl(&mlun, 1);
				}
				else { dev->StallControl(); return false; }
				break;
			case _BOT_RESET:
				if (req.value == 0 && req.length == 0 && (req.request_type & 0x80) != 0x80) {
					BotReset();
				}
				else { dev->StallControl(); return false; }
				break;
			default:
				dev->StallControl(); return false;
			}
		}
		else if (rtype == _USB_REQ_TYPE_STANDARD) {
			switch (req.request) {
			case _USB_REQ_GET_INTERFACE:
				dev->SendControl(&interface, 1);
				break;
			case _USB_REQ_SET_INTERFACE:
				interface = (byte)req.value;
				break;
			case _USB_REQ_CLEAR_FEATURE: {
				byte ep = (byte)req.index;
				dev->FlushEndpoint(ep);
				dev->CloseEndpoint(ep);
				byte mps = (dev->getSpeed() == PeripheralSpeed::High) ? _MSC_MAX_HS_PACKET : _MSC_MAX_FS_PACKET;
				if (ep & 0x80) dev->OpenEndpoint(ep_in_addr, 2, mps);
				else dev->OpenEndpoint(ep_out_addr, 2, mps);
				CompleteClearFeature(ep);
				break;
			}
			default:
				break;
			}
		}
		return true;
	}

	// ---- ClassPeripheral: data stages ----
	bool USBPeri_MSC::inn(byte epnum) {
		(void)epnum;
		switch (bot_state) {
		case BotState::DataInn:
			if (ProcessCmd() < 0) SendCSW(_CSW_CMD_FAILED);
			break;
		case BotState::SendData:
		case BotState::LastDataInn:
			SendCSW(_CSW_CMD_PASSED);
			break;
		default:
			break;
		}
		return true;
	}

	bool USBPeri_MSC::out(byte epnum) {
		(void)epnum;
		switch (bot_state) {
		case BotState::Idle:
			CBWDecode();
			break;
		case BotState::DataOut:
			if (ProcessCmd() < 0) SendCSW(_CSW_CMD_FAILED);
			break;
		default:
			break;
		}
		return true;
	}

	// ---- BOT: reset (AKA MSC_BOT_Reset) ----
	void USBPeri_MSC::BotReset() {
		bot_state = BotState::Idle;
		bot_status = 1;   // recovery
		PeripheralDevice* dev = Parent();
		if (dev) dev->ReceiveEndpoint(ep_out_addr, reinterpret_cast<byte*>(&cbw), _CBW_LENGTH);
	}

	// ---- BOT: CBW decode (AKA MSC_BOT_CBW_Decode) ----
	void USBPeri_MSC::CBWDecode() {
		PeripheralDevice* dev = Parent();
		csw.tag = cbw.tag;
		csw.data_residue = cbw.data_length;

		if ((dev && dev->getRxCount(ep_out_addr) != _CBW_LENGTH) ||
			(cbw.signature != _CBW_SIGNATURE) ||
			(cbw.lun > 0) ||
			(cbw.cb_length < 1) ||
			(cbw.cb_length > 16)) {
			SenseCode(SenseKey::IllegalRequest, AdditionalSenseCode::InvalidCdb);
			bot_status = 2;   // error
			BotAbort();
			return;
		}

		if (ProcessCmd() < 0) {
			if (bot_state == BotState::NoData) SendCSW(_CSW_CMD_FAILED);
			else BotAbort();
		}
		else if (bot_state != BotState::DataInn && bot_state != BotState::DataOut && bot_state != BotState::LastDataInn) {
			if (bot_data_length > 0) SendData(bot_data, bot_data_length);
			else if (bot_data_length == 0) SendCSW(_CSW_CMD_PASSED);
		}
	}

	// ---- BOT: send data (AKA MSC_BOT_SendData) ----
	void USBPeri_MSC::SendData(byte* buf, uint16 len) {
		PeripheralDevice* dev = Parent();
		if (!dev) return;
		stduint want = cbw.data_length;
		if (len > want) len = (uint16)want;
		csw.data_residue -= len;
		csw.status = _CSW_CMD_PASSED;
		bot_state = BotState::SendData;
		dev->TransmitEndpoint(ep_in_addr, buf, len);
	}

	// ---- BOT: send CSW (AKA MSC_BOT_SendCSW) ----
	void USBPeri_MSC::SendCSW(byte status) {
		PeripheralDevice* dev = Parent();
		if (!dev) return;
		csw.signature = _CSW_SIGNATURE;
		csw.status = status;
		bot_state = BotState::Idle;
		dev->TransmitEndpoint(ep_in_addr, reinterpret_cast<byte*>(&csw), _CSW_LENGTH);
		dev->ReceiveEndpoint(ep_out_addr, reinterpret_cast<byte*>(&cbw), _CBW_LENGTH);
	}

	// ---- BOT: abort (AKA MSC_BOT_Abort) ----
	void USBPeri_MSC::BotAbort() {
		PeripheralDevice* dev = Parent();
		if (!dev) return;
		if ((cbw.flags == 0) && (cbw.data_length != 0) && (bot_status == 0)) {
			dev->ConfigStall(ep_out_addr, true);
		}
		dev->ConfigStall(ep_in_addr, true);
		if (bot_status == 2) {
			dev->ReceiveEndpoint(ep_out_addr, reinterpret_cast<byte*>(&cbw), _CBW_LENGTH);
		}
	}

	// ---- BOT: complete clear feature (AKA MSC_BOT_CplClrFeature) ----
	void USBPeri_MSC::CompleteClearFeature(byte epnum) {
		PeripheralDevice* dev = Parent();
		if (!dev) return;
		if (bot_status == 2) {
			dev->ConfigStall(ep_in_addr, true);
			bot_status = 0;
		}
		else if (((epnum & 0x80) == 0x80) && (bot_status != 1)) {
			SendCSW(_CSW_CMD_FAILED);
		}
	}

	// ---- SCSI: dispatch (AKA SCSI_ProcessCmd) ----
	sint USBPeri_MSC::ProcessCmd() {
		byte op = cbw.cb[0];
		switch (op) {
		case (byte)SCSICommand::TestUnitReady:      return TestUnitReady();
		case (byte)SCSICommand::RequestSense:       return RequestSense();
		case (byte)SCSICommand::Inquiry:            return Inquiry();
		case (byte)SCSICommand::StartStopUnit:
		case (byte)SCSICommand::AllowMediumRemoval: return StartStopUnit();
		case (byte)SCSICommand::ModeSense6:         return ModeSense6();
		case (byte)SCSICommand::ModeSense10:        return ModeSense10();
		case (byte)SCSICommand::ReadFormatCapacity: return ReadFormatCapacity();
		case (byte)SCSICommand::ReadCapacity10:     return ReadCapacity10();
		case (byte)SCSICommand::Read10:             return Read10();
		case (byte)SCSICommand::Write10:            return Write10();
		case (byte)SCSICommand::Verify10:           return Verify10();
		default:
			SenseCode(SenseKey::IllegalRequest, AdditionalSenseCode::InvalidCdb);
			return -1;
		}
	}

	sint USBPeri_MSC::TestUnitReady() {
		if (cbw.data_length != 0) {
			SenseCode(SenseKey::IllegalRequest, AdditionalSenseCode::InvalidCdb);
			return -1;
		}
		if (!storage_ || storage_->getUnits() == 0) {
			SenseCode(SenseKey::NotReady, AdditionalSenseCode::MediumNotPresent);
			bot_state = BotState::NoData;
			return -1;
		}
		bot_data_length = 0;
		return 0;
	}

	sint USBPeri_MSC::Inquiry() {
		const byte* page;
		uint16 len;
		if (cbw.cb[1] & 0x01) {
			page = _page00_inquiry;
			len = _LENGTH_INQUIRY_PAGE00;
		}
		else {
			// standard inquiry: 36 bytes supplied by the storage backend
			page = inquiry_;
			if (!page) {
				SenseCode(SenseKey::IllegalRequest, AdditionalSenseCode::InvalidCdb);
				return -1;
			}
			len = page[4] + 5;
			if (cbw.cb[4] <= len) len = cbw.cb[4];
		}
		bot_data_length = len;
		for (uint16 i = 0; i < len; i++) bot_data[i] = page[i];
		return 0;
	}

	sint USBPeri_MSC::ReadCapacity10() {
		if (!storage_) {
			SenseCode(SenseKey::NotReady, AdditionalSenseCode::MediumNotPresent);
			return -1;
		}
		scsi_blk_nbr = storage_->getUnits();
		scsi_blk_size = storage_->Block_Size;
		stduint last = scsi_blk_nbr - 1;
		bot_data[0] = (byte)(last >> 24);
		bot_data[1] = (byte)(last >> 16);
		bot_data[2] = (byte)(last >> 8);
		bot_data[3] = (byte)(last);
		bot_data[4] = (byte)(scsi_blk_size >> 24);
		bot_data[5] = (byte)(scsi_blk_size >> 16);
		bot_data[6] = (byte)(scsi_blk_size >> 8);
		bot_data[7] = (byte)(scsi_blk_size);
		bot_data_length = _READ_CAPACITY10_DATA_LEN;
		return 0;
	}

	sint USBPeri_MSC::ReadFormatCapacity() {
		for (byte i = 0; i < 12; i++) bot_data[i] = 0;
		if (!storage_) {
			SenseCode(SenseKey::NotReady, AdditionalSenseCode::MediumNotPresent);
			return -1;
		}
		stduint blk_nbr = storage_->getUnits();
		uint16 blk_size = storage_->Block_Size;
		bot_data[3] = 0x08;
		bot_data[4] = (byte)((blk_nbr - 1) >> 24);
		bot_data[5] = (byte)((blk_nbr - 1) >> 16);
		bot_data[6] = (byte)((blk_nbr - 1) >> 8);
		bot_data[7] = (byte)(blk_nbr - 1);
		bot_data[8] = 0x02;
		bot_data[9] = (byte)(blk_size >> 16);
		bot_data[10] = (byte)(blk_size >> 8);
		bot_data[11] = (byte)(blk_size);
		bot_data_length = _READ_FORMAT_CAPACITY_DATA_LEN;
		return 0;
	}

	sint USBPeri_MSC::ModeSense6() {
		bot_data_length = _MODE_SENSE6_DATA_LEN;
		for (byte i = 0; i < _MODE_SENSE6_DATA_LEN; i++) bot_data[i] = _mode_sense6[i];
		return 0;
	}

	sint USBPeri_MSC::ModeSense10() {
		bot_data_length = _MODE_SENSE10_DATA_LEN;
		for (byte i = 0; i < _MODE_SENSE10_DATA_LEN; i++) bot_data[i] = _mode_sense10[i];
		return 0;
	}

	sint USBPeri_MSC::RequestSense() {
		for (byte i = 0; i < _REQUEST_SENSE_DATA_LEN; i++) bot_data[i] = 0;
		bot_data[0] = 0x70;
		bot_data[7] = _REQUEST_SENSE_DATA_LEN - 6;
		if (sense_head != sense_tail) {
			bot_data[2] = (byte)sense_key[sense_head];
			bot_data[12] = (byte)sense_asc[sense_head];
			sense_head++;
			if (sense_head == _SENSE_LIST_DEPTH) sense_head = 0;
		}
		bot_data_length = _REQUEST_SENSE_DATA_LEN;
		if (cbw.cb[4] <= _REQUEST_SENSE_DATA_LEN) bot_data_length = cbw.cb[4];
		return 0;
	}

	sint USBPeri_MSC::StartStopUnit() {
		bot_data_length = 0;
		return 0;
	}

	sint USBPeri_MSC::Read10() {
		if (bot_state == BotState::Idle) {
			if ((cbw.flags & 0x80) != 0x80) {
				SenseCode(SenseKey::IllegalRequest, AdditionalSenseCode::InvalidCdb);
				return -1;
			}
			if (!storage_ || storage_->getUnits() == 0) {
				SenseCode(SenseKey::NotReady, AdditionalSenseCode::MediumNotPresent);
				return -1;
			}
			scsi_blk_addr = ((stduint)cbw.cb[2] << 24) | ((stduint)cbw.cb[3] << 16) |
				((stduint)cbw.cb[4] << 8) | cbw.cb[5];
			scsi_blk_len = ((stduint)cbw.cb[7] << 8) | cbw.cb[8];
			if (CheckAddressRange(scsi_blk_addr, (uint16)scsi_blk_len) < 0) return -1;
			bot_state = BotState::DataInn;
			scsi_blk_addr *= scsi_blk_size;
			scsi_blk_len *= scsi_blk_size;
			if (cbw.data_length != scsi_blk_len) {
				SenseCode(SenseKey::IllegalRequest, AdditionalSenseCode::InvalidCdb);
				return -1;
			}
		}
		bot_data_length = (uint16)media_packet;
		return ProcessRead();
	}

	sint USBPeri_MSC::Write10() {
		if (bot_state == BotState::Idle) {
			if ((cbw.flags & 0x80) == 0x80) {
				SenseCode(SenseKey::IllegalRequest, AdditionalSenseCode::InvalidCdb);
				return -1;
			}
			if (!storage_ || storage_->getUnits() == 0) {
				SenseCode(SenseKey::NotReady, AdditionalSenseCode::MediumNotPresent);
				return -1;
			}
			scsi_blk_addr = ((stduint)cbw.cb[2] << 24) | ((stduint)cbw.cb[3] << 16) |
				((stduint)cbw.cb[4] << 8) | cbw.cb[5];
			scsi_blk_len = ((stduint)cbw.cb[7] << 8) | cbw.cb[8];
			if (CheckAddressRange(scsi_blk_addr, (uint16)scsi_blk_len) < 0) return -1;
			scsi_blk_addr *= scsi_blk_size;
			scsi_blk_len *= scsi_blk_size;
			if (cbw.data_length != scsi_blk_len) {
				SenseCode(SenseKey::IllegalRequest, AdditionalSenseCode::InvalidCdb);
				return -1;
			}
			bot_state = BotState::DataOut;
			PeripheralDevice* dev = Parent();
			stduint rlen = scsi_blk_len < media_packet ? scsi_blk_len : media_packet;
			if (dev) dev->ReceiveEndpoint(ep_out_addr, bot_data, (uint16)rlen);
		}
		else {
			return ProcessWrite();
		}
		return 0;
	}

	sint USBPeri_MSC::Verify10() {
		if ((cbw.cb[1] & 0x02) == 0x02) {
			SenseCode(SenseKey::IllegalRequest, AdditionalSenseCode::InvalidFieldInCommand);
			return -1;
		}
		if (CheckAddressRange(scsi_blk_addr, (uint16)scsi_blk_len) < 0) return -1;
		bot_data_length = 0;
		return 0;
	}

	sint USBPeri_MSC::CheckAddressRange(stduint blk_offset, uint16 blk_nbr) {
		if (!storage_) return -1;
		if ((blk_offset + blk_nbr) > storage_->getUnits()) {
			SenseCode(SenseKey::IllegalRequest, AdditionalSenseCode::AddressOutOfRange);
			return -1;
		}
		return 0;
	}

	sint USBPeri_MSC::ProcessRead() {
		if (!storage_) return -1;
		PeripheralDevice* dev = Parent();
		stduint len = scsi_blk_len < media_packet ? scsi_blk_len : media_packet;
		stduint blocks = len / scsi_blk_size;
		if (!storage_->Read((stduint)(scsi_blk_addr / scsi_blk_size), bot_data, blocks)) {
			SenseCode(SenseKey::HardwareError, AdditionalSenseCode::UnrecoveredReadError);
			return -1;
		}
		if (dev) dev->TransmitEndpoint(ep_in_addr, bot_data, (uint16)len);
		scsi_blk_addr += len;
		scsi_blk_len -= len;
		csw.data_residue -= len;
		if (scsi_blk_len == 0) bot_state = BotState::LastDataInn;
		return 0;
	}

	sint USBPeri_MSC::ProcessWrite() {
		if (!storage_) return -1;
		PeripheralDevice* dev = Parent();
		stduint len = scsi_blk_len < media_packet ? scsi_blk_len : media_packet;
		stduint blocks = len / scsi_blk_size;
		if (!storage_->Write((stduint)(scsi_blk_addr / scsi_blk_size), bot_data, blocks)) {
			SenseCode(SenseKey::HardwareError, AdditionalSenseCode::WriteFault);
			return -1;
		}
		scsi_blk_addr += len;
		scsi_blk_len -= len;
		csw.data_residue -= len;
		if (scsi_blk_len == 0) {
			SendCSW(_CSW_CMD_PASSED);
		}
		else {
			stduint rlen = scsi_blk_len < media_packet ? scsi_blk_len : media_packet;
			if (dev) dev->ReceiveEndpoint(ep_out_addr, bot_data, (uint16)rlen);
		}
		return 0;
	}

	void USBPeri_MSC::SenseCode(SenseKey skey, AdditionalSenseCode asc) {
		sense_key[sense_tail] = skey;
		sense_asc[sense_tail] = asc;
		sense_tail++;
		if (sense_tail == _SENSE_LIST_DEPTH) sense_tail = 0;
	}

	// ---- configuration descriptors ----
	const byte* USBPeri_MSC::getConfigurationDescriptor(uint16& len) {
		if (Parent() && Parent()->getSpeed() == PeripheralSpeed::High) {
			len = sizeof(_cfg_desc_hs);
			return _cfg_desc_hs;
		}
		len = sizeof(_cfg_desc_fs);
		return _cfg_desc_fs;
	}
	const byte* USBPeri_MSC::getOtherSpeedConfigurationDescriptor(uint16& len) {
		// other speed is the opposite of the current speed
		if (Parent() && Parent()->getSpeed() == PeripheralSpeed::High) {
			len = sizeof(_cfg_desc_fs);
			return _cfg_desc_fs;
		}
		len = sizeof(_cfg_desc_hs);
		return _cfg_desc_hs;
	}
	const byte* USBPeri_MSC::getDeviceQualifierDescriptor(uint16& len) {
		static const byte qual[] = { 0x0A, _USB_DESC_TYPE_DEVICE_QUALIFIER, 0x00, 0x02, 0x00, 0x00, 0x00, _MSC_MAX_FS_PACKET, 0x01, 0x00 };
		len = sizeof(qual);
		return qual;
	}

#endif // _MCU_STM32H7x
}

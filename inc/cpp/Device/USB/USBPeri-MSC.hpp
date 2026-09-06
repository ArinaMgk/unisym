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

#ifndef _INC_DEVICE_USB_PERIPHERAL_MSC
#define _INC_DEVICE_USB_PERIPHERAL_MSC

#include "../../../c/stdinc.h"
#include "../../reference"
#include "../../trait/StorageTrait.hpp"
#include "USBPeri-Device.hpp"
#include "USBPeri-Class.hpp"

#if defined(_MCU_STM32H7x)

namespace uni::device::SpaceUSB {

	// AKA USBD_BOT states
	enum class BotState : byte {
		Idle = 0, DataOut = 1, DataInn = 2, LastDataInn = 3,
		SendData = 4, NoData = 5
	};

	// AKA SCSI opcode set
	enum class SCSICommand : byte {
		TestUnitReady = 0x00,
		RequestSense = 0x03,
		FormatUnit = 0x04,
		Read6 = 0x08,
		Write6 = 0x0A,
		Inquiry = 0x12,
		ModeSelect6 = 0x15,
		ModeSense6 = 0x1A,
		StartStopUnit = 0x1B,
		AllowMediumRemoval = 0x1E,
		ReadFormatCapacity = 0x23,
		ReadCapacity10 = 0x25,
		Read10 = 0x28,
		Write10 = 0x2A,
		Verify10 = 0x2F,
		ModeSelect10 = 0x55,
		ModeSense10 = 0x5A,
		ReadCapacity16 = 0x9E,
	};

	// AKA USBD_SCSI_SenseKey
	enum class SenseKey : byte {
		NoSense = 0, RecoveredError = 1, NotReady = 2, MediumError = 3,
		HardwareError = 4, IllegalRequest = 5, UnitAttention = 6,
		DataProtect = 7, BlankCheck = 8, VendorSpecific = 9,
		CopyAborted = 10, AbortedCommand = 11, VolumeOverflow = 13, Miscompare = 14
	};

	// AKA USBD_SCSI Additional Sense Code
	enum class AdditionalSenseCode : byte {
		InvalidCdb = 0x20,
		InvalidFieldInCommand = 0x24,
		ParameterListLengthError = 0x1A,
		InvalidFieldInParameterList = 0x26,
		AddressOutOfRange = 0x21,
		MediumNotPresent = 0x3A,
		MediumHaveChanged = 0x28,
		WriteProtected = 0x27,
		UnrecoveredReadError = 0x11,
		WriteFault = 0x03,
	};

	// AKA USBD_MSC_BOT_CBWTypeDef (Bulk-Only Transport command block wrapper)
	_PACKED(struct) CBW {
		stduint signature;
		stduint tag;
		stduint data_length;
		byte flags;
		byte lun;
		byte cb_length;
		byte cb[16];
	};

	// AKA USBD_MSC_BOT_CSWTypeDef (command status wrapper)
	_PACKED(struct) CSW {
		stduint signature;
		stduint tag;
		stduint data_residue;
		byte status;
	};

	// Mass Storage Class (AKA USBD_MSC + BOT + SCSI): exposes one or more
	// StorageTrait block devices as USB MSC logical units (multi-LUN reader).
	class USBPeri_MSC : public ClassPeripheral {
	public:
		USBPeri_MSC();
		virtual ~USBPeri_MSC();

		// ---- configuration ----
		// packet size per bulk transfer (AKA MSC_MEDIA_PACKET)
		stduint media_packet = 32 * 1024;
		byte ep_in_addr = 0x81;
		byte ep_out_addr = 0x01;
		// max logical units (AKA STORAGE_LUN_NBR in usbd_storage: SD/W25Q256/NAND)
		static const byte _MSC_LUN_MAX = 3;

		// bind a set of block devices + per-LUN standard inquiry data
		// (36 bytes each, AKA USBD_MSC_RegisterStorage + STORAGE_Inquirydata)
		void Bind(PeripheralDevice& dev, StorageTrait* const* storage, const byte* const* inquiry, byte count);
		// single-LUN convenience (kept for simple readers)
		void Bind(PeripheralDevice& dev, StorageTrait* storage, const byte* inquiry);

		// ---- ClassPeripheral overrides ----
		bool setMode(byte cfgidx) override;
		bool canMode(byte cfgidx) override;
		bool Setup() override;
		bool inn(byte epnum) override;
		bool out(byte epnum) override;
		bool EP0TxSent() override { return true; }
		bool EP0RxReady() override { return true; }

		const byte* getConfigurationDescriptor(uint16& len) override;
		const byte* getOtherSpeedConfigurationDescriptor(uint16& len) override;
		const byte* getDeviceQualifierDescriptor(uint16& len) override;

	private:
		// ---- BOT machine (AKA MSC_BOT_*) ----
		void BotReset();
		void BotAbort();
		void SendCSW(byte status);
		void SendData(byte* buf, uint16 len);
		void CBWDecode();
		void CompleteClearFeature(byte epnum);

		// ---- SCSI machine (AKA SCSI_*) ----
		stdsint ProcessCmd();
		stdsint TestUnitReady();
		stdsint Inquiry();
		stdsint ReadCapacity10();
		stdsint ReadFormatCapacity();
		stdsint ModeSense6();
		stdsint ModeSense10();
		stdsint RequestSense();
		stdsint StartStopUnit();
		stdsint Read10();
		stdsint Write10();
		stdsint Verify10();
		stdsint CheckAddressRange(stduint blk_offset, uint16 blk_nbr);
		stdsint ProcessRead();
		stdsint ProcessWrite();
		void SenseCode(SenseKey skey, AdditionalSenseCode asc);

		// ---- state (AKA USBD_MSC_BOT_HandleTypeDef) ----
		StorageTrait* storage_[_MSC_LUN_MAX] = { nullptr };
		const byte* inquiry_[_MSC_LUN_MAX] = { nullptr };// 36-byte standard inquiry per LUN
		byte lun_count = 0;          // number of bound LUNs (AKA STORAGE_LUN_NBR)
		byte cur_lun = 0;            // LUN of the CBW being processed (AKA cbw.bLUN)
		BotState bot_state = BotState::Idle;
		byte bot_status = 0;           // 0 normal, 1 recovery, 2 error
		uint16 bot_data_length = 0;
		byte* bot_data = nullptr;      // burst buffer (heap, MSC_MEDIA_PACKET)
		CBW cbw{};
		CSW csw{};
		byte interface = 0;
		stduint max_lun = 0;           // highest LUN index (lun_count-1), AKA GetMaxLun
		stduint scsi_blk_size = 0;
		stduint scsi_blk_nbr = 0;
		uint64 scsi_blk_addr = 0;
		stduint scsi_blk_len = 0;
		// sense queue (AKA scsi_sense[SENSE_LIST_DEEPTH] ring)
		SenseKey sense_key[4]{};
		AdditionalSenseCode sense_asc[4]{};
		byte sense_head = 0;
		byte sense_tail = 0;
	};

}

#endif // _MCU_STM32H7x

#endif // _INC_DEVICE_USB_PERIPHERAL_MSC

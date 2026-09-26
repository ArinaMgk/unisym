// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [Device.USB] Host Mass Storage Class Driver, MSC/BOT
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

#if (defined(_MCCA) && _MCCA == 0x8664) || defined(_MCU_STM32H7x)

#include "../../../../inc/cpp/Device/USB/USBHost-MSC.hpp"

namespace uni::device::SpaceUSB {

	// ---- SCSI command bytes (AKA usbh_msc_scsi.c) ----
	#define _SCSI_INQUIRY         0x12
	#define _SCSI_READ_CAPACITY10 0x25
	#define _SCSI_READ10          0x28
	#define _SCSI_WRITE10         0x2A
	#define _SCSI_INQUIRY_ALLOC   0x24
	#define _SCSI_GET_MAX_LUN     0xFE

	USBHost_MSC::USBHost_MSC(USBHostDevice* dev, int interface_index)
		: ClassDriver{ dev }, interface_index_{ interface_index } {
	}

	Error USBHost_MSC::Initialize() {
		return MAKE_ERROR(Error::kSuccess);
	}

	Error USBHost_MSC::SetEndpoint(const EndpointConfig& config) {
		if (config.ep_type == EndpointType::kBulk) {
			if (config.ep_id.IsIn()) ep_bulk_in_ = config.ep_id;
			else ep_bulk_out_ = config.ep_id;
			bulk_mps_ = static_cast<uint16>(config.max_packet_size);
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	// AKA USBH_MSC_Init/USBH_MSC_Process: the device is configured, so start the
	// bring-up with GET_MAX_LUN.
	Error USBHost_MSC::OnEndpointsConfigured() {
		if (!bulk_mps_ || ep_bulk_in_.Number() == 0 || ep_bulk_out_.Number() == 0) {
			result_ = Result::kNoBulkPair;
			phase_ = 4;
			return MAKE_ERROR(Error::kSuccess);
		}
		SetupData setup_data{};
		setup_data.request_type.bits.direction = request_type::kIn;
		setup_data.request_type.bits.type = request_type::kClass;
		setup_data.request_type.bits.recipient = request_type::kInterface;
		setup_data.request = _SCSI_GET_MAX_LUN;
		setup_data.value = 0;
		setup_data.index = static_cast<uint16>(interface_index_);
		setup_data.length = 1;
		phase_ = 1;
		return ParentDevice()->ControlIn(kDefaultControlPipeID, setup_data, &max_lun_, 1, this);
	}

	Error USBHost_MSC::OnControlCompleted(EndpointID ep_id, SetupData setup_data,
		const void* buf, int len) {
		(void)ep_id;
		(void)setup_data;
		(void)buf;
		completion_count_++;
		if (phase_ == 1) {
			// Devices that do not implement GET_MAX_LUN stall the request; that is a
			// device with LUN 0 only, AKA what Linux assumes as well.
			if (len < 1 || max_lun_ > kMaxLun) max_lun_ = 0;
			return Inquiry();
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error USBHost_MSC::OnInterruptCompleted(EndpointID ep_id, const void* buf, int len) {
		(void)ep_id;
		(void)buf;
		(void)len;
		return MAKE_ERROR(Error::kNotImplemented);// MSC has no interrupt endpoint
	}

	// AKA USBH_MSC_BOT_Process: one completion of the running transaction
	Error USBHost_MSC::OnBulkCompleted(EndpointID ep_id, const void* buf, int len) {
		(void)len;
		completion_count_++;
		switch (stage_) {
		case Stage::Cbw:
			if (buf == nullptr) {
				RecoverHalt(ep_id);// release the pipe before reporting the failure
				Finish(Result::kTransferFailed);
				return MAKE_ERROR(Error::kSuccess);
			}
			if (data_len_) {
				if (dir_in_) return SubmitDataIn();
				out_sent_ = 0;
				return SubmitDataOutChunk();
			}
			return SubmitCsw();
		case Stage::Data:
			if (buf == nullptr) {
				RecoverHalt(ep_id);
				Finish(Result::kTransferFailed);
				return MAKE_ERROR(Error::kSuccess);
			}
			if (!dir_in_) {
				out_sent_ += out_chunk_;
				if (out_sent_ < data_len_) return SubmitDataOutChunk();
			}
			return SubmitCsw();
		case Stage::Csw:
			if (buf == nullptr) {
				RecoverHalt(ep_id);
				Finish(Result::kTransferFailed);
				return MAKE_ERROR(Error::kSuccess);
			}
			ParseCsw(len);
			return MAKE_ERROR(Error::kSuccess);
		default:
			break;
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	// AKA USBH_MSC_BOT_Abort: CLEAR_FEATURE(ENDPOINT_HALT) on the stalled endpoint.
	// Fire and forget: the device releases the halt when it processes the request,
	// and the next command on that pipe is what proves it.
	Error USBHost_MSC::RecoverHalt(EndpointID ep_id) {
		SetupData setup_data{};
		setup_data.request_type.bits.direction = request_type::kOut;
		setup_data.request_type.bits.type = request_type::kStandard;
		setup_data.request_type.bits.recipient = request_type::kEndpoint;
		setup_data.request = 0x01;// CLEAR_FEATURE
		setup_data.value = 0x00;// FEATURE_SELECTOR_ENDPOINT = ENDPOINT_HALT
		setup_data.index = static_cast<uint16>(
			ep_id.Number() | (ep_id.IsIn() ? 0x80 : 0x00));// AKA the descriptor address
		setup_data.length = 0;
		halt_recoveries_++;
		return ParentDevice()->ControlOut(kDefaultControlPipeID, setup_data, nullptr, 0, nullptr);
	}

	// Drop an in-flight command whose completion never arrived (caller timed out), so
	// StartCommand does not answer kFull to every later command forever.
	void USBHost_MSC::AbortCommand() {
		stage_ = Stage::Idle;
		out_sent_ = 0;
		out_chunk_ = 0;
		if (result_ == Result::kBusy) result_ = Result::kTransferFailed;
	}

	Error USBHost_MSC::Inquiry() {
		byte cdb[kCdbLength] = {};
		cdb[0] = _SCSI_INQUIRY;
		cdb[1] = static_cast<byte>(lun_ << 5);// EVPD = 0, LUN in bits 7..5 (AKA the reference)
		cdb[4] = _SCSI_INQUIRY_ALLOC;// allocation length: 36 bytes
		phase_ = 2;
		return StartCommand(cdb, true, inquiry_, kInquiryLength);
	}

	Error USBHost_MSC::ReadCapacity10() {
		byte cdb[kCdbLength] = {};
		cdb[0] = _SCSI_READ_CAPACITY10;
		phase_ = 3;
		return StartCommand(cdb, true, capacity_, kCapacityLength);
	}

	// AKA USBH_MSC_BOT_SendCBW: the command block wrapper goes out on the bulk OUT
	// endpoint, then the data stage (if any) and finally the status wrapper.
	Error USBHost_MSC::StartCommand(const byte* cdb, bool dir_in, void* data, uint32 data_len) {
		if (stage_ != Stage::Idle) return MAKE_ERROR(Error::kFull);
		cbw_.signature = kSignatureCbw;
		cbw_.tag = kTag;
		cbw_.transfer_length = data_len;
		cbw_.flags = dir_in ? 0x80 : 0x00;
		cbw_.lun = lun_;
		cbw_.cb_length = kCdbLength;
		for (byte i = 0; i < 16; i++) cbw_.cb[i] = (i < kCdbLength) ? cdb[i] : 0;
		dir_in_ = dir_in;
		data_ = data;
		data_len_ = data_len;
		stage_ = Stage::Cbw;
		result_ = Result::kBusy;
		command_count_++;
		if (auto err = ParentDevice()->BulkTransfer(ep_bulk_out_, false, &cbw_,
			kCbwLength)) {
			Finish(Result::kTransferFailed);// never leave the machine busy
			return err;
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	// DATA-IN: one pipe transfer for the whole requested length (the controller splits
	// it into endpoint-size packets itself and the receive path re-arms per packet).
	Error USBHost_MSC::SubmitDataIn() {
		stage_ = Stage::Data;
		if (auto err = ParentDevice()->BulkTransfer(ep_bulk_in_, true, data_,
			static_cast<int>(data_len_))) {
			Finish(Result::kTransferFailed);// never leave the machine busy
			return err;
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	// DATA-OUT: the payload is pushed one endpoint packet at a time.
	Error USBHost_MSC::SubmitDataOutChunk() {
		const uint32 mps = bulk_mps_ ? bulk_mps_ : 64;
		uint32 chunk = data_len_ - out_sent_;
		if (chunk > mps) chunk = mps;
		out_chunk_ = chunk;
		stage_ = Stage::Data;
		if (auto err = ParentDevice()->BulkTransfer(ep_bulk_out_, false,
			static_cast<byte*>(data_) + out_sent_, static_cast<int>(chunk))) {
			Finish(Result::kTransferFailed);// never leave the machine busy
			return err;
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error USBHost_MSC::SubmitCsw() {
		stage_ = Stage::Csw;
		if (auto err = ParentDevice()->BulkTransfer(ep_bulk_in_, true, csw_raw_,
			kCswLength)) {
			Finish(Result::kTransferFailed);// never leave the machine busy
			return err;
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	// AKA USBH_MSC_BOT_DecodeCSW: 13 bytes, matching signature and tag required
	void USBHost_MSC::ParseCsw(int len) {
		if (len != kCswLength) {
			Finish(Result::kShortCsw);
			return;
		}
		// csw_raw_ is the padded staging buffer the transport filled
		const BotCSW* csw = reinterpret_cast<const BotCSW*>(csw_raw_);
		if (csw->signature != kSignatureCsw || csw->tag != kTag) {
			Finish(Result::kCswSignature);
			return;
		}
		if (csw->status == 0) {
			Finish(Result::kOk);
			return;
		}
		Finish(csw->status == 2 ? Result::kCswPhase : Result::kCswStatus);
	}

	void USBHost_MSC::Finish(Result res) {
		const Stage was = stage_;
		stage_ = Stage::Idle;
		result_ = res;
		if (res != Result::kOk && res != Result::kBusy && fail_stage_ == 0) {
			fail_stage_ = static_cast<byte>(was);// remember where the first failure was
		}
		if (phase_ == 2) {
			// INQUIRY done (or refused): the capacity matters more, so keep going
			if (res == Result::kOk) CaptureInquiry();
			ReadCapacity10();
			return;
		}
		if (phase_ == 3) {
			if (res == Result::kOk) ParseCapacity();
			phase_ = 4;
			return;
		}
	}

	// AKA USBH_MSC_SCSI_Inquiry: vendor at 8, product at 16, revision at 32
	void USBHost_MSC::CaptureInquiry() {
		for (byte i = 0; i < 8; i++) vendor_[i] = static_cast<char>(inquiry_[8 + i]);
		vendor_[8] = 0;
		for (byte i = 0; i < 16; i++) product_[i] = static_cast<char>(inquiry_[16 + i]);
		product_[16] = 0;
		for (byte i = 0; i < 4; i++) revision_[i] = static_cast<char>(inquiry_[32 + i]);
		revision_[4] = 0;
		// the wire fields are space padded
		for (int i = 7; i >= 0 && vendor_[i] == ' '; i--) vendor_[i] = 0;
		for (int i = 15; i >= 0 && product_[i] == ' '; i--) product_[i] = 0;
		for (int i = 3; i >= 0 && revision_[i] == ' '; i--) revision_[i] = 0;
	}

	// AKA USBH_MSC_SCSI_ReadCapacity: the reply is big-endian, and the block length
	// is only taken as 16 bits by the reference implementation.
	void USBHost_MSC::ParseCapacity() {
		const uint32 last_lba = (static_cast<uint32>(capacity_[0]) << 24)
			| (static_cast<uint32>(capacity_[1]) << 16)
			| (static_cast<uint32>(capacity_[2]) << 8)
			| static_cast<uint32>(capacity_[3]);
		const uint32 block_len = (static_cast<uint32>(capacity_[4]) << 24)
			| (static_cast<uint32>(capacity_[5]) << 16)
			| (static_cast<uint32>(capacity_[6]) << 8)
			| static_cast<uint32>(capacity_[7]);
		block_count_ = last_lba + (last_lba ? 1 : 0);
		block_size_ = block_len ? block_len : 512;
		ready_ = true;
	}

	Error USBHost_MSC::ReadBlocks(uint32 lba, uint16 count, void* buf) {
		if (!ready_ || count == 0 || buf == nullptr) return MAKE_ERROR(Error::kIndexOutOfRange);
		byte cdb[kCdbLength] = {};
		cdb[0] = _SCSI_READ10;
		cdb[2] = static_cast<byte>(lba >> 24);
		cdb[3] = static_cast<byte>(lba >> 16);
		cdb[4] = static_cast<byte>(lba >> 8);
		cdb[5] = static_cast<byte>(lba);
		cdb[7] = static_cast<byte>(count >> 8);
		cdb[8] = static_cast<byte>(count);
		return StartCommand(cdb, true, buf, static_cast<uint32>(count) * block_size_);
	}

	Error USBHost_MSC::WriteBlocks(uint32 lba, uint16 count, const void* buf) {
		if (!ready_ || count == 0 || buf == nullptr) return MAKE_ERROR(Error::kIndexOutOfRange);
		byte cdb[kCdbLength] = {};
		cdb[0] = _SCSI_WRITE10;
		cdb[2] = static_cast<byte>(lba >> 24);
		cdb[3] = static_cast<byte>(lba >> 16);
		cdb[4] = static_cast<byte>(lba >> 8);
		cdb[5] = static_cast<byte>(lba);
		cdb[7] = static_cast<byte>(count >> 8);
		cdb[8] = static_cast<byte>(count);
		return StartCommand(cdb, false, const_cast<void*>(buf),
			static_cast<uint32>(count) * block_size_);
	}

	const char* USBHost_MSC::PhaseName() const {
		switch (phase_) {
		case 0: return "idle";
		case 1: return "get-max-lun";
		case 2: return "inquiry";
		case 3: return "read-capacity";
		default: return "done";
		}
	}

	const char* USBHost_MSC::StageName() const {
		switch (stage_) {
		case Stage::Cbw: return "cbw";
		case Stage::Data: return "data";
		case Stage::Csw: return "csw";
		default: return "idle";
		}
	}

	const char* USBHost_MSC::ResultName() const {
		static const char* const names[] = {
			"ok", "busy", "no bulk pair", "transfer failed",
			"short CSW", "CSW signature", "CSW status", "CSW phase error"
		};
		const int idx = static_cast<int>(result_);
		if (idx < 0 || idx >= static_cast<int>(sizeof(names) / sizeof(names[0]))) return "?";
		return names[idx];
	}

}

#endif // _MCU_STM32H7x

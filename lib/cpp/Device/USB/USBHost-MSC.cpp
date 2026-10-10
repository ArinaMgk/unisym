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

#include "../../../../inc/cpp/Device/USB/USBHost-MSC.hpp"

namespace uni::device::SpaceUSB {

	// ---- SCSI command bytes (AKA usbh_msc_scsi.c) ----
	#define _SCSI_INQUIRY         0x12
	#define _SCSI_READ_CAPACITY10 0x25
	#define _SCSI_READ10          0x28
	#define _SCSI_WRITE10         0x2A
	#define _SCSI_INQUIRY_ALLOC   0x24
	#define _SCSI_GET_MAX_LUN     0xFE
	#define _MSC_BULK_ONLY_RESET  0xFF

	static bool IsClearHaltRequest(const SetupData& setup_data, EndpointID ep_id) {
		const uint16 endpoint_address = static_cast<uint16>(
			ep_id.Number() | (ep_id.IsIn() ? 0x80 : 0x00));
		return setup_data.request_type.bits.direction == request_type::kOut &&
			setup_data.request_type.bits.type == request_type::kStandard &&
			setup_data.request_type.bits.recipient == request_type::kEndpoint &&
			setup_data.request == static_cast<uint8>(StandardRequest::ClearFeature) &&
			setup_data.value == 0 && setup_data.index == endpoint_address &&
			setup_data.length == 0;
	}

	USBHost_MSC::USBHost_MSC(USBHostDevice* dev, int interface_index)
		: ClassDriver{ dev }, interface_index_{ interface_index } {
	}

	void USBHost_MSC::SetObserver(Observer observer, void* context) {
		observer_ = observer;
		observer_context_ = context;
		if (observer_ && BringUpDone()) {
			Notify(Event::BringUpComplete);
		}
	}

	void USBHost_MSC::Notify(Event event) {
		if (observer_) observer_(*this, event, command_id_, observer_context_);
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
			result_ = Result::NoBulkPair;
			phase_ = 4;
			plogwarn("USB MSC bring-up failed dev=%p interface=%u result=%s bulk-in=%u bulk-out=%u mps=%u",
				ParentDevice(), (stduint)interface_index_, ResultName(),
				(stduint)ep_bulk_in_.Address(), (stduint)ep_bulk_out_.Address(),
				(stduint)bulk_mps_);
			Notify(Event::BringUpComplete);
			return MAKE_ERROR(Error::kSuccess);
		}
		ploginfo("USB MSC bring-up start dev=%p interface=%u bulk-in=%u bulk-out=%u mps=%u",
			ParentDevice(), (stduint)interface_index_,
			(stduint)ep_bulk_in_.Address(), (stduint)ep_bulk_out_.Address(),
			(stduint)bulk_mps_);
		SetupData setup_data{};
		setup_data.request_type.bits.direction = request_type::kIn;
		setup_data.request_type.bits.type = request_type::kClass;
		setup_data.request_type.bits.recipient = request_type::kInterface;
		setup_data.request = _SCSI_GET_MAX_LUN;
		setup_data.value = 0;
		setup_data.index = static_cast<uint16>(interface_index_);
		setup_data.length = 1;
		phase_ = 1;
		auto error = ParentDevice()->ControlIn(kDefaultControlPipeID, setup_data,
			&max_lun_, 1, this);
		if (error) {
			result_ = Result::TransferFailed;
			phase_ = 4;
			plogwarn("USB MSC GET_MAX_LUN submit failed dev=%p interface=%u error=%s",
				ParentDevice(), (stduint)interface_index_, error.Name());
			Notify(Event::BringUpComplete);
		}
		return error;
	}

	Error USBHost_MSC::OnControlCompleted(EndpointID ep_id, SetupData setup_data,
		const void* buf, int len) {
		(void)ep_id;
		(void)buf;
		completion_count_++;
		if (recovery_stage_ != RecoveryStage::None) {
			if (len < 0) {
				FinishRecovery(false);
				return MAKE_ERROR(Error::kSuccess);
			}
			if (recovery_stage_ == RecoveryStage::ClearHalt) {
				if (!IsClearHaltRequest(setup_data, recovery_ep_)) {
					FinishRecovery(false);
					return MAKE_ERROR(Error::kInvalidPhase);
				}
				if (ParentDevice()->IsBulkRecoveryPending(recovery_ep_)) {
					recovery_stage_ = RecoveryStage::AwaitTransport;
				}
				else {
					FinishRecovery(true);
				}
				return MAKE_ERROR(Error::kSuccess);
			}
			if (recovery_stage_ == RecoveryStage::MassStorageReset) {
				if (setup_data.request_type.bits.direction != request_type::kOut ||
					setup_data.request != _MSC_BULK_ONLY_RESET ||
					setup_data.request_type.bits.type != request_type::kClass ||
					setup_data.request_type.bits.recipient != request_type::kInterface ||
					setup_data.value != 0 ||
					setup_data.index != static_cast<uint16>(interface_index_) ||
					setup_data.length != 0) {
					FinishRecovery(false);
					return MAKE_ERROR(Error::kInvalidPhase);
				}
				recovery_stage_ = RecoveryStage::ClearBulkIn;
				if (auto error = SubmitClearHalt(ep_bulk_in_)) {
					FinishRecovery(false);
					return error;
				}
				return MAKE_ERROR(Error::kSuccess);
			}
			if (recovery_stage_ == RecoveryStage::ClearBulkIn) {
				if (!IsClearHaltRequest(setup_data, ep_bulk_in_)) {
					FinishRecovery(false);
					return MAKE_ERROR(Error::kInvalidPhase);
				}
				recovery_stage_ = RecoveryStage::ClearBulkOut;
				if (auto error = SubmitClearHalt(ep_bulk_out_)) {
					FinishRecovery(false);
					return error;
				}
				return MAKE_ERROR(Error::kSuccess);
			}
			if (recovery_stage_ == RecoveryStage::ClearBulkOut) {
				if (!IsClearHaltRequest(setup_data, ep_bulk_out_)) {
					FinishRecovery(false);
					return MAKE_ERROR(Error::kInvalidPhase);
				}
				FinishRecovery(true);
				return MAKE_ERROR(Error::kSuccess);
			}
			return MAKE_ERROR(Error::kInvalidPhase);
		}
		if (phase_ == 1) {
			// Devices that do not implement GET_MAX_LUN stall the request; that is a
			// device with LUN 0 only, AKA what Linux assumes as well.
			if (len < 1 || max_lun_ > kMaxLun) max_lun_ = 0;
			ploginfo("USB MSC GET_MAX_LUN complete dev=%p interface=%u length=%d max-lun=%u completions=%u",
				ParentDevice(), (stduint)interface_index_, len,
				(stduint)max_lun_, (stduint)completion_count_);
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
		completion_count_++;
		switch (stage_) {
		case Stage::Cbw:
			if (buf == nullptr) {
				return BeginHaltRecovery(ep_id, Result::TransferFailed);
			}
			if (len != kCbwLength) {
				return BeginResetRecovery(Result::TransferFailed);
			}
			if (data_len_) {
				if (dir_in_) return SubmitDataIn();
				out_sent_ = 0;
				return SubmitDataOutChunk();
			}
			return SubmitCsw();
		case Stage::Data:
			if (buf == nullptr) {
				return BeginHaltRecovery(ep_id, Result::TransferFailed);
			}
			if (!dir_in_) {
				if (len != static_cast<int>(out_chunk_)) {
					return BeginResetRecovery(Result::TransferFailed);
				}
				out_sent_ += out_chunk_;
				if (out_sent_ < data_len_) return SubmitDataOutChunk();
			}
			return SubmitCsw();
		case Stage::Csw:
			if (buf == nullptr) {
				return BeginHaltRecovery(ep_id, Result::TransferFailed);
			}
			ParseCsw(len);
			return MAKE_ERROR(Error::kSuccess);
		default:
			break;
		}
		plogwarn("USB MSC unexpected bulk completion dev=%p interface=%u phase=%s ep=%u length=%d",
			ParentDevice(), (stduint)interface_index_, PhaseName(),
			(stduint)ep_id.Address(), len);
		return MAKE_ERROR(Error::kSuccess);
	}

	Error USBHost_MSC::OnBulkRecoveryCompleted(EndpointID ep_id, bool success) {
		if (recovery_stage_ == RecoveryStage::None && stage_ != Stage::Idle && !success) {
			Finish(Result::RecoveryFailed);
			return MAKE_ERROR(Error::kSuccess);
		}
		if (recovery_stage_ != RecoveryStage::AwaitTransport ||
			ep_id.Address() != recovery_ep_.Address()) {
			return MAKE_ERROR(Error::kInvalidPhase);
		}
		FinishRecovery(success);
		return MAKE_ERROR(Error::kSuccess);
	}

	Error USBHost_MSC::SubmitClearHalt(EndpointID ep_id) {
		SetupData setup_data{};
		setup_data.request_type.bits.direction = request_type::kOut;
		setup_data.request_type.bits.type = request_type::kStandard;
		setup_data.request_type.bits.recipient = request_type::kEndpoint;
		setup_data.request = static_cast<uint8>(StandardRequest::ClearFeature);
		setup_data.value = 0x00;// FEATURE_SELECTOR_ENDPOINT = ENDPOINT_HALT
		setup_data.index = static_cast<uint16>(
			ep_id.Number() | (ep_id.IsIn() ? 0x80 : 0x00));// AKA the descriptor address
		setup_data.length = 0;
		return ParentDevice()->ControlOut(kDefaultControlPipeID, setup_data, nullptr, 0, this);
	}

	Error USBHost_MSC::SubmitMassStorageReset() {
		SetupData setup_data{};
		setup_data.request_type.bits.direction = request_type::kOut;
		setup_data.request_type.bits.type = request_type::kClass;
		setup_data.request_type.bits.recipient = request_type::kInterface;
		setup_data.request = _MSC_BULK_ONLY_RESET;
		setup_data.value = 0;
		setup_data.index = static_cast<uint16>(interface_index_);
		setup_data.length = 0;
		return ParentDevice()->ControlOut(kDefaultControlPipeID, setup_data, nullptr, 0, this);
	}

	Error USBHost_MSC::BeginHaltRecovery(EndpointID ep_id, Result result) {
		if (recovery_stage_ != RecoveryStage::None) {
			return MAKE_ERROR(Error::kInvalidPhase);
		}
		recovery_result_ = result;
		recovery_ep_ = ep_id;
		recovery_stage_ = RecoveryStage::ClearHalt;
		halt_recoveries_++;
		if (auto error = SubmitClearHalt(ep_id)) {
			FinishRecovery(false);
			return error;
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error USBHost_MSC::BeginResetRecovery(Result result) {
		if (recovery_stage_ != RecoveryStage::None) {
			return MAKE_ERROR(Error::kInvalidPhase);
		}
		recovery_result_ = result;
		recovery_stage_ = RecoveryStage::MassStorageReset;
		reset_recoveries_++;
		if (auto error = SubmitMassStorageReset()) {
			FinishRecovery(false);
			return error;
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	void USBHost_MSC::FinishRecovery(bool success) {
		const Result result = success ? recovery_result_ : Result::RecoveryFailed;
		recovery_stage_ = RecoveryStage::None;
		Finish(result);
	}

	// Drop an in-flight command whose completion never arrived (caller timed out), so
	// StartCommand does not answer kFull to every later command forever.
	void USBHost_MSC::AbortCommand() {
		const bool was_busy = stage_ != Stage::Idle;
		stage_ = Stage::Idle;
		recovery_stage_ = RecoveryStage::None;
		out_sent_ = 0;
		out_chunk_ = 0;
		if (result_ == Result::Busy) result_ = Result::TransferFailed;
		if (was_busy) Notify(Event::CommandComplete);
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
		cbw_.tag = next_tag_++;
		if (next_tag_ == 0) next_tag_ = kInitialTag;
		cbw_.transfer_length = data_len;
		cbw_.flags = dir_in ? 0x80 : 0x00;
		cbw_.lun = lun_;
		cbw_.cb_length = kCdbLength;
		for (byte i = 0; i < 16; i++) cbw_.cb[i] = (i < kCdbLength) ? cdb[i] : 0;
		dir_in_ = dir_in;
		data_ = data;
		data_len_ = data_len;
		stage_ = Stage::Cbw;
		result_ = Result::Busy;
		command_count_++;
		command_id_ = command_count_;
		if (auto err = ParentDevice()->BulkTransfer(ep_bulk_out_, false, &cbw_,
			kCbwLength)) {
			plogwarn("USB MSC CBW submit failed dev=%p interface=%u phase=%s error=%s",
				ParentDevice(), (stduint)interface_index_, PhaseName(), err.Name());
			Finish(Result::TransferFailed);// never leave the machine busy
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
			plogwarn("USB MSC data-IN submit failed dev=%p interface=%u phase=%s length=%u error=%s",
				ParentDevice(), (stduint)interface_index_, PhaseName(),
				(stduint)data_len_, err.Name());
			Finish(Result::TransferFailed);// never leave the machine busy
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
			plogwarn("USB MSC data-OUT submit failed dev=%p interface=%u phase=%s offset=%u length=%u error=%s",
				ParentDevice(), (stduint)interface_index_, PhaseName(),
				(stduint)out_sent_, (stduint)chunk, err.Name());
			Finish(Result::TransferFailed);// never leave the machine busy
			return err;
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error USBHost_MSC::SubmitCsw() {
		stage_ = Stage::Csw;
		if (auto err = ParentDevice()->BulkTransfer(ep_bulk_in_, true, csw_raw_,
			kCswLength)) {
			plogwarn("USB MSC CSW submit failed dev=%p interface=%u phase=%s error=%s",
				ParentDevice(), (stduint)interface_index_, PhaseName(), err.Name());
			Finish(Result::TransferFailed);// never leave the machine busy
			return err;
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	// AKA USBH_MSC_BOT_DecodeCSW: 13 bytes, matching signature and tag required
	void USBHost_MSC::ParseCsw(int len) {
		if (len != kCswLength) {
			plogwarn("USB MSC invalid CSW length dev=%p interface=%u actual=%d expected=%u",
				ParentDevice(), (stduint)interface_index_, len, (stduint)kCswLength);
			BeginResetRecovery(Result::ShortCsw);
			return;
		}
		// csw_raw_ is the padded staging buffer the transport filled
		const BotCSW* csw = reinterpret_cast<const BotCSW*>(csw_raw_);
		if (csw->signature != kSignatureCsw || csw->tag != cbw_.tag) {
			plogwarn("USB MSC invalid CSW identity dev=%p interface=%u signature=%[32H] tag=%[32H] expected-tag=%[32H]",
				ParentDevice(), (stduint)interface_index_, (stduint)csw->signature,
				(stduint)csw->tag, (stduint)cbw_.tag);
			BeginResetRecovery(Result::CswSignature);
			return;
		}
		if (csw->status == 0 && csw->residue == 0) {
			Finish(Result::Ok);
			return;
		}
		if (csw->status == 0) {
			plogwarn("USB MSC incomplete command dev=%p interface=%u residue=%u",
				ParentDevice(), (stduint)interface_index_, (stduint)csw->residue);
			Finish(Result::CswResidue);
			return;
		}
		plogwarn("USB MSC CSW command error dev=%p interface=%u status=%u residue=%u",
			ParentDevice(), (stduint)interface_index_, (stduint)csw->status,
			(stduint)csw->residue);
		if (csw->status == 2) {
			BeginResetRecovery(Result::CswPhase);
			return;
		}
		Finish(Result::CswStatus);
	}

	void USBHost_MSC::Finish(Result res) {
		const Stage was = stage_;
		stage_ = Stage::Idle;
		result_ = res;
		if (res != Result::Ok && res != Result::Busy && fail_stage_ == 0) {
			fail_stage_ = static_cast<byte>(was);// remember where the first failure was
		}
		if (res != Result::Ok) {
			plogwarn("USB MSC command failed dev=%p interface=%u phase=%s stage=%s result=%s commands=%u completions=%u recoveries=%u resets=%u",
				ParentDevice(), (stduint)interface_index_, PhaseName(),
				was == Stage::Cbw ? "cbw" : was == Stage::Data ? "data" :
				was == Stage::Csw ? "csw" : "idle",
				ResultName(), (stduint)command_count_, (stduint)completion_count_,
				(stduint)halt_recoveries_, (stduint)reset_recoveries_);
		}
		if (phase_ == 2) {
			Notify(Event::CommandComplete);
			// INQUIRY done (or refused): the capacity matters more, so keep going
			if (res == Result::Ok) CaptureInquiry();
			ReadCapacity10();
			return;
		}
		if (phase_ == 3) {
			if (res == Result::Ok) ParseCapacity();
			phase_ = 4;
			if (ready_) {
				ploginfo("USB MSC ready dev=%p interface=%u lun=%u vendor=%s product=%s revision=%s blocks=%u block-size=%u commands=%u completions=%u recoveries=%u resets=%u",
					ParentDevice(), (stduint)interface_index_, (stduint)lun_,
					vendor_, product_, revision_, (stduint)block_count_,
					(stduint)block_size_, (stduint)command_count_,
					(stduint)completion_count_, (stduint)halt_recoveries_,
					(stduint)reset_recoveries_);
			} else {
				plogwarn("USB MSC bring-up failed dev=%p interface=%u phase=%s result=%s commands=%u completions=%u recoveries=%u resets=%u",
					ParentDevice(), (stduint)interface_index_, PhaseName(), ResultName(),
					(stduint)command_count_, (stduint)completion_count_,
					(stduint)halt_recoveries_, (stduint)reset_recoveries_);
			}
			Notify(Event::CommandComplete);
			Notify(Event::BringUpComplete);
			return;
		}
		Notify(Event::CommandComplete);
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
		if (!ready_ || count == 0 || buf == nullptr || !block_size_ ||
			lba >= block_count_ || uint32(count) > block_count_ - lba ||
			uint32(count) > ~uint32(0) / block_size_) {
			return MAKE_ERROR(Error::kIndexOutOfRange);
		}
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
		if (!ready_ || count == 0 || buf == nullptr || !block_size_ ||
			lba >= block_count_ || uint32(count) > block_count_ - lba ||
			uint32(count) > ~uint32(0) / block_size_) {
			return MAKE_ERROR(Error::kIndexOutOfRange);
		}
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
			"short CSW", "CSW signature", "CSW status", "CSW phase error",
			"recovery failed", "CSW residue"
		};
		const int idx = static_cast<int>(result_);
		if (idx < 0 || idx >= static_cast<int>(sizeof(names) / sizeof(names[0]))) return "?";
		return names[idx];
	}

}


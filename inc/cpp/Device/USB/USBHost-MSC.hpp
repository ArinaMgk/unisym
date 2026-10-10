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

#ifndef _INC_DEVICE_USB_HOST_MSC
#define _INC_DEVICE_USB_HOST_MSC

#include "USB.hpp"

namespace uni::device::SpaceUSB {

	// USB Mass Storage Class on the Bulk-Only Transport with the SCSI transparent
	// command set (class 0x08 / subclass 0x06 / protocol 0x50), AKA the
	// USBH_MSC_CLASS + usbh_msc_bot + usbh_msc_scsi trio of the ST USB host library.
	//
	// The bring-up sequence runs by itself once the endpoints are configured:
	// GET_MAX_LUN (class request, answered with a STALL by many devices: LUN 0 only)
	// -> INQUIRY -> READ CAPACITY(10). Every step is one BOT transaction:
	// CBW(31, bulk OUT) -> data (bulk IN or OUT) -> CSW(13, bulk IN).
	//
	// The calls are asynchronous like the rest of the host stack: ReadBlocks and
	// WriteBlocks only submit the command, the completions run in interrupt context
	// and advance the state machine, so a caller polls IsBusy()/LastResult().
	class USBHost_MSC : public ClassDriver {
	public:
		enum class Event : byte {
			BringUpComplete,
			CommandComplete,
		};
		// Observers run in the transport completion context and may only record state
		// or wake a waiter; they must not submit another USB command directly.
		using Observer = void (*)(USBHost_MSC& driver, Event event,
			uint32 command_id, void* context);

		// AKA USBH_MSC_BOT_CBWTypeDef (31 bytes on the wire)
		struct BotCBW {
			uint32 signature;// 0x43425355, "USBC" little-endian on the wire
			uint32 tag;
			uint32 transfer_length;
			uint8 flags;// bit 7: 1 = data IN, 0 = data OUT
			uint8 lun;
			uint8 cb_length;
			uint8 cb[16];
		} __attribute__((packed));

		// AKA USBH_MSC_BOT_CSWTypeDef (13 bytes on the wire)
		struct BotCSW {
			uint32 signature;// 0x53425355, "USBS" little-endian on the wire
			uint32 tag;
			uint32 residue;
			uint8 status;// 0 = passed, 1 = failed, 2 = phase error
		} __attribute__((packed));

		// Result of the last transaction or of the bring-up sequence, AKA the
		// DRESULT a FatFS diskio layer would collapse into RES_OK/RES_ERROR.
		enum class Result : byte {
			Ok = 0,
			Busy,          // a command is still in flight
			NoBulkPair,    // the configuration gave no bulk IN/OUT endpoint pair
			TransferFailed,// a bulk transfer did not complete (stall/error/timeout)
			ShortCsw,      // CSW shorter than 13 bytes
			CswSignature,  // CSW signature or tag mismatch
			CswStatus,     // CSW status = command failed
			CswPhase,      // CSW status = phase error
			RecoveryFailed,
			CswResidue,
		};

		static const uint32 kSignatureCbw = 0x43425355U;
		static const uint32 kSignatureCsw = 0x53425355U;
		static const uint32 kInitialTag = 0x20304050U;
		static const byte kCdbLength = 10;
		static const byte kMaxLun = 1;// LUNs 0..kMaxLun are accepted
		// Wire lengths of the BOT/SCSI payloads. The transport copies a received
		// packet word-wise (AKA USB_ReadPacket), so every receive buffer below is
		// padded past its length: a 13-byte CSW would otherwise spill three bytes
		// into the member that follows it.
		static const byte kCbwLength = 31;
		static const byte kCswLength = 13;
		static const byte kInquiryLength = 36;
		static const byte kCapacityLength = 8;

		USBHost_MSC(USBHostDevice* dev, int interface_index);

		Error Initialize() override;
		Error SetEndpoint(const EndpointConfig& config) override;
		Error OnEndpointsConfigured() override;
		Error OnControlCompleted(EndpointID ep_id, SetupData setup_data, const void* buf, int len) override;
		Error OnInterruptCompleted(EndpointID ep_id, const void* buf, int len) override;
		Error OnBulkCompleted(EndpointID ep_id, const void* buf, int len) override;
		Error OnBulkRecoveryCompleted(EndpointID ep_id, bool success) override;
		ClassDriverType Type() const override { return ClassDriverType::MassStorage; }
		int InterfaceNumber() const override { return interface_index_; }
		void SetObserver(Observer observer, void* context = nullptr);

		// ---- state ----
		bool IsReady() const { return ready_; }// INQUIRY and READ CAPACITY are done
		bool IsBusy() const { return stage_ != Stage::Idle; }
		void AbortCommand();
		// The bring-up sequence reached its end (successfully or not)
		bool BringUpDone() const { return phase_ == 4; }
		Result LastResult() const { return result_; }
		const char* ResultName() const;
		byte MaxLun() const { return max_lun_; }
		byte Lun() const { return lun_; }

		// ---- bring-up progress, for platform logs ----
		// CommandCount counts the BOT commands submitted, CompletionCount every
		// control/bulk completion seen: a driver that reports cmds=1, completions=0
		// and stage="cbw" never got the first bulk transfer back.
		uint32 CommandCount() const { return command_count_; }
		uint32 CommandID() const { return command_id_; }
		uint32 CommandTag() const { return cbw_.tag; }
		uint32 CompletionCount() const { return completion_count_; }
		uint32 HaltRecoveries() const { return halt_recoveries_; }
		uint32 ResetRecoveries() const { return reset_recoveries_; }
		const char* PhaseName() const;
		const char* StageName() const;

		// ---- INQUIRY (ASCII, space padded on the wire) ----
		const char* Vendor() const { return vendor_; }// 8 bytes
		const char* Product() const { return product_; }// 16 bytes
		const char* Revision() const { return revision_; }// 4 bytes

		// ---- READ CAPACITY(10) ----
		uint32 BlockCount() const { return block_count_; }// last LBA + 1
		uint32 BlockSize() const { return block_size_; }// bytes per block, usually 512

		// ---- sector I/O, AKA the diskio read/write of a FatFS volume ----
		Error ReadBlocks(uint32 lba, uint16 count, void* buf);
		Error WriteBlocks(uint32 lba, uint16 count, const void* buf);

	private:
		enum class Stage : byte { Idle, Cbw, Data, Csw };
		enum class RecoveryStage : byte {
			None,
			ClearHalt,
			AwaitTransport,
			MassStorageReset,
			ClearBulkIn,
			ClearBulkOut,
		};

		// AKA the SCSI commands of usbh_msc_scsi.c
		Error Inquiry();
		Error ReadCapacity10();
		Error StartCommand(const byte* cdb, bool dir_in, void* data, uint32 data_len);
		Error SubmitCsw();
		Error SubmitDataIn();
		// The non-periodic TxFIFO holds only 96 words (384 bytes) and this controller
		// has no FIFO-empty handling, so an OUT payload goes out in single-packet
		// chunks (AKA USBH_BulkSendData with OutEpSize each time).
		Error SubmitDataOutChunk();
		// A stalled bulk endpoint stays halted until the host clears the feature
		// (AKA USBH_MSC_BOT_Abort), otherwise that pipe stays dead for every later
		// command on it.
		Error BeginHaltRecovery(EndpointID ep_id, Result result);
		Error BeginResetRecovery(Result result);
		Error SubmitClearHalt(EndpointID ep_id);
		Error SubmitMassStorageReset();
		void FinishRecovery(bool success);
		void Finish(Result res);
		void ParseCsw(int len);
		void ParseCapacity();
		void CaptureInquiry();
		void Notify(Event event);

		EndpointID ep_bulk_in_{};
		EndpointID ep_bulk_out_{};
		uint16 bulk_mps_ = 0;
		int interface_index_ = 0;

		BotCBW cbw_{};
		Stage stage_ = Stage::Idle;
		Result result_ = Result::Ok;
		bool ready_ = false;
		bool dir_in_ = false;
		void* data_ = nullptr;
		uint32 data_len_ = 0;
		// data-OUT progress: bytes already handed to the pipe and the size of the
		// chunk currently in flight (one endpoint packet)
		uint32 out_sent_ = 0;
		uint32 out_chunk_ = 0;
		byte lun_ = 0;
		byte max_lun_ = 0;
		// bring-up phases: 1 = GET_MAX_LUN, 2 = INQUIRY, 3 = READ CAPACITY, 4 = done
		int phase_ = 0;
		// diagnostics: how many commands were submitted, how many completions arrived
		// and which stage the first failure happened in (0 = none yet)
		uint32 command_count_ = 0;
		uint32 command_id_ = 0;
		uint32 next_tag_ = kInitialTag;
		uint32 completion_count_ = 0;
		uint32 halt_recoveries_ = 0;
		uint32 reset_recoveries_ = 0;
		byte fail_stage_ = 0;
		RecoveryStage recovery_stage_ = RecoveryStage::None;
		Result recovery_result_ = Result::Ok;
		EndpointID recovery_ep_{};

		byte inquiry_[40] = {};// kInquiryLength + word padding
		byte capacity_[12] = {};// kCapacityLength + word padding
		byte csw_raw_[16] = {};// kCswLength + word padding, parsed as a BotCSW
		uint32 block_count_ = 0;
		uint32 block_size_ = 0;
		char vendor_[9] = {};
		char product_[17] = {};
		char revision_[5] = {};
		Observer observer_ = nullptr;
		void* observer_context_ = nullptr;
	};

}

#endif // _INC_DEVICE_USB_HOST_MSC

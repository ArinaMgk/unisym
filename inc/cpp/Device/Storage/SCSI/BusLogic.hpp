// ASCII CPP-ISO11 TAB4 LF
// Docutitle: BusLogic MultiMaster SCSI controller
// Attribute: Arn-Covenant x86-and-x64 Env-Freestanding
// Copyright: UNISYM, under Apache License 2.0

#ifndef _INCPP_Storage_SCSI_BusLogic
#define _INCPP_Storage_SCSI_BusLogic
#include "../../../../c/storage/harddisk.h"

#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
namespace uni::SCSI {
	class BusLogic {
	public:
		static constexpr uint16 VendorId = 0x104Bu;
		static constexpr uint16 DeviceId = 0x1040u;
		static constexpr byte MaxTargets = 16;
		static constexpr byte MaxLuns = 8;

		enum class DmaSlot : byte { Mailbox, Ccb, Data, Sense, Count };
		struct DmaBuffer {
			byte* virt = nullptr;
			uint32 device_address = 0;
			stduint size = 0;
		};
		struct Services {
			void* context = nullptr;
			// The host owns mapping and lifetime; replace a region only while no command is in flight.
			bool (*EnsureDma)(void* context, DmaSlot slot, stduint bytes, DmaBuffer& buffer) = nullptr;
			void (*DisableDma)(void* context) = nullptr;
			CommandServices commands{};
			bool (*LockInterrupt)(void* context) = nullptr;
			void (*UnlockInterrupt)(void* context, bool previous_if) = nullptr;
		};

		bool Bind(word port, const Services& host_services);
		bool Initialize(bool pci_intx_masked);
		bool IsReady() const { return !faulted && mailbox_count != 0; }
		bool IsFaulted() const { return faulted; }
		bool QueryInstalledTargets(uint16& out_mask) const;
		CommandHandle SubmitScsi(stduint target, byte lun, const byte* cdb, byte cdb_length,
			const void* data, stduint bytes, bool data_in, uint32 timeout_ms = 60000, stduint origin = 0);
		void Service();
		bool OnInterrupt();
		bool IsInterruptDriven() const { return interrupts_enabled.load(MemoryOrder_Acquire) != 0; }
		void SetInterrupts(bool enabled);
		bool Recover();
		uint32 NextTimeout() const { return queue.NextTimeout(); }
		stduint getWaiter() const { return queue.NextOrigin(); }
		// The host serializes calls; raw mailbox/adapter codes never cross this controller-neutral boundary.
		bool ExecuteScsi(byte target_id, byte lun, const byte* cdb, byte cdb_length,
			void* data_buf, stduint data_length, bool data_in,
			byte& completion_code, byte& adapter_status, byte& target_status);

	private:
		struct BusLogicBoardId {
			byte type = 0;
			byte custom_features = 0;
			byte fw_ver_digit1 = 0;
			byte fw_ver_digit2 = 0;
		};

		struct BusLogicConfig {
			byte byte0 = 0;
			byte byte1 = 0;
			byte byte2 = 0;
		};

		struct BusLogicSetupInfo {
			byte bytes[34] = {};
		};

		struct BusLogicExtSetup {
			byte bytes[14] = {};
		};

		struct BusLogicPciInfo {
			byte isa_port = 0;
			byte irq_channel = 0;
			byte termination = 0;
			byte reserved = 0;
		};

		_PACKED(struct) BusLogicExtMailboxInitReq {
			byte mbox_count;
			uint32 base_mbox_addr;
		};

		_PACKED(struct) BusLogicOutbox {
			uint32 ccb;
			byte reserved[3];
			byte action;
		};

		_PACKED(struct) BusLogicInbox {
			uint32 ccb;
			byte adapter_status;
			byte target_status;
			byte reserved;
			byte completion_code;
		};

		_PACKED(struct) BusLogicCcb32 {
			byte opcode;
			byte control;
			byte cdb_length;
			byte sense_length;
			uint32 data_length;
			uint32 data_addr;
			byte reserved12;
			byte reserved13;
			byte adapter_status;
			byte target_status;
			byte target_id;
			byte lun_legacy;
			byte cdb[12];
			byte reserved30;
			byte reserved31;
			uint32 host_reserved;
			uint32 sense_addr;
		};
		// BusLogic MultiMaster 32-bit wire layouts: Linux drivers/scsi/BusLogic.h, blogic_ccb/outbox/inbox.
		static_assert(sizeof(BusLogicExtMailboxInitReq) == 5);
		static_assert(sizeof(BusLogicOutbox) == 8);
		static_assert(sizeof(BusLogicInbox) == 8);
		static_assert(sizeof(BusLogicCcb32) == 40);

		word io_base = 0;
		Services services{};
		Atomic<byte> faulted{0};
		Atomic<byte> interrupts_enabled{0};
		mutable Atomic<byte> pending_interrupt{0};
		CommandQueue queue;
		bool extended_ccb = false;
		stduint active_count = 0;
		volatile BusLogicOutbox* outboxes = nullptr;
		volatile BusLogicInbox* inboxes = nullptr;
		stduint mailbox_count = 0;
		stduint next_outbox_index = 0;
		DmaBuffer mailbox_region{};
		bool HasIO() const { return io_base != 0; }
		bool EnsureRegion(DmaSlot slot, DmaBuffer& region, stduint bytes);
		byte ReadStatus() const;
		byte ReadInterrupt() const;
		byte ReadDataIn() const;
		void WriteControl(byte value) const;
		void WriteCommand(byte value) const;
		void InterruptReset() const;
		void ClearAdapterInterruptState() const;
		void DisableAfterCommandFailure();
		void ClearLocalCommandState() const;
		bool HardwareReset(bool hard_reset);
		bool WaitUntilReady(stduint limit = 100000) const;
		int Command(byte opcode, const void* param_data, stduint param_len, void* reply_data, stduint reply_len) const;
		bool QueryBoardId(BusLogicBoardId& out) const;
		bool PreparePolling(bool pci_intx_masked) const;
		bool QueryConfig(BusLogicConfig& out) const;
		bool QuerySetupInfo(BusLogicSetupInfo& out) const;
		bool QueryExtSetup(BusLogicExtSetup& out) const;
		bool QueryPciInfo(BusLogicPciInfo& out) const;
		bool QueryFirmwareExtra(byte& digit3, byte& letter) const;
		bool QueryModelNumber(char model[6]) const;
		bool QueryInstalledTargetsLegacy(byte out_luns0to7[8]) const;
		bool InitializeMailbox(byte requested_count);
		bool SetCcbFormat(byte fmt);
		void StartQueued();
		void DumpInquiryResults();
	};
}
#endif
#endif

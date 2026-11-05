// ASCII CPP-ISO11 TAB4 LF
// Docutitle: LSI Logic Parallel SCSI controller (Fusion-MPT MPI 1.x)
// Attribute: Arn-Covenant x86-and-x64 Env-Freestanding
// Copyright: UNISYM, under Apache License 2.0

#ifndef _INCPP_Storage_SCSI_LsiLogicParallel
#define _INCPP_Storage_SCSI_LsiLogicParallel
#include "../../../../c/storage/harddisk.h"
#include "../../../vector"
#include "../../../ISO_IEC_STD/memory"

#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
namespace uni::SCSI {
	// OS-independent MPI 1.x message unit shared by Parallel SCSI and SAS controllers.
	class LsiLogicMPI {
	public:
		static constexpr uint16 VendorId = 0x1000u;
		static constexpr stduint MaxLuns = 0x100; // The current flat-LUN interface carries one byte, not three legacy LUN bits.

		enum class DmaSlot : byte { Request, Reply, Data, Sense, Count };
		struct DmaBuffer {
			byte* virt = nullptr;
			uint32 device_address = 0;
			stduint size = 0;
		};
		struct Services {
			void* context = nullptr;
			// The host owns DMA mappings/lifetime and serializes initialization and command submission.
			bool (*EnsureDma)(void* context, DmaSlot slot, stduint bytes, DmaBuffer& buffer) = nullptr;
			void (*DisableDma)(void* context) = nullptr;
			uint32 (*Milliseconds)(void* context) = nullptr;
			CommandServices commands{};
		};

		bool Bind(word port, const Services& host_services);
		bool IsReady() const { return ready && !faulted; }
		bool IsFaulted() const { return faulted; }
		stduint getTargetCount() const { return target_count; }
		byte getInitiatorId() const { return initiator_id; }
		// Submit/Service/Recover are serialized by the host; OnInterrupt only masks the reply interrupt.
		void Service();
		bool OnInterrupt();
		bool IsInterruptDriven() const { return interrupts_enabled.load(MemoryOrder_Acquire) != 0; }
		void SetInterrupts(bool enabled);
		bool Recover();
		uint32 NextTimeout() const { return queue.NextTimeout(); }
		stduint getWaiter() const { return queue.NextOrigin(); }

	protected:
		explicit LsiLogicMPI(const char* name) : log_name(name) {}
		bool InitializePort(byte port_type);
		// Completion/status values follow the common SCSI callback, not MPI IOCStatus values.
		bool ExecuteAddressed(byte bus, byte target_id, byte lun, const byte* cdb, byte cdb_length,
			void* data_buf, stduint data_length, bool data_in,
			byte& completion_code, byte& adapter_status, byte& target_status);
		CommandHandle SubmitAddressed(byte bus, byte target, byte lun, const byte* cdb, byte cdb_length,
			const void* data, stduint bytes, bool data_in, uint32 timeout_ms, stduint origin);
		virtual bool ValidateAfterReset() { return true; }
		bool Handshake(const void* request, stduint request_bytes, void* reply,
			stduint reply_bytes, stduint minimum_reply_bytes, uint32 timeout_ms, uint16* ioc_status = nullptr);
		bool EnsureRegion(DmaSlot slot, DmaBuffer& region, stduint bytes);
		void DisableAfterFailure(const char* reason);
		DmaBuffer data_region{};

	private:
		const char* log_name;
		static constexpr stduint ReplyFrameSize = 128;
		static constexpr stduint SenseStride = 32;
		word io_base = 0;
		Services services{};
		Atomic<byte> ready{0};
		Atomic<byte> faulted{0};
		Atomic<byte> interrupts_enabled{0};
		byte port_type = 0;
		stduint target_count = 0;
		byte initiator_id = 0xFF;
		uint16 request_frame_size = 0;
		uint16 request_block_bytes = 0;
		uint16 reply_count = 0;
		stduint active_count = 0, credits = 0;
		uint32 next_context = 0;
		CommandQueue queue;
		DmaBuffer reply_region{};
		void StartQueued();
		uint32 ReadRegister(byte offset) const;
		void WriteRegister(byte offset, uint32 value) const;
		bool WaitRegister(byte offset, uint32 mask, uint32 value, uint32 timeout_ms) const;
		bool Reset();
	};

	class LsiLogicParallel : public LsiLogicMPI {
	public:
		static constexpr stduint MaxTargets = 16; // Parallel SCSI bus address width; not a storage-object slot limit.
		static constexpr uint16 DeviceId = 0x0030u; // LSI53C1030; not the SCRIPTS-based 53C895A.
		static constexpr const char* Name = "LSI-Parallel";
		LsiLogicParallel() : LsiLogicMPI(Name) {}
		bool Initialize();
		bool IsTargetPresent(stduint target) const { return target < MaxTargets && target < getTargetCount() && target != getInitiatorId(); }
		bool ExecuteScsi(stduint target_id, byte lun, const byte* cdb, byte cdb_length,
			void* data_buf, stduint data_length, bool data_in,
			byte& completion_code, byte& adapter_status, byte& target_status);
		CommandHandle SubmitScsi(stduint target, byte lun, const byte* cdb, byte cdb_length,
			const void* data, stduint bytes, bool data_in, uint32 timeout_ms = 60000, stduint origin = 0);
	};
}
#endif
#endif

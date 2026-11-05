// ASCII CPP-ISO11 TAB4 LF
// Docutitle: LSI Logic SAS1068 controller (Fusion-MPT MPI 1.5)
// Attribute: Arn-Covenant x86-and-x64 Env-Freestanding
// Copyright: UNISYM, under Apache License 2.0

#ifndef _INCPP_Storage_SCSI_LsiLogicSAS
#define _INCPP_Storage_SCSI_LsiLogicSAS
#include "LsiLogic-Para.hpp"

#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
namespace uni::SCSI {
	class LsiLogicSAS : public LsiLogicMPI {
	public:
		static constexpr uint16 DeviceId = 0x0054u; // MPI Manufacturing page: SAS1068, not MegaRAID or MPI 2.x.
		static constexpr const char* Name = "LSI-SAS";
		LsiLogicSAS() : LsiLogicMPI(Name) {}
		bool Initialize();
		bool IsReady() const { return discovery_complete && LsiLogicMPI::IsReady(); }
		stduint getTargetCount() const { return targets.Count(); }
		bool IsTargetPresent(stduint target) const { return IsReady() && target < targets.Count(); }
		bool ExecuteScsi(stduint target_id, byte lun, const byte* cdb, byte cdb_length,
			void* data_buf, stduint data_length, bool data_in,
			byte& completion_code, byte& adapter_status, byte& target_status);
		CommandHandle SubmitScsi(stduint target, byte lun, const byte* cdb, byte cdb_length,
			const void* data, stduint bytes, bool data_in, uint32 timeout_ms = 60000, stduint origin = 0);

	private:
		struct Target {
			uint64 sas_address = 0;
			uint32 device_info = 0;
			uint16 handle = 0, flags = 0;
			byte bus = 0, id = 0, access_status = 0;
		};
		// Compact host slots, not firmware IDs: a mapped target may have any 8-bit Bus/TargetID.
		uni::Vector<uni::SharedPtr<Target>> targets;
		bool discovery_complete = false;
		bool ReadNextTarget(uint16 previous_handle, Target& target, bool& found);
		bool DiscoverTargets();
		bool ValidateAfterReset() override;
	};
}
#endif
#endif

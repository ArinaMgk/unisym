// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Storage) Harddisk - NVMe
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/c/storage/harddisk.h"

#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
namespace uni {

	void Harddisk_NVMe::Bind(void* context, uint32 nsid, uint32 block_size, uint64 block_count, ExecBlocksFn exec_fn) {
		exec_context = context;
		namespace_id = nsid;
		fn_exec = exec_fn;
		total_blocks = block_count;
		Block_Size = block_size ? block_size : 512;
		hd_info.whole_disk.address = 0;
		hd_info.whole_disk.length = (stduint)block_count;
		hd_info.whole_disk.sys_id = 0;
		hd_info.scheme_kind = PartitionSchemeKind::Unknown;
		hd_info.ResetUnifiedParts();
		hd_info_valid = false;
	}

	bool Harddisk_NVMe::ReadBlocks(uint64 lba, void* dest, stduint block_count) {
		if (!fn_exec || !dest || !block_count || !Block_Size) return false;
		if (lba >= total_blocks || lba + block_count > total_blocks) return false;
		return fn_exec(exec_context, namespace_id, lba, dest, block_count, false);
	}

	bool Harddisk_NVMe::WriteBlocks(uint64 lba, const void* src, stduint block_count) {
		if (!fn_exec || !src || !block_count || !Block_Size) return false;
		if (lba >= total_blocks || lba + block_count > total_blocks) return false;
		return fn_exec(exec_context, namespace_id, lba, const_cast<void*>(src), block_count, true);
	}

	bool Harddisk_NVMe::Read(stduint BlockIden, void* Dest, stduint Times) {
		return ReadBlocks(BlockIden, Dest, Times);
	}

	bool Harddisk_NVMe::Write(stduint BlockIden, const void* Sors, stduint Times) {
		return WriteBlocks(BlockIden, Sors, Times);
	}

	stduint Harddisk_NVMe::getUnits() {
		return (stduint)total_blocks;
	}

	PartitionSlice Harddisk_NVMe::getSlice(stduint dev) {
		return GetPartitionSlice(hd_info, dev);
	}

}
#endif

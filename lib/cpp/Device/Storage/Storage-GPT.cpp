// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Storage) GPT
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence

#include "../../../../inc/cpp/unisym"
#include "../../../../inc/cpp/string"
#include "../../../../inc/cpp/trait/StorageTrait.hpp"
#include "../../../../inc/c/format/filesys.h"

namespace uni {

	_PACKED(struct) GptHeader {
		byte signature[8];
		uint32 revision;
		uint32 header_size;
		uint32 header_crc32;
		uint32 reserved;
		uint64 my_lba;
		uint64 alternate_lba;
		uint64 first_usable_lba;
		uint64 last_usable_lba;
		byte disk_guid[16];
		uint64 partition_entry_lba;
		uint32 number_of_partition_entries;
		uint32 size_of_partition_entry;
		uint32 partition_entry_array_crc32;
	};

	_PACKED(struct) GptEntry {
		byte type_guid[16];
		byte uniq_guid[16];
		uint64 first_lba;
		uint64 last_lba;
		uint64 attributes;
		uint16 name_utf16[36];
	};

	static bool _LOCAL_IsZeroGuid(const byte guid[16]) {
		for0(i, 16) if (guid[i]) return false;
		return true;
	}

	static bool _LOCAL_GuidEq(const byte lhs[16], const byte rhs[16]) {
		for0(i, 16) if (lhs[i] != rhs[i]) return false;
		return true;
	}

	static uint32 _LOCAL_Crc32(const byte* data, stduint size) {
		uint32 crc = 0xFFFFFFFFu;
		for0(i, size) {
			crc ^= data[i];
			for0(j, 8) {
				uint32 mask = -(crc & 1u);
				crc = (crc >> 1) ^ (0xEDB88320u & mask);
			}
		}
		return ~crc;
	}

	static bool _LOCAL_IsPow2(stduint value) {
		return value && !(value & (value - 1));
	}

	static byte _LOCAL_MapGptTypeToSysId(const byte guid[16]) {
		static const byte guid_esp[16] = {
			0x28, 0x73, 0x2A, 0xC1, 0x1F, 0xF8, 0xD2, 0x11,
			0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B,
		};
		static const byte guid_basic_data[16] = {
			0xA2, 0xA0, 0xD0, 0xEB, 0xE5, 0xB9, 0x33, 0x44,
			0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7,
		};
		static const byte guid_linux_fs[16] = {
			0xAF, 0x3D, 0xC6, 0x0F, 0x83, 0x84, 0x72, 0x47,
			0x8E, 0x79, 0x3D, 0x69, 0xD8, 0x47, 0x7D, 0xE4,
		};
		if (_LOCAL_GuidEq(guid, guid_esp)) return FILESYS_EFI_SYS;
		if (_LOCAL_GuidEq(guid, guid_basic_data)) return FILESYS_FAT32_LBA;
		if (_LOCAL_GuidEq(guid, guid_linux_fs)) return FILESYS_LINUX_NATIVE;
		return 0x00;
	}

	static bool _LOCAL_ReadBlock(StorageTrait& base, uint64 lba, byte* dest) {
		const uint64 max_stduint = (uint64)(stduint)~0u;
		if (lba > max_stduint) return false;
		return base.Read((stduint)lba, dest);
	}

	static bool _LOCAL_IsProtectiveMbr(byte* block, stduint block_size) {
		if (block_size < 512) return false;
		if (block[510] != 0x55 || block[511] != 0xAA) return false;
		PartitionTableX86 part_tbl[4] = {};
		MemCopyN(part_tbl, block + MBR_PARTITION_TABLE_OFFSET, sizeof(part_tbl));
		for0(i, 4) if (part_tbl[i].type == 0xEE) return true;
		return false;
	}

	static bool _LOCAL_ReadGptHeader(StorageTrait& base, uint64 lba, byte* block_buf, GptHeader& out) {
		if (!_LOCAL_ReadBlock(base, lba, block_buf)) return false;
		MemCopyN(&out, block_buf, sizeof(GptHeader));
		return true;
	}

	static bool _LOCAL_ValidateGptHeader(const GptHeader& hdr, uint64 expected_lba, uint64 expected_alt_lba, uint64 disk_last_lba, stduint block_size, const byte* block_buf) {
		static const byte sig[8] = { 'E','F','I',' ','P','A','R','T' };
		for0(i, 8) if (hdr.signature[i] != sig[i]) return false;
		if (hdr.header_size < 92 || hdr.header_size > block_size) return false;
		if (hdr.my_lba != expected_lba) return false;
		if (hdr.alternate_lba != expected_alt_lba) return false;
		if (hdr.first_usable_lba > hdr.last_usable_lba) return false;
		if (hdr.size_of_partition_entry < sizeof(GptEntry)) return false;
		if ((hdr.size_of_partition_entry & 127u) != 0) return false;
		if (!_LOCAL_IsPow2(hdr.size_of_partition_entry / 128u)) return false;
		if (!hdr.number_of_partition_entries) return false;

		uint64 entry_bytes = (uint64)hdr.number_of_partition_entries * hdr.size_of_partition_entry;
		uint64 entry_blocks = (entry_bytes + block_size - 1) / block_size;
		if (!entry_blocks) return false;
		if (hdr.partition_entry_lba < 2) return false;
		if (hdr.partition_entry_lba + entry_blocks - 1 > disk_last_lba) return false;
		if (hdr.first_usable_lba < 2 || hdr.last_usable_lba > disk_last_lba) return false;

		byte* hdr_copy = new byte[hdr.header_size];
		MemCopyN(hdr_copy, block_buf, hdr.header_size);
		cast<GptHeader*>(hdr_copy)->header_crc32 = 0;
		uint32 crc = _LOCAL_Crc32(hdr_copy, hdr.header_size);
		delete[] hdr_copy;
		return crc == hdr.header_crc32;
	}

	static bool _LOCAL_LoadGpt(StorageTrait& base, HD_Info& hdi, byte* block_buf, const GptHeader& hdr, PartitionSchemeKind kind) {
		stduint block_size = base.Block_Size ? base.Block_Size : 512;
		uint64 entry_bytes64 = (uint64)hdr.number_of_partition_entries * hdr.size_of_partition_entry;
		const uint64 max_stduint = (uint64)(stduint)~0u;
		if (entry_bytes64 > max_stduint) return false;
		stduint entry_bytes = (stduint)entry_bytes64;
		byte* entry_buf = new byte[entry_bytes];
		stduint blocks = (entry_bytes + block_size - 1) / block_size;
		for0(i, blocks) {
			if (!_LOCAL_ReadBlock(base, hdr.partition_entry_lba + i, block_buf)) {
				delete[] entry_buf;
				return false;
			}
			stduint todo = block_size;
			stduint copied = i * block_size;
			if (copied + todo > entry_bytes) todo = entry_bytes - copied;
			MemCopyN(entry_buf + copied, block_buf, todo);
		}
		uint32 entry_crc = _LOCAL_Crc32(entry_buf, entry_bytes);
		if (entry_crc != hdr.partition_entry_array_crc32) {
			delete[] entry_buf;
			return false;
		}

		hdi.scheme_kind = kind;
		hdi.ResetUnifiedParts();

		for0(i, hdr.number_of_partition_entries) {
			byte* raw = entry_buf + i * hdr.size_of_partition_entry;
			GptEntry ent = {};
			MemCopyN(&ent, raw, sizeof(GptEntry));
			if (_LOCAL_IsZeroGuid(ent.type_guid)) continue;
			if (ent.first_lba > ent.last_lba) {
				delete[] entry_buf;
				return false;
			}
			if (ent.first_lba < hdr.first_usable_lba || ent.last_lba > hdr.last_usable_lba) {
				delete[] entry_buf;
				return false;
			}
			uint64 slice_len = ent.last_lba - ent.first_lba + 1;
			if (ent.first_lba > max_stduint || slice_len > max_stduint) {
				delete[] entry_buf;
				return false;
			}
			for0(j, hdi.part_count) {
				auto prev = hdi.parts[j];
				uint64 prev_start = prev.address;
				uint64 prev_end = prev.address + prev.length - 1;
				if (!(ent.last_lba < prev_start || ent.first_lba > prev_end)) {
					delete[] entry_buf;
					return false;
				}
			}

			PartitionSlice slice = {};
			slice.address = (stduint)ent.first_lba;
			slice.length = (stduint)slice_len;
			slice.sys_id = _LOCAL_MapGptTypeToSysId(ent.type_guid);
			stduint meta_index = hdi.part_count < MAX_PARTITIONS ? hdi.part_count : MAX_PARTITIONS - 1;
			bool appended = hdi.AppendPart(slice);
			if (appended) {
				MemCopyN(hdi.gpt_meta[meta_index].type_guid, ent.type_guid, 16);
				MemCopyN(hdi.gpt_meta[meta_index].uniq_guid, ent.uniq_guid, 16);
				hdi.gpt_meta[meta_index].attributes = ent.attributes;
			}
		}

		delete[] entry_buf;
		return true;
	}

	static bool _LOCAL_TryGpt(StorageTrait& base, HD_Info& hdi, byte* block_buf, uint64 header_lba, uint64 expected_alt_lba, uint64 disk_last_lba, PartitionSchemeKind kind) {
		GptHeader hdr = {};
		if (!_LOCAL_ReadGptHeader(base, header_lba, block_buf, hdr)) return false;
		stduint block_size = base.Block_Size ? base.Block_Size : 512;
		if (!_LOCAL_ValidateGptHeader(hdr, header_lba, expected_alt_lba, disk_last_lba, block_size, block_buf)) return false;
		return _LOCAL_LoadGpt(base, hdi, block_buf, hdr, kind);
	}

	bool LoadGPTPartitions(StorageTrait& base, HD_Info& hdi, byte* block_buf) {
		stduint block_size = base.Block_Size ? base.Block_Size : 512;
		uint64 units = base.getUnits();
		if (!units) return false;
		uint64 last_lba = units - 1;
		if (!_LOCAL_ReadBlock(base, 0, block_buf) || !_LOCAL_IsProtectiveMbr(block_buf, block_size)) return false;
		if (_LOCAL_TryGpt(base, hdi, block_buf, 1, last_lba, last_lba, PartitionSchemeKind::GPT)) return true;
		if (_LOCAL_TryGpt(base, hdi, block_buf, last_lba, 1, last_lba, PartitionSchemeKind::GPT_Backup)) return true;
		return false;
	}
}

// ASCII C++11 TAB4 CRLF
// Docutitle: (Format/FileSystem) Compact Disc
// Attribute: Arn-Covenant Any-Architect Non-Dependence

#include "../../../inc/c/format/filesys/CD.h"

namespace uni {

	static inline uint32 iso_min_u32(uint32 a, uint32 b) { return a < b ? a : b; }
	static inline uint32 udf_min_u32(uint32 a, uint32 b) { return a < b ? a : b; }

	_PACKED(struct) ISO9660_SystemUseEntryHeader {
		char signature[2];
		byte length;
		byte version;
	};

	static bool iso_name_equals(const char* a, const char* b) {
		return !StrCompareInsensitive(a, b);
	}

	static uint32 udf_ad_length(uint32 raw_length) {
		return raw_length & 0x3FFFFFFFu;
	}

	static uint32 udf_ad_flags(uint32 raw_length) {
		return raw_length >> 30;
	}

	static stduint udf_align4(stduint value) {
		return (value + 3u) & ~3u;
	}

	static void iso_append_utf8(char* out_name, stduint out_size, stduint& name_len, uint32_t codepoint);
	static bool udf_load_file_entry(FilesysUDF* fs, uint32_t abs_lba, UDF_FileHandle* out_handle);
	static void udf_set_root_handle(FilesysUDF* fs, UDF_FileHandle* out);

	static void udf_decode_dstring(const byte* src, stduint len, char* out_name, stduint out_size) {
		if (!out_name || out_size == 0) return;
		out_name[0] = '\0';
		if (!src || len == 0) return;

		byte comp_id = src[0];
		stduint name_len = 0;
		if (comp_id == 8) {
			for (stduint i = 1; i < len && name_len + 1 < out_size; i++) {
				if (src[i] == 0) break;
				out_name[name_len++] = (char)src[i];
			}
		} else if (comp_id == 16) {
			for (stduint i = 1; i + 1 < len; i += 2) {
				uint16_t ch = ((uint16_t)src[i] << 8) | (uint16_t)src[i + 1];
				if (ch == 0) break;
				iso_append_utf8(out_name, out_size, name_len, ch);
			}
		}
		out_name[name_len] = '\0';
	}

	static void udf_decode_fid_name(const UDF_FileIdentifierDescriptor* fid, const byte* src, char* out_name, stduint out_size) {
		if (!out_name || out_size == 0) return;
		out_name[0] = '\0';
		if (!fid) return;

		udf_decode_dstring(src, fid->length_of_file_identifier, out_name, out_size);
		if (out_name[0]) return;
		if (fid->file_characteristics & 0x08u) {
			StrCopy(out_name, "..");
			return;
		}
		if (fid->file_characteristics & 0x02u) {
			StrCopy(out_name, ".");
		}
	}

	static bool udf_should_skip_fid(const UDF_FileIdentifierDescriptor* fid) {
		if (!fid) return true;
		return (fid->file_characteristics & 0x01u) != 0;
	}

	static void udf_copy_parent(UDF_FileHandle* dst, const UDF_FileHandle& src) {
		if (!dst) return;
		dst->parent_valid = 1;
		dst->parent_entry_lba = src.entry_lba;
		dst->parent_data_lba = src.data_lba;
		dst->parent_length = src.length;
		dst->parent_partition_ref = src.partition_ref;
		dst->parent_partition_start = src.partition_start;
		dst->parent_alloc_type = src.alloc_type;
		dst->parent_embedded = src.embedded;
		dst->parent_embedded_offset = src.embedded_offset;
	}

	static bool udf_load_parent_handle(FilesysUDF* fs, const UDF_FileHandle& child, UDF_FileHandle* out) {
		if (!fs || !out) return false;
		if (child.entry_lba == fs->root_handle.entry_lba) {
			udf_set_root_handle(fs, out);
			return true;
		}
		if (child.icb_parent_ref != fs->partition_ref || child.icb_parent_lba == 0) {
			if (child.parent_valid) {
				out->entry_lba = child.parent_entry_lba;
				out->data_lba = child.parent_data_lba;
				out->length = child.parent_length;
				out->partition_ref = child.parent_partition_ref;
				out->partition_start = child.parent_partition_start;
				out->alloc_type = child.parent_alloc_type;
				out->embedded = child.parent_embedded;
				out->embedded_offset = child.parent_embedded_offset;
				out->is_dir = 1;
				if (out->entry_lba == fs->root_handle.entry_lba) {
					udf_set_root_handle(fs, out);
				}
				return true;
			}
			udf_set_root_handle(fs, out);
			return true;
		}

		*out = UDF_FileHandle{};
		out->partition_ref = fs->partition_ref;
		out->partition_start = fs->partition_start;
		if (!udf_load_file_entry(fs, child.icb_parent_lba, out)) return false;
		if (out->entry_lba == fs->root_handle.entry_lba) {
			udf_set_root_handle(fs, out);
		}
		return true;
	}

	static void udf_set_root_handle(FilesysUDF* fs, UDF_FileHandle* out) {
		if (!fs || !out) return;
		*out = fs->root_handle;
		udf_copy_parent(out, fs->root_handle);
	}

	static bool udf_load_embedded_sector(FilesysUDF* fs, const UDF_FileHandle& handle, uint32 sector_index) {
		if (!fs || !handle.embedded) return false;
		if (!fs->storage->Read(handle.entry_lba, fs->sector_buffer)) return false;

		stduint offset = (stduint)handle.embedded_offset + (stduint)sector_index * fs->logical_block_size;
		if (offset >= fs->logical_block_size) return false;

		stduint available = fs->logical_block_size - offset;
		MemAbsolute((char*)fs->sector_buffer, (const char*)(fs->sector_buffer + offset), available);
		if (available < fs->logical_block_size) {
			MemSet(fs->sector_buffer + available, 0, fs->logical_block_size - available);
		}
		return true;
	}

	static bool udf_map_block(FilesysUDF* fs, const UDF_FileHandle& handle, uint32 sector_index, uint32* abs_lba) {
		if (!fs || !abs_lba || handle.embedded) return false;
		if (!fs->storage->Read(handle.entry_lba, fs->sector_buffer)) return false;

		UDF_DescriptorTag* tag = (UDF_DescriptorTag*)fs->sector_buffer;
		if (tag->tag_identifier != 261 && tag->tag_identifier != 266) return false;

		uint32_t length_of_extended_attributes = 0;
		uint32_t length_of_allocation_descriptors = 0;
		stduint entry_header_size = 0;

		if (tag->tag_identifier == 261) {
			UDF_FileEntry* fe = (UDF_FileEntry*)fs->sector_buffer;
			length_of_extended_attributes = fe->length_of_extended_attributes;
			length_of_allocation_descriptors = fe->length_of_allocation_descriptors;
			entry_header_size = sizeof(UDF_FileEntry);
		}
		else {
			UDF_ExtendedFileEntry* efe = (UDF_ExtendedFileEntry*)fs->sector_buffer;
			length_of_extended_attributes = efe->length_of_extended_attributes;
			length_of_allocation_descriptors = efe->length_of_allocation_descriptors;
			entry_header_size = sizeof(UDF_ExtendedFileEntry);
		}

		const byte* alloc_base = fs->sector_buffer + entry_header_size + length_of_extended_attributes;
		uint32_t alloc_len = length_of_allocation_descriptors;
		uint32_t remain = sector_index;

		if (handle.alloc_type == 0) {
			for (uint32_t off = 0; off + sizeof(UDF_ShortAD) <= alloc_len; off += sizeof(UDF_ShortAD)) {
				const UDF_ShortAD* ad = (const UDF_ShortAD*)(alloc_base + off);
				uint32_t seg_len = udf_ad_length(ad->length);
				uint32_t seg_flags = udf_ad_flags(ad->length);
				uint32_t seg_blocks = (seg_len + fs->logical_block_size - 1) / fs->logical_block_size;
				if (remain < seg_blocks) {
					if (seg_flags != 0) return false;
					*abs_lba = handle.partition_start + ad->position + remain;
					return true;
				}
				remain -= seg_blocks;
			}
			return false;
		}
		if (handle.alloc_type == 1) {
			for (uint32_t off = 0; off + sizeof(UDF_LongAD) <= alloc_len; off += sizeof(UDF_LongAD)) {
				const UDF_LongAD* ad = (const UDF_LongAD*)(alloc_base + off);
				uint32_t seg_len = udf_ad_length(ad->length);
				uint32_t seg_flags = udf_ad_flags(ad->length);
				uint32_t seg_blocks = (seg_len + fs->logical_block_size - 1) / fs->logical_block_size;
				if (remain < seg_blocks) {
					if (seg_flags != 0) return false;
					if (ad->location.partition_reference_number != handle.partition_ref) return false;
					*abs_lba = handle.partition_start + ad->location.logical_block_number + remain;
					return true;
				}
				remain -= seg_blocks;
			}
			return false;
		}
		return false;
	}

	static bool udf_load_file_sector(FilesysUDF* fs, const UDF_FileHandle& handle, uint32 sector_index) {
		if (!fs) return false;
		if (handle.embedded) return udf_load_embedded_sector(fs, handle, sector_index);

		uint32 abs_lba = 0;
		if (!udf_map_block(fs, handle, sector_index, &abs_lba)) return false;
		return fs->storage->Read(abs_lba, fs->sector_buffer);
	}

	static bool udf_load_dir_sector(FilesysUDF* fs, const UDF_FileHandle& dir, uint32 sector_index) {
		if (!fs || !dir.is_dir) return false;
		return udf_load_file_sector(fs, dir, sector_index);
	}

	static void iso_append_utf8(char* out_name, stduint out_size, stduint& name_len, uint32_t codepoint) {
		if (codepoint <= 0x7F) {
			if (name_len + 1 >= out_size) return;
			out_name[name_len++] = (char)codepoint;
			return;
		}
		if (codepoint <= 0x7FF) {
			if (name_len + 2 >= out_size) return;
			out_name[name_len++] = (char)(0xC0 | ((codepoint >> 6) & 0x1F));
			out_name[name_len++] = (char)(0x80 | (codepoint & 0x3F));
			return;
		}
		if (name_len + 3 >= out_size) return;
		out_name[name_len++] = (char)(0xE0 | ((codepoint >> 12) & 0x0F));
		out_name[name_len++] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
		out_name[name_len++] = (char)(0x80 | (codepoint & 0x3F));
	}

	static void iso_decode_name_ascii(const ISO9660_DirectoryRecord* rec, char* out_name, stduint out_size) {
		if (!out_name || out_size == 0) return;
		out_name[0] = '\0';
		if (!rec) return;

		if (rec->file_id_length == 1 && (byte)rec->file_id[0] == 0) {
			StrCopy(out_name, ".");
			return;
		}
		if (rec->file_id_length == 1 && (byte)rec->file_id[0] == 1) {
			StrCopy(out_name, "..");
			return;
		}

		stduint name_len = 0;
		for (stduint i = 0; i < rec->file_id_length && name_len + 1 < out_size; i++) {
			char ch = rec->file_id[i];
			if (ch == ';') break;
			out_name[name_len++] = ascii_tolower(ch);
		}
		while (name_len > 0 && out_name[name_len - 1] == '.') {
			name_len--;
		}
		out_name[name_len] = '\0';
	}

	static void iso_decode_name_joliet(const ISO9660_DirectoryRecord* rec, char* out_name, stduint out_size) {
		if (!out_name || out_size == 0) return;
		out_name[0] = '\0';
		if (!rec) return;

		if (rec->file_id_length == 1 && (byte)rec->file_id[0] == 0) {
			StrCopy(out_name, ".");
			return;
		}
		if (rec->file_id_length == 1 && (byte)rec->file_id[0] == 1) {
			StrCopy(out_name, "..");
			return;
		}

		stduint name_len = 0;
		for (stduint i = 0; i + 1 < rec->file_id_length; i += 2) {
			uint16_t ch = ((uint16_t)(byte)rec->file_id[i] << 8) | (uint16_t)(byte)rec->file_id[i + 1];
			if (ch == ';') break;
			iso_append_utf8(out_name, out_size, name_len, ch);
		}
		out_name[name_len] = '\0';
	}

	static const byte* iso_get_system_use_area(const ISO9660_DirectoryRecord* rec, stduint& su_len) {
		su_len = 0;
		if (!rec || rec->length < 33) return nullptr;
		stduint base = 33 + rec->file_id_length;
		if (base & 1) base++;
		if (base >= rec->length) return nullptr;
		su_len = rec->length - base;
		return ((const byte*)rec) + base;
	}

	static bool iso_extract_rr_name(const ISO9660_DirectoryRecord* rec, char* out_name, stduint out_size) {
		if (!out_name || out_size == 0) return false;
		out_name[0] = '\0';
		if (!rec) return false;
		if (rec->file_id_length == 1 && ((byte)rec->file_id[0] == 0 || (byte)rec->file_id[0] == 1)) return false;

		stduint su_len = 0;
		const byte* su = iso_get_system_use_area(rec, su_len);
		if (!su) return false;

		stduint used = 0;
		stduint name_len = 0;
		bool found = false;
		while (used + sizeof(ISO9660_SystemUseEntryHeader) <= su_len) {
			const ISO9660_SystemUseEntryHeader* ent = (const ISO9660_SystemUseEntryHeader*)(su + used);
			if (ent->length < sizeof(ISO9660_SystemUseEntryHeader) || used + ent->length > su_len) break;

			if (ent->signature[0] == 'N' && ent->signature[1] == 'M' && ent->length >= 5) {
				byte flags = su[used + 4];
				if (!(flags & 0x06)) {
					found = true;
					for (stduint i = 5; i < ent->length && name_len + 1 < out_size; i++) {
						out_name[name_len++] = (char)su[used + i];
					}
				}
			}
			used += ent->length;
		}
		out_name[name_len] = '\0';
		return found && name_len > 0;
	}

	static bool iso_is_valid_record(const ISO9660_DirectoryRecord* rec, stduint remain) {
		if (!rec || remain < 1) return false;
		if (rec->length == 0) return false;
		if (rec->length > remain) return false;
		if (rec->length < 33) return false;
		return true;
	}

	static bool iso_dir_has_rockridge(FilesysISO9660* fs, uint32_t dir_extent_lba, uint32_t dir_data_length) {
		if (!fs) return false;
		uint32 total = dir_data_length;
		uint32 processed = 0;
		uint32 sector_count = (total + fs->logical_block_size - 1) / fs->logical_block_size;

		for (uint32 sec = 0; sec < sector_count; sec++) {
			uint32 lba = dir_extent_lba + sec;
			if (!fs->storage->Read(lba, fs->sector_buffer)) return false;

			uint32 sector_used = iso_min_u32(fs->logical_block_size, total - processed);
			uint32 off = 0;
			while (off < sector_used) {
				ISO9660_DirectoryRecord* rec = (ISO9660_DirectoryRecord*)(fs->sector_buffer + off);
				if (rec->length == 0) break;
				if (!iso_is_valid_record(rec, sector_used - off)) break;

				stduint su_len = 0;
				const byte* su = iso_get_system_use_area(rec, su_len);
				if (su && su_len >= 7) {
					const ISO9660_SystemUseEntryHeader* ent = (const ISO9660_SystemUseEntryHeader*)su;
					if (ent->signature[0] == 'S' && ent->signature[1] == 'P' && ent->length >= 7 &&
						su[4] == 0xBE && su[5] == 0xEF) {
						return true;
					}
				}

				char rr_name[128];
				if (iso_extract_rr_name(rec, rr_name, sizeof(rr_name))) return true;
				off += rec->length;
			}
			processed += sector_used;
		}
		return false;
	}

	static void iso_decode_name(FilesysISO9660* fs, const ISO9660_DirectoryRecord* rec, char* out_name, stduint out_size) {
		if (fs && fs->rockridge_enabled) {
			if (iso_extract_rr_name(rec, out_name, out_size)) return;
			iso_decode_name_ascii(rec, out_name, out_size);
		} else if (fs && fs->joliet_enabled) {
			iso_decode_name_joliet(rec, out_name, out_size);
		} else {
			iso_decode_name_ascii(rec, out_name, out_size);
		}
	}

	static void iso_fill_handle(ISO9660_FileHandle* out, const ISO9660_DirectoryRecord* rec, uint32 sector_lba, uint16_t sector_off) {
		if (!out || !rec) return;
		out->extent_lba = rec->extent_lba_le;
		out->data_length = rec->data_length_le;
		out->record_lba = sector_lba;
		out->record_offset = sector_off;
		out->flags = rec->flags;
	}

	static void iso_set_root_handle(FilesysISO9660* fs, ISO9660_FileHandle* out) {
		if (!fs || !out) return;
		*out = ISO9660_FileHandle{};
		out->extent_lba = fs->root_extent_lba;
		out->data_length = fs->root_data_length;
		out->flags = 0x02;
		out->parent_valid = 1;
		out->parent_extent_lba = fs->root_extent_lba;
		out->parent_data_length = fs->root_data_length;
		out->parent_flags = 0x02;
	}

	bool FilesysISO9660::makefs(rostr vol_label, void* moreinfo) {
		(void)vol_label;
		(void)moreinfo;
		return false;
	}

	bool FilesysISO9660::create(rostr fullpath, stduint flags, void* exinfo, rostr linkdest) {
		(void)fullpath;
		(void)flags;
		(void)exinfo;
		(void)linkdest;
		return false;
	}

	bool FilesysISO9660::remove(rostr pathname) {
		(void)pathname;
		return false;
	}

	stduint FilesysISO9660::writfl(void* fil_handler, Slice file_slice, const byte* src) {
		(void)fil_handler;
		(void)file_slice;
		(void)src;
		return 0;
	}

	bool FilesysISO9660::loadfs(void* moreinfo) {
		(void)moreinfo;
		if (!storage || !sector_buffer) {
			plogwarn("[ISO9660] missing storage or sector buffer");
			return false;
		}
		if (storage->Block_Size != 2048) {
			plogwarn("[ISO9660] unexpected block size %u", storage->Block_Size);
			return false;
		}

		bool found_pvd = false;
		uint32_t pvd_root_extent = 0;
		uint32_t pvd_root_size = 0;
		uint32_t joliet_root_extent = 0;
		uint32_t joliet_root_size = 0;
		bool found_joliet = false;

		for (uint32 sector = 16; sector < 64; sector++) {
			if (!storage->Read(sector, sector_buffer)) {
				plogwarn("[ISO9660] read sector %u failed", sector);
				return false;
			}

			ISO9660_VolumeDescriptorHeader* hdr = (ISO9660_VolumeDescriptorHeader*)sector_buffer;
			if (StrCompareN(hdr->id, "CD001", 5) != 0 || hdr->version != 1) {
				continue;
			}

			if (hdr->type == 1) {
				logical_block_size = *(uint16_t*)(sector_buffer + 128);
				if (logical_block_size != 2048) {
					plogwarn("[ISO9660] unsupported logical block size %u", logical_block_size);
					return false;
				}

				ISO9660_DirectoryRecord* root = (ISO9660_DirectoryRecord*)(sector_buffer + 156);
				if (!iso_is_valid_record(root, storage->Block_Size - 156)) {
					plogwarn("[ISO9660] invalid root directory record");
					return false;
				}

				found_pvd = true;
				pvd_root_extent = root->extent_lba_le;
				pvd_root_size = root->data_length_le;
				ploginfo("[ISO9660] pvd root extent=%u size=%u", pvd_root_extent, pvd_root_size);
				continue;
			}

			if (hdr->type == 2) {
				byte esc1 = sector_buffer[88];
				byte esc2 = sector_buffer[89];
				byte esc3 = sector_buffer[90];
				if (esc1 == 0x25 && esc2 == 0x2F && (esc3 == 0x40 || esc3 == 0x43 || esc3 == 0x45)) {
					ISO9660_DirectoryRecord* root = (ISO9660_DirectoryRecord*)(sector_buffer + 156);
					if (!iso_is_valid_record(root, storage->Block_Size - 156)) {
						plogwarn("[ISO9660] invalid joliet root directory record");
						return false;
					}
					joliet_root_extent = root->extent_lba_le;
					joliet_root_size = root->data_length_le;
					found_joliet = true;
					ploginfo("[ISO9660] joliet root extent=%u size=%u", joliet_root_extent, joliet_root_size);
				}
			}

			if (hdr->type == 255) break;
		}
		if (found_pvd) {
			root_extent_lba = pvd_root_extent;
			root_data_length = pvd_root_size;
			joliet_enabled = false;
			rockridge_enabled = false;

			if (iso_dir_has_rockridge(this, pvd_root_extent, pvd_root_size)) {
				rockridge_enabled = true;
			} else if (found_joliet) {
				root_extent_lba = joliet_root_extent;
				root_data_length = joliet_root_size;
				joliet_enabled = true;
			}

			ploginfo("[ISO9660] root extent=%u size=%u mode=%s",
				root_extent_lba,
				root_data_length,
				rockridge_enabled ? "rockridge" : (joliet_enabled ? "joliet" : "iso"));
			return fs_loaded = true;
		}
		ploginfo("[ISO9660] no primary volume descriptor found");
		return false;
	}

	static bool iso_find_in_dir(FilesysISO9660* fs, const ISO9660_FileHandle& dir, const char* target, ISO9660_FileHandle* out_handle) {
		if (!fs || !target || !out_handle) return false;
		if (!dir.isDir()) return false;

		uint32 total = dir.data_length;
		uint32 processed = 0;
		uint32 sector_count = (total + fs->logical_block_size - 1) / fs->logical_block_size;

		for (uint32 sec = 0; sec < sector_count; sec++) {
			uint32 lba = dir.extent_lba + sec;
			if (!fs->storage->Read(lba, fs->sector_buffer)) return false;

			uint32 sector_used = iso_min_u32(fs->logical_block_size, total - processed);
			uint32 off = 0;
			while (off < sector_used) {
				ISO9660_DirectoryRecord* rec = (ISO9660_DirectoryRecord*)(fs->sector_buffer + off);
				if (rec->length == 0) break;
				if (!iso_is_valid_record(rec, sector_used - off)) break;

				char name[128];
				iso_decode_name(fs, rec, name, sizeof(name));
				if (iso_name_equals(name, target)) {
					iso_fill_handle(out_handle, rec, lba, (uint16_t)off);
					return true;
				}
				off += rec->length;
			}
			processed += sector_used;
		}
		return false;
	}

	static bool iso_find_special_in_dir(FilesysISO9660* fs, const ISO9660_FileHandle& dir, byte special_id, ISO9660_FileHandle* out_handle) {
		if (!fs || !out_handle || !dir.isDir()) return false;

		uint32 total = dir.data_length;
		uint32 processed = 0;
		uint32 sector_count = (total + fs->logical_block_size - 1) / fs->logical_block_size;

		for (uint32 sec = 0; sec < sector_count; sec++) {
			uint32 lba = dir.extent_lba + sec;
			if (!fs->storage->Read(lba, fs->sector_buffer)) return false;

			uint32 sector_used = iso_min_u32(fs->logical_block_size, total - processed);
			uint32 off = 0;
			while (off < sector_used) {
				ISO9660_DirectoryRecord* rec = (ISO9660_DirectoryRecord*)(fs->sector_buffer + off);
				if (rec->length == 0) break;
				if (!iso_is_valid_record(rec, sector_used - off)) break;

				if (rec->file_id_length == 1 && (byte)rec->file_id[0] == special_id) {
					*out_handle = ISO9660_FileHandle{};
					iso_fill_handle(out_handle, rec, lba, (uint16_t)off);
					return true;
				}
				off += rec->length;
			}
			processed += sector_used;
		}
		return false;
	}

	void* FilesysISO9660::search(rostr fullpath, FilesysSearchArgs* args) {
		if (!fs_loaded || !args || !args->handle_buffer) return nullptr;

		ISO9660_FileHandle* handle = (ISO9660_FileHandle*)args->handle_buffer;
		iso_set_root_handle(this, handle);

		if (!fullpath || !*fullpath || !StrCompare(fullpath, "/")) {
			return handle;
		}

		const char* p = fullpath;
		if (*p == '/') p++;
		while (*p) {
			char segment[128];
			stduint seg_len = 0;
			while (*p && *p != '/' && seg_len + 1 < sizeof(segment)) {
				segment[seg_len++] = *p++;
			}
			segment[seg_len] = '\0';
			while (*p == '/') p++;

			if (!seg_len || !StrCompare(segment, ".")) continue;
			if (!StrCompare(segment, "..")) {
				if (handle->parent_valid) {
					ISO9660_FileHandle parent = {};
					parent.extent_lba = handle->parent_extent_lba;
					parent.data_length = handle->parent_data_length;
					parent.flags = handle->parent_flags;

					if (parent.extent_lba == root_extent_lba && parent.data_length == root_data_length) {
						iso_set_root_handle(this, &parent);
					} else {
						ISO9660_FileHandle grand = {};
						if (iso_find_special_in_dir(this, parent, 1, &grand)) {
							parent.parent_valid = 1;
							parent.parent_extent_lba = grand.extent_lba;
							parent.parent_data_length = grand.data_length;
							parent.parent_flags = grand.flags;
						}
					}
					*handle = parent;
				}
				if (args->on_segment) args->on_segment(handle, segment, 1, handle->data_length, args->user_data);
				continue;
			}

			ISO9660_FileHandle next = {};
			if (!iso_find_in_dir(this, *handle, segment, &next)) return nullptr;
			next.parent_valid = 1;
			next.parent_extent_lba = handle->extent_lba;
			next.parent_data_length = handle->data_length;
			next.parent_flags = handle->flags;
			if (next.extent_lba == root_extent_lba && next.data_length == root_data_length && next.isDir()) {
				iso_set_root_handle(this, &next);
			}
			*handle = next;

			if (args->on_segment) {
				if (!args->on_segment(handle, segment, handle->isDir() ? 1 : 0, handle->data_length, args->user_data)) {
					return handle;
				}
			}

			if (*p && !handle->isDir()) return nullptr;
		}
		return handle;
	}

	bool FilesysISO9660::proper(void* handler, stduint cmd, const void* moreinfo) {
		ISO9660_FileHandle* fh = (ISO9660_FileHandle*)handler;
		if (!fh || !moreinfo) return false;

		switch ((FilesysCmd)cmd) {
		case FilesysCmd::FS_CMD_GET_SIZE:
			*(stduint*)moreinfo = fh->data_length;
			return true;
		case FilesysCmd::FS_CMD_GET_ISDIR:
			*(bool*)moreinfo = fh->isDir();
			return true;
		default:
			return false;
		}
	}

	bool FilesysISO9660::enumer(void* dir_handler, _tocall_ft _fn) {
		ISO9660_FileHandle* dir = (ISO9660_FileHandle*)dir_handler;
		if (!dir || !dir->isDir() || !_fn) return false;

		uint32 total = dir->data_length;
		uint32 processed = 0;
		uint32 sector_count = (total + logical_block_size - 1) / logical_block_size;

		for (uint32 sec = 0; sec < sector_count; sec++) {
			uint32 lba = dir->extent_lba + sec;
			if (!storage->Read(lba, sector_buffer)) return false;

			uint32 sector_used = iso_min_u32(logical_block_size, total - processed);
			uint32 off = 0;
			while (off < sector_used) {
				ISO9660_DirectoryRecord* rec = (ISO9660_DirectoryRecord*)(sector_buffer + off);
				if (rec->length == 0) break;
				if (!iso_is_valid_record(rec, sector_used - off)) break;

				char name[128];
				iso_decode_name(this, rec, name, sizeof(name));
				_fn((void*)(stduint)(rec->flags & 0x02u ? 1 : 0), (void*)name);
				off += rec->length;
			}
			processed += sector_used;
		}
		return true;
	}

	stduint FilesysISO9660::readfl(void* fil_handler, Slice file_slice, byte* dst) {
		ISO9660_FileHandle* fh = (ISO9660_FileHandle*)fil_handler;
		if (!fh || !dst || fh->isDir()) return 0;
		if (file_slice.address >= fh->data_length) return 0;

		uint32 remaining = fh->data_length - file_slice.address;
		uint32 need = iso_min_u32((uint32)file_slice.length, remaining);
		uint32 total_read = 0;

		while (total_read < need) {
			uint32 offset = (uint32)file_slice.address + total_read;
			uint32 sector_index = offset / logical_block_size;
			uint32 sector_offset = offset % logical_block_size;
			uint32 can_read = iso_min_u32(logical_block_size - sector_offset, need - total_read);

			if (!storage->Read(fh->extent_lba + sector_index, sector_buffer)) break;
			MemCopyN(dst + total_read, sector_buffer + sector_offset, can_read);
			total_read += can_read;
		}
		return total_read;
	}

	static bool udf_load_file_entry(FilesysUDF* fs, uint32_t abs_lba, UDF_FileHandle* out_handle) {
		if (!fs || !out_handle) return false;
		if (!fs->storage->Read(abs_lba, fs->sector_buffer)) return false;

		UDF_DescriptorTag* tag = (UDF_DescriptorTag*)fs->sector_buffer;
		if (tag->tag_identifier != 261 && tag->tag_identifier != 266) return false;

		const UDF_ICBTag* icb = nullptr;
		uint64_t information_length = 0;
		uint32_t length_of_extended_attributes = 0;
		uint32_t length_of_allocation_descriptors = 0;
		stduint entry_header_size = 0;

		if (tag->tag_identifier == 261) {
			UDF_FileEntry* fe = (UDF_FileEntry*)fs->sector_buffer;
			icb = &fe->icb_tag;
			information_length = fe->information_length;
			length_of_extended_attributes = fe->length_of_extended_attributes;
			length_of_allocation_descriptors = fe->length_of_allocation_descriptors;
			entry_header_size = sizeof(UDF_FileEntry);
		}
		else {
			UDF_ExtendedFileEntry* efe = (UDF_ExtendedFileEntry*)fs->sector_buffer;
			icb = &efe->icb_tag;
			information_length = efe->information_length;
			length_of_extended_attributes = efe->length_of_extended_attributes;
			length_of_allocation_descriptors = efe->length_of_allocation_descriptors;
			entry_header_size = sizeof(UDF_ExtendedFileEntry);
		}
		if (!icb) return false;

		out_handle->entry_lba = abs_lba;
		out_handle->length = (uint32_t)information_length;
		out_handle->alloc_type = icb->flags & 0x0007u;
		out_handle->is_dir = (icb->file_type == 4) ? 1 : 0;
		out_handle->embedded = 0;
		out_handle->embedded_offset = 0;
		out_handle->data_lba = 0;
		out_handle->icb_parent_ref = icb->parent_icb_location.partition_reference_number;
		out_handle->icb_parent_lba = out_handle->partition_start + icb->parent_icb_location.logical_block_number;

		const byte* alloc_base = fs->sector_buffer + entry_header_size + length_of_extended_attributes;
		uint32_t alloc_len = length_of_allocation_descriptors;
		if (out_handle->length == 0) return true;

		if (out_handle->alloc_type == 0 && alloc_len >= sizeof(UDF_ShortAD)) {
			const UDF_ShortAD* ad = (const UDF_ShortAD*)alloc_base;
			out_handle->data_lba = out_handle->partition_start + ad->position;
			return true;
		}
		if (out_handle->alloc_type == 1 && alloc_len >= sizeof(UDF_LongAD)) {
			const UDF_LongAD* ad = (const UDF_LongAD*)alloc_base;
			out_handle->partition_ref = ad->location.partition_reference_number;
			out_handle->data_lba = out_handle->partition_start + ad->location.logical_block_number;
			return true;
		}
		if (out_handle->alloc_type == 3) {
			out_handle->data_lba = abs_lba;
			out_handle->embedded = 1;
			out_handle->embedded_offset = (uint16_t)(entry_header_size + length_of_extended_attributes);
			out_handle->length = (uint32_t)information_length;
			return true;
		}
		return false;
	}

	static bool udf_find_in_dir(FilesysUDF* fs, const UDF_FileHandle& dir, const char* target, UDF_FileHandle* out_handle) {
		if (!fs || !target || !out_handle || !dir.is_dir) return false;

		uint32 total = dir.length;
		uint32 processed = 0;
		uint32 sector_count = (total + fs->logical_block_size - 1) / fs->logical_block_size;

		for (uint32 sec = 0; sec < sector_count; sec++) {
			if (!udf_load_dir_sector(fs, dir, sec)) return false;

			uint32 sector_used = udf_min_u32(fs->logical_block_size, total - processed);
			uint32 off = 0;
			while (off + sizeof(UDF_FileIdentifierDescriptor) <= sector_used) {
				UDF_FileIdentifierDescriptor* fid = (UDF_FileIdentifierDescriptor*)(fs->sector_buffer + off);
				if (fid->tag.tag_identifier == 0 || fid->tag.tag_identifier == 8) break;
				if (fid->tag.tag_identifier != 257) break;

				stduint name_off = off + sizeof(UDF_FileIdentifierDescriptor) + fid->length_of_implementation_use;
				if (name_off > sector_used || name_off + fid->length_of_file_identifier > sector_used) break;
				if (udf_should_skip_fid(fid)) {
					stduint fid_len = sizeof(UDF_FileIdentifierDescriptor) + fid->length_of_implementation_use + fid->length_of_file_identifier;
					fid_len = udf_align4(fid_len);
					if (fid_len == 0) break;
					off += fid_len;
					continue;
				}

				char name[256];
				udf_decode_fid_name(fid, fs->sector_buffer + name_off, name, sizeof(name));
				if (!StrCompareInsensitive(name, target)) {
					if (fid->icb.location.partition_reference_number != fs->partition_ref) return false;
					UDF_FileHandle next = {};
					next.partition_start = fs->partition_start;
					next.partition_ref = fid->icb.location.partition_reference_number;
					if (!udf_load_file_entry(fs, fs->partition_start + fid->icb.location.logical_block_number, &next)) return false;
					udf_copy_parent(&next, dir);
					*out_handle = next;
					return true;
				}

				stduint fid_len = sizeof(UDF_FileIdentifierDescriptor) + fid->length_of_implementation_use + fid->length_of_file_identifier;
				fid_len = udf_align4(fid_len);
				if (fid_len == 0) break;
				off += fid_len;
			}
			processed += sector_used;
		}
		return false;
	}

	bool FilesysUDF::makefs(rostr vol_label, void* moreinfo) {
		(void)vol_label;
		(void)moreinfo;
		return false;
	}

	bool FilesysUDF::create(rostr fullpath, stduint flags, void* exinfo, rostr linkdest) {
		(void)fullpath;
		(void)flags;
		(void)exinfo;
		(void)linkdest;
		return false;
	}

	bool FilesysUDF::remove(rostr pathname) {
		(void)pathname;
		return false;
	}

	stduint FilesysUDF::writfl(void* fil_handler, Slice file_slice, const byte* src) {
		(void)fil_handler;
		(void)file_slice;
		(void)src;
		return 0;
	}

	bool FilesysUDF::loadfs(void* moreinfo) {
		(void)moreinfo;
		if (!storage || !sector_buffer) {
			ploginfo("[UDF] missing storage or sector buffer");
			return false;
		}
		if (storage->Block_Size != 2048) {
			ploginfo("[UDF] unexpected block size %u", storage->Block_Size);
			return false;
		}

		bool found_nsr = false;
		for (uint32 sector = 16; sector < 64; sector++) {
			if (!storage->Read(sector, sector_buffer)) {
				ploginfo("[UDF] read sector %u failed during NSR scan", sector);
				return false;
			}
			if (StrCompareN((const char*)(sector_buffer + 1), "NSR02", 5) == 0 ||
				StrCompareN((const char*)(sector_buffer + 1), "NSR03", 5) == 0) {
				ploginfo("[UDF] found NSR at sector %u", sector);
				found_nsr = true;
				break;
			}
		}
		if (!found_nsr) {
			ploginfo("[UDF] no NSR02/NSR03 found");
			return false;
		}

		uint32 avdp_lba = 256;
		if (!storage->Read(avdp_lba, sector_buffer)) {
			ploginfo("[UDF] read AVDP at %u failed", avdp_lba);
			return false;
		}
		UDF_AnchorVolumeDescriptorPointer* avdp = (UDF_AnchorVolumeDescriptorPointer*)sector_buffer;
		if (avdp->tag.tag_identifier != 2) {
			avdp_lba = storage->getUnits() > 256 ? (storage->getUnits() - 256) : 256;
			ploginfo("[UDF] primary AVDP missing, try backup at %u", avdp_lba);
			if (!storage->Read(avdp_lba, sector_buffer)) {
				ploginfo("[UDF] read backup AVDP at %u failed", avdp_lba);
				return false;
			}
			avdp = (UDF_AnchorVolumeDescriptorPointer*)sector_buffer;
			if (avdp->tag.tag_identifier != 2) {
				ploginfo("[UDF] no valid AVDP found");
				return false;
			}
		}
		ploginfo("[UDF] AVDP at %u mvds=%u len=%u", avdp_lba,
			avdp->main_volume_descriptor_sequence_extent.location,
			udf_ad_length(avdp->main_volume_descriptor_sequence_extent.length));

		uint32 mvds_lba = avdp->main_volume_descriptor_sequence_extent.location;
		uint32 mvds_len = udf_ad_length(avdp->main_volume_descriptor_sequence_extent.length);
		uint32 mvds_count = (mvds_len + logical_block_size - 1) / logical_block_size;

		uint32 partition_start = 0;
		uint16_t partition_number = 0;
		UDF_LongAD fsd_icb = {};
		bool found_partition = false;
		bool found_lvd = false;

		for (uint32 i = 0; i < mvds_count; i++) {
			if (!storage->Read(mvds_lba + i, sector_buffer)) {
				ploginfo("[UDF] read MVDS sector %u failed", mvds_lba + i);
				return false;
			}
			UDF_DescriptorTag* tag = (UDF_DescriptorTag*)sector_buffer;
			if (tag->tag_identifier == 5) {
				UDF_PartitionDescriptor* pd = (UDF_PartitionDescriptor*)sector_buffer;
				partition_number = pd->partition_number;
				partition_start = pd->partition_starting_location;
				found_partition = true;
				ploginfo("[UDF] partition num=%u start=%u", partition_number, partition_start);
			} else if (tag->tag_identifier == 6) {
				UDF_LogicalVolumeDescriptor* lvd = (UDF_LogicalVolumeDescriptor*)sector_buffer;
				logical_block_size = (uint16_t)lvd->logical_block_size;
				if (logical_block_size != storage->Block_Size) {
					ploginfo("[UDF] unexpected logical block size %u", logical_block_size);
					return false;
				}
				fsd_icb = *(UDF_LongAD*)lvd->logical_volume_contents_use;
				found_lvd = true;
				ploginfo("[UDF] lvd fsd part=%u lbn=%u", fsd_icb.location.partition_reference_number, fsd_icb.location.logical_block_number);
			} else if (tag->tag_identifier == 8) {
				break;
			}
		}
		if (!found_partition || !found_lvd) {
			ploginfo("[UDF] missing partition or logical volume descriptor");
			return false;
		}
		if (fsd_icb.location.partition_reference_number != partition_number) {
			ploginfo("[UDF] fsd partition mismatch fsd=%u partition=%u",
				fsd_icb.location.partition_reference_number, partition_number);
			return false;
		}

		uint32 fsd_lba = partition_start + fsd_icb.location.logical_block_number;
		if (!storage->Read(fsd_lba, sector_buffer)) {
			ploginfo("[UDF] read FSD at %u failed", fsd_lba);
			return false;
		}
		UDF_FileSetDescriptor* fsd = (UDF_FileSetDescriptor*)sector_buffer;
		if (fsd->tag.tag_identifier != 256) {
			ploginfo("[UDF] invalid FSD tag %u at %u", fsd->tag.tag_identifier, fsd_lba);
			return false;
		}

		root_handle = UDF_FileHandle{};
		this->partition_ref = partition_number;
		this->partition_start = partition_start;
		root_handle.partition_ref = fsd->root_directory_icb.location.partition_reference_number;
		root_handle.partition_start = partition_start;
		if (root_handle.partition_ref != partition_number) {
			ploginfo("[UDF] root partition mismatch root=%u partition=%u",
				root_handle.partition_ref, partition_number);
			return false;
		}
		if (!udf_load_file_entry(this, partition_start + fsd->root_directory_icb.location.logical_block_number, &root_handle)) {
			ploginfo("[UDF] load root file entry failed at %u",
				partition_start + fsd->root_directory_icb.location.logical_block_number);
			return false;
		}
		fs_loaded = true;
		ploginfo("[UDF] root entry=%u data=%u len=%u", root_handle.entry_lba, root_handle.data_lba, root_handle.length);
		return true;
	}

	void* FilesysUDF::search(rostr fullpath, FilesysSearchArgs* args) {
		if (!fs_loaded || !args || !args->handle_buffer) return nullptr;
		UDF_FileHandle* handle = (UDF_FileHandle*)args->handle_buffer;
		udf_set_root_handle(this, handle);
		if (!fullpath || !*fullpath || !StrCompare(fullpath, "/")) return handle;

		const char* p = fullpath;
		if (*p == '/') p++;
		while (*p) {
			char segment[256];
			stduint seg_len = 0;
			while (*p && *p != '/' && seg_len + 1 < sizeof(segment)) {
				segment[seg_len++] = *p++;
			}
			segment[seg_len] = '\0';
			while (*p == '/') p++;

			if (!seg_len || !StrCompare(segment, ".")) continue;
			if (!StrCompare(segment, "..")) {
				UDF_FileHandle parent = {};
				if (!udf_load_parent_handle(this, *handle, &parent)) return nullptr;
				*handle = parent;
				if (args->on_segment) args->on_segment(handle, segment, 1, handle->length, args->user_data);
				continue;
			}

			UDF_FileHandle next = {};
			if (!udf_find_in_dir(this, *handle, segment, &next)) return nullptr;
			*handle = next;
			if (args->on_segment) {
				if (!args->on_segment(handle, segment, handle->is_dir ? 1 : 0, handle->length, args->user_data)) {
					return handle;
				}
			}
			if (*p && !handle->is_dir) return nullptr;
		}
		return handle;
	}

	bool FilesysUDF::proper(void* handler, stduint cmd, const void* moreinfo) {
		UDF_FileHandle* fh = (UDF_FileHandle*)handler;
		if (!fh || !moreinfo) return false;
		switch ((FilesysCmd)cmd) {
		case FilesysCmd::FS_CMD_GET_SIZE:
			*(stduint*)moreinfo = fh->length;
			return true;
		case FilesysCmd::FS_CMD_GET_ISDIR:
			*(bool*)moreinfo = fh->is_dir != 0;
			return true;
		default:
			return false;
		}
	}

	bool FilesysUDF::enumer(void* dir_handler, _tocall_ft _fn) {
		UDF_FileHandle* dir = (UDF_FileHandle*)dir_handler;
		if (!dir || !dir->is_dir || !_fn) return false;

		uint32 total = dir->length;
		uint32 processed = 0;
		uint32 sector_count = (total + logical_block_size - 1) / logical_block_size;

		for (uint32 sec = 0; sec < sector_count; sec++) {
			if (!udf_load_dir_sector(this, *dir, sec)) return false;

			uint32 sector_used = udf_min_u32(logical_block_size, total - processed);
			uint32 off = 0;
			while (off + sizeof(UDF_FileIdentifierDescriptor) <= sector_used) {
				UDF_FileIdentifierDescriptor* fid = (UDF_FileIdentifierDescriptor*)(sector_buffer + off);
				if (fid->tag.tag_identifier == 0 || fid->tag.tag_identifier == 8) break;
				if (fid->tag.tag_identifier != 257) break;

				stduint name_off = off + sizeof(UDF_FileIdentifierDescriptor) + fid->length_of_implementation_use;
				if (name_off > sector_used || name_off + fid->length_of_file_identifier > sector_used) break;
				if (udf_should_skip_fid(fid)) {
					stduint fid_len = sizeof(UDF_FileIdentifierDescriptor) + fid->length_of_implementation_use + fid->length_of_file_identifier;
					fid_len = udf_align4(fid_len);
					if (fid_len == 0) break;
					off += fid_len;
					continue;
				}

				char name[256];
				udf_decode_fid_name(fid, sector_buffer + name_off, name, sizeof(name));
				if (name[0] && StrCompare(name, ".") && StrCompare(name, "..")) {
					_fn((void*)(stduint)((fid->file_characteristics & 0x02u) ? 1 : 0), (void*)name);
				}

				stduint fid_len = sizeof(UDF_FileIdentifierDescriptor) + fid->length_of_implementation_use + fid->length_of_file_identifier;
				fid_len = udf_align4(fid_len);
				if (fid_len == 0) break;
				off += fid_len;
			}
			processed += sector_used;
		}
		return true;
	}

	stduint FilesysUDF::readfl(void* fil_handler, Slice file_slice, byte* dst) {
		UDF_FileHandle* fh = (UDF_FileHandle*)fil_handler;
		if (!fh || !dst || fh->is_dir) return 0;
		if (file_slice.address >= fh->length) return 0;

		uint32 remaining = fh->length - file_slice.address;
		uint32 need = udf_min_u32((uint32)file_slice.length, remaining);
		uint32 total_read = 0;

		while (total_read < need) {
			uint32 offset = (uint32)file_slice.address + total_read;
			uint32 sector_index = offset / logical_block_size;
			uint32 sector_offset = offset % logical_block_size;
			uint32 can_read = udf_min_u32(logical_block_size - sector_offset, need - total_read);

			if (!udf_load_file_sector(this, *fh, sector_index)) break;
			MemCopyN(dst + total_read, sector_buffer + sector_offset, can_read);
			total_read += can_read;
		}
		return total_read;
	}
}

// ASCII C/C++ TAB4 CRLF
// Docutitle: (Format: File-System) Compact Disc
// Attribute: Arn-Covenant Any-Architect Environment Reference/Dependence

#ifndef _INC_CD_FILESYS
#define _INC_CD_FILESYS

#include "../../../cpp/trait/StorageTrait.hpp"
#include "../../../cpp/trait/FilesysTrait.hpp"

_PACKED(struct) ISO9660_VolumeDescriptorHeader {
	byte type;
	char id[5];
	byte version;
};

_PACKED(struct) ISO9660_DirectoryRecord {
	byte length;
	byte ext_attr_length;
	uint32_t extent_lba_le;
	uint32_t extent_lba_be;
	uint32_t data_length_le;
	uint32_t data_length_be;
	byte recording_time[7];
	byte flags;
	byte file_unit_size;
	byte interleave_gap_size;
	uint16_t volume_sequence_number_le;
	uint16_t volume_sequence_number_be;
	byte file_id_length;
	char file_id[1];
};

_PACKED(struct) UDF_DescriptorTag {
	uint16_t tag_identifier;
	uint16_t descriptor_version;
	byte tag_checksum;
	byte reserved;
	uint16_t tag_serial_number;
	uint16_t descriptor_crc;
	uint16_t descriptor_crc_length;
	uint32_t tag_location;
};

_PACKED(struct) UDF_ExtentAD {
	uint32_t length;
	uint32_t location;
};

_PACKED(struct) UDF_LogicalBlockAddress {
	uint32_t logical_block_number;
	uint16_t partition_reference_number;
};

_PACKED(struct) UDF_LongAD {
	uint32_t length;
	UDF_LogicalBlockAddress location;
	byte implementation_use[6];
};

_PACKED(struct) UDF_AnchorVolumeDescriptorPointer {
	UDF_DescriptorTag tag;
	UDF_ExtentAD main_volume_descriptor_sequence_extent;
	UDF_ExtentAD reserve_volume_descriptor_sequence_extent;
	byte reserved[480];
};

_PACKED(struct) UDF_EntityID {
	byte flags;
	char identifier[23];
	byte identifier_suffix[8];
};

_PACKED(struct) UDF_Charspec {
	byte character_set_type;
	byte character_set_info[63];
};

_PACKED(struct) UDF_LogicalVolumeDescriptor {
	UDF_DescriptorTag tag;
	uint32_t volume_descriptor_sequence_number;
	UDF_Charspec descriptor_character_set;
	byte logical_volume_identifier[128];
	uint32_t logical_block_size;
	UDF_EntityID domain_identifier;
	byte logical_volume_contents_use[16];
	uint32_t map_table_length;
	uint32_t number_of_partition_maps;
	UDF_EntityID implementation_identifier;
	byte implementation_use[128];
	UDF_ExtentAD integrity_sequence_extent;
};

_PACKED(struct) UDF_PartitionDescriptor {
	UDF_DescriptorTag tag;
	uint32_t volume_descriptor_sequence_number;
	uint16_t partition_flags;
	uint16_t partition_number;
	UDF_EntityID partition_contents;
	byte partition_contents_use[128];
	uint32_t access_type;
	uint32_t partition_starting_location;
	uint32_t partition_length;
	UDF_EntityID implementation_identifier;
	byte implementation_use[128];
	byte reserved[156];
};

_PACKED(struct) UDF_FileSetDescriptor {
	UDF_DescriptorTag tag;
	byte recording_date_and_time[12];
	uint16_t interchange_level;
	uint16_t maximum_interchange_level;
	uint32_t character_set_list;
	uint32_t maximum_character_set_list;
	uint32_t file_set_number;
	uint32_t file_set_descriptor_number;
	UDF_Charspec logical_volume_identifier_character_set;
	byte logical_volume_identifier[128];
	UDF_Charspec file_set_character_set;
	byte file_set_identifier[32];
	byte copyright_file_identifier[32];
	byte abstract_file_identifier[32];
	UDF_LongAD root_directory_icb;
	UDF_EntityID domain_identifier;
	UDF_LongAD next_extent;
	UDF_LongAD system_stream_directory_icb;
	byte reserved[32];
};

_PACKED(struct) UDF_ICBTag {
	uint32_t prior_recorded_number_of_direct_entries;
	uint16_t strategy_type;
	uint16_t strategy_parameter;
	uint16_t maximum_number_of_entries;
	byte reserved;
	byte file_type;
	UDF_LogicalBlockAddress parent_icb_location;
	uint16_t flags;
};

_PACKED(struct) UDF_FileEntry {
	UDF_DescriptorTag tag;
	UDF_ICBTag icb_tag;
	uint32_t uid;
	uint32_t gid;
	uint32_t permissions;
	uint16_t file_link_count;
	byte record_format;
	byte record_display_attributes;
	uint32_t record_length;
	uint64_t information_length;
	uint64_t logical_blocks_recorded;
	byte access_time[12];
	byte modification_time[12];
	byte attribute_time[12];
	uint32_t checkpoint;
	UDF_LongAD extended_attribute_icb;
	UDF_EntityID implementation_identifier;
	uint64_t unique_id;
	uint32_t length_of_extended_attributes;
	uint32_t length_of_allocation_descriptors;
};

_PACKED(struct) UDF_ExtendedFileEntry {
	UDF_DescriptorTag tag;
	UDF_ICBTag icb_tag;
	uint32_t uid;
	uint32_t gid;
	uint32_t permissions;
	uint16_t file_link_count;
	byte record_format;
	byte record_display_attributes;
	uint32_t record_length;
	uint64_t information_length;
	uint64_t object_size;
	uint64_t logical_blocks_recorded;
	byte access_time[12];
	byte modification_time[12];
	byte creation_time[12];
	byte attribute_time[12];
	uint32_t checkpoint;
	uint32_t reserved;
	UDF_LongAD extended_attribute_icb;
	UDF_LongAD stream_directory_icb;
	UDF_EntityID implementation_identifier;
	uint64_t unique_id;
	uint32_t length_of_extended_attributes;
	uint32_t length_of_allocation_descriptors;
};

_PACKED(struct) UDF_FileIdentifierDescriptor {
	UDF_DescriptorTag tag;
	uint16_t file_version_number;
	byte file_characteristics;
	byte length_of_file_identifier;
	UDF_LongAD icb;
	uint16_t length_of_implementation_use;
};

_PACKED(struct) UDF_ShortAD {
	uint32_t length;
	uint32_t position;
};

#ifdef _INC_CPP
namespace uni {

	struct ISO9660_FileHandle {
		uint32_t extent_lba = 0;
		uint32_t data_length = 0;
		uint32_t record_lba = 0;
		uint16_t record_offset = 0;
		byte flags = 0;
		byte parent_valid = 0;
		uint32_t parent_extent_lba = 0;
		uint32_t parent_data_length = 0;
		byte parent_flags = 0;
		byte reserved[4] = {};

		bool isDir() const { return (flags & 0x02u) != 0; }
	};

	struct UDF_FileHandle {
		uint32_t entry_lba = 0;
		uint32_t data_lba = 0;
		uint32_t length = 0;
		byte is_dir = 1;
		uint16_t partition_ref = 0;
		uint32_t partition_start = 0;
		uint16_t icb_parent_ref = 0;
		uint32_t icb_parent_lba = 0;
		uint16_t alloc_type = 0;
		byte embedded = 0;
		uint16_t embedded_offset = 0;
		byte parent_valid = 0;
		uint32_t parent_entry_lba = 0;
		uint32_t parent_data_lba = 0;
		uint32_t parent_length = 0;
		uint16_t parent_partition_ref = 0;
		uint32_t parent_partition_start = 0;
		uint16_t parent_alloc_type = 0;
		byte parent_embedded = 0;
		uint16_t parent_embedded_offset = 0;
	};

	class FilesysISO9660 : public FilesysTrait
	{
	public:
		byte* sector_buffer = nullptr;
		uint16_t logical_block_size = 0;
		uint32_t root_extent_lba = 0;
		uint32_t root_data_length = 0;
		bool joliet_enabled = false;
		bool rockridge_enabled = false;
		bool fs_loaded = false;

	public:
		FilesysISO9660(StorageTrait& s, byte* buffer) {
			storage = &s;
			sector_buffer = buffer;
		}

		virtual ~FilesysISO9660() override {
			if (sector_buffer) delete[] sector_buffer;
		}

		virtual bool makefs(rostr vol_label, void* moreinfo = 0) override;
		virtual bool loadfs(void* moreinfo = 0) override;
		virtual bool create(rostr fullpath, stduint flags, void* exinfo, rostr linkdest = 0) override;
		virtual bool remove(rostr pathname) override;
		virtual void* search(rostr fullpath, FilesysSearchArgs* args) override;
		virtual bool proper(void* handler, stduint cmd, const void* moreinfo = 0) override;
		virtual bool enumer(void* dir_handler, _tocall_ft _fn) override;
		virtual stduint readfl(void* fil_handler, Slice file_slice, byte* dst) override;
		virtual stduint writfl(void* fil_handler, Slice file_slice, const byte* src) override;
	};

	class FilesysUDF : public FilesysTrait
	{
	public:
		byte* sector_buffer = nullptr;
		uint16_t logical_block_size = 2048;
		bool fs_loaded = false;
		uint16_t partition_ref = 0;
		uint32_t partition_start = 0;
		UDF_FileHandle root_handle = {};

	public:
		FilesysUDF(StorageTrait& s, byte* buffer) {
			storage = &s;
			sector_buffer = buffer;
		}

		virtual ~FilesysUDF() override {
			if (sector_buffer) delete[] sector_buffer;
		}

		virtual bool makefs(rostr vol_label, void* moreinfo = 0) override;
		virtual bool loadfs(void* moreinfo = 0) override;
		virtual bool create(rostr fullpath, stduint flags, void* exinfo, rostr linkdest = 0) override;
		virtual bool remove(rostr pathname) override;
		virtual void* search(rostr fullpath, FilesysSearchArgs* args) override;
		virtual bool proper(void* handler, stduint cmd, const void* moreinfo = 0) override;
		virtual bool enumer(void* dir_handler, _tocall_ft _fn) override;
		virtual stduint readfl(void* fil_handler, Slice file_slice, byte* dst) override;
		virtual stduint writfl(void* fil_handler, Slice file_slice, const byte* src) override;
	};
}
#endif

#endif

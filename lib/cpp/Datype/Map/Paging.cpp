// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (System) Paging
// Codifiers: @dosconio: 20241209 ~ <Last-check> 
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

#include "../../../../inc/c/system/paging.h"
#include "../../../../inc/c/ustring.h"
#if defined(_ARC_x86) || defined(_ARC_x64)
#include "../../../../inc/c/proctrl/IAx86_64.msr.h"
#endif
// a32 (unchk)

// a64 (unchk)


#if defined(_ARC_x86)
// x86 Lev2: 00000000~FFFFFFFF
//     [v]4KB [v]4MB
// [x86] 4MB
// - set CR4.PSE
// - set PDE.PAT of l1p_index
constexpr stduint PAGE_LEVELS = 2;
constexpr stduint VPN_SHIFTS[] = {12, 22};
constexpr stduint PAGE_ENTRY_COUNTS[] = {1024, 1024};
constexpr bool PAGE_LEVEL_CAN_LEAF[] = {true, true};

#elif defined(_ARC_x64)
// x64 Lev4: 000000000000~FFFFFFFFFFFF
//     [v]4KB [v]2MB [v]1GB ---
// x64 2MB/1GB
// - enable PAE
// - set PDE.PAT of l1p_index(2M) or l2p_index(1G)
constexpr stduint PAGE_LEVELS = 4;
constexpr stduint VPN_SHIFTS[] = {12, 21, 30, 39};
constexpr stduint PAGE_ENTRY_COUNTS[] = {512, 512, 512, 512};
constexpr bool PAGE_LEVEL_CAN_LEAF[] = {true, true, true, false};

#elif defined(_ARC_ARM_32)
constexpr stduint PAGE_LEVELS = 2;
constexpr stduint VPN_SHIFTS[] = {12, 20};
constexpr stduint PAGE_ENTRY_COUNTS[] = {256, 4096};
constexpr bool PAGE_LEVEL_CAN_LEAF[] = {true, true};

#elif defined(_ARC_ARM_64)
constexpr stduint PAGE_LEVELS = 4;
constexpr stduint VPN_SHIFTS[] = {12, 21, 30, 39};
constexpr stduint PAGE_ENTRY_COUNTS[] = {512, 512, 512, 512};
constexpr bool PAGE_LEVEL_CAN_LEAF[] = {true, true, true, false};

#elif defined(_ARC_RISCV_32)
// r32 Lev2: 00000000~FFFFFFFF
//     [v]4KB [v]4MB
// [RISCV]
// - if R = W = X = 0 then current table is the table of table but a page
constexpr stduint PAGE_LEVELS = 2;
constexpr stduint VPN_SHIFTS[] = {12, 22};
constexpr stduint PAGE_ENTRY_COUNTS[] = {1024, 1024};
constexpr bool PAGE_LEVEL_CAN_LEAF[] = {true, true};

#elif defined(_ARC_RISCV_64)
// r64
constexpr stduint PAGE_LEVELS = 3;
constexpr stduint VPN_SHIFTS[] = {12, 21, 30};
constexpr stduint PAGE_ENTRY_COUNTS[] = {512, 512, 512};
constexpr bool PAGE_LEVEL_CAN_LEAF[] = {true, true, true};
#endif

static_assert(sizeof(VPN_SHIFTS) / sizeof(VPN_SHIFTS[0]) == PAGE_LEVELS, "invalid paging shift count");
static_assert(sizeof(PAGE_ENTRY_COUNTS) / sizeof(PAGE_ENTRY_COUNTS[0]) == PAGE_LEVELS, "invalid paging entry count");
static_assert(sizeof(PAGE_LEVEL_CAN_LEAF) / sizeof(PAGE_LEVEL_CAN_LEAF[0]) == PAGE_LEVELS, "invalid paging leaf count");

namespace uni {
	inline stduint getVPN(stduint address, stduint level) {
		return (address >> VPN_SHIFTS[level]) & (PAGE_ENTRY_COUNTS[level] - 1);
	}
}

namespace uni {
	static stduint ComparablePageType(stduint type) {
		#if defined(_ARC_x86) || defined(_ARC_x64)
		if (type == PGTYPE_device) return PGTYPE_uncacheable;
		#endif
		return type;
	}

	static bool PagePropertiesCompatible(const PageEntry& entry, stduint level, stduint requested) {
		stduint existing;
		if (!entry.DecodeLeafProperties(level, existing)) return false;
		const stduint permission_mask = PGPROP_writable | PGPROP_user_access |
			PGPROP_global | PGPROP_nonexecutable;
		if ((requested & permission_mask) != (existing & permission_mask)) return false;
		const stduint requested_type = requested & PGPROP_MASK_TYPE;
		if (requested_type != PGTYPE_default &&
			ComparablePageType(requested_type) !=
			ComparablePageType(existing & PGPROP_MASK_TYPE)) return false;
		return true;
	}

	#if defined(_ARC_ARM_32)

	Paging::~Paging() {
		if (!root_level_page || !uni_default_allocator) return;
		for (stduint i = 0; i < PAGE_ENTRY_COUNTS[1]; i++) {
			PageEntry& pde = root_level_page[i];
			if (pde.isTable(1)) {
				void* pt = (void*)pde.getAddress(1);
				uni_default_allocator->deallocate(pt, 0x1000);
			}
		}
		uni_default_allocator->deallocate(root_level_page, 0x4000);
		root_level_page = nullptr;
	}

	auto Paging::PageMap(stduint laddr, stduint paddr, stduint pgsize, stduint pgporp) -> bool {
		int target_level = -1;
		for (stduint level = 0; level < PAGE_LEVELS; level++) {
			if (PAGE_LEVEL_CAN_LEAF[level] && VPN_SHIFTS[level] == pgsize) {
				target_level = level;
				break;
			}
		}
		if (target_level < 0) return false;
		pageint p = laddr;
		auto pe = (PageEntry*)root_level_page + p.l1p_index;

		if (target_level == 1) {
			if (pgporp & PGPROP_present) {
				if (!pe->SetupAsLeaf(paddr, 1, pgporp)) return false;
			}
			else pe->Clear();
			return true;
		}

		if (!pe->isPresent()) {
			if (!(pgporp & PGPROP_present)) return true;
			void* new_pg = uni_default_allocator ?
				uni_default_allocator->allocate(0x1000, PAGESIZE_4KB) : nullptr;
			if (!new_pg) return false;
			MemSet(new_pg, 0, 0x1000);
			if (!pe->SetupAsTable(new_pg, 1)) {
				uni_default_allocator->deallocate(new_pg, 0x1000);
				return false;
			}
		}
		else if (!pe->isTable(1)) return false;

		pe = (PageEntry*)(pe->getAddress(1)) + p.l0p_index;
		if (pgporp & PGPROP_present) {
			if (!pe->SetupAsLeaf(paddr, 0, pgporp)) return false;
		}
		else pe->Clear();
		return true;
	}

	#else

	static void FreePageTable(PageEntry* table, int level) {
		if (!table) return;
		if (level > 0) {
			stduint max_entries = PAGE_ENTRY_COUNTS[level];
			for (stduint i = 0; i < max_entries; i++) {
				PageEntry& pe = table[i];
				if (pe.isTable(level)) {
					PageEntry* next_table = (PageEntry*)pe.getAddress(level);
					FreePageTable(next_table, level - 1);
				}
			}
		}
		if (uni_default_allocator) {
			uni_default_allocator->deallocate(table, 0x1000);
		}
	}

	Paging::~Paging() {
		if (!root_level_page || !uni_default_allocator) return;
		FreePageTable(root_level_page, PAGE_LEVELS - 1);
		root_level_page = nullptr;
	}

	auto Paging::PageMap(stduint laddr, stduint paddr, stduint pgsize, stduint pgporp) -> bool {
		int target_level = -1;
		for (int i = 0; i < PAGE_LEVELS; i++) {
			if (PAGE_LEVEL_CAN_LEAF[i] && VPN_SHIFTS[i] == pgsize) { target_level = i; break; }
		}
		if (target_level == -1) return false;

		PageEntry* table = root_level_page;
		PageEntry* pe = nullptr;

		for (int level = PAGE_LEVELS - 1; level >= target_level; --level) {
			pe = &table[getVPN(laddr, level)];
			if (level == target_level) break;

			if (!pe->isPresent()) {
				if (!(pgporp & PGPROP_present)) return true;
				void* new_pg = uni_default_allocator ?
					uni_default_allocator->allocate(0x1000, PAGESIZE_4KB) : nullptr;
				if (!new_pg) return false;
				MemSet(new_pg, 0, 0x1000);
				if (!pe->SetupAsTable(new_pg, level)) {
					uni_default_allocator->deallocate(new_pg, 0x1000);
					return false;
				}
			}
			else if (pe->isHuge(level)) {
				const stduint huge_base = pe->getAddress(level);
				const stduint huge_size = _IMM1 << VPN_SHIFTS[level];
				const stduint req_size = _IMM1 << VPN_SHIFTS[target_level];
				const stduint huge_offset = laddr & (huge_size - 1);
				if (huge_offset + req_size > huge_size) return false;
				if (paddr != huge_base + huge_offset) return false;
				return PagePropertiesCompatible(*pe, level, pgporp);
			}
			else if (!pe->isTable(level)) return false;
			table = (PageEntry*)pe->getAddress(level);
		}

		if (pgporp & PGPROP_present) {
			if (!pe->SetupAsLeaf(paddr, target_level, pgporp)) return false;
		}
		else {
			pe->Clear();
		}
		return true;
	}
	#endif

	auto Paging::Walk(pageint& p, stduint target_level, PageEntry*& entry) const -> bool {
		entry = nullptr;
		if (!root_level_page || target_level >= PAGE_LEVELS) return false;

		const stduint address = (stduint)p;
		PageEntry* table = root_level_page;
		for (int level = PAGE_LEVELS - 1; level >= (int)target_level; --level) {
			entry = &table[getVPN(address, level)];
			p.crt_level = level;
			if (!entry->isPresent() || entry->isLeaf(level) || level == target_level)
				return true;
			if (!entry->isTable(level)) return false;
			table = (PageEntry*)entry->getAddress(level);
		}
		return false;
	}

	auto Paging::SplitLeaf(PageEntry* leaf, stduint level, stduint virtual_leaf_base) -> bool {
		if (!leaf || !level || level >= PAGE_LEVELS ||
			!PAGE_LEVEL_CAN_LEAF[level] || !PAGE_LEVEL_CAN_LEAF[level - 1] ||
			!leaf->isLeaf(level) || !uni_default_allocator) return false;

		const uint64 parent_unit = uint64(1) << VPN_SHIFTS[level];
		const uint64 child_unit = uint64(1) << VPN_SHIFTS[level - 1];
		if ((virtual_leaf_base & (parent_unit - 1)) ||
			parent_unit / child_unit != PAGE_ENTRY_COUNTS[level - 1]) return false;

		pageint p = virtual_leaf_base;
		PageEntry* walked_leaf;
		if (!Walk(p, level, walked_leaf) || p.crt_level != level || walked_leaf != leaf)
			return false;

		stduint properties;
		if (!leaf->DecodeLeafProperties(level, properties)) return false;
		const uint64 physical_base = leaf->getAddress(level);
		if (physical_base & (parent_unit - 1)) return false;

		void* new_pg = uni_default_allocator->allocate(0x1000, PAGESIZE_4KB);
		if (!new_pg) return false;
		MemSet(new_pg, 0, 0x1000);
		PageEntry* child_table = (PageEntry*)new_pg;
		for (stduint index = 0; index < PAGE_ENTRY_COUNTS[level - 1]; index++) {
			const uint64 child_physical = physical_base + uint64(index) * child_unit;
			stduint child_properties;
			if (child_physical < physical_base ||
				!child_table[index].SetupAsLeaf(child_physical, level - 1, properties) ||
				uint64(child_table[index].getAddress(level - 1)) != child_physical ||
				!child_table[index].DecodeLeafProperties(level - 1, child_properties) ||
				child_properties != properties) {
				uni_default_allocator->deallocate(new_pg, 0x1000);
				return false;
			}
		}

		PageEntry table_entry;
		if (!table_entry.SetupAsTable(new_pg, level) ||
			uint64(table_entry.getAddress(level)) != uint64(_IMM(new_pg))) {
			uni_default_allocator->deallocate(new_pg, 0x1000);
			return false;
		}
		*leaf = table_entry;
		return true;
	}

	auto Paging::ChooseLargestPageSize(stduint virtual_address, stduint physical_address,
		stduint remaining, PageSizeShift& result) const -> bool {
		for (int level = PAGE_LEVELS - 1; level >= 0; --level) {
			if (!PAGE_LEVEL_CAN_LEAF[level]) continue;
			const stduint shift = VPN_SHIFTS[level];
			if (!isSupportedSize(shift)) continue;

			const stduint unit = _IMM1 << shift;
			const stduint mask = unit - 1;
			if ((virtual_address & mask) || (physical_address & mask) || remaining < unit)
				continue;

			PageEntry encoded;
			if (!encoded.SetupAsLeaf(physical_address, level, PGPROP_present) ||
				encoded.getAddress(level) != physical_address) continue;

			pageint p = virtual_address;
			PageEntry* entry;
			if (!Walk(p, level, entry)) continue;
			if (entry && entry->isPresent()) continue;

			result = (PageSizeShift)shift;
			return true;
		}
		return false;
	}

	auto Paging::refEntry(pageint p) -> PageEntry* {
		const stduint target_level = p.crt_level;
		PageEntry* entry;
		if (!Walk(p, target_level, entry) || p.crt_level != target_level ||
			!entry || !entry->isPresent()) return (PageEntry*)~_IMM0;
		return entry;
	}

	auto Paging::getEntry(pageint& p) const -> PageEntry* {
		PageEntry* entry;
		if (!Walk(p, 0, entry) || !entry || !entry->isLeaf(p.crt_level))
			return (PageEntry*)~_IMM0;
		return entry;
	}

	auto Paging::getEntry(stduint address) const -> PageEntry* {
		pageint p = address;
		return getEntry(p);
	}

	auto Paging::getPageSizeShift(stduint address) const -> stduint {
		pageint p = address;
		PageEntry* entry = getEntry(p);
		if (_IMM(entry) == ~_IMM0) return 0;
		return VPN_SHIFTS[p.crt_level];
	}

	void* Paging::operator[](stduint address) const {
		pageint p = address;
		auto entry = getEntry(p);
		if (_IMM(entry) == ~_IMM0) return (void*)~_IMM0;

		stduint shift = VPN_SHIFTS[p.crt_level];
		stduint mask = (1ULL << shift) - 1;
		return (void*)(entry->getAddress(p.crt_level) + (address & mask));
	}

	void Paging::Reset() {
		stduint root_size = 0x1000;
		stduint root_align = PAGESIZE_4KB;
		#if defined(_ARC_ARM_32)
		root_size = 0x4000;
		root_align = 14;
		#endif

		if (uni_default_allocator) {
			root_level_page = (uni::PageEntry*)uni_default_allocator->allocate(root_size, root_align);
		}
		#if defined(_ARC_x86)
		else if (_physical_allocate) {
			root_level_page = (uni::PageEntry*)_physical_allocate(root_size);
		}
		#endif

		if (root_level_page) MemSet(root_level_page, 0, root_size);
	}

	auto Paging::Map(stduint ln_address, stduint ph_address, stduint length, PageSizeShift pgsize, stduint pgporp) -> bool {
		if (!length) return true;
		stduint normalized;
		if (!NormalizePageProperties(pgporp, normalized) ||
			!PageTypeSupported(normalized & PGPROP_MASK_TYPE)) return false;
		pgporp = normalized;
		if (pgporp & PGPROP_present) {
			PageEntry property_probe;
			if (!property_probe.SetupAsLeaf(0, 0, pgporp) ||
				!PagePropertiesCompatible(property_probe, 0, pgporp)) return false;
		}
		if (pgsize == PAGESIZE_ANY) {
			const stduint requested_linear = ln_address;
			const stduint requested_length = length;
			const stduint page_mask = (_IMM1 << PAGESIZE_4KB) - 1;
			const stduint virtual_offset = ln_address & page_mask;
			if (virtual_offset != (ph_address & page_mask)) return false;
			if (ln_address + length - 1 < ln_address ||
				ph_address + length - 1 < ph_address) return false;

			ln_address -= virtual_offset;
			ph_address -= virtual_offset;
			if (length > ~_IMM0 - virtual_offset) return false;
			length += virtual_offset;
			if (length > ~_IMM0 - page_mask) return false;
			length = (length + page_mask) & ~page_mask;
			if (ln_address + length - 1 < ln_address ||
				ph_address + length - 1 < ph_address) return false;

			bool saw_present = false;
			bool saw_not_present = false;
			bool warned_property_change = false;
			bool warned_mixed_presence = false;
			while (length) {
				pageint p = ln_address;
				PageEntry* entry;
				if (!Walk(p, 0, entry) || !entry) return false;

				if (entry->isPresent()) {
					saw_present = true;
					if (!warned_mixed_presence && saw_present && saw_not_present) {
						plogwarn("Paging::Map range contains both present and non-present pages at VA=%[x] length=%[u]",
							requested_linear, requested_length);
						warned_mixed_presence = true;
					}
					if (!entry->isLeaf(p.crt_level)) return false;
					const stduint unit = _IMM1 << VPN_SHIFTS[p.crt_level];
					const stduint mask = unit - 1;
					const stduint leaf_base = ln_address & ~mask;
					const stduint offset = ln_address & mask;
					const stduint step = minof(unit - offset, length);
					const bool properties_compatible =
						PagePropertiesCompatible(*entry, p.crt_level, pgporp);
					stduint replacement_properties = pgporp;
					if ((pgporp & PGPROP_present) &&
						(pgporp & PGPROP_MASK_TYPE) == PGTYPE_default) {
						stduint existing_properties;
						if (!entry->DecodeLeafProperties(p.crt_level, existing_properties)) return false;
						replacement_properties |= existing_properties & PGPROP_MASK_TYPE;
					}
					const bool mapping_compatible = (pgporp & PGPROP_present) &&
						entry->getAddress(p.crt_level) + offset == ph_address &&
						properties_compatible;

					if ((pgporp & PGPROP_weak) || mapping_compatible) {
						length -= step;
						if (length) {
							ln_address += step;
							ph_address += step;
						}
						continue;
					}

					if ((pgporp & PGPROP_present) && !properties_compatible &&
						!warned_property_change) {
						plogwarn("Paging::Map PAGESIZE_ANY requests changes to existing page properties at VA=%[x] level=%u",
							ln_address, p.crt_level);
						warned_property_change = true;
					}

					bool can_replace = !(pgporp & PGPROP_present);
					if ((pgporp & PGPROP_present) && ln_address == leaf_base && length >= unit) {
						PageEntry replacement;
						can_replace = replacement.SetupAsLeaf(ph_address, p.crt_level, replacement_properties) &&
							replacement.getAddress(p.crt_level) == ph_address &&
							PagePropertiesCompatible(replacement, p.crt_level, replacement_properties);
					}
					if (ln_address == leaf_base && length >= unit && can_replace) {
						if (!PageMap(ln_address, ph_address,
							(PageSizeShift)VPN_SHIFTS[p.crt_level], replacement_properties)) return false;
						length -= unit;
						if (length) {
							ln_address += unit;
							ph_address += unit;
						}
						continue;
					}

					if (!p.crt_level || !SplitLeaf(entry, p.crt_level, leaf_base)) return false;
					continue;
				}

				saw_not_present = true;
				if (!warned_mixed_presence && saw_present && saw_not_present) {
					plogwarn("Paging::Map range contains both present and non-present pages at VA=%[x] length=%[u]",
						requested_linear, requested_length);
					warned_mixed_presence = true;
				}
				if (!(pgporp & PGPROP_present)) {
					const stduint empty_unit = _IMM1 << VPN_SHIFTS[p.crt_level];
					const stduint step = minof(empty_unit - (ln_address & (empty_unit - 1)), length);
					length -= step;
					if (length) {
						ln_address += step;
						ph_address += step;
					}
					continue;
				}

				PageSizeShift selected;
				if (!ChooseLargestPageSize(ln_address, ph_address, length, selected)) return false;
				stduint selected_unit = 0;
				for (stduint level = 0; level < PAGE_LEVELS; level++) {
					if (PAGE_LEVEL_CAN_LEAF[level] && VPN_SHIFTS[level] == selected) {
						selected_unit = _IMM1 << VPN_SHIFTS[level];
						break;
					}
				}
				if (!selected_unit || !PageMap(ln_address, ph_address, selected, pgporp)) return false;
				length -= selected_unit;
				if (length) {
					ln_address += selected_unit;
					ph_address += selected_unit;
				}
			}
			return true;
		}
		int target_level = -1;
		for (stduint level = 0; level < PAGE_LEVELS; level++) {
			if (PAGE_LEVEL_CAN_LEAF[level] && VPN_SHIFTS[level] == pgsize) {
				target_level = level;
				break;
			}
		}
		if (target_level < 0 || !isSupportedSize(pgsize)) return false;
		const stduint unit = _IMM1 << VPN_SHIFTS[target_level];
		stduint alignmask = unit - 1;
		ph_address &= ~alignmask;
		if (ln_address + length - 1 < ln_address) return false;

		length += ln_address & alignmask;
		ln_address &= ~alignmask;
		do {
			bool ok = true;
			if (pgporp & PGPROP_weak) {
				auto entry = getEntry(ln_address);
				if (_IMM(entry) == ~_IMM0 || !entry->isPresent())
					ok = PageMap(ln_address, ph_address, pgsize, pgporp);
			}
			else {
				ok = PageMap(ln_address, ph_address, pgsize, pgporp);
			}
			if (!ok) return false;
			ln_address += unit;
			ph_address += unit;
			length -= minof(unit, length);
		} while (length);

		return true;
	}

	auto Paging::Unmap(stduint ln_address, stduint length) -> bool {
		if (!length) return true;
		while (length > 0) {
			pageint p = ln_address;
			auto pe = getEntry(p);
			if (_IMM(pe) != ~_IMM0) {
				stduint shift = VPN_SHIFTS[p.crt_level];
				stduint unit = _IMM1 << shift;
				stduint step = minof(unit - (ln_address & (unit - 1)), length);
				Map(ln_address, 0, step, (PageSizeShift)shift, 0);
				ln_address += step;
				length -= step;
			}
			else {
				stduint step = minof((_IMM1 << PAGESIZE_4KB) - (ln_address & ((_IMM1 << PAGESIZE_4KB) - 1)), length);
				ln_address += step;
				length -= step;
			}
		}
		return true;
	}


	stduint MemCopyP(void* dest, Paging& pg_d, const void* sors, Paging& pg_s, size_t length)
	{
		stduint ret = 0;

		while (length) {
			stduint phy_d, phy_s;
			stduint unit = length;

			// --- Process Destination ---

			if (pg_d.root_level_page) {
				pageint p = _IMM(dest);
				auto crt = pg_d.getEntry(p);
				if (_IMM(crt) == ~_IMM0) {
					plogerro(" at %s(dest %[x], %[x], sors %[x], %[x], length %[u])",
						__FUNCIDEN__, dest, pg_d.root_level_page, sors, pg_s.root_level_page, length
					);
					plogerro("MemCopyP: dest page is not present. rest %[u]B", length);
					return 0;
				}
				stduint shift = VPN_SHIFTS[p.crt_level];
				stduint mask = (1ULL << shift) - 1;
				stduint offset = _IMM(dest) & mask;
				phy_d = crt->getAddress(p.crt_level) + offset;
				MIN(unit, (1ULL << shift) - offset);
			}
			else {
				// Treat as raw physical address
				phy_d = _IMM(dest);
			}

			// --- Process Source ---
			if (pg_s.root_level_page) {
				pageint p = _IMM(sors);
				auto crt = pg_s.getEntry(p);
				if (_IMM(crt) == ~_IMM0) {
					plogerro("MemCopyP: sors page is not present for %[x] in %[x]", sors, pg_s.root_level_page);
					return 0;
				}
				stduint shift = VPN_SHIFTS[p.crt_level];
				stduint mask = (1ULL << shift) - 1;
				stduint offset = _IMM(sors) & mask;
				phy_s = crt->getAddress(p.crt_level) + offset;
				MIN(unit, (1ULL << shift) - offset);
			}
			else {
				// Treat as raw physical address
				phy_s = _IMM(sors);
			}
			// plogwarn("= %x %x %x", phy_d, phy_s, unit);

			MemCopyN((void*)(phy_d), (void*)(phy_s), unit);
			cast<char*>(dest) += unit;
			cast<char*>(sors) += unit;
			length -= unit;
			ret += unit;
		}
		return ret;
	}

	stduint StrCopyP(char* dest, Paging& pg_d, const char* sors, Paging& pg_s, size_t length)
	{
		stduint ret = 0;

		while (length) {
			stduint phy_d, phy_s;
			stduint unit = length;

			// --- Process Destination ---

			if (pg_d.root_level_page) {
				pageint p = _IMM(dest);
				auto crt = pg_d.getEntry(p);
				if (_IMM(crt) == ~_IMM0) return 0;
				stduint shift = VPN_SHIFTS[p.crt_level];
				stduint mask = (1ULL << shift) - 1;
				stduint offset = _IMM(dest) & mask;
				phy_d = crt->getAddress(p.crt_level) + offset;
				MIN(unit, (1ULL << shift) - offset);
			}
			else {
				phy_d = _IMM(dest);
			}

			// --- Process Source ---
			if (pg_s.root_level_page) {
				pageint p = _IMM(sors);
				auto crt = pg_s.getEntry(p);
				if (_IMM(crt) == ~_IMM0) return 0;
				stduint shift = VPN_SHIFTS[p.crt_level];
				stduint mask = (1ULL << shift) - 1;
				stduint offset = _IMM(sors) & mask;
				phy_s = crt->getAddress(p.crt_level) + offset;
				MIN(unit, (1ULL << shift) - offset);
			}
			else {
				phy_s = _IMM(sors);
			}

			for0(i, unit) {
				char ch = cast<char*>(phy_s)[i];
				if (!ch) {
					cast<char*>(phy_d)[i] = nil; // ASCIZ zero-termination
					return ret += i;
				}
				cast<char*>(phy_d)[i] = ch;
			}
			dest += unit, sors += unit;
			length -= unit;
			ret += unit;
		}

		// --- Handle Trailing Null ---
		if (length) {
			stduint phy_d;
			if (pg_d.root_level_page) {
				pageint p = _IMM(dest);
				auto crt = pg_d.getEntry(p);
				if (_IMM(crt) == ~_IMM0) return ret;
				stduint shift = VPN_SHIFTS[p.crt_level];
				phy_d = crt->getAddress(p.crt_level) + (_IMM(dest) & ((1ULL << shift) - 1));
			}
			else {
				phy_d = _IMM(dest);
			}
			treat<char>(phy_d) = nil;
		}
		return ret;
	}


}

#if defined(_ARC_x86)
#include "../../../../inc/c/proctrl/IAx86_64.h"

bool uni::Paging::isSupportedSize(stduint exponent) {
	unsigned a{}, b{}, c{}, d{};
	switch (exponent) {
	case PAGESIZE_4KB: return true;
	case PAGESIZE_4MB:
		_IO_CPUID(1, 0, &a, &b, &c, &d);
		return (d & (1u << 3)) != 0;
	case PAGESIZE_ANY: return false;
	default: return false;
	}
}

#elif defined(_ARC_x64)
#include "../../../../inc/c/proctrl/IAx86_64.h"

static bool paging_nx_enabled = false;

bool uni::EnablePagingNX() {
	unsigned a{}, b{}, c{}, d{};
	_IO_CPUID(0x80000000, 0, &a, &b, &c, &d);
	if (a < 0x80000001u) return false;
	_IO_CPUID(0x80000001, 0, &a, &b, &c, &d);
	if (!(d & (1u << 20))) return false;

	constexpr uint64 EFER_NXE = uint64(1) << 11;
	setMSR(x86MSR::EFER, getMSR(x86MSR::EFER) | EFER_NXE);
	paging_nx_enabled = !!(getMSR(x86MSR::EFER) & EFER_NXE);
	return paging_nx_enabled;
}

bool uni::PagingNXEnabled() {
	return paging_nx_enabled;
}

bool uni::Paging::isSupportedSize(stduint exponent) {
	unsigned a{}, b{}, c{}, d{};
	switch (exponent) {
	case PAGESIZE_4KB:
	case PAGESIZE_2MB: return true;
	case PAGESIZE_1GB: {
		_IO_CPUID(0x80000000, 0, &a, &b, &c, &d);
		if (a < 0x80000001u) return false;
		_IO_CPUID(0x80000001, 0, &a, &b, &c, &d);
		return (d & (1u << 26)) != 0;
	}
	case PAGESIZE_ANY: return false;
	default: return false;
	}
}

#elif defined(_ARC_ARM_32)
bool uni::Paging::isSupportedSize(stduint exponent) {
	return exponent == PAGESIZE_4KB || exponent == PAGESIZE_1MB;
}

#elif defined(_ARC_ARM_64)
bool uni::Paging::isSupportedSize(stduint exponent) {
	return exponent == PAGESIZE_4KB ||
		exponent == PAGESIZE_2MB || exponent == PAGESIZE_1GB;
}

#elif defined(_ARC_RISCV_32)
bool uni::Paging::isSupportedSize(stduint exponent) {
	return exponent == PAGESIZE_4KB || exponent == PAGESIZE_4MB;
}

#elif defined(_ARC_RISCV_64)
bool uni::Paging::isSupportedSize(stduint exponent) {
	return exponent == PAGESIZE_4KB ||
		exponent == PAGESIZE_2MB || exponent == PAGESIZE_1GB;
}
#endif

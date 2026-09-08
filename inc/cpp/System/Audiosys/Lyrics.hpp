// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Audio - Synchronized Lyrics (LRC) Parser
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM

#ifndef _INCPP_Device_Audio_Lyrics
#define _INCPP_Device_Audio_Lyrics

#include "../../../c/stdinc.h"
#include "../../../c/ustring.h"
#include "../../trait/MallocTrait.hpp"

namespace uni {

	struct LyricLine {
		uint32 time_ms;
		char*  text;
	};

	struct LyricMetadata {
		char   title[64];
		char   artist[64];
		char   album[64];
		char   by[64];
		int32  offset_ms;
	};

	class Lyrics {
	private:
		trait::Malloc* allocator;
		LyricLine*     lines;
		uint32         line_count;
		uint32         line_capacity;
		LyricMetadata  metadata;
		char*          text_pool;
		uint32         text_pool_used;
		uint32         text_pool_cap;

		void Reset() {
			if (allocator) {
				if (lines) {
					allocator->deallocate(lines);
					lines = nullptr;
				}
				if (text_pool) {
					allocator->deallocate(text_pool);
					text_pool = nullptr;
				}
			}
			line_count = 0;
			line_capacity = 0;
			text_pool_used = 0;
			text_pool_cap = 0;
			MemSet(&metadata, 0, sizeof(metadata));
		}

		bool EnsureLineCapacity(uint32 needed) {
			if (needed <= line_capacity) return true;
			if (!allocator) return false;
			uint32 new_cap = line_capacity ? (line_capacity * 2) : 64;
			if (new_cap < needed) new_cap = needed;
			LyricLine* new_lines = (LyricLine*)allocator->allocate(new_cap * sizeof(LyricLine));
			if (!new_lines) return false;
			if (lines && line_count) {
				MemCopyN(new_lines, lines, line_count * sizeof(LyricLine));
				allocator->deallocate(lines);
			}
			lines = new_lines;
			line_capacity = new_cap;
			return true;
		}

		char* StoreString(const char* src, uint32 len) {
			if (!allocator) return nullptr;
			if (!text_pool || text_pool_used + len + 1 > text_pool_cap) {
				uint32 new_cap = text_pool_cap ? (text_pool_cap * 2) : 4096;
				if (new_cap < text_pool_used + len + 1) new_cap = text_pool_used + len + 1 + 2048;
				char* new_pool = (char*)allocator->allocate(new_cap);
				if (!new_pool) return nullptr;
				if (text_pool && text_pool_used) {
					stduint delta = (stduint)new_pool - (stduint)text_pool;
					MemCopyN(new_pool, text_pool, text_pool_used);
					for (uint32 i = 0; i < line_count; ++i) {
						if (lines[i].text) {
							lines[i].text = (char*)((stduint)lines[i].text + delta);
						}
					}
					allocator->deallocate(text_pool);
				}
				text_pool = new_pool;
				text_pool_cap = new_cap;
			}
			char* dest = text_pool + text_pool_used;
			MemCopyN(dest, src, len);
			dest[len] = '\0';
			text_pool_used += len + 1;
			return dest;
		}

		static bool IsDigit(char c) { return c >= '0' && c <= '9'; }

		static bool ParseTimeTag(const char* tag, uint32 tag_len, uint32& out_ms) {
			if (tag_len < 5 || tag[0] != '[') return false;
			const char* p = tag + 1;
			uint32 minutes = 0;
			while (*p && *p != ':' && *p != ']') {
				if (!IsDigit(*p)) return false;
				minutes = minutes * 10 + (*p - '0');
				++p;
			}
			if (*p != ':') return false;
			++p;

			uint32 seconds = 0;
			while (*p && *p != '.' && *p != ']') {
				if (!IsDigit(*p)) return false;
				seconds = seconds * 10 + (*p - '0');
				++p;
			}

			uint32 fraction_ms = 0;
			if (*p == '.') {
				++p;
				uint32 frac_val = 0;
				uint32 frac_digits = 0;
				while (*p && *p != ']') {
					if (!IsDigit(*p)) return false;
					frac_val = frac_val * 10 + (*p - '0');
					++frac_digits;
					++p;
				}
				if (frac_digits == 1) fraction_ms = frac_val * 100;
				else if (frac_digits == 2) fraction_ms = frac_val * 10;
				else if (frac_digits == 3) fraction_ms = frac_val;
				else {
					while (frac_digits > 3) {
						frac_val /= 10;
						--frac_digits;
					}
					fraction_ms = frac_val;
				}
			}

			if (*p != ']') return false;
			out_ms = minutes * 60000 + seconds * 1000 + fraction_ms;
			return true;
		}

		static void CopyTrimmed(char* dest, uint32 dest_max, const char* src, uint32 src_len) {
			while (src_len && (*src == ' ' || *src == '\t')) {
				++src;
				--src_len;
			}
			while (src_len && (src[src_len - 1] == ' ' || src[src_len - 1] == '\t' || src[src_len - 1] == '\r' || src[src_len - 1] == '\n')) {
				--src_len;
			}
			uint32 take = src_len < dest_max - 1 ? src_len : dest_max - 1;
			MemCopyN(dest, src, take);
			dest[take] = '\0';
		}

	public:
		Lyrics() : allocator(nullptr), lines(nullptr), line_count(0), line_capacity(0),
				   text_pool(nullptr), text_pool_used(0), text_pool_cap(0) {
			MemSet(&metadata, 0, sizeof(metadata));
		}

		~Lyrics() {
			Reset();
		}

		void Release() {
			Reset();
		}

		bool Parse(const char* lrc_data, uint32 lrc_len, trait::Malloc& alloc) {
			Reset();
			allocator = &alloc;
			if (!lrc_data || !lrc_len) return false;

			const char* ptr = lrc_data;
			const char* end = lrc_data + lrc_len;

			while (ptr < end) {
				const char* line_start = ptr;
				while (ptr < end && *ptr != '\n' && *ptr != '\r') ++ptr;
				const char* line_end = ptr;
				while (ptr < end && (*ptr == '\n' || *ptr == '\r')) ++ptr;

				while (line_start < line_end && (*line_start == ' ' || *line_start == '\t')) ++line_start;
				if (line_start >= line_end) continue;

				if (*line_start != '[') continue;

				if (StrCompareN(line_start, "[ti:", 4) == 0) {
					const char* val = line_start + 4;
					const char* val_end = line_end;
					while (val_end > val && *(val_end - 1) != ']') --val_end;
					if (val_end > val && *(val_end - 1) == ']') --val_end;
					CopyTrimmed(metadata.title, sizeof(metadata.title), val, (uint32)(val_end - val));
					continue;
				} else if (StrCompareN(line_start, "[ar:", 4) == 0) {
					const char* val = line_start + 4;
					const char* val_end = line_end;
					while (val_end > val && *(val_end - 1) != ']') --val_end;
					if (val_end > val && *(val_end - 1) == ']') --val_end;
					CopyTrimmed(metadata.artist, sizeof(metadata.artist), val, (uint32)(val_end - val));
					continue;
				} else if (StrCompareN(line_start, "[al:", 4) == 0) {
					const char* val = line_start + 4;
					const char* val_end = line_end;
					while (val_end > val && *(val_end - 1) != ']') --val_end;
					if (val_end > val && *(val_end - 1) == ']') --val_end;
					CopyTrimmed(metadata.album, sizeof(metadata.album), val, (uint32)(val_end - val));
					continue;
				} else if (StrCompareN(line_start, "[by:", 4) == 0) {
					const char* val = line_start + 4;
					const char* val_end = line_end;
					while (val_end > val && *(val_end - 1) != ']') --val_end;
					if (val_end > val && *(val_end - 1) == ']') --val_end;
					CopyTrimmed(metadata.by, sizeof(metadata.by), val, (uint32)(val_end - val));
					continue;
				} else if (StrCompareN(line_start, "[offset:", 8) == 0) {
					const char* val = line_start + 8;
					int32 sign = 1;
					if (*val == '-') { sign = -1; ++val; }
					else if (*val == '+') { ++val; }
					int32 off_val = 0;
					while (val < line_end && IsDigit(*val)) {
						off_val = off_val * 10 + (*val - '0');
						++val;
					}
					metadata.offset_ms = off_val * sign;
					continue;
				}

				uint32 time_tags[16];
				uint32 time_tag_count = 0;
				const char* cur = line_start;

				while (cur < line_end && *cur == '[' && time_tag_count < 16) {
					const char* tag_close = cur;
					while (tag_close < line_end && *tag_close != ']') ++tag_close;
					if (tag_close >= line_end) break;
					uint32 tag_len = (uint32)(tag_close - cur + 1);
					uint32 parsed_ms = 0;
					if (ParseTimeTag(cur, tag_len, parsed_ms)) {
						time_tags[time_tag_count++] = parsed_ms;
						cur = tag_close + 1;
					} else {
						break;
					}
				}

				if (time_tag_count == 0) continue;

				const char* text_start = cur;
				while (text_start < line_end && (*text_start == ' ' || *text_start == '\t')) ++text_start;
				const char* text_end = line_end;
				while (text_end > text_start && (*(text_end - 1) == ' ' || *(text_end - 1) == '\t')) --text_end;
				uint32 text_len = (uint32)(text_end - text_start);

				char* stored_text = StoreString(text_start, text_len);
				if (!stored_text) break;

				if (!EnsureLineCapacity(line_count + time_tag_count)) break;

				for (uint32 i = 0; i < time_tag_count; ++i) {
					lines[line_count].time_ms = time_tags[i];
					lines[line_count].text = stored_text;
					++line_count;
				}
			}

			if (line_count > 0) {
				if (metadata.offset_ms != 0) {
					for (uint32 i = 0; i < line_count; ++i) {
						int32 t = (int32)lines[i].time_ms + metadata.offset_ms;
						lines[i].time_ms = (t > 0) ? (uint32)t : 0;
					}
				}

				for (uint32 i = 1; i < line_count; ++i) {
					LyricLine key = lines[i];
					int32 j = (int32)i - 1;
					while (j >= 0 && lines[j].time_ms > key.time_ms) {
						lines[j + 1] = lines[j];
						--j;
					}
					lines[j + 1] = key;
				}
			}

			return line_count > 0;
		}

		uint32 GetLineCount() const { return line_count; }

		const LyricLine* GetLine(uint32 index) const {
			if (index >= line_count) return nullptr;
			return &lines[index];
		}

		const LyricMetadata& GetMetadata() const { return metadata; }

		int32 FindLineIndex(uint32 current_ms) const {
			if (line_count == 0 || current_ms < lines[0].time_ms) return -1;
			int32 low = 0;
			int32 high = (int32)line_count - 1;
			int32 result = -1;

			while (low <= high) {
				int32 mid = low + (high - low) / 2;
				if (lines[mid].time_ms <= current_ms) {
					result = mid;
					low = mid + 1;
				} else {
					high = mid - 1;
				}
			}
			return result;
		}
	};

}

#endif
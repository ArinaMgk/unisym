// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [Format.ConfigText] INI
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

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>

#include "../../../../inc/cpp/Format/ConfigText/INI.hpp"
#include "../../../../inc/c/ustring.h"

namespace uni {

	struct ININode {
		bool is_section;
		String str_val;
		stdsint num_val;

		struct KeyValuePair {
			String key;
			ININode* value;
			KeyValuePair* next;
		};
		KeyValuePair* head;

		ININode(bool section = true)
			: is_section(section), num_val(0), head(nullptr) {}

		~ININode() {
			clear();
		}

		void clear() {
			KeyValuePair* curr = head;
			while (curr) {
				KeyValuePair* next = curr->next;
				delete curr->value;
				delete curr;
				curr = next;
			}
			head = nullptr;
		}

		ININode* findChild(const char* key) const {
			if (!is_section) return nullptr;
			KeyValuePair* curr = head;
			while (curr) {
				if (StrCompare(curr->key.reference(), key) == 0) {
					return curr->value;
				}
				curr = curr->next;
			}
			return nullptr;
		}

		ININode* getOrCreateSection(const char* key) {
			if (!is_section) {
				clear();
				is_section = true;
			}
			ININode* existing = findChild(key);
			if (existing) {
				if (!existing->is_section) {
					existing->clear();
					existing->is_section = true;
				}
				return existing;
			}

			ININode* new_section = new ININode(true);
			KeyValuePair* pair = new KeyValuePair;
			pair->key = key;
			pair->value = new_section;
			pair->next = nullptr;

			if (!head) {
				head = pair;
			} else {
				KeyValuePair* tail = head;
				while (tail->next) {
					tail = tail->next;
				}
				tail->next = pair;
			}
			return new_section;
		}

		void setValue(const char* key, ININode* child) {
			KeyValuePair* curr = head;
			while (curr) {
				if (StrCompare(curr->key.reference(), key) == 0) {
					delete curr->value;
					curr->value = child;
					return;
				}
				curr = curr->next;
			}

			KeyValuePair* pair = new KeyValuePair;
			pair->key = key;
			pair->value = child;
			pair->next = nullptr;

			if (!head) {
				head = pair;
			} else {
				KeyValuePair* tail = head;
				while (tail->next) {
					tail = tail->next;
				}
				tail->next = pair;
			}
		}
	};

	static void skip_ini_inline_spaces(const char*& p) {
		while (*p && (*p == ' ' || *p == '\t')) {
			p++;
		}
	}

	static ININode* navigate_ini_section(ININode* root, const char* sec_name) {
		if (!root || !sec_name) return root;
		const char* p = sec_name;
		while (*p == '/' || *p == '.') p++;
		if (!*p) return root;

		ININode* curr = root;
		char segment[128];
		while (*p) {
			stduint idx = 0;
			while (*p && *p != '/' && *p != '.') {
				if (idx < sizeof(segment) - 1) {
					segment[idx++] = *p;
				}
				p++;
			}
			segment[idx] = '\0';
			while (*p == '/' || *p == '.') p++;

			curr = curr->getOrCreateSection(segment);
		}
		return curr;
	}

	void ConfigINI::parse_ini(rostr text) {
		if (!text) {
			is_valid = false;
			return;
		}

		ININode* root = new ININode(true);
		ININode* current_section = root;
		const char* p = text;
		bool error_occurred = false;

		while (*p) {
			// Skip whitespace
			while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) {
				p++;
			}
			if (!*p) break;

			// Comment line (; or #)
			if (*p == ';' || *p == '#') {
				while (*p && *p != '\n') p++;
				continue;
			}

			// Section header [section] or [section.subsection]
			if (*p == '[') {
				p++; // skip '['
				skip_ini_inline_spaces(p);
				char sec_buf[256];
				stduint len = 0;
				while (*p && *p != ']' && *p != '\r' && *p != '\n') {
					if (len < sizeof(sec_buf) - 1) {
						sec_buf[len++] = *p;
					}
					p++;
				}
				if (*p != ']') {
					error_occurred = true;
					break;
				}
				p++; // skip ']'
				sec_buf[len] = '\0';

				// Trim trailing spaces in sec_buf
				while (len > 0 && (sec_buf[len - 1] == ' ' || sec_buf[len - 1] == '\t')) {
					sec_buf[--len] = '\0';
				}

				current_section = navigate_ini_section(root, sec_buf);
				continue;
			}

			// Key-value pair: key = value or key: value
			char key_buf[256];
			stduint klen = 0;
			while (*p && *p != '=' && *p != ':' && *p != '\r' && *p != '\n') {
				if (klen < sizeof(key_buf) - 1) {
					key_buf[klen++] = *p;
				}
				p++;
			}
			if (*p != '=' && *p != ':') {
				error_occurred = true;
				break;
			}
			p++; // skip '=' or ':'
			key_buf[klen] = '\0';

			// Trim spaces around key
			while (klen > 0 && (key_buf[klen - 1] == ' ' || key_buf[klen - 1] == '\t')) {
				key_buf[--klen] = '\0';
			}

			skip_ini_inline_spaces(p);

			// Extract value
			char val_buf[1024];
			stduint vlen = 0;
			bool in_quote = false;
			char quote_char = 0;
			if (*p == '"' || *p == '\'') {
				in_quote = true;
				quote_char = *p++;
			}

			if (in_quote) {
				while (*p && *p != quote_char && *p != '\r' && *p != '\n') {
					if (vlen < sizeof(val_buf) - 1) {
						val_buf[vlen++] = *p;
					}
					p++;
				}
				if (*p == quote_char) p++;
			} else {
				while (*p && *p != '\r' && *p != '\n' && *p != ';' && *p != '#') {
					if (vlen < sizeof(val_buf) - 1) {
						val_buf[vlen++] = *p;
					}
					p++;
				}
				// Trim trailing spaces in unquoted value
				while (vlen > 0 && (val_buf[vlen - 1] == ' ' || val_buf[vlen - 1] == '\t')) {
					val_buf[--vlen] = '\0';
				}
			}
			val_buf[vlen] = '\0';

			ININode* val_node = new ININode(false);
			val_node->str_val = val_buf;
			val_node->num_val = (stdsint)atoins(val_buf);
			current_section->setValue(key_buf, val_node);

			// Consume until newline
			while (*p && *p != '\n') p++;
			if (*p == '\n') p++;
		}

		if (error_occurred) {
			delete root;
			root_node = new ININode(true);
			is_valid = false;
		} else {
			root_node = root;
			is_valid = true;
		}
	}

	ConfigINI::ConfigINI(HostFile& file) {
		root_node = nullptr;
		is_valid = false;

		String text_buf;
		if (file.fptr) {
			FILE* fp = (FILE*)file.fptr;
			fseek(fp, 0, SEEK_END);
			long sz = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			if (sz > 0) {
				char* raw = (char*)malloc(sz + 1);
				if (raw) {
					size_t read_bytes = fread(raw, 1, sz, fp);
					raw[read_bytes] = '\0';
					text_buf = raw;
					free(raw);
				}
			}
		}

		parse_ini(text_buf.reference());
	}

	ConfigINI::~ConfigINI() {
		if (root_node) {
			delete (ININode*)root_node;
			root_node = nullptr;
		}
	}

	bool ConfigINI::Probe() {
		return is_valid;
	}

	static ININode* traverse_ini_path(ININode* root, rostr path) {
		if (!root || !path) return nullptr;
		const char* p = path;
		while (*p == '/' || *p == '.') p++;
		if (!*p) return root;

		ININode* curr = root;
		char segment[128];

		while (*p) {
			stduint idx = 0;
			while (*p && *p != '/' && *p != '.') {
				if (idx < sizeof(segment) - 1) {
					segment[idx++] = *p;
				}
				p++;
			}
			segment[idx] = '\0';
			while (*p == '/' || *p == '.') p++;

			if (curr->is_section) {
				curr = curr->findChild(segment);
				if (!curr) return nullptr;
			} else {
				return nullptr;
			}
		}

		return curr;
	}

	String ConfigINI::get(rostr name) const {
		if (!root_node || !name) return String("");
		ININode* node = traverse_ini_path((ININode*)root_node, name);
		if (!node) return String("");
		return node->str_val;
	}

	stdsint ConfigINI::getNumber(rostr name) const {
		if (!root_node || !name) return 0;
		ININode* node = traverse_ini_path((ININode*)root_node, name);
		if (!node) return 0;
		return node->num_val;
	}

	bool ConfigINI::set(rostr name, rostr val) {
		if (!name || !val) return false;
		if (!root_node) {
			root_node = new ININode(true);
		}

		const char* p = name;
		while (*p == '/' || *p == '.') p++;
		if (!*p) return false;

		ININode* curr = (ININode*)root_node;
		char segment[128];

		while (*p) {
			stduint idx = 0;
			while (*p && *p != '/' && *p != '.') {
				if (idx < sizeof(segment) - 1) {
					segment[idx++] = *p;
				}
				p++;
			}
			segment[idx] = '\0';
			while (*p == '/' || *p == '.') p++;

			if (*p == '\0') {
				// Leaf
				if (!curr->is_section) {
					curr->clear();
					curr->is_section = true;
				}
				ININode* leaf = curr->findChild(segment);
				if (!leaf) {
					leaf = new ININode(false);
					curr->setValue(segment, leaf);
				} else {
					leaf->clear();
					leaf->is_section = false;
				}
				leaf->str_val = val;
				leaf->num_val = (stdsint)atoins(val);
				return true;
			} else {
				// Intermediate
				curr = curr->getOrCreateSection(segment);
			}
		}

		return false;
	}

}

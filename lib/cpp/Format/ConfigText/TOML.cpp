// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [Format.ConfigText] TOML
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

#include "../../../../inc/cpp/Format/ConfigText/TOML.hpp"
#include "../../../../inc/c/ustring.h"

namespace uni {

	enum class TOMLType {
		Table,
		String,
		Number,
		Boolean
	};

	struct TOMLNode {
		TOMLType type;
		String str_val;
		stdsint num_val;
		bool bool_val;

		struct KeyValuePair {
			String key;
			TOMLNode* value;
			KeyValuePair* next;
		};
		KeyValuePair* head;

		TOMLNode(TOMLType t = TOMLType::Table)
			: type(t), num_val(0), bool_val(false), head(nullptr) {}

		~TOMLNode() {
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

		TOMLNode* findChild(const char* key) const {
			if (type != TOMLType::Table) return nullptr;
			KeyValuePair* curr = head;
			while (curr) {
				if (StrCompare(curr->key.reference(), key) == 0) {
					return curr->value;
				}
				curr = curr->next;
			}
			return nullptr;
		}

		TOMLNode* getOrCreateTable(const char* key) {
			if (type != TOMLType::Table) {
				clear();
				type = TOMLType::Table;
			}
			TOMLNode* existing = findChild(key);
			if (existing) {
				if (existing->type != TOMLType::Table) {
					existing->clear();
					existing->type = TOMLType::Table;
				}
				return existing;
			}

			TOMLNode* new_table = new TOMLNode(TOMLType::Table);
			KeyValuePair* pair = new KeyValuePair;
			pair->key = key;
			pair->value = new_table;
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
			return new_table;
		}

		void setValue(const char* key, TOMLNode* child) {
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

	static void skip_toml_inline_spaces(const char*& p) {
		while (*p && (*p == ' ' || *p == '\t')) {
			p++;
		}
	}

	static bool parse_toml_key(const char*& p, String& out_key) {
		skip_toml_inline_spaces(p);
		if (!*p) return false;

		if (*p == '"' || *p == '\'') {
			char quote = *p++;
			char buf[256];
			stduint len = 0;
			while (*p && *p != quote) {
				if (*p == '\\' && quote == '"') {
					p++;
					if (!*p) return false;
				}
				if (len < sizeof(buf) - 1) {
					buf[len++] = *p;
				}
				p++;
			}
			if (*p != quote) return false;
			p++; // skip trailing quote
			buf[len] = '\0';
			out_key = buf;
			return true;
		}

		char buf[256];
		stduint len = 0;
		while (*p && (*p > ' ') && *p != '=' && *p != ']' && *p != '.' && *p != '/' && *p != '#') {
			if (len < sizeof(buf) - 1) {
				buf[len++] = *p;
			}
			p++;
		}
		if (len == 0) return false;
		buf[len] = '\0';
		out_key = buf;
		return true;
	}

	static TOMLNode* navigate_toml_table(TOMLNode* root, const char* path) {
		if (!root || !path) return root;
		const char* p = path;
		while (*p == '.' || *p == '/') p++;
		if (!*p) return root;

		TOMLNode* curr = root;
		while (*p) {
			String seg;
			if (!parse_toml_key(p, seg)) break;
			curr = curr->getOrCreateTable(seg.reference());
			skip_toml_inline_spaces(p);
			if (*p == '.' || *p == '/') p++;
		}
		return curr;
	}

	static TOMLNode* parse_toml_value(const char*& p) {
		skip_toml_inline_spaces(p);
		if (!*p) return nullptr;

		if (*p == '"' || *p == '\'') {
			char quote = *p++;
			String str;
			char buf[1024];
			stduint len = 0;
			while (*p && *p != quote) {
				if (*p == '\\' && quote == '"') {
					p++;
					if (!*p) return nullptr;
					char ch = *p;
					if (ch == '"') buf[len++] = '"';
					else if (ch == '\\') buf[len++] = '\\';
					else if (ch == 'n') buf[len++] = '\n';
					else if (ch == 't') buf[len++] = '\t';
					else if (ch == 'r') buf[len++] = '\r';
					else buf[len++] = ch;
				} else {
					buf[len++] = *p;
				}
				if (len >= sizeof(buf) - 1) {
					buf[len] = '\0';
					str += buf;
					len = 0;
				}
				p++;
			}
			if (*p != quote) return nullptr;
			p++; // skip quote
			buf[len] = '\0';
			str += buf;

			TOMLNode* node = new TOMLNode(TOMLType::String);
			node->str_val = str;
			return node;
		}

		if (StrCompareN(p, "true", 4) == 0) {
			p += 4;
			TOMLNode* node = new TOMLNode(TOMLType::Boolean);
			node->bool_val = true;
			node->str_val = "true";
			return node;
		}
		if (StrCompareN(p, "false", 5) == 0) {
			p += 5;
			TOMLNode* node = new TOMLNode(TOMLType::Boolean);
			node->bool_val = false;
			node->str_val = "false";
			return node;
		}

		if (*p == '+' || *p == '-' || (*p >= '0' && *p <= '9')) {
			const char* start = p;
			if (*p == '+' || *p == '-') p++;
			while (*p >= '0' && *p <= '9') p++;

			char num_buf[64];
			stduint len = p - start;
			if (len >= sizeof(num_buf)) len = sizeof(num_buf) - 1;
			MemCopyN(num_buf, start, len);
			num_buf[len] = '\0';

			TOMLNode* node = new TOMLNode(TOMLType::Number);
			node->num_val = (stdsint)atoins(num_buf);
			node->str_val = num_buf;
			return node;
		}

		return nullptr;
	}

	void ConfigTOML::parse_toml(rostr text) {
		if (!text) {
			is_valid = false;
			return;
		}

		TOMLNode* root = new TOMLNode(TOMLType::Table);
		TOMLNode* current_table = root;
		const char* p = text;
		bool error_occurred = false;

		while (*p) {
			// Skip whitespace and empty lines
			while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) {
				p++;
			}
			if (!*p) break;

			// Comment line
			if (*p == '#') {
				while (*p && *p != '\n') p++;
				continue;
			}

			// Table header [section] or [section.subsection]
			if (*p == '[') {
				p++; // skip '['
				skip_toml_inline_spaces(p);
				char header_buf[512];
				stduint hlen = 0;
				while (*p && *p != ']' && *p != '\n' && *p != '\r') {
					if (hlen < sizeof(header_buf) - 1) {
						header_buf[hlen++] = *p;
					}
					p++;
				}
				if (*p != ']') {
					error_occurred = true;
					break;
				}
				p++; // skip ']'
				header_buf[hlen] = '\0';
				current_table = navigate_toml_table(root, header_buf);
				continue;
			}

			// Key-value pair: key = value
			String key;
			if (!parse_toml_key(p, key)) {
				error_occurred = true;
				break;
			}

			skip_toml_inline_spaces(p);
			if (*p != '=') {
				error_occurred = true;
				break;
			}
			p++; // skip '='

			TOMLNode* val_node = parse_toml_value(p);
			if (!val_node) {
				error_occurred = true;
				break;
			}

			current_table->setValue(key.reference(), val_node);

			// Consume rest of line / comments
			skip_toml_inline_spaces(p);
			if (*p == '#') {
				while (*p && *p != '\n') p++;
			}
			if (*p == '\r') p++;
			if (*p == '\n') p++;
		}

		if (error_occurred) {
			delete root;
			root_node = new TOMLNode(TOMLType::Table);
			is_valid = false;
		} else {
			root_node = root;
			is_valid = true;
		}
	}

	ConfigTOML::ConfigTOML(HostFile& file) {
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

		parse_toml(text_buf.reference());
	}

	ConfigTOML::~ConfigTOML() {
		if (root_node) {
			delete (TOMLNode*)root_node;
			root_node = nullptr;
		}
	}

	bool ConfigTOML::Probe() {
		return is_valid;
	}

	static TOMLNode* traverse_toml_path(TOMLNode* root, rostr path) {
		if (!root || !path) return nullptr;
		const char* p = path;
		while (*p == '/' || *p == '.') p++;
		if (!*p) return root;

		TOMLNode* curr = root;
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

			if (curr->type == TOMLType::Table) {
				curr = curr->findChild(segment);
				if (!curr) return nullptr;
			} else {
				return nullptr;
			}
		}

		return curr;
	}

	String ConfigTOML::get(rostr name) const {
		if (!root_node || !name) return String("");
		TOMLNode* node = traverse_toml_path((TOMLNode*)root_node, name);
		if (!node) return String("");

		if (node->type == TOMLType::String || node->type == TOMLType::Number || node->type == TOMLType::Boolean) {
			return node->str_val;
		}
		return String("");
	}

	stdsint ConfigTOML::getNumber(rostr name) const {
		if (!root_node || !name) return 0;
		TOMLNode* node = traverse_toml_path((TOMLNode*)root_node, name);
		if (!node) return 0;

		if (node->type == TOMLType::Number) {
			return node->num_val;
		} else if (node->type == TOMLType::String) {
			return (stdsint)atoins(node->str_val.reference());
		} else if (node->type == TOMLType::Boolean) {
			return node->bool_val ? 1 : 0;
		}
		return 0;
	}

	bool ConfigTOML::set(rostr name, rostr val) {
		if (!name || !val) return false;
		if (!root_node) {
			root_node = new TOMLNode(TOMLType::Table);
		}

		const char* p = name;
		while (*p == '/' || *p == '.') p++;
		if (!*p) return false;

		TOMLNode* curr = (TOMLNode*)root_node;
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
				// Leaf node
				if (curr->type != TOMLType::Table) {
					curr->clear();
					curr->type = TOMLType::Table;
				}
				TOMLNode* leaf = curr->findChild(segment);
				if (!leaf) {
					leaf = new TOMLNode(TOMLType::String);
					curr->setValue(segment, leaf);
				} else {
					leaf->clear();
					leaf->type = TOMLType::String;
				}
				leaf->str_val = val;
				leaf->num_val = (stdsint)atoins(val);
				return true;
			} else {
				// Intermediate node
				curr = curr->getOrCreateTable(segment);
			}
		}

		return false;
	}

}

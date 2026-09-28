// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [Format.ConfigText] JSON
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

#include "../../../../inc/cpp/Format/ConfigText/JSON.hpp"
#include "../../../../inc/c/ustring.h"

namespace uni {

	enum class JSONType {
		Null,
		Boolean,
		Number,
		String,
		Array,
		Object
	};

	struct JSONNode {
		JSONType type;
		String str_val;
		stdsint num_val;
		bool bool_val;

		struct KeyValuePair {
			String key;
			JSONNode* value;
			KeyValuePair* next;
		};
		KeyValuePair* object_head;

		struct ArrayElement {
			JSONNode* value;
			ArrayElement* next;
		};
		ArrayElement* array_head;

		JSONNode(JSONType t = JSONType::Null)
			: type(t), num_val(0), bool_val(false), object_head(nullptr), array_head(nullptr) {}

		~JSONNode() {
			clear();
		}

		void clear() {
			KeyValuePair* kv = object_head;
			while (kv) {
				KeyValuePair* next_kv = kv->next;
				delete kv->value;
				delete kv;
				kv = next_kv;
			}
			object_head = nullptr;

			ArrayElement* elem = array_head;
			while (elem) {
				ArrayElement* next_elem = elem->next;
				delete elem->value;
				delete elem;
				elem = next_elem;
			}
			array_head = nullptr;
		}

		JSONNode* findChild(const char* key) const {
			if (type != JSONType::Object) return nullptr;
			KeyValuePair* curr = object_head;
			while (curr) {
				if (StrCompare(curr->key.reference(), key) == 0) {
					return curr->value;
				}
				curr = curr->next;
			}
			return nullptr;
		}

		JSONNode* getOrCreateChild(const char* key) {
			if (type != JSONType::Object) {
				clear();
				type = JSONType::Object;
			}
			JSONNode* existing = findChild(key);
			if (existing) return existing;

			JSONNode* new_node = new JSONNode(JSONType::Object);
			KeyValuePair* pair = new KeyValuePair;
			pair->key = key;
			pair->value = new_node;
			pair->next = nullptr;

			if (!object_head) {
				object_head = pair;
			} else {
				KeyValuePair* tail = object_head;
				while (tail->next) {
					tail = tail->next;
				}
				tail->next = pair;
			}
			return new_node;
		}

		void addObjectMember(const char* key, JSONNode* child) {
			KeyValuePair* pair = new KeyValuePair;
			pair->key = key;
			pair->value = child;
			pair->next = nullptr;

			if (!object_head) {
				object_head = pair;
			} else {
				KeyValuePair* tail = object_head;
				while (tail->next) {
					tail = tail->next;
				}
				tail->next = pair;
			}
		}

		void addArrayElement(JSONNode* child) {
			ArrayElement* elem = new ArrayElement;
			elem->value = child;
			elem->next = nullptr;

			if (!array_head) {
				array_head = elem;
			} else {
				ArrayElement* tail = array_head;
				while (tail->next) {
					tail = tail->next;
				}
				tail->next = elem;
			}
		}
	};

	static void skip_json_whitespace(const char*& p) {
		while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) {
			p++;
		}
	}

	static bool parse_json_string(const char*& p, String& out_str) {
		if (*p != '"') return false;
		p++; // skip leading quote
		char buf[1024];
		stduint len = 0;
		while (*p && *p != '"') {
			if (*p == '\\') {
				p++;
				if (!*p) return false;
				char ch = *p;
				if (ch == '"') buf[len++] = '"';
				else if (ch == '\\') buf[len++] = '\\';
				else if (ch == '/') buf[len++] = '/';
				else if (ch == 'b') buf[len++] = '\b';
				else if (ch == 'f') buf[len++] = '\f';
				else if (ch == 'n') buf[len++] = '\n';
				else if (ch == 'r') buf[len++] = '\r';
				else if (ch == 't') buf[len++] = '\t';
				else buf[len++] = ch;
			} else {
				buf[len++] = *p;
			}
			if (len >= sizeof(buf) - 1) {
				buf[len] = '\0';
				out_str += buf;
				len = 0;
			}
			p++;
		}
		if (*p != '"') return false;
		p++; // skip trailing quote
		buf[len] = '\0';
		out_str += buf;
		return true;
	}

	static JSONNode* parse_json_value(const char*& p);

	static JSONNode* parse_json_object(const char*& p) {
		if (*p != '{') return nullptr;
		p++; // skip '{'
		skip_json_whitespace(p);

		JSONNode* obj = new JSONNode(JSONType::Object);
		if (*p == '}') {
			p++;
			return obj;
		}

		while (*p) {
			skip_json_whitespace(p);
			if (*p != '"') {
				delete obj;
				return nullptr;
			}

			String key;
			if (!parse_json_string(p, key)) {
				delete obj;
				return nullptr;
			}

			skip_json_whitespace(p);
			if (*p != ':') {
				delete obj;
				return nullptr;
			}
			p++; // skip ':'

			skip_json_whitespace(p);
			JSONNode* val = parse_json_value(p);
			if (!val) {
				delete obj;
				return nullptr;
			}

			obj->addObjectMember(key.reference(), val);

			skip_json_whitespace(p);
			if (*p == ',') {
				p++;
			} else if (*p == '}') {
				p++;
				return obj;
			} else {
				delete obj;
				return nullptr;
			}
		}

		delete obj;
		return nullptr;
	}

	static JSONNode* parse_json_array(const char*& p) {
		if (*p != '[') return nullptr;
		p++; // skip '['
		skip_json_whitespace(p);

		JSONNode* arr = new JSONNode(JSONType::Array);
		if (*p == ']') {
			p++;
			return arr;
		}

		while (*p) {
			skip_json_whitespace(p);
			JSONNode* val = parse_json_value(p);
			if (!val) {
				delete arr;
				return nullptr;
			}

			arr->addArrayElement(val);

			skip_json_whitespace(p);
			if (*p == ',') {
				p++;
			} else if (*p == ']') {
				p++;
				return arr;
			} else {
				delete arr;
				return nullptr;
			}
		}

		delete arr;
		return nullptr;
	}

	static JSONNode* parse_json_value(const char*& p) {
		skip_json_whitespace(p);
		if (!*p) return nullptr;

		if (*p == '{') {
			return parse_json_object(p);
		} else if (*p == '[') {
			return parse_json_array(p);
		} else if (*p == '"') {
			String str;
			if (!parse_json_string(p, str)) return nullptr;
			JSONNode* node = new JSONNode(JSONType::String);
			node->str_val = str;
			return node;
		} else if (*p == 't' || *p == 'f') {
			bool is_true = (*p == 't');
			if (is_true) {
				if (StrCompareN(p, "true", 4) == 0) {
					p += 4;
					JSONNode* node = new JSONNode(JSONType::Boolean);
					node->bool_val = true;
					node->str_val = "true";
					return node;
				}
			} else {
				if (StrCompareN(p, "false", 5) == 0) {
					p += 5;
					JSONNode* node = new JSONNode(JSONType::Boolean);
					node->bool_val = false;
					node->str_val = "false";
					return node;
				}
			}
			return nullptr;
		} else if (*p == 'n') {
			if (StrCompareN(p, "null", 4) == 0) {
				p += 4;
				return new JSONNode(JSONType::Null);
			}
			return nullptr;
		} else if (*p == '-' || (*p >= '0' && *p <= '9')) {
			const char* start = p;
			if (*p == '-') p++;
			while (*p >= '0' && *p <= '9') p++;

			char num_buf[64];
			stduint len = p - start;
			if (len >= sizeof(num_buf)) len = sizeof(num_buf) - 1;
			MemCopyN(num_buf, start, len);
			num_buf[len] = '\0';

			JSONNode* node = new JSONNode(JSONType::Number);
			node->num_val = (stdsint)atoins(num_buf);
			node->str_val = num_buf;
			return node;
		}

		return nullptr;
	}

	void ConfigJSON::parse_json(rostr text) {
		if (!text) {
			is_valid = false;
			return;
		}
		const char* p = text;
		skip_json_whitespace(p);
		if (!*p) {
			is_valid = false;
			return;
		}

		JSONNode* parsed = parse_json_value(p);
		if (parsed) {
			skip_json_whitespace(p);
			if (*p == '\0') {
				root_node = parsed;
				is_valid = true;
				return;
			}
			delete parsed;
		}

		root_node = new JSONNode(JSONType::Object);
		is_valid = false;
	}

	ConfigJSON::ConfigJSON(HostFile& file) {
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

		parse_json(text_buf.reference());
	}

	ConfigJSON::~ConfigJSON() {
		if (root_node) {
			delete (JSONNode*)root_node;
			root_node = nullptr;
		}
	}

	bool ConfigJSON::Probe() {
		return is_valid;
	}

	static JSONNode* traverse_json_path(JSONNode* root, rostr path) {
		if (!root || !path) return nullptr;
		const char* p = path;
		while (*p == '/') p++;
		if (!*p) return root;

		JSONNode* curr = root;
		char segment[128];

		while (*p) {
			stduint idx = 0;
			while (*p && *p != '/') {
				if (idx < sizeof(segment) - 1) {
					segment[idx++] = *p;
				}
				p++;
			}
			segment[idx] = '\0';
			while (*p == '/') p++;

			if (curr->type == JSONType::Object) {
				curr = curr->findChild(segment);
				if (!curr) return nullptr;
			} else {
				return nullptr;
			}
		}

		return curr;
	}

	String ConfigJSON::get(rostr name) const {
		if (!root_node || !name) return String("");
		JSONNode* node = traverse_json_path((JSONNode*)root_node, name);
		if (!node) return String("");

		if (node->type == JSONType::String || node->type == JSONType::Number || node->type == JSONType::Boolean) {
			return node->str_val;
		}
		return String("");
	}

	stdsint ConfigJSON::getNumber(rostr name) const {
		if (!root_node || !name) return 0;
		JSONNode* node = traverse_json_path((JSONNode*)root_node, name);
		if (!node) return 0;

		if (node->type == JSONType::Number) {
			return node->num_val;
		} else if (node->type == JSONType::String) {
			return (stdsint)atoins(node->str_val.reference());
		} else if (node->type == JSONType::Boolean) {
			return node->bool_val ? 1 : 0;
		}
		return 0;
	}

	bool ConfigJSON::set(rostr name, rostr val) {
		if (!name || !val) return false;
		if (!root_node) {
			root_node = new JSONNode(JSONType::Object);
		}

		const char* p = name;
		while (*p == '/') p++;
		if (!*p) return false;

		JSONNode* curr = (JSONNode*)root_node;
		char segment[128];

		while (*p) {
			stduint idx = 0;
			while (*p && *p != '/') {
				if (idx < sizeof(segment) - 1) {
					segment[idx++] = *p;
				}
				p++;
			}
			segment[idx] = '\0';
			while (*p == '/') p++;

			if (*p == '\0') {
				// Leaf node
				if (curr->type != JSONType::Object) {
					curr->clear();
					curr->type = JSONType::Object;
				}
				JSONNode* leaf = curr->findChild(segment);
				if (!leaf) {
					leaf = new JSONNode(JSONType::String);
					curr->addObjectMember(segment, leaf);
				} else {
					leaf->clear();
					leaf->type = JSONType::String;
				}
				leaf->str_val = val;
				leaf->num_val = (stdsint)atoins(val);
				return true;
			} else {
				// Intermediate node
				curr = curr->getOrCreateChild(segment);
			}
		}

		return false;
	}

}

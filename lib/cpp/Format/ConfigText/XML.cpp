// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [Format.ConfigText] XML
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

#include "../../../../inc/cpp/Format/ConfigText/XML.hpp"
#include "../../../../inc/c/ustring.h"

namespace uni {

	struct XMLNode {
		String tag_name;
		String text_content;
		stdsint num_val;

		struct KeyValuePair {
			String key;
			XMLNode* value;
			KeyValuePair* next;
		};
		KeyValuePair* children_head;

		XMLNode(const char* tag = "")
			: tag_name(tag), num_val(0), children_head(nullptr) {}

		~XMLNode() {
			clear();
		}

		void clear() {
			KeyValuePair* curr = children_head;
			while (curr) {
				KeyValuePair* next = curr->next;
				delete curr->value;
				delete curr;
				curr = next;
			}
			children_head = nullptr;
		}

		XMLNode* findChild(const char* key) const {
			KeyValuePair* curr = children_head;
			while (curr) {
				if (StrCompare(curr->key.reference(), key) == 0) {
					return curr->value;
				}
				curr = curr->next;
			}
			return nullptr;
		}

		XMLNode* getOrCreateChild(const char* key) {
			XMLNode* existing = findChild(key);
			if (existing) return existing;

			XMLNode* new_child = new XMLNode(key);
			KeyValuePair* pair = new KeyValuePair;
			pair->key = key;
			pair->value = new_child;
			pair->next = nullptr;

			if (!children_head) {
				children_head = pair;
			} else {
				KeyValuePair* tail = children_head;
				while (tail->next) {
					tail = tail->next;
				}
				tail->next = pair;
			}
			return new_child;
		}

		void addChild(const char* key, XMLNode* child) {
			KeyValuePair* pair = new KeyValuePair;
			pair->key = key;
			pair->value = child;
			pair->next = nullptr;

			if (!children_head) {
				children_head = pair;
			} else {
				KeyValuePair* tail = children_head;
				while (tail->next) {
					tail = tail->next;
				}
				tail->next = pair;
			}
		}
	};

	static void skip_xml_whitespace(const char*& p) {
		while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) {
			p++;
		}
	}

	static void decode_xml_entities(String& str) {
		// Replace standard 5 entities if needed
	}

	static XMLNode* parse_xml_element(const char*& p);

	static void skip_xml_prolog_or_comment(const char*& p) {
		while (*p) {
			skip_xml_whitespace(p);
			if (*p == '<' && *(p + 1) == '?') {
				p += 2;
				while (*p && !(*p == '?' && *(p + 1) == '>')) p++;
				if (*p) p += 2;
			} else if (*p == '<' && *(p + 1) == '!' && *(p + 2) == '-' && *(p + 3) == '-') {
				p += 4;
				while (*p && !(*p == '-' && *(p + 1) == '-' && *(p + 2) == '>')) p++;
				if (*p) p += 3;
			} else {
				break;
			}
		}
	}

	static XMLNode* parse_xml_element(const char*& p) {
		skip_xml_whitespace(p);
		if (*p != '<' || *(p + 1) == '/' || *(p + 1) == '?' || *(p + 1) == '!') return nullptr;

		p++; // skip '<'
		char tag_buf[128];
		stduint tlen = 0;
		while (*p && *p != '>' && *p != '/' && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
			if (tlen < sizeof(tag_buf) - 1) {
				tag_buf[tlen++] = *p;
			}
			p++;
		}
		tag_buf[tlen] = '\0';
		if (tlen == 0) return nullptr;

		// Skip attributes until '>' or '/>'
		while (*p && *p != '>' && !(*p == '/' && *(p + 1) == '>')) {
			p++;
		}

		if (*p == '/' && *(p + 1) == '>') {
			p += 2; // self-closing
			XMLNode* node = new XMLNode(tag_buf);
			return node;
		}

		if (*p == '>') {
			p++; // skip '>'
		} else {
			return nullptr;
		}

		XMLNode* node = new XMLNode(tag_buf);

		// Parse content / children
		while (*p) {
			skip_xml_whitespace(p);
			if (*p == '<' && *(p + 1) == '!' && *(p + 2) == '-' && *(p + 3) == '-') {
				// Comment
				p += 4;
				while (*p && !(*p == '-' && *(p + 1) == '-' && *(p + 2) == '>')) p++;
				if (*p) p += 3;
				continue;
			}
			if (*p == '<' && *(p + 1) == '/') {
				// Closing tag
				p += 2;
				char close_tag[128];
				stduint clen = 0;
				while (*p && *p != '>' && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
					if (clen < sizeof(close_tag) - 1) {
						close_tag[clen++] = *p;
					}
					p++;
				}
				close_tag[clen] = '\0';
				while (*p && *p != '>') p++;
				if (*p == '>') p++;
				return node;
			} else if (*p == '<') {
				// Child element
				XMLNode* child = parse_xml_element(p);
				if (!child) {
					delete node;
					return nullptr;
				}
				node->addChild(child->tag_name.reference(), child);
			} else {
				// Text content
				char text_buf[1024];
				stduint txt_len = 0;
				while (*p && *p != '<') {
					if (txt_len < sizeof(text_buf) - 1) {
						text_buf[txt_len++] = *p;
					}
					p++;
				}
				text_buf[txt_len] = '\0';
				// Trim spaces
				while (txt_len > 0 && (text_buf[txt_len - 1] == ' ' || text_buf[txt_len - 1] == '\t' || text_buf[txt_len - 1] == '\r' || text_buf[txt_len - 1] == '\n')) {
					text_buf[--txt_len] = '\0';
				}
				stduint start_idx = 0;
				while (text_buf[start_idx] == ' ' || text_buf[start_idx] == '\t' || text_buf[start_idx] == '\r' || text_buf[start_idx] == '\n') {
					start_idx++;
				}
				if (start_idx < txt_len) {
					node->text_content = (text_buf + start_idx);
					node->num_val = (stdsint)atoins(node->text_content.reference());
				}
			}
		}

		delete node;
		return nullptr;
	}

	void ConfigXML::parse_xml(rostr text) {
		if (!text) {
			is_valid = false;
			return;
		}

		const char* p = text;
		skip_xml_prolog_or_comment(p);
		if (!*p) {
			is_valid = false;
			return;
		}

		XMLNode* root = parse_xml_element(p);
		if (root) {
			skip_xml_whitespace(p);
			skip_xml_prolog_or_comment(p);
			skip_xml_whitespace(p);
			root_node = root;
			is_valid = true;
		} else {
			root_node = new XMLNode("root");
			is_valid = false;
		}
	}

	ConfigXML::ConfigXML(HostFile& file) {
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

		parse_xml(text_buf.reference());
	}

	ConfigXML::~ConfigXML() {
		if (root_node) {
			delete (XMLNode*)root_node;
			root_node = nullptr;
		}
	}

	bool ConfigXML::Probe() {
		return is_valid;
	}

	static XMLNode* traverse_xml_path(XMLNode* root, rostr path) {
		if (!root || !path) return nullptr;
		const char* p = path;
		while (*p == '/') p++;
		if (!*p) return root;

		XMLNode* curr = root;
		char segment[128];

		// Check if first segment matches the root tag name
		const char* check_p = p;
		stduint cidx = 0;
		while (*check_p && *check_p != '/') {
			if (cidx < sizeof(segment) - 1) {
				segment[cidx++] = *check_p;
			}
			check_p++;
		}
		segment[cidx] = '\0';

		if (StrCompare(root->tag_name.reference(), segment) == 0) {
			p = check_p;
			while (*p == '/') p++;
		}

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

			curr = curr->findChild(segment);
			if (!curr) return nullptr;
		}

		return curr;
	}

	String ConfigXML::get(rostr name) const {
		if (!root_node || !name) return String("");
		XMLNode* node = traverse_xml_path((XMLNode*)root_node, name);
		if (!node) return String("");
		return node->text_content;
	}

	stdsint ConfigXML::getNumber(rostr name) const {
		if (!root_node || !name) return 0;
		XMLNode* node = traverse_xml_path((XMLNode*)root_node, name);
		if (!node) return 0;
		if (node->num_val != 0) return node->num_val;
		return (stdsint)atoins(node->text_content.reference());
	}

	bool ConfigXML::set(rostr name, rostr val) {
		if (!name || !val) return false;
		if (!root_node) {
			root_node = new XMLNode("root");
		}

		const char* p = name;
		while (*p == '/') p++;
		if (!*p) return false;

		XMLNode* curr = (XMLNode*)root_node;
		char segment[128];

		// Check if first segment matches root tag
		const char* check_p = p;
		stduint cidx = 0;
		while (*check_p && *check_p != '/') {
			if (cidx < sizeof(segment) - 1) {
				segment[cidx++] = *check_p;
			}
			check_p++;
		}
		segment[cidx] = '\0';

		if (curr->tag_name.reference() && StrCompare(curr->tag_name.reference(), segment) == 0) {
			p = check_p;
			while (*p == '/') p++;
		}

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
				// Leaf
				XMLNode* leaf = curr->findChild(segment);
				if (!leaf) {
					leaf = new XMLNode(segment);
					curr->addChild(segment, leaf);
				}
				leaf->text_content = val;
				leaf->num_val = (stdsint)atoins(val);
				return true;
			} else {
				// Intermediate
				curr = curr->getOrCreateChild(segment);
			}
		}

		return false;
	}

}

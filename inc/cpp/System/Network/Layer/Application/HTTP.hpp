// ASCII CPP-ISO11 TAB4 LF
// Docutitle: [System.Network.Layer.Application] HTTP
// Codifiers: @AringMgk
// Attribute: <ArnCovenant> Any-Architect <Environment> <Reference/Dependence>
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2026 ArinaMgk

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

#ifndef _INCPP_System_NETWORK_LAYER_APPLICATION_HTTP
#define _INCPP_System_NETWORK_LAYER_APPLICATION_HTTP

#include "../Application.hpp"

namespace uni {
namespace Network {

	struct HTTPStatusLine {
		uint16 status_code;
		const char* reason;
		stduint reason_length;
	};

	struct HTTPHeaderView {
		const char* header;
		stduint header_length;
		const char* body;
		stduint body_length;
	};

	inline stduint HTTPStringLength(const char* text) {
		stduint length = 0;
		if (text) while (text[length]) length++;
		return length;
	}

	inline bool HTTPAsciiEqual(char lhs, char rhs) {
		if (lhs >= 'A' && lhs <= 'Z') lhs = char(lhs - 'A' + 'a');
		if (rhs >= 'A' && rhs <= 'Z') rhs = char(rhs - 'A' + 'a');
		return lhs == rhs;
	}

	inline bool HTTPHeaderNameMatch(const char* line, stduint line_length, const char* name) {
		stduint i = 0;
		while (name && name[i]) {
			if (i >= line_length || !HTTPAsciiEqual(line[i], name[i])) return false;
			i++;
		}
		return i < line_length && line[i] == ':';
	}

	inline stdsint HTTPBuildGET(char* output, stduint capacity, const char* host, const char* path) {
		if (!output || !capacity || !host || !host[0]) return -1;
		if (!path || !path[0]) path = "/";
		const char* prefix = "GET ";
		const char* middle = " HTTP/1.0\r\nHost: ";
		const char* suffix = "\r\nConnection: close\r\n\r\n";
		const char* parts[] = { prefix, path, middle, host, suffix };
		stduint length = 0;
		for0(i, numsof(parts)) length += HTTPStringLength(parts[i]);
		if (length + 1 > capacity) return -1;
		char* cursor = output;
		for0(i, numsof(parts)) {
			const char* part = parts[i];
			while (*part) *cursor++ = *part++;
		}
		*cursor = 0;
		return stdsint(length);
	}

	inline stduint HTTPHeaderLength(const char* response, stduint length) {
		if (!response || length < 4) return 0;
		for (stduint i = 0; i + 3 < length; i++) {
			if (response[i] == '\r' && response[i + 1] == '\n' &&
				response[i + 2] == '\r' && response[i + 3] == '\n') return i + 4;
		}
		return 0;
	}

	inline bool HTTPParseHeader(const char* response, stduint length, HTTPHeaderView& view) {
		const stduint header_length = HTTPHeaderLength(response, length);
		if (!header_length) return false;
		view.header = response;
		view.header_length = header_length;
		view.body = response + header_length;
		view.body_length = length - header_length;
		return true;
	}

	inline int HTTPParseStatusCode(const char* response, stduint length) {
		if (!response || length < 12) return 0;
		if (response[0] != 'H' || response[1] != 'T' || response[2] != 'T' || response[3] != 'P' ||
			response[4] != '/') return 0;
		stduint cursor = 5;
		while (cursor < length && response[cursor] != ' ') cursor++;
		if (cursor + 4 > length || response[cursor] != ' ') return 0;
		cursor++;
		int code = 0;
		for0(i, 3) {
			const char digit = response[cursor + i];
			if (digit < '0' || digit > '9') return 0;
			code = code * 10 + int(digit - '0');
		}
		return code;
	}

	inline bool HTTPParseStatusLine(const char* response, stduint length, HTTPStatusLine& status) {
		const int code = HTTPParseStatusCode(response, length);
		if (!code) return false;
		stduint cursor = 0;
		while (cursor < length && response[cursor] != ' ' && response[cursor] != '\r' && response[cursor] != '\n') {
			cursor++;
		}
		if (cursor < length && response[cursor] == ' ') cursor++;
		while (cursor < length && response[cursor] >= '0' && response[cursor] <= '9') cursor++;
		if (cursor < length && response[cursor] == ' ') cursor++;
		stduint reason_end = cursor;
		while (reason_end < length && response[reason_end] != '\r' && response[reason_end] != '\n') {
			reason_end++;
		}
		status.status_code = uint16(code);
		status.reason = response + cursor;
		status.reason_length = reason_end - cursor;
		return true;
	}

	inline bool HTTPContentLength(const char* response, stduint length, stduint& output) {
		output = 0;
		const stduint header_length = HTTPHeaderLength(response, length);
		if (!header_length) return false;
		stduint line = 0;
		while (line < header_length) {
			stduint end = line;
			while (end < header_length && response[end] != '\r' && response[end] != '\n') end++;
			const stduint line_length = end - line;
			if (HTTPHeaderNameMatch(response + line, line_length, "content-length")) {
				stduint cursor = line + HTTPStringLength("content-length:");
				while (cursor < end && (response[cursor] == ' ' || response[cursor] == '\t')) cursor++;
				stduint value = 0;
				bool any = false;
				while (cursor < end && response[cursor] >= '0' && response[cursor] <= '9') {
					any = true;
					value = value * 10 + stduint(response[cursor] - '0');
					cursor++;
				}
				if (!any) return false;
				output = value;
				return true;
			}
			while (end < header_length && (response[end] == '\r' || response[end] == '\n')) end++;
			line = end;
		}
		return false;
	}

}
}

#endif

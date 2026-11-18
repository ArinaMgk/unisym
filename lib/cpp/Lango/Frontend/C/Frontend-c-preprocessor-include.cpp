// ASCII C++11 TAB4 LF
// Docutitle: Lango C Frontend Include Directives
// Codifiers: @ArinaMgk
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

#include "../../../../../inc/cpp/Lango/Frontend/C/Frontend-c-preprocessor.hpp"
#include "../../../../../inc/c/file.h"

namespace uni::lango::frontend_c_preprocessor {

namespace {

	bool IsToken(const Token& token, const char* spelling) {
		return token.spelling.getByteCount() &&
			!StrCompare(token.spelling.reference(), spelling);
	}

	bool IsAbsolutePath(const char* path) {
		if (!path || !path[0]) return false;
		if (path[0] == '/' || path[0] == '\\') return true;
		return ((path[0] >= 'A' && path[0] <= 'Z') ||
			(path[0] >= 'a' && path[0] <= 'z')) && path[1] == ':';
	}

	uni::String SourceDirectory(const char* source_name) {
		uni::String directory;
		if (!source_name || source_name[0] == '<') return directory;
		stduint end = StrLength(source_name);
		while (end && source_name[end - 1] != '/' && source_name[end - 1] != '\\')
			end--;
		for (stduint index = 0; index < end; index++) directory << source_name[index];
		return directory;
	}

	uni::String JoinPath(const char* directory, const char* header) {
		uni::String path;
		if (directory) while (*directory) path << *directory++;
		if (path.getByteCount()) {
			char tail = path[(stdsint)path.getByteCount() - 1];
			if (tail != '/' && tail != '\\') path << '/';
		}
		if (header) while (*header) path << *header++;
		return path;
	}

	bool OpenIncludedFile(const uni::String& path, uni::HostFile*& file) {
		file = new uni::HostFile(path.reference(), uni::FileOpenType::Read);
		if (*file) return true;
		delete file;
		file = nullptr;
		return false;
	}

	bool ExtractHeaderName(const uni::Vector<Token>& tokens,
		stduint begin, stduint end, uni::String& header, bool& quoted) {
		if (begin >= end) return false;
		if (end == begin + 1 &&
			tokens.begin()[begin].kind == TokenKind::StringLiteral) {
			const uni::String& spelling = tokens.begin()[begin].spelling;
			if (spelling.getByteCount() < 2) return false;
			for (stduint index = 1; index + 1 < spelling.getByteCount(); index++)
				header << spelling[(stdsint)index];
			quoted = true;
			return true;
		}
		if (end < begin + 2 || !IsToken(tokens.begin()[begin], "<") ||
			!IsToken(tokens.begin()[end - 1], ">")) return false;
		for (stduint index = begin + 1; index + 1 < end; index++) {
			const char* spelling = tokens.begin()[index].spelling.reference();
			while (*spelling) header << *spelling++;
		}
		return true;
	}

}

bool Preprocessor::HandleInclude(const uni::Vector<Token>& input,
	stduint begin, stduint end, const Token& directive,
	uni::Vector<Token>& output, const char* source_name,
	stduint include_depth) {
	if (include_depth >= MaximumIncludeDepth)
		return Report("maximum #include depth exceeded", directive);
	uni::String header;
	bool quoted = false;
	if (!ExtractHeaderName(input, begin, end, header, quoted)) {
		uni::Vector<Token> expanded;
		uni::Vector<uni::String> disabled;
		if (!ExpandSequence(input, begin, end, expanded, disabled, 0)) return false;
		if (!expanded.Count()) return Report("#include requires a header name", directive);
		if (!ExtractHeaderName(expanded, 0, expanded.Count(), header, quoted))
			return Report("#include expects \"file\" or <file>", expanded.begin()[0]);
	}
	if (!header.getByteCount()) return Report("empty #include header name", directive);

	uni::HostFile* file = nullptr;
	uni::String resolved;
	if (IsAbsolutePath(header.reference())) {
		resolved = header;
		OpenIncludedFile(resolved, file);
	}
	else if (quoted) {
		resolved = JoinPath(SourceDirectory(source_name).reference(), header.reference());
		OpenIncludedFile(resolved, file);
	}
	if (!file && include_paths) {
		for (const uni::String& directory : *include_paths) {
			resolved = JoinPath(directory.reference(), header.reference());
			if (OpenIncludedFile(resolved, file)) break;
		}
	}
	if (!file) {
		uni::String message("included file not found: ");
		message += header;
		return Report(message.reference(), directive);
	}
	bool success = ProcessSource(*file, output,
		resolved.reference(), include_depth + 1);
	delete file;
	return success;
}

}

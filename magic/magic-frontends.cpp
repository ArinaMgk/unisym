// ASCII C++11 TAB4 LF
// Docutitle: Magice Frontend Dispatcher
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

#include "./magice.hpp"

#include "../inc/cpp/Lango/Frontend/Frontend-c.hpp"
#include "../inc/cpp/Lango/Frontend/Frontend-systemverilog.hpp"

namespace {

	const MagicFrontendInfo frontend_infos[] = {
		{ MagicFrontend::C, "c", nullptr, ".c", "c99", "C99 subset",
			clover::IRClass::Soft, true },
		{ MagicFrontend::SystemVerilog, "systemverilog", "sv", ".sv",
			"systemverilog-subset", "synthesizable subset",
			clover::IRClass::Hard, false },
	};

	bool HasSuffix(rostr path, rostr suffix) {
		stduint path_length = StrLength(path);
		stduint suffix_length = StrLength(suffix);
		return path_length >= suffix_length &&
			!StrCompareInsensitive(path + path_length - suffix_length, suffix);
	}

	void PrintColumn(uni::OstreamTrait& output, const char* text, stduint width) {
		output.OutFormat("%s", text);
		stduint length = StrLength(text);
		while (length++ < width) output.OutChar(' ');
	}

}

bool FrontendFromName(rostr name, MagicFrontend& frontend) {
	for (const MagicFrontendInfo& info : frontend_infos) {
		if (!StrCompare(name, info.name) ||
			(info.alias && !StrCompare(name, info.alias))) {
			frontend = info.frontend;
			return true;
		}
	}
	return false;
}

bool FrontendFromPath(rostr path, MagicFrontend& frontend) {
	for (const MagicFrontendInfo& info : frontend_infos) {
		if (HasSuffix(path, info.suffix)) {
			frontend = info.frontend;
			return true;
		}
	}
	return false;
}

const MagicFrontendInfo& FrontendInformation(MagicFrontend frontend) {
	for (const MagicFrontendInfo& info : frontend_infos)
		if (info.frontend == frontend) return info;
	return frontend_infos[0];
}

void PrintFrontendInformation(uni::OstreamTrait& output) {
	output.OutFormat("Supported source frontends:\n");
	output.OutFormat("  Soft IR:\n");
	for (const MagicFrontendInfo& info : frontend_infos) {
		if (info.ir_class != clover::IRClass::Soft) continue;
		output.OutFormat("    %c ", info.is_default ? '*' : ' ');
		PrintColumn(output, info.name, 16);
		PrintColumn(output, info.suffix, 6);
		output.OutFormat("%s\n", info.description);
	}
	output.OutFormat("  Hard IR:\n");
	for (const MagicFrontendInfo& info : frontend_infos) {
		if (info.ir_class != clover::IRClass::Hard) continue;
		output.OutFormat("    %c ", info.is_default ? '*' : ' ');
		PrintColumn(output, info.name, 16);
		PrintColumn(output, info.suffix, 6);
		output.OutFormat("%s\n", info.description);
	}
}

uni::lango::FrontendInterface* FrontendCreate(MagicFrontend frontend) {
	switch (frontend) {
	case MagicFrontend::C:
		return new uni::lango::FrontendC();
	case MagicFrontend::SystemVerilog:
		return new uni::lango::FrontendSystemVerilog();
	}
	plogerro("Magice: unsupported frontend");
	return nullptr;
}

bool FrontendPrintAST(MagicFrontend frontend, uni::IstreamTrait& input,
	uni::OstreamTrait& output, const uni::lango::FrontendOptions& options) {
	switch (frontend) {
	case MagicFrontend::C: {
		uni::lango::FrontendC frontend_c;
		return frontend_c.PrintAST(input, output, options);
	}
	case MagicFrontend::SystemVerilog: {
		uni::lango::FrontendSystemVerilog frontend_systemverilog;
		return frontend_systemverilog.PrintAST(input, output, options);
	}
	}
	plogerro("Magice: frontend does not support AST output");
	return false;
}

// ASCII C++11 TAB4 LF
// Docutitle: Lango Diagnostic Interface
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

#include "../../../inc/cpp/Lango/Diagnostic.hpp"

namespace uni::lango {

namespace {

	const char* DiagnosticLevelName(DiagnosticLevel level) {
		switch (level) {
		case DiagnosticLevel::Note: return "note";
		case DiagnosticLevel::Warning: return "warning";
		case DiagnosticLevel::Error: return "error";
		}
		return "diagnostic";
	}

	loglevel_t DiagnosticLogLevel(DiagnosticLevel level) {
		switch (level) {
		case DiagnosticLevel::Note: return _LOG_INFO;
		case DiagnosticLevel::Warning: return _LOG_WARN;
		case DiagnosticLevel::Error: return _LOG_ERROR;
		}
		return _LOG_ERROR;
	}

	class LoggerDiagnosticInterface : public DiagnosticInterface {
	public:
		virtual void Report(const Diagnostic& diagnostic) override {
			const char* level = DiagnosticLevelName(diagnostic.level);
			loglevel_t log_level = DiagnosticLogLevel(diagnostic.level);
			const char* file = diagnostic.location.file.getByteCount() ?
				diagnostic.location.file.reference() : nullptr;
			if (file && diagnostic.location.row && diagnostic.location.column)
				printlog(log_level, "%s:%[u]:%[u]: %s: %s", file,
					diagnostic.location.row, diagnostic.location.column, level,
					diagnostic.message.reference());
			else if (file)
				printlog(log_level, "%s: %s: %s", file, level,
					diagnostic.message.reference());
			else printlog(log_level, "%s: %s", level, diagnostic.message.reference());
		}
	};

}

DiagnosticInterface& DefaultDiagnosticInterface() {
	static LoggerDiagnosticInterface diagnostic_interface;
	return diagnostic_interface;
}

}

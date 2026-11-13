// ASCII C TAB4 CRLF
// Docutitle: (Module) 
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

#include "inc/aasm.h"
#include "../inc/c/stdinc.h"
#include "../inc/cpp/endian"
#include "../inc/cpp/Format/Code.hpp"

#define LIST_MAX_LEN 216        /* something sensible */
#define LIST_INDENT  40
#define LIST_HEXBIT  18

class AssemblyListing::Implementation {
public:
	struct MacroInhibit {
		MacroInhibit* Next;
		int Level;
		bool Inhibiting;
	};

	MacroInhibit* InhibitStack = nullptr;
	char Line[LIST_MAX_LEN] = {};
	bool HasLine = false;
	char Error[LIST_MAX_LEN] = {};
	char Data[2 * LIST_INDENT] = {};
	int64_t Offset = 0;
	int32_t LineNumber = 0;
	int Suppress = 0;
	int Level = 0;
	int EmittedLevel = 0;
	FILE* File = nullptr;
};

AssemblyListing* AssemblyListing::LegacyTarget = nullptr;

static const char HexDigits[] = "0123456789ABCDEF";

#define HEX(a,b) (*(a)=HexDigits[((b)>>4)&15],(a)[1]=HexDigits[(b)&15]);

AssemblyListing::AssemblyListing()
	: State(new Implementation()), LegacyAdapter {
		LegacyInitialize,
		LegacyCleanup,
		LegacyOutput,
		LegacyLine,
		LegacyEnter,
		LegacyLeave,
		LegacyError
	}
{
}

AssemblyListing::~AssemblyListing()
{
	Cleanup();
	delete State;
}

void AssemblyListing::Flush()
{
	int i;

	if (!State->File || (!State->HasLine && !State->Data[0]))
		return;

	fprintf(State->File, "%6" PRId32 " ", ++State->LineNumber);

	if (State->Data[0])
		fprintf(State->File, "%08" PRIX64 " %-*s",
			static_cast<uint64_t>(State->Offset), LIST_HEXBIT + 1, State->Data);
	else
		fprintf(State->File, "%*s", LIST_HEXBIT + 10, "");

	if (State->EmittedLevel)
		fprintf(State->File, "%s<%d>", (State->Level < 10 ? " " : ""),
			State->EmittedLevel);
	else if (State->HasLine)
		fprintf(State->File, "    ");

	if (State->HasLine)
		fprintf(State->File, " %s", State->Line);

	putc('\n', State->File);
	State->HasLine = false;
	State->Data[0] = '\0';

	if (State->Error[0]) {
		fprintf(State->File, "%6" PRId32 "          ", ++State->LineNumber);
		for (i = 0; i < LIST_HEXBIT; i++)
			putc('*', State->File);
	
		if (State->EmittedLevel)
			fprintf(State->File, " %s<%d>", (State->Level < 10 ? " " : ""),
				State->EmittedLevel);
		else
			fprintf(State->File, "     ");

		fprintf(State->File, "  %s\n", State->Error);
		State->Error[0] = '\0';
	}
}

void AssemblyListing::Initialize(const char* filename)
{
	Cleanup();
	State->File = fopen(filename, "w");
	if (!State->File) {
		//--auto clean flag
		aasm_log(_LOG_ERROR, "unable to open listing file `%s'", filename);
		return;
	}

	State->Line[0] = '\0';
	State->HasLine = false;
	State->LineNumber = 0;
	State->Error[0] = '\0';
	State->Data[0] = '\0';
	State->Offset = 0;
	State->Level = 0;
	State->EmittedLevel = 0;
	State->Suppress = 0;
	State->InhibitStack = malcof(Implementation::MacroInhibit);
	State->InhibitStack->Next = nullptr;
	State->InhibitStack->Level = 0;
	State->InhibitStack->Inhibiting = true;
	LegacyTarget = this;
}

void AssemblyListing::Cleanup()
{
	while (State->InhibitStack) {
		Implementation::MacroInhibit* temp = State->InhibitStack;
		State->InhibitStack = temp->Next;
		memf(temp);
	}

	if (State->File) {
		Flush();
		fclose(State->File);
		State->File = nullptr;
	}
	if (LegacyTarget == this) LegacyTarget = nullptr;
}

void AssemblyListing::Append(int64_t offset, const char* text)
{
	if (StrLength(State->Data) + StrLength(text) > LIST_HEXBIT) {
		StrAppend(State->Data, "-");
		Flush();
	}
	if (!State->Data[0]) State->Offset = offset;
	StrAppend(State->Data, text);
}

void AssemblyListing::EmitLegacy(int64_t offset, const void* data,
	enum out_type type, uint64_t size)
{
	if (!State->File || State->Suppress || user_nolist)
		return;

	switch (type) {
	case OUT_RAWDATA:
	{
		uint8_t const *p = (uint8_t const*)data;
		char q[3];
	if (size == 0 && !State->Data[0])
		State->Offset = offset;
		while (size--) {
			HEX(q, *p);
			q[2] = '\0';
			Append(offset++, q);
			p++;
		}
	break;
	}
	case OUT_ADDRESS:
	{
		uint64_t d = *(int64_t *)data;
		char q[20];
		LitEndian<uint64_t, true> encoded;
		encoded = d;
		if (size == 4) {
			q[0] = '[';
			q[9] = ']';
			q[10] = '\0';
			HEX(q + 1, encoded.v[0]);
			HEX(q + 3, encoded.v[1]);
			HEX(q + 5, encoded.v[2]);
			HEX(q + 7, encoded.v[3]);
			Append(offset, q);
		} else if (size == 8) {
			q[0] = '[';
			q[17] = ']';
			q[18] = '\0';
			HEX(q + 1,  encoded.v[0]);
			HEX(q + 3,  encoded.v[1]);
			HEX(q + 5,  encoded.v[2]);
			HEX(q + 7,  encoded.v[3]);
			HEX(q + 9,  encoded.v[4]);
			HEX(q + 11, encoded.v[5]);
			HEX(q + 13, encoded.v[6]);
			HEX(q + 15, encoded.v[7]);
			Append(offset, q);
		} else {
			q[0] = '[';
			q[5] = ']';
			q[6] = '\0';
			HEX(q + 1, encoded.v[0]);
			HEX(q + 3, encoded.v[1]);
			Append(offset, q);
		}
	break;
	}
	case OUT_REL2ADR:
	{
		uint32_t d = *(int32_t *)data;
		char q[11];
		LitEndian<uint16_t, true> encoded;
		encoded = d;
		q[0] = '(';
		q[5] = ')';
		q[6] = '\0';
		HEX(q + 1, encoded.v[0]);
		HEX(q + 3, encoded.v[1]);
		Append(offset, q);
	break;
	}
	case OUT_REL4ADR:
	{
		uint32_t d = *(int32_t *)data;
		char q[11];
		LitEndian<uint32_t, true> encoded;
		encoded = d;
		q[0] = '(';
		q[9] = ')';
		q[10] = '\0';
		HEX(q + 1, encoded.v[0]);
		HEX(q + 3, encoded.v[1]);
		HEX(q + 5, encoded.v[2]);
		HEX(q + 7, encoded.v[3]);
		Append(offset, q);
	break;
	}
	case OUT_REL8ADR:
	{
		uint64_t d = *(int64_t *)data;
		char q[19];
		LitEndian<uint64_t, true> encoded;
		encoded = d;
		q[0] = '(';
		q[17] = ')';
		q[18] = '\0';
		HEX(q + 1, encoded.v[0]);
		HEX(q + 3, encoded.v[1]);
		HEX(q + 5, encoded.v[2]);
		HEX(q + 7, encoded.v[3]);
		HEX(q + 9, encoded.v[4]);
		HEX(q + 11, encoded.v[5]);
		HEX(q + 13, encoded.v[6]);
		HEX(q + 15, encoded.v[7]);
		Append(offset, q);
	break;
	}
	case OUT_RESERVE:
	{
		char q[20];
		snprintf(q, sizeof(q), "<res %08" PRIX64 ">", size);
		Append(offset, q);
	break;
	}
	}
}

void AssemblyListing::EmitCode(
	int64_t offset, const uni::CodeendOutput& output)
{
	EmitLegacy(offset, output.data,
		output.type == uni::CodeendOutputType::Reserve ?
		OUT_RESERVE : OUT_RAWDATA, output.size);
}

void AssemblyListing::EmitFixup(const uni::CodeFixup& fixup)
{
	uint64_t size = (fixup.encoding.storage_bits + 7) / 8;
	out_type type = OUT_ADDRESS;
	if (fixup.kind == uni::CodeFixupKind::PcRelative ||
		fixup.kind == uni::CodeFixupKind::Branch ||
		fixup.kind == uni::CodeFixupKind::Call) {
		switch (size) {
		case 2: type = OUT_REL2ADR; break;
		case 4: type = OUT_REL4ADR; break;
		case 8: type = OUT_REL8ADR; break;
		default: type = OUT_ADDRESS; break;
		}
	}
	EmitLegacy(fixup.offset, &fixup.initial_value, type, size);
}

void AssemblyListing::SetOffset(int64_t offset)
{
	EmitLegacy(offset, nullptr, OUT_RAWDATA, 0);
}

void AssemblyListing::Line(int type, const char* line)
{
	if (!State->File) return;
	if (user_nolist) {          /* fbk - 9/2/00 */
		State->LineNumber++;
		return;
	}

	if (State->InhibitStack && State->InhibitStack->Inhibiting) {
		if (type == LIST_MACRO)
			return;
		else {                  /* pop the m i stack */
			Implementation::MacroInhibit* temp = State->InhibitStack;
			State->InhibitStack = temp->Next;
			memf(temp);
		}
	}
	Flush();
	State->HasLine = true;
	StrCopyN(State->Line, line, LIST_MAX_LEN - 1);
	State->Line[LIST_MAX_LEN - 1] = '\0';
	State->EmittedLevel = State->Level;
}

void AssemblyListing::Enter(int type)
{
	if (!State->File) return;
	if (type == LIST_INCBIN || type == LIST_TIMES) {
		State->Suppress |= (type == LIST_INCBIN ? 1 : 2);
		Append(State->Offset,
			type == LIST_INCBIN ? "<incbin>" : "<rept>");
		return;
	}

	State->Level++;

	if (State->InhibitStack && State->InhibitStack->Inhibiting &&
		type == LIST_INCLUDE) {
		Implementation::MacroInhibit* temp =
			malcof(Implementation::MacroInhibit);
		temp->Next = State->InhibitStack;
		temp->Level = State->Level;
		temp->Inhibiting = false;
		State->InhibitStack = temp;
	} else if (type == LIST_MACRO_NOLIST) {
		Implementation::MacroInhibit* temp =
			malcof(Implementation::MacroInhibit);
		temp->Next = State->InhibitStack;
		temp->Level = State->Level;
		temp->Inhibiting = true;
		State->InhibitStack = temp;
	}
}

void AssemblyListing::Leave(int type)
{
	if (!State->File) return;

	if (type == LIST_INCBIN || type == LIST_TIMES) {
		State->Suppress &= ~(type == LIST_INCBIN ? 1 : 2);
		return;
	}

	State->Level--;
	while (State->InhibitStack &&
		State->InhibitStack->Level > State->Level) {
		Implementation::MacroInhibit* temp = State->InhibitStack;
		State->InhibitStack = temp->Next;
		memf(temp);
	}
}

void AssemblyListing::Error(loglevel_t severity,
	const char* prefix, const char* message)
{
	if (!State->File) return;

	snprintf(State->Error, sizeof State->Error, "%s%s", prefix, message);

	if (severity == _LOG_FATAL) Flush();
}

ListGen& AssemblyListing::Legacy()
{
	LegacyTarget = this;
	return LegacyAdapter;
}

void AssemblyListing::LegacyInitialize(char* filename)
{
	if (LegacyTarget) LegacyTarget->Initialize(filename);
}

void AssemblyListing::LegacyCleanup()
{
	if (LegacyTarget) LegacyTarget->Cleanup();
}

void AssemblyListing::LegacyOutput(int32_t offset, const void* data,
	enum out_type type, uint64_t size)
{
	if (LegacyTarget) LegacyTarget->EmitLegacy(offset, data, type, size);
}

void AssemblyListing::LegacyLine(int type, char* line)
{
	if (LegacyTarget) LegacyTarget->Line(type, line);
}

void AssemblyListing::LegacyEnter(int type)
{
	if (LegacyTarget) LegacyTarget->Enter(type);
}

void AssemblyListing::LegacyLeave(int type)
{
	if (LegacyTarget) LegacyTarget->Leave(type);
}

void AssemblyListing::LegacyError(loglevel_t severity,
	const char* prefix, const char* message)
{
	if (LegacyTarget) LegacyTarget->Error(severity, prefix, message);
}

AssemblyListing aasmlist;

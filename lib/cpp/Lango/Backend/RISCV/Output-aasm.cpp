// ASCII C++11 TAB4 LF
// Docutitle: RISC-V AASM Assembly Output
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

#include "../../../../../inc/cpp/Lango/Backend/RISCV/Output-aasm.hpp"

namespace uni::lango {

namespace {

	bool StartsWith(const char* text, const char* prefix) {
		while (*prefix)
			if (*text++ != *prefix++) return false;
		return true;
	}

	bool EqualWord(const char* text, stduint length, const char* word) {
		stduint index = 0;
		while (index < length && word[index]) {
			if (ascii_tolower(text[index]) != ascii_tolower(word[index])) return false;
			index++;
		}
		return index == length && !word[index];
	}

	bool IsIdentifierCharacter(char character) {
		return ascii_isalpha(character) || ascii_isdigit(character) ||
			character == '_';
	}

	void EmitQuotedPath(const uni::String& path, uni::OstreamTrait& output) {
		output.OutChar('"');
		for (const char* current = path.reference(); current && *current; current++) {
			if (*current == '"' || *current == '\\') output.OutChar('\\');
			output.OutChar(*current);
		}
		output.OutChar('"');
	}

	const clover::SourceFile* FindSourceFile(const clover::Module& module,
		uint32 id) {
		for (const clover::SourceFile& source : module.source_files)
			if (source.id == id) return &source;
		return nullptr;
	}

	void EmitSymbolText(const char* text, uni::OstreamTrait& output) {
		while (*text) {
			if (text[0] == '.' && text[1] == 'L' && text[2] == '.') {
				output.OutFormat("__L_");
				text += 3;
			}
			else output.OutChar(*text++);
		}
	}

	void EmitConvertedOperands(const char* text, uni::OstreamTrait& output) {
		while (*text) {
			if (text[0] == '.' && text[1] == 'L' && text[2] == '.') {
				output.OutFormat("__L_");
				text += 3;
				continue;
			}
			const char* number = text;
			if (*number == '-') number++;
			const char* digit = number;
			while (ascii_isdigit(*digit)) digit++;
			if (digit != number && *digit == '(') {
				const char* register_name = digit + 1;
				const char* close = register_name;
				while (IsIdentifierCharacter(*close)) close++;
				if (*close == ')') {
					output.OutChar('[');
					output.out(register_name, close - register_name);
					if (digit - text != 1 || *text != '0') {
						if (*text == '-') {
							output.OutFormat(" - ");
							output.out(number, digit - number);
						}
						else {
							output.OutFormat(" + ");
							output.out(text, digit - text);
						}
					}
					output.OutChar(']');
					text = close + 1;
					continue;
				}
			}
			output.OutChar(*text++);
		}
	}

	bool SplitTwoOperands(const char* text, const char*& comma) {
		comma = text;
		while (*comma && *comma != ',') comma++;
		return *comma == ',';
	}

	void EmitTwoOperandPseudo(const char* mnemonic, const char* operands,
		uni::OstreamTrait& output) {
		const char* comma;
		if (!SplitTwoOperands(operands, comma)) {
			output.OutFormat("\t%s %s\n", mnemonic, operands);
			return;
		}
		const char* right = comma + 1;
		while (ascii_isspace(*right)) right++;
		if (!StrCompare(mnemonic, "beq") || !StrCompare(mnemonic, "bne")) {
			output.OutFormat("\t%s ", mnemonic);
			output.out(operands, comma - operands);
			output.OutFormat(", zero, ");
			EmitSymbolText(right, output);
			output.OutChar('\n');
		}
		else if (!StrCompare(mnemonic, "sltiu")) {
			output.OutFormat("\tsltiu ");
			output.out(operands, comma - operands);
			output.OutFormat(", ");
			EmitSymbolText(right, output);
			output.OutFormat(", 1\n");
		}
		else {
			output.OutFormat("\t%s ", mnemonic);
			output.out(operands, comma - operands);
			output.OutFormat(", zero, ");
			EmitSymbolText(right, output);
			output.OutChar('\n');
		}
	}

}

RISCVAssemblyOutputAASM::RISCVAssemblyOutputAASM(unsigned bits,
	const char* instructions, const char* abi, uni::OstreamTrait& output) :
	destination(output), target_bits(bits), target_instructions(instructions),
	target_abi(abi) {}

bool RISCVAssemblyOutputAASM::Begin() {
	destination.OutFormat("[ARCH riscv]\n[BITS %[u]]\n[INST %s]\n[ABI %s]\n",
		target_bits, target_instructions.reference(), target_abi.reference());
	return true;
}

int RISCVAssemblyOutputAASM::out(const char* text, stduint length) {
	for (stduint index = 0; index < length; index++) {
		if (text[index] == '\n') EmitLine();
		else line << text[index];
	}
	out_count += length;
	return (int)length;
}

bool RISCVAssemblyOutputAASM::Flush() {
	return !line.getByteCount() || EmitLine();
}

bool RISCVAssemblyOutputAASM::EmitLine() {
	const char* text = line.reference();
	if (!*text) destination.OutChar('\n');
	else if (!StrCompare(text, ".data")) {
		bss_section = false;
		destination.OutFormat("SECTION .data\n");
	}
	else if (!StrCompare(text, ".bss")) {
		bss_section = true;
		destination.OutFormat("SECTION .bss\n");
	}
	else if (!StrCompare(text, ".text")) {
		bss_section = false;
		destination.OutFormat("SECTION .text\n");
	}
	else if (StartsWith(text, ".section ")) {
		bss_section = !StrCompare(text + 9, ".bss");
		destination.OutFormat("SECTION %s\n", text + 9);
	}
	else if (StartsWith(text, ".globl "))
		destination.OutFormat("GLOBAL %s\n", text + 7);
	else if (StartsWith(text, ".type ") || StartsWith(text, ".size ")) {
		line.Clear();
		return true;
	}
	else if (StartsWith(text, ".balign ")) {
		if (bss_section) destination.OutFormat("ALIGNB %s\n", text + 8);
		else destination.OutFormat("ALIGNB %s, DB 0\n", text + 8);
	}
	else if (StartsWith(text, "\t.byte ")) {
		destination.OutFormat("\tDB ");
		EmitSymbolText(text + 7, destination);
		destination.OutChar('\n');
	}
	else if (StartsWith(text, "\t.half ")) {
		destination.OutFormat("\tDW ");
		EmitSymbolText(text + 7, destination);
		destination.OutChar('\n');
	}
	else if (StartsWith(text, "\t.word ")) {
		destination.OutFormat("\tDD ");
		EmitSymbolText(text + 7, destination);
		destination.OutChar('\n');
	}
	else if (StartsWith(text, "\t.quad ")) {
		destination.OutFormat("\tDQ ");
		EmitSymbolText(text + 7, destination);
		destination.OutChar('\n');
	}
	else if (StartsWith(text, "\t.zero ")) {
		if (bss_section) destination.OutFormat("\tRESB %s\n", text + 7);
		else destination.OutFormat("\tTIMES %s DB 0\n", text + 7);
	}
	else {
		const char* mnemonic = text;
		while (ascii_isspace(*mnemonic)) mnemonic++;
		const char* mnemonic_end = mnemonic;
		while (*mnemonic_end && !ascii_isspace(*mnemonic_end)) mnemonic_end++;
		const char* operands = mnemonic_end;
		while (ascii_isspace(*operands)) operands++;
		stduint mnemonic_length = mnemonic_end - mnemonic;
		if (EqualWord(mnemonic, mnemonic_length, "li") ||
			EqualWord(mnemonic, mnemonic_length, "la") ||
			EqualWord(mnemonic, mnemonic_length, "mv")) {
			destination.OutFormat("\tmov ");
			EmitConvertedOperands(operands, destination);
			destination.OutChar('\n');
		}
		else if (EqualWord(mnemonic, mnemonic_length, "beqz"))
			EmitTwoOperandPseudo("beq", operands, destination);
		else if (EqualWord(mnemonic, mnemonic_length, "bnez"))
			EmitTwoOperandPseudo("bne", operands, destination);
		else if (EqualWord(mnemonic, mnemonic_length, "seqz"))
			EmitTwoOperandPseudo("sltiu", operands, destination);
		else if (EqualWord(mnemonic, mnemonic_length, "snez"))
			EmitTwoOperandPseudo("sltu", operands, destination);
		else if (EqualWord(mnemonic, mnemonic_length, "sltz")) {
			const char* comma;
			if (!SplitTwoOperands(operands, comma))
				destination.OutFormat("\tsltz %s\n", operands);
			else {
				const char* right = comma + 1;
				while (ascii_isspace(*right)) right++;
				destination.OutFormat("\tslt ");
				destination.out(operands, comma - operands);
				destination.OutFormat(", %s, zero\n", right);
			}
		}
		else if (EqualWord(mnemonic, mnemonic_length, "sgtz")) {
			const char* comma;
			if (!SplitTwoOperands(operands, comma))
				destination.OutFormat("\tsgtz %s\n", operands);
			else {
				const char* right = comma + 1;
				while (ascii_isspace(*right)) right++;
				destination.OutFormat("\tslt ");
				destination.out(operands, comma - operands);
				destination.OutFormat(", zero, %s\n", right);
			}
		}
		else {
			if (EqualWord(mnemonic, mnemonic_length, "call") && operands[0] &&
				operands[0] == '_' && operands[1] == '_')
				destination.OutFormat("[EXTERN %s]\n", operands);
			if (mnemonic != text) destination.out(text, mnemonic - text);
			bool is_label = mnemonic_length && mnemonic[mnemonic_length - 1] == ':';
			if (mnemonic_length >= 3 && mnemonic[0] == '.' &&
				mnemonic[1] == 'L' && mnemonic[2] == '.') {
				destination.OutFormat("__L_");
				destination.out(mnemonic + 3, mnemonic_length - 3);
			}
			else destination.out(mnemonic, mnemonic_length);
			if (*operands) {
				destination.OutChar(' ');
				EmitConvertedOperands(operands, destination);
			}
			destination.OutChar('\n');
		}
	}
	line.Clear();
	return true;
}

bool RISCVAssemblyOutputAASM::EmitSourceFiles(const clover::Module& module,
	uni::OstreamTrait&) {
	source_module = &module;
	return true;
}

bool RISCVAssemblyOutputAASM::EmitLocation(uint32 file, uint32 source_line,
	uni::OstreamTrait& output) {
	if (!source_module || !file || !source_line) return false;
	const clover::SourceFile* source = FindSourceFile(*source_module, file);
	if (!source) return false;
	output.OutFormat("%%line %[u]+0 ", source_line);
	EmitQuotedPath(source->path, output);
	output.OutChar('\n');
	return true;
}

}

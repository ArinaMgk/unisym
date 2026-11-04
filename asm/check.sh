#!/bin/bash
# ASCII ShellScript TAB4 LF
# Docutitle: AASM x86-64 Regression Check
# Copyright: UNISYM, under Apache License 2.0

set -u

ScriptDirectory="$(cd "$(dirname "$0")" && pwd)"
RepositoryDirectory="$(cd "$ScriptDirectory/.." && pwd)"
TestDirectory="$RepositoryDirectory/demo/test/clover/arch-x86_64"
WorkDirectory="$(mktemp -d "${TMPDIR:-/tmp}/aasm-stage1.XXXXXX")"

Cleanup()
{
	case "$WorkDirectory" in
	"${TMPDIR:-/tmp}"/aasm-stage1.*)
		find "$WorkDirectory" -depth -delete
		;;
	esac
}

trap Cleanup EXIT

Fail()
{
	printf '\033[1;31mfailed\033[0m: %s\n' "$1" >&2
	exit 1
}

Pass()
{
	printf '\033[1;32mpassed\033[0m: %s\n' "$1"
}

RequireCommand()
{
	command -v "$1" >/dev/null 2>&1 || Fail "required command not found: $1"
}

if [ -n "${AASM:-}" ]; then
	AasmExecutable="$AASM"
else
	AasmExecutable="$(command -v aasm 2>/dev/null || true)"
fi

[ -n "$AasmExecutable" ] || Fail "set AASM to the built assembler executable"
[ -x "$AasmExecutable" ] || Fail "assembler is not executable: $AasmExecutable"

RequireCommand awk
RequireCommand cmp
RequireCommand diff
RequireCommand find
RequireCommand grep
RequireCommand mktemp
RequireCommand od
RequireCommand readelf
RequireCommand sed
RequireCommand tr
RequireCommand wc

NormalizeHex()
{
	HexText="$(od -An -v -tx1 "$1" | tr -s '[:space:]' ' ' | sed 's/^ //;s/ $//')"
	printf '%s\n' "$HexText"
}

CheckHex()
{
	TestName="$1"
	ExpectedHex="$2"
	shift 2
	OutputFile="$WorkDirectory/$TestName.bin"
	ActualFile="$WorkDirectory/$TestName.hex"

	"$AasmExecutable" "$@" -f bin -o "$OutputFile" "$TestDirectory/$TestName.asm" \
		>"$WorkDirectory/$TestName.stdout" 2>"$WorkDirectory/$TestName.stderr" || \
		Fail "$TestName did not assemble"

	[ ! -s "$WorkDirectory/$TestName.stderr" ] || {
		sed -n '1,20p' "$WorkDirectory/$TestName.stderr" >&2
		Fail "$TestName produced diagnostics"
	}

	NormalizeHex "$OutputFile" > "$ActualFile"
	ActualHex="$(sed -n '1p' "$ActualFile")"
	if [ "$ExpectedHex" != "$ActualHex" ]; then
		printf '%s\n' "$ExpectedHex" > "$WorkDirectory/$TestName.expected"
		diff -u "$WorkDirectory/$TestName.expected" "$ActualFile" >&2
		Fail "$TestName bytes differ"
	fi
	Pass "$TestName bytes"
}

CheckBranch()
{
	ActualFile="$WorkDirectory/branch.txt"
	ExpectedFile="$WorkDirectory/branch.expected"
	: > "$ActualFile"
	printf '%s\n' \
		'O1 size=265 prefix=e9 78 00 00 00' \
		'Ox size=262 prefix=eb 78' > "$ExpectedFile"

	for Optimization in O1 Ox; do
		OutputFile="$WorkDirectory/branch-$Optimization.bin"
		"$AasmExecutable" -"$Optimization" -f bin -o "$OutputFile" \
			"$TestDirectory/branch.asm" \
			>"$WorkDirectory/branch-$Optimization.stdout" \
			2>"$WorkDirectory/branch-$Optimization.stderr" || \
			Fail "branch -$Optimization did not assemble"

		[ ! -s "$WorkDirectory/branch-$Optimization.stderr" ] || \
			Fail "branch -$Optimization produced diagnostics"

		Size="$(wc -c < "$OutputFile" | tr -d '[:space:]')"
		if [ "$Optimization" = O1 ]; then
			Prefix="$(od -An -N5 -v -tx1 "$OutputFile" | tr -s '[:space:]' ' ' | sed 's/^ //;s/ $//')"
		else
			Prefix="$(od -An -N2 -v -tx1 "$OutputFile" | tr -s '[:space:]' ' ' | sed 's/^ //;s/ $//')"
		fi
		printf '%s size=%s prefix=%s\n' "$Optimization" "$Size" "$Prefix" >> "$ActualFile"
	done

	if ! cmp -s "$ExpectedFile" "$ActualFile"; then
		diff -u "$ExpectedFile" "$ActualFile" >&2
		Fail "branch optimization differs"
	fi
	Pass "branch optimization"
}

CheckPreprocessor()
{
	RawFile="$WorkDirectory/preprocess.raw"
	ActualFile="$WorkDirectory/preprocess.txt"
	ExpectedFile="$WorkDirectory/preprocess.expected"
	printf '%s\n' \
		'[bits 64]' \
		'[section .text]' \
		'db 0x31' \
		'db 0x22' \
		'db 0x22' \
		'db 0x22' > "$ExpectedFile"

	"$AasmExecutable" -E -I"$TestDirectory/include/" -o "$RawFile" \
		"$TestDirectory/preprocess.asm" \
		>"$WorkDirectory/preprocess.stdout" 2>"$WorkDirectory/preprocess.stderr" || \
		Fail "preprocessor check failed"

	[ ! -s "$WorkDirectory/preprocess.stderr" ] || Fail "preprocessor produced diagnostics"

	sed -e '/^%line /d' -e '/^[[:space:]]*$/d' \
		-e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//' "$RawFile" > "$ActualFile"

	if ! cmp -s "$ExpectedFile" "$ActualFile"; then
		diff -u "$ExpectedFile" "$ActualFile" >&2
		Fail "preprocessor output differs"
	fi
	Pass "preprocessor output"
}

CheckRelocation()
{
	ObjectFile="$WorkDirectory/relocation.o"
	HeaderFile="$WorkDirectory/relocation.header"
	SectionFile="$WorkDirectory/relocation.sections"
	SymbolFile="$WorkDirectory/relocation.symbols"
	RelocationFile="$WorkDirectory/relocation.relocations"
	ActualFile="$WorkDirectory/relocation.txt"
	ExpectedFile="$WorkDirectory/relocation.expected"
	printf '%s\n' \
		'format ELF64' \
		'machine Advanced Micro Devices X86-64' \
		'section .text PROGBITS' \
		'section .data PROGBITS' \
		'section .rela.text RELA' \
		'section .rela.data RELA' \
		'symbol GLOBAL UND external_function' \
		'symbol GLOBAL UND external_data' \
		'symbol GLOBAL 1 aasm_relocation_entry' \
		'relocation R_X86_64_PC32 external_function - 4' \
		'relocation R_X86_64_PC32 external_data - 4' \
		'relocation R_X86_64_PC32 .data - 4' \
		'relocation R_X86_64_64 external_data + 0' > "$ExpectedFile"

	"$AasmExecutable" -Ox -f elf64 -o "$ObjectFile" "$TestDirectory/relocation.asm" \
		>"$WorkDirectory/relocation.stdout" 2>"$WorkDirectory/relocation.stderr" || \
		Fail "relocation input did not assemble"

	[ ! -s "$WorkDirectory/relocation.stderr" ] || Fail "relocation input produced diagnostics"

	LC_ALL=C readelf -hW "$ObjectFile" > "$HeaderFile"
	LC_ALL=C readelf -SW "$ObjectFile" > "$SectionFile"
	LC_ALL=C readelf -sW "$ObjectFile" > "$SymbolFile"
	LC_ALL=C readelf -rW "$ObjectFile" > "$RelocationFile"

	grep -Fq 'Class:                             ELF64' "$HeaderFile" || Fail "ELF class differs"
	grep -Fq 'Machine:                           Advanced Micro Devices X86-64' "$HeaderFile" || Fail "ELF machine differs"
	grep -Eq '[[:space:]]\.text[[:space:]]+PROGBITS' "$SectionFile" || Fail "missing .text section"
	grep -Eq '[[:space:]]\.data[[:space:]]+PROGBITS' "$SectionFile" || Fail "missing .data section"
	grep -Eq '[[:space:]]\.rela\.text[[:space:]]+RELA' "$SectionFile" || Fail "missing .rela.text section"
	grep -Eq '[[:space:]]\.rela\.data[[:space:]]+RELA' "$SectionFile" || Fail "missing .rela.data section"

	{
		printf '%s\n' 'format ELF64'
		printf '%s\n' 'machine Advanced Micro Devices X86-64'
		printf '%s\n' 'section .text PROGBITS'
		printf '%s\n' 'section .data PROGBITS'
		printf '%s\n' 'section .rela.text RELA'
		printf '%s\n' 'section .rela.data RELA'
		awk '$8 == "external_function" || $8 == "external_data" || $8 == "aasm_relocation_entry" { print "symbol", $5, $7, $8 }' "$SymbolFile"
		awk '$3 ~ /^R_X86_64_/ { print "relocation", $3, $5, $6, $7 }' "$RelocationFile"
	} > "$ActualFile"

	if ! cmp -s "$ExpectedFile" "$ActualFile"; then
		diff -u "$ExpectedFile" "$ActualFile" >&2
		Fail "ELF relocation summary differs"
	fi
	Pass "ELF sections, symbols, and relocations"
}

CheckDebugOutput()
{
	ObjectFile="$WorkDirectory/debug.o"
	SectionFile="$WorkDirectory/debug.sections"

	"$AasmExecutable" -Ox -f elf64 -g -F dwarf -o "$ObjectFile" \
		"$TestDirectory/basic.asm" \
		>"$WorkDirectory/debug.stdout" 2>"$WorkDirectory/debug.stderr" || \
		Fail "debug input did not assemble"

	[ ! -s "$WorkDirectory/debug.stderr" ] || \
		Fail "debug input produced diagnostics"

	LC_ALL=C readelf -SW "$ObjectFile" > "$SectionFile" 2> /dev/null
	grep -Eq '[[:space:]]\.debug_info[[:space:]]+PROGBITS' "$SectionFile" || \
		Fail "missing .debug_info section"
	grep -Eq '[[:space:]]\.debug_line[[:space:]]+PROGBITS' "$SectionFile" || \
		Fail "missing .debug_line section"
	grep -Eq '[[:space:]]\.rela\.debug_line[[:space:]]+RELA' "$SectionFile" || \
		Fail "missing .rela.debug_line section"
	Pass "ELF debug output"
}

CheckListing()
{
	ObjectFile="$WorkDirectory/listing.o"
	ListingFile="$WorkDirectory/listing.lst"

	"$AasmExecutable" -Ox -f elf64 -l "$ListingFile" -o "$ObjectFile" \
		"$TestDirectory/relocation.asm" \
		>"$WorkDirectory/listing.stdout" 2>"$WorkDirectory/listing.stderr" || \
		Fail "listing input did not assemble"

	[ ! -s "$WorkDirectory/listing.stderr" ] || \
		Fail "listing input produced diagnostics"
	grep -Eq 'E8\(00000000\).*call external_function' "$ListingFile" || \
		Fail "relative call listing differs"
	grep -Eq '488B05\(00000000\).*mov rax' "$ListingFile" || \
		Fail "relative address listing differs"
	Pass "listing output"
}

CheckInvalidAddress()
{
	OutputFile="$WorkDirectory/invalid-address.bin"
	RawFile="$WorkDirectory/invalid-address.raw"
	ActualFile="$WorkDirectory/invalid-address.txt"
	ExpectedFile="$WorkDirectory/invalid-address.expected"
	printf '%s\n' \
		'invalid-address.asm:6: error: impossible combination of address sizes' \
		'invalid-address.asm:6: error: invalid effective address' > "$ExpectedFile"

	"$AasmExecutable" -f bin -o "$OutputFile" "$TestDirectory/invalid-address.asm" \
		>"$WorkDirectory/invalid-address.stdout" 2>"$RawFile"
	ResultCode="$?"

	[ "$ResultCode" -ne 0 ] || Fail "invalid address was accepted"
	[ ! -e "$OutputFile" ] || Fail "invalid address left an output file"

	tr -d '\r' < "$RawFile" | \
		sed -E 's/\x1B\[[0-9;]*[mK]//g' | \
		sed -n 's|^.*invalid-address\.asm:|invalid-address.asm:|p' > "$ActualFile"

	if ! cmp -s "$ExpectedFile" "$ActualFile"; then
		diff -u "$ExpectedFile" "$ActualFile" >&2
		Fail "invalid-address diagnostics differ"
	fi
	Pass "invalid-address rejection"
}

CheckCodeends()
{
	FormatOutput="$WorkDirectory/formats.txt"
	FormatError="$WorkDirectory/formats.stderr"
	DebugOutput="$WorkDirectory/debug-formats.txt"
	TextOutput="$WorkDirectory/basic.ith"
	InvalidError="$WorkDirectory/invalid-format.stderr"

	"$AasmExecutable" -hf > "$FormatOutput" 2> "$FormatError" || \
		Fail "output format listing failed"
	[ ! -s "$FormatError" ] || Fail "output format listing produced diagnostics"
	grep -Eq '^  \* bin[[:space:]]' "$FormatOutput" || \
		Fail "default output format differs"
	grep -Eq '^    elf64[[:space:]]' "$FormatOutput" || \
		Fail "ELF64 output format is missing"

	"$AasmExecutable" -f elf64 -y > "$DebugOutput" 2> "$FormatError" || \
		Fail "debug format listing failed"
	[ ! -s "$FormatError" ] || Fail "debug format listing produced diagnostics"
	grep -Fq "valid debug formats for 'elf64'" "$DebugOutput" || \
		Fail "debug format listing selected the wrong Codeend"

	"$AasmExecutable" -Ox -f ith -o "$TextOutput" "$TestDirectory/basic.asm" \
		>"$WorkDirectory/text-format.stdout" 2> "$FormatError" || \
		Fail "text Codeend output failed"
	[ ! -s "$FormatError" ] || Fail "text Codeend output produced diagnostics"
	grep -Eq '^:[0-9A-F]+$' "$TextOutput" || Fail "text Codeend output differs"

	"$AasmExecutable" -f invalid-codeend "$TestDirectory/basic.asm" \
		>"$WorkDirectory/invalid-format.stdout" 2> "$InvalidError"
	ResultCode="$?"
	[ "$ResultCode" -ne 0 ] || Fail "invalid output format was accepted"
	grep -Fq "unrecognised output format \`invalid-codeend'" "$InvalidError" || \
		Fail "invalid output format diagnostic differs"
	Pass "Codeend selection"
}

CheckArchends()
{
	ArchitectureOutput="$WorkDirectory/architectures.txt"
	ArchitectureError="$WorkDirectory/architectures.stderr"
	DefaultOutput="$WorkDirectory/architecture-default.bin"
	MarchOutput="$WorkDirectory/architecture-march.bin"
	LongOutput="$WorkDirectory/architecture-long.bin"
	InvalidError="$WorkDirectory/invalid-architecture.stderr"

	"$AasmExecutable" -ha > "$ArchitectureOutput" 2> "$ArchitectureError" || \
		Fail "architecture listing failed"
	[ ! -s "$ArchitectureError" ] || \
		Fail "architecture listing produced diagnostics"
	grep -Eq '^  \* x86[[:space:]]' "$ArchitectureOutput" || \
		Fail "default architecture differs"

	"$AasmExecutable" -Ox -f bin -o "$DefaultOutput" \
		"$TestDirectory/basic.asm" \
		>"$WorkDirectory/architecture-default.stdout" 2> "$ArchitectureError" || \
		Fail "default architecture did not assemble"
	[ ! -s "$ArchitectureError" ] || \
		Fail "default architecture produced diagnostics"

	"$AasmExecutable" -march=x86 -Ox -f bin -o "$MarchOutput" \
		"$TestDirectory/basic.asm" \
		>"$WorkDirectory/architecture-march.stdout" 2> "$ArchitectureError" || \
		Fail "-march architecture did not assemble"
	[ ! -s "$ArchitectureError" ] || \
		Fail "-march architecture produced diagnostics"

	"$AasmExecutable" --arch x86 -Ox -f bin -o "$LongOutput" \
		"$TestDirectory/basic.asm" \
		>"$WorkDirectory/architecture-long.stdout" 2> "$ArchitectureError" || \
		Fail "--arch architecture did not assemble"
	[ ! -s "$ArchitectureError" ] || \
		Fail "--arch architecture produced diagnostics"

	cmp -s "$DefaultOutput" "$MarchOutput" || \
		Fail "-march changed x86 output"
	cmp -s "$DefaultOutput" "$LongOutput" || \
		Fail "--arch changed x86 output"

	"$AasmExecutable" -march=invalid-architecture \
		"$TestDirectory/basic.asm" \
		>"$WorkDirectory/invalid-architecture.stdout" 2> "$InvalidError"
	ResultCode="$?"
	[ "$ResultCode" -ne 0 ] || Fail "invalid architecture was accepted"
	grep -Fq "unrecognised architecture \`invalid-architecture'" \
		"$InvalidError" || Fail "invalid architecture diagnostic differs"
	Pass "Archend selection"
}

CheckCommonStatements()
{
	SourceFile="$WorkDirectory/common-statements.asm"
	InputFile="$WorkDirectory/common-statements.input"
	OutputFile="$WorkDirectory/common-statements.bin"
	ExpectedFile="$WorkDirectory/common-statements.expected"
	ActualHexFile="$WorkDirectory/common-statements.hex"
	ExpectedHexFile="$WorkDirectory/common-statements.expected.hex"

	printf '\x10\x20\x30\x40\x50' > "$InputFile"
	printf '\x03\x04\x34\x12\x34\x12\x88\x77\x66\x55\x08\x07\x06\x05\x04\x03\x02\x01' > "$ExpectedFile"
	printf '\x54\x00\x00\x00\x00\x00\x00\x00\x00\x00' >> "$ExpectedFile"
	printf '\x4f\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' >> "$ExpectedFile"
	printf '\x59\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' >> "$ExpectedFile"
	printf '\x20\x30\x40' >> "$ExpectedFile"
	printf '%s\n' \
		'bits 64' \
		'section .data' \
		'constant equ 3' \
		'db constant' \
		'eax: equ 4' \
		'db eax' \
		'times 2 dw 0x1234' \
		'dd 0x55667788' \
		'dq 0x0102030405060708' \
		'dt "T"' \
		'do "O"' \
		'dy "Y"' \
		"incbin \"$InputFile\", 1, 3" \
		'section .bss' \
		'resb constant' > "$SourceFile"

	"$AasmExecutable" -Ox -f bin -o "$OutputFile" "$SourceFile" \
		>"$WorkDirectory/common-statements.stdout" \
		2>"$WorkDirectory/common-statements.stderr" || \
		Fail "common statements did not assemble"
	[ ! -s "$WorkDirectory/common-statements.stderr" ] || \
		Fail "common statements produced diagnostics"

	if ! cmp -s "$ExpectedFile" "$OutputFile"; then
		NormalizeHex "$ExpectedFile" > "$ExpectedHexFile"
		NormalizeHex "$OutputFile" > "$ActualHexFile"
		diff -u "$ExpectedHexFile" "$ActualHexFile" >&2
		Fail "common statement bytes differ"
	fi
	Pass "common statement bytes"
}

CheckPrefixOnly()
{
	SourceFile="$WorkDirectory/prefix-only.asm"
	OutputFile="$WorkDirectory/prefix-only.bin"
	ExpectedFile="$WorkDirectory/prefix-only.expected"

	printf '%s\n' \
		'bits 64' \
		'section .text' \
		'lock' \
		'rep' \
		'o16' \
		'times 2 lock' > "$SourceFile"
	printf '\xf0\xf3\x66\xf0\xf0' > "$ExpectedFile"

	"$AasmExecutable" -Ox -f bin -o "$OutputFile" "$SourceFile" \
		>"$WorkDirectory/prefix-only.stdout" \
		2>"$WorkDirectory/prefix-only.stderr" || \
		Fail "prefix-only input did not assemble"
	[ ! -s "$WorkDirectory/prefix-only.stderr" ] || \
		Fail "prefix-only input produced diagnostics"
	cmp -s "$ExpectedFile" "$OutputFile" || Fail "prefix-only bytes differ"
	Pass "prefix-only bytes"
}

CheckHex basic '48 b8 01 00 00 00 00 00 00 00 48 83 c0 02 48 83 f8 03 75 03 31 c0 c3 b8 01 00 00 00 c3 00 00 00 ef cd ab 89 67 45 23 01' -Ox
CheckHex encoding '48 b8 88 77 66 55 44 33 22 11 49 89 c0 49 83 c1 7f 49 83 ea ff 31 c0 49 d1 e3 41 54 41 5d c3' -Ox
CheckHex addressing '48 8b 03 48 8b 4d 00 4b 8b 94 ec 34 12 00 00 44 89 46 07 4c 8d 0d 01 00 00 00 c3 11 22 33 44 55 66 77 88' -Ox
CheckHex symbols '11 01 00 03 00 00 00 12 00 00 00 00 00 00 00 aa aa aa 12 2a' -Ox
CheckHex data '41 41 53 4d 00 34 12 ef cd ab 89 ef cd ab 89 67 45 23 01 5a 5a 5a 5a cc cc cc cc cc cc cc cc cc' -Ox
CheckHex prefix 'f0 ff 00 f3 a4 64 48 8b 04 25 20 00 00 00 66 89 d8 67 8b 40 04 c3' -Ox
CheckHex structure '8b 45 01 00 7a 78 56 34 12' -Ox
CheckBranch
CheckPreprocessor
CheckRelocation
CheckDebugOutput
CheckListing
CheckInvalidAddress
CheckCodeends
CheckArchends
CheckCommonStatements
CheckPrefixOnly

printf '\033[1;32mall AASM x86-64 regression checks passed\033[0m\n'

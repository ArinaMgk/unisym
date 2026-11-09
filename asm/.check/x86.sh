#!/bin/bash
# ASCII ShellScript TAB4 LF
# Docutitle: AASM x86 Regression Checks
# Copyright: UNISYM, under Apache License 2.0

CheckHex()
{
	TestName="$1"
	ExpectedHex="$2"
	shift 2
	OutputFile="$WorkDirectory/$TestName.bin"
	ActualFile="$WorkDirectory/$TestName.hex"

	AssembleSuccess "$TestName" "$@" -f bin -o "$OutputFile" \
		"$TestDirectory/$TestName.asm"

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

CheckBinaryOutputChain()
{
	OutputFile="$WorkDirectory/binary-output.bin"
	ActualFile="$WorkDirectory/binary-output.hex"
	ExpectedFile="$WorkDirectory/binary-output.expected"
	UnresolvedOutput="$WorkDirectory/binary-unresolved.bin"
	UnresolvedError="$WorkDirectory/binary-unresolved.stderr"
	ExpectedHex='30 10 00 00 00 00 00 00 48 8d 05 21 00 00 00 e8 0c 00 00 00 e9 08 00 00 00 5a 00 00 00 00 00 00 c3 c3 00 00 00 00 00 00 00 00 00 00 00 00 00 00 78 56 34 12 40 10 00 00 00 00 00 00'

	"$AasmExecutable" -Ox -f bin -o "$OutputFile" \
		"$TestDirectory/binary-output.asm" \
		>"$WorkDirectory/binary-output.stdout" \
		2>"$WorkDirectory/binary-output.stderr" || \
		Fail "Binary output chain did not assemble"
	[ ! -s "$WorkDirectory/binary-output.stderr" ] || \
		Fail "Binary output chain produced diagnostics"

	NormalizeHex "$OutputFile" > "$ActualFile"
	printf '%s\n' "$ExpectedHex" > "$ExpectedFile"
	if ! cmp -s "$ExpectedFile" "$ActualFile"; then
		diff -u "$ExpectedFile" "$ActualFile" >&2
		Fail "Binary output chain bytes differ"
	fi

	"$AasmExecutable" -Ox -f bin -o "$UnresolvedOutput" \
		"$TestDirectory/relocation.asm" \
		>"$WorkDirectory/binary-unresolved.stdout" 2>"$UnresolvedError"
	ResultCode="$?"
	[ "$ResultCode" -ne 0 ] || Fail "Binary accepted unresolved symbols"
	[ ! -e "$UnresolvedOutput" ] || \
		Fail "Binary unresolved symbols left an output file"
	ReferenceErrors="$(grep -Fc \
		'binary output format does not support external references' \
		"$UnresolvedError")"
	[ "$ReferenceErrors" -eq 3 ] || \
		Fail "Binary unresolved-symbol diagnostics differ"
	Pass "Binary raw data, reserve, layout, and local fixups"
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
	RawFile="$WorkDirectory/invalid-address.stderr"
	ActualFile="$WorkDirectory/invalid-address.txt"
	ExpectedFile="$WorkDirectory/invalid-address.expected"
	printf '%s\n' \
		'invalid-address.asm:6: error: impossible combination of address sizes' \
		'invalid-address.asm:6: error: invalid effective address' > "$ExpectedFile"

	AssembleFailure invalid-address -f bin -o "$OutputFile" \
		"$TestDirectory/invalid-address.asm"
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

	AssembleSuccess prefix-only -Ox -f bin -o "$OutputFile" "$SourceFile"
	cmp -s "$ExpectedFile" "$OutputFile" || Fail "prefix-only bytes differ"
	Pass "prefix-only bytes"
}


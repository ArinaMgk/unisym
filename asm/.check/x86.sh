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

CheckDisassembler()
{
	DasmExecutable="${DASM:-$(dirname "$AasmExecutable")/dasm}"
	[ -x "$DasmExecutable" ] || Fail "disassembler is not executable: $DasmExecutable"

	HelpFile="$WorkDirectory/dasm-help.txt"
	ArchitectureFile="$WorkDirectory/dasm-architectures.txt"
	FormatFile="$WorkDirectory/dasm-formats.txt"
	"$DasmExecutable" -h > "$HelpFile" || Fail "dasm -h failed"
	"$DasmExecutable" -ha > "$ArchitectureFile" || Fail "dasm -ha failed"
	"$DasmExecutable" -hf > "$FormatFile" || Fail "dasm -hf failed"
	grep -Fq 'Supported disassembly architectures' "$HelpFile" ||
		Fail "dasm help omits architectures"
	grep -Fq 'Supported input formats' "$HelpFile" ||
		Fail "dasm help omits input formats"
	grep -Fq 'Codeend output formats' "$HelpFile" ||
		Fail "dasm help omits Codeend information"
	grep -Fq 'x86' "$ArchitectureFile" ||
		Fail "dasm architecture list omits x86"
	grep -Fq 'bin' "$FormatFile" ||
		Fail "dasm format list omits bin"

	Input16="$WorkDirectory/dasm16.bin"
	Input32="$WorkDirectory/dasm32.bin"
	Input64="$WorkDirectory/dasm64.bin"
	Output16="$WorkDirectory/dasm16.txt"
	Output32="$WorkDirectory/dasm32.txt"
	Output64="$WorkDirectory/dasm64.txt"
	printf '\xb8\x34\x12\xc3' > "$Input16"
	printf '\xb8\x78\x56\x34\x12\xc3' > "$Input32"
	printf '\x48\x89\xd8\xc3' > "$Input64"
	"$DasmExecutable" -f bin -march=x86 -b16 "$Input16" > "$Output16" ||
		Fail "dasm 16-bit input failed"
	"$DasmExecutable" -f bin --arch x86 -b32 "$Input32" > "$Output32" ||
		Fail "dasm 32-bit input failed"
	"$DasmExecutable" -f bin -march=x86 -b64 "$Input64" > "$Output64" ||
		Fail "dasm 64-bit input failed"
	grep -Fq 'mov ax,0x1234' "$Output16" || Fail "dasm 16-bit output differs"
	grep -Fq 'mov eax,0x12345678' "$Output32" || Fail "dasm 32-bit output differs"
	grep -Fq 'mov rax,rbx' "$Output64" || Fail "dasm 64-bit output differs"

	TruncatedOpcode="$WorkDirectory/dasm-truncated-opcode.bin"
	TruncatedAddress="$WorkDirectory/dasm-truncated-address.bin"
	TruncatedOpcodeOutput="$WorkDirectory/dasm-truncated-opcode.txt"
	TruncatedAddressOutput="$WorkDirectory/dasm-truncated-address.txt"
	printf '\x0f' > "$TruncatedOpcode"
	printf '\x8b\x84\x25' > "$TruncatedAddress"
	"$DasmExecutable" -b64 "$TruncatedOpcode" > "$TruncatedOpcodeOutput" ||
		Fail "dasm truncated opcode failed"
	"$DasmExecutable" -b64 "$TruncatedAddress" > "$TruncatedAddressOutput" ||
		Fail "dasm truncated effective address failed"
	grep -Fq 'db 0x0f' "$TruncatedOpcodeOutput" ||
		Fail "dasm truncated opcode fallback differs"
	grep -Fq 'db 0x8b' "$TruncatedAddressOutput" ||
		Fail "dasm truncated effective-address fallback differs"

	BoundaryInput="$WorkDirectory/dasm-prefix-boundary.bin"
	BoundaryOutput="$WorkDirectory/dasm-prefix-boundary.txt"
	printf '\x66\x66\x66\x66\x66\x66\x66\x66\x66\x66\x66\x66\x66\x90\xc3' \
		> "$BoundaryInput"
	"$DasmExecutable" -b64 "$BoundaryInput" > "$BoundaryOutput" ||
		Fail "dasm instruction-length boundary failed"
	grep -Fq 'xchg ax,ax' "$BoundaryOutput" ||
		Fail "dasm instruction-length boundary differs"
	grep -Fq 'ret' "$BoundaryOutput" ||
		Fail "dasm instruction after boundary is missing"

	VendorInput="$WorkDirectory/dasm-vendor.bin"
	VendorOutput="$WorkDirectory/dasm-vendor.txt"
	printf '\x0f\x0d\x00' > "$VendorInput"
	for Vendor in intel amd cyrix; do
		"$DasmExecutable" -b32 -p "$Vendor" "$VendorInput" \
			> "$VendorOutput" || Fail "dasm vendor $Vendor failed"
		grep -Fq 'prefetch [eax]' "$VendorOutput" ||
			Fail "dasm vendor $Vendor output differs"
	done

	InvalidError="$WorkDirectory/dasm-invalid.stderr"
	"$DasmExecutable" -binvalid "$Input16" > /dev/null 2> "$InvalidError"
	[ "$?" -ne 0 ] || Fail "dasm accepted a malformed bit width"
	grep -Fq 'invalid bit width `invalid' "$InvalidError" ||
		Fail "dasm bit-width diagnostic differs"
	"$DasmExecutable" -b20 "$Input16" > /dev/null 2> "$InvalidError"
	[ "$?" -ne 0 ] || Fail "dasm accepted an unsupported bit width"
	grep -Fq "architecture \`x86' does not support 20-bit code" \
		"$InvalidError" || Fail "dasm unsupported bit-width diagnostic differs"
	"$DasmExecutable" -march=riscv "$Input16" > /dev/null 2> "$InvalidError"
	[ "$?" -ne 0 ] || Fail "dasm accepted an unsupported architecture"
	grep -Fq 'unsupported disassembly architecture `riscv' "$InvalidError" ||
		Fail "dasm architecture diagnostic differs"
	Pass "x86 disassembler interface and options"
}


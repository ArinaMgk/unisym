#!/bin/bash
# ASCII ShellScript TAB4 LF
# Docutitle: AASM Codeend Regression Checks
# Copyright: UNISYM, under Apache License 2.0

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

	AssembleFailure invalid-format -f invalid-codeend "$TestDirectory/basic.asm"
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
	grep -Eq '^    riscv[[:space:]]' "$ArchitectureOutput" || \
		Fail "RISC-V architecture is missing"

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

	AssembleFailure invalid-architecture -march=invalid-architecture \
		"$TestDirectory/basic.asm"
	grep -Fq "unrecognised architecture \`invalid-architecture'" \
		"$InvalidError" || Fail "invalid architecture diagnostic differs"
	Pass "Archend selection"
}


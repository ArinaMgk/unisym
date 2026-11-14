#!/bin/bash
# ASCII ShellScript TAB4 LF
# Docutitle: AASM Codeend Regression Checks
# Copyright: UNISYM, under Apache License 2.0

CheckCodeends()
{
	FormatOutput="$WorkDirectory/formats.txt"
	FormatError="$WorkDirectory/formats.stderr"
	HelpOutput="$WorkDirectory/help.txt"
	DebugOutput="$WorkDirectory/debug-formats.txt"
	TextOutput="$WorkDirectory/basic.ith"
	InvalidError="$WorkDirectory/invalid-format.stderr"

	"$AasmExecutable" -h > "$HelpOutput" 2> "$FormatError" || \
		Fail "general help listing failed"
	[ ! -s "$FormatError" ] || Fail "general help listing produced diagnostics"
	grep -Fq 'Supported architectures and INST/CPU choices:' "$HelpOutput" || \
		Fail "general help omits architecture and INST summary"
	grep -Fq -- '-ha         list architectures, BITS, and INST/CPU choices' "$HelpOutput" || \
		Fail "general help omits the architecture-list option"

	"$AasmExecutable" -hf > "$FormatOutput" 2> "$FormatError" || \
		Fail "output format listing failed"
	[ ! -s "$FormatError" ] || Fail "output format listing produced diagnostics"
	grep -Eq '^  \* bin[[:space:]]' "$FormatOutput" || \
		Fail "default output format differs"
	grep -Eq '^      Architectures: x86, riscv, arm32, arm64$' "$FormatOutput" || \
		Fail "binary architecture support listing differs"
	grep -Eq '^    elf64[[:space:]]' "$FormatOutput" || \
		Fail "ELF64 output format is missing"
	grep -Eq '^      Architectures: x86, riscv$' "$FormatOutput" || \
		Fail "ELF64 architecture support listing differs"

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
	ARM32Source="$WorkDirectory/architecture-arm32.asm"
	ARM32Output="$WorkDirectory/architecture-arm32.bin"
	ARM64Source="$WorkDirectory/architecture-arm64.asm"
	ARM64Output="$WorkDirectory/architecture-arm64.bin"
	ARMMixedSource="$WorkDirectory/architecture-arm-mixed.asm"
	ARMMixedOutput="$WorkDirectory/architecture-arm-mixed.bin"
	ARMInvalidBitsSource="$WorkDirectory/architecture-arm-invalid-bits.asm"
	ARMELFSource="$WorkDirectory/architecture-arm-elf.asm"
	InvalidError="$WorkDirectory/invalid-architecture.stderr"

	"$AasmExecutable" -ha > "$ArchitectureOutput" 2> "$ArchitectureError" || \
		Fail "architecture listing failed"
	[ ! -s "$ArchitectureError" ] || \
		Fail "architecture listing produced diagnostics"
	grep -Eq '^  \* x86[[:space:]]' "$ArchitectureOutput" || \
		Fail "default architecture differs"
	grep -Eq '^    riscv[[:space:]]' "$ArchitectureOutput" || \
		Fail "RISC-V architecture is missing"
	grep -Eq '^    arm32[[:space:]]' "$ArchitectureOutput" || \
		Fail "ARM32 architecture is missing"
	grep -Eq '^    arm64[[:space:]]' "$ArchitectureOutput" || \
		Fail "ARM64 architecture is missing"
	grep -Fq 'INST: CPU levels: 8086, 186, 286, 386, 486, 586, 686, P3, P4, Prescott, x64, IA64' "$ArchitectureOutput" || \
		Fail "x86 instruction selection summary is missing"
	grep -Fq 'INST: I, M, A, F, D, C, V, Zicsr, Zifencei (combine in canonical order; default I)' "$ArchitectureOutput" || \
		Fail "RISC-V instruction-set summary is missing"
	grep -Fq 'INST: none (instruction encoding not implemented)' "$ArchitectureOutput" || \
		Fail "unimplemented ARM instruction status is missing"

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

	printf '%s\n' \
		'[BITS 32]' \
		'%ifidni __ARCH__,arm32' \
		'DB 0x32' \
		'%else' \
		'DB 0' \
		'%endif' > "$ARM32Source"
	printf '%s\n' \
		'[BITS 64]' \
		'%ifidni __ARCH__,arm64' \
		'DB 0x64' \
		'%else' \
		'DB 0' \
		'%endif' > "$ARM64Source"
	AssembleSuccess architecture-arm32 -march=arm32 -f bin \
		-o "$ARM32Output" "$ARM32Source"
	AssembleSuccess architecture-arm64 -march=arm64 -f bin \
		-o "$ARM64Output" "$ARM64Source"
	[ "$(NormalizeHex "$ARM32Output")" = '32' ] || \
		Fail "ARM32 architecture macro differs"
	[ "$(NormalizeHex "$ARM64Output")" = '64' ] || \
		Fail "ARM64 architecture macro differs"

	printf '%s\n' \
		'[ARCH arm32]' \
		'[BITS 32]' \
		'%ifidni __ARCH__,arm32' \
		'DB 0x32' \
		'%endif' \
		'[ARCH arm64]' \
		'[BITS 64]' \
		'%ifidni __ARCH__,arm64' \
		'DB 0x64' \
		'%endif' > "$ARMMixedSource"
	AssembleSuccess architecture-arm-mixed -f bin \
		-o "$ARMMixedOutput" "$ARMMixedSource"
	[ "$(NormalizeHex "$ARMMixedOutput")" = '32 64' ] || \
		Fail "ARM architecture switching differs"

	printf '%s\n' '[BITS 64]' 'DB 0' > "$ARMInvalidBitsSource"
	AssembleFailure architecture-arm32-invalid-bits -march=arm32 -f bin \
		-o "$WorkDirectory/architecture-arm32-invalid-bits.bin" \
		"$ARMInvalidBitsSource"
	grep -Fq 'cannot specify 64-bit mode for the selected architecture or cpu' \
		"$WorkDirectory/architecture-arm32-invalid-bits.stderr" || \
		Fail "ARM32 invalid bit-width diagnostic differs"
	printf '%s\n' '[BITS 32]' 'DB 0' > "$ARMInvalidBitsSource"
	AssembleFailure architecture-arm64-invalid-bits -march=arm64 -f bin \
		-o "$WorkDirectory/architecture-arm64-invalid-bits.bin" \
		"$ARMInvalidBitsSource"
	grep -Fq 'cannot specify 32-bit mode for the selected architecture or cpu' \
		"$WorkDirectory/architecture-arm64-invalid-bits.stderr" || \
		Fail "ARM64 invalid bit-width diagnostic differs"

	printf '%s\n' 'DB 0' > "$ARMELFSource"
	AssembleFailure architecture-arm32-elf -march=arm32 -f elf32 \
		-o "$WorkDirectory/architecture-arm32.o" "$ARMELFSource"
	grep -Fq "output format \`elf32' does not support architecture \`arm32'" \
		"$WorkDirectory/architecture-arm32-elf.stderr" || \
		Fail "ARM32 ELF compatibility diagnostic differs"
	AssembleFailure architecture-arm64-elf -march=arm64 -f elf64 \
		-o "$WorkDirectory/architecture-arm64.o" "$ARMELFSource"
	grep -Fq "output format \`elf64' does not support architecture \`arm64'" \
		"$WorkDirectory/architecture-arm64-elf.stderr" || \
		Fail "ARM64 ELF compatibility diagnostic differs"

	AssembleFailure invalid-architecture -march=invalid-architecture \
		"$TestDirectory/basic.asm"
	grep -Fq "unrecognised architecture \`invalid-architecture'" \
		"$InvalidError" || Fail "invalid architecture diagnostic differs"
	Pass "Archend selection"
}


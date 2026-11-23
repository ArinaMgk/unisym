#!/bin/bash
# ASCII ShellScript TAB4 LF
# Docutitle: AASM ARM64 Regression Checks
# Copyright: UNISYM, under Apache License 2.0

CheckARM64()
{
	OutputFile="$WorkDirectory/arm64-basic.bin"
	CaseSource="$WorkDirectory/arm64-case.asm"
	CaseOutput="$WorkDirectory/arm64-case.bin"
	Expected='1f 20 03 d5 e0 03 01 aa 02 cf 8a 52 82 46 a2 72 03 00 80 92 04 00 01 8b 45 40 00 11 20 00 02 ab ff 07 00 b1 86 04 40 d1 83 20 00 f1 c5 00 07 9a 28 01 0a 5a df 00 04 eb bf 40 00 71 1f 00 01 ea 87 00 06 8a a8 00 02 2a e9 00 06 ca 8b 7d 0d 9b ee 0d d0 9a 51 0a d3 1a b4 22 d6 9a 17 27 d9 1a 7a 2b dc 9a dd 2f c0 9a e9 0b 00 f9 ea 0b 40 f9 e8 07 00 39 eb 07 40 39 ec 07 80 39 ed 07 40 39 ee 07 80 39 ea 0f 00 f9 40 00 00 54 0f 00 80 52 02 00 00 94 02 00 00 14 c0 03 5f d6 40 01 3f d6 20 01 1f d6 00 01 3f d6 c0 03 5f d6 20 80 5f f8 62 10 40 f8 a4 78 66 f8 fd 0f 1f f8 fd 7b c1 a8'

	AssembleSuccess arm64-basic -march=arm64 -f bin -o "$OutputFile" \
		"$RepositoryDirectory/demo/test/clover/arch-arm64/basic.asm"
	[ "$(NormalizeHex "$OutputFile")" = "$Expected" ] || \
		Fail "ARM64 minimal instruction bytes differ"
	printf '%s\n' \
		'[arch arm64]' \
		'[bits 64]' \
		'mov x0, x1' \
		'ret lr' > "$CaseSource"
	AssembleSuccess arm64-case -f bin -o "$CaseOutput" "$CaseSource"
	[ "$(NormalizeHex "$CaseOutput")" = 'e0 03 01 aa c0 03 5f d6' ] || \
		Fail "ARM64 case-insensitive instruction bytes differ"
	Pass "ARM64 minimal instruction encoding"
}

CheckARM64Fixups()
{
	SourceFile="$WorkDirectory/arm64-fixup.asm"
	OutputFile="$WorkDirectory/arm64-fixup.bin"
	AddressSource="$WorkDirectory/arm64-address-fixup.asm"
	AddressOutput="$WorkDirectory/arm64-address-fixup.bin"
	CallSource="$WorkDirectory/arm64-call-fixup.asm"
	CallOutput="$WorkDirectory/arm64-call-fixup.bin"
	Expected='03 00 00 14 00 00 00 90 00 90 00 91 03 00 00 94 e0 00 00 54 06 00 00 14 01 00 00 90 21 b0 00 91 c0 03 5f d6 00 00 00 00 00 00 00 00 1f 20 03 d5'
	printf '%s\n' \
		'[ARCH arm64]' \
		'[BITS 64]' \
		'[SECTION .text]' \
		'start:' \
		'B local' \
		'MOV X0, data_label' \
		'local:' \
		'BL worker' \
		'B.EQ data_code' \
		'JMP data_code' \
		'worker:' \
		'MOV X1, data_label + 8' \
		'RET' \
		'[SECTION .data]' \
		'data_label:' \
		'DQ 0' \
		'data_code:' \
		'NOP' > "$SourceFile"
	AssembleSuccess arm64-fixup -f bin -o "$OutputFile" "$SourceFile"
	[ "$(NormalizeHex "$OutputFile")" = "$Expected" ] || \
		Fail "ARM64 branch and address fixup bytes differ"
	printf '%s\n' \
		'[ARCH arm64]' \
		'[SECTION .text vstart=0x1000]' \
		'MOV X2, far_data + 0x678' \
		'[SECTION .data vstart=0x12345000]' \
		'far_data:' \
		'DQ 0' > "$AddressSource"
	AssembleSuccess arm64-address-fixup -f bin \
		-o "$AddressOutput" "$AddressSource"
	[ "$(NormalizeHex "$AddressOutput")" = \
		'22 1a 09 90 42 e0 19 91 00 00 00 00 00 00 00 00' ] || \
		Fail "ARM64 split page fixup bytes differ"
	printf '%s\n' \
		'[ARCH arm64]' \
		'[SECTION .text]' \
		'CALL target' \
		'[SECTION .other]' \
		'target:' \
		'RET' > "$CallSource"
	AssembleSuccess arm64-call-fixup -f bin -o "$CallOutput" "$CallSource"
	[ "$(NormalizeHex "$CallOutput")" = '01 00 00 94 c0 03 5f d6' ] || \
		Fail "ARM64 cross-section call fixup bytes differ"
	Pass "ARM64 branch and address fixups"
}

CheckARM64Addressing()
{
	SourceFile="$WorkDirectory/arm64-addressing.asm"
	OutputFile="$WorkDirectory/arm64-addressing.bin"
	InvalidFile="$WorkDirectory/arm64-addressing-invalid.asm"
	printf '%s\n' \
		'[ARCH arm64]' \
		'[BITS 64]' \
		'LDR X0, [X1 - 8]' \
		'LDUR X0, [X1 - 8]' \
		'STR W2, [SP - 4]' \
		'LDRB W3, [X4 - 1]' \
		'LDRSW X5, [X6 - 4]' \
		'LDR X7, [X8 + 32760]' \
		'STR X29, [SP - 16]!' \
		'LDR X0, [X1 + 8]!' \
		'STR W2, [X3], 4' \
		'LDRB W4, [X5], -1' \
		'LDR X0, [X1 + X2]' \
		'LDR X3, [X4 + X5, LSL 3]' \
		'LDR X6, [X7 + W8, UXTW]' \
		'LDR X9, [X10 + W11, SXTW 3]' \
		'STR W12, [SP + X13, LSL 2]' \
		'LDR X17, [X18 + X19, LSL 3]' \
		'LDUR X20, [X21 - 256]' \
		'LDUR X22, [X23 + 255]' \
		'STR X24, [X25 - 256]!' \
		'LDR X26, [X27], 255' \
		'STP X29, X30, [SP - 16]!' \
		'LDP X29, X30, [SP], 16' \
		'STP W0, W1, [X2 + 8]' \
		'LDP X3, X4, [X5 - 512]' \
		'LDP X6, X7, [X8 + 504]' \
		'MOV X0, BYT8 [X1 - 8]' \
		'MOV X2, BYT4 [X3 + X4, LSL 2]' \
		'MOV X5, BYT8 [SP], 8' \
		'MOV BYT8 [SP - 8]!, X6' > "$SourceFile"
	AssembleSuccess arm64-addressing -march=arm64 -f bin \
		-o "$OutputFile" "$SourceFile"
	Actual="$(NormalizeHex "$OutputFile")"
	[ "$Actual" = \
		'20 80 5f f8 20 80 5f f8 e2 c3 1f b8 83 f0 5f 38 c5 c0 9f b8 07 fd 7f f9 fd 0f 1f f8 20 8c 40 f8 62 44 00 b8 a4 f4 5f 38 20 68 62 f8 83 78 65 f8 e6 48 68 f8 49 d9 6b f8 ec 7b 2d b8 51 7a 73 f8 b4 02 50 f8 f6 f2 4f f8 38 0f 10 f8 7a f7 4f f8 fd 7b bf a9 fd 7b c1 a8 40 04 01 29 a3 10 60 a9 06 9d 5f a9 20 80 5f f8 62 78 64 b8 e5 87 40 f8 e6 8f 1f f8' ] || \
		Fail "ARM64 addressing encodings differ"
	printf '%s\n' \
		'[ARCH arm64]' \
		'LDR X0, [X1 + X2, LSL 2]' > "$InvalidFile"
	AssembleFailure arm64-addressing-invalid -f bin \
		-o "$WorkDirectory/arm64-addressing-invalid.bin" "$InvalidFile"
	grep -Fq 'ARM64 memory index shift must be zero or match the access size' \
		"$WorkDirectory/arm64-addressing-invalid.stderr" || \
		Fail "ARM64 register-offset shift diagnostic differs"
	printf '%s\n' \
		'[ARCH arm64]' \
		'LDP X0, X0, [X1]' > "$InvalidFile"
	AssembleFailure arm64-pair-overlap -f bin \
		-o "$WorkDirectory/arm64-pair-overlap.bin" "$InvalidFile"
	grep -Fq 'ARM64 LDP destination registers must be different' \
		"$WorkDirectory/arm64-pair-overlap.stderr" || \
		Fail "ARM64 pair-register diagnostic differs"
	printf '%s\n' \
		'[ARCH arm64]' \
		'LDR X0, [X0 + 8]!' > "$InvalidFile"
	AssembleFailure arm64-writeback-overlap -f bin \
		-o "$WorkDirectory/arm64-writeback-overlap.bin" "$InvalidFile"
	grep -Fq 'ARM64 load/store writeback base overlaps the data register' \
		"$WorkDirectory/arm64-writeback-overlap.stderr" || \
		Fail "ARM64 writeback overlap diagnostic differs"
	Pass "ARM64 load/store addressing modes"
}

CheckARM64ELF()
{
	RequireCommand llvm-readelf
	RequireCommand ld.lld
	SourceFile="$WorkDirectory/arm64-elf.asm"
	ProviderSource="$WorkDirectory/arm64-elf-provider.asm"
	ObjectFile="$WorkDirectory/arm64-elf.o"
	ProviderObject="$WorkDirectory/arm64-elf-provider.o"
	LinkedFile="$WorkDirectory/arm64-elf-linked"
	HeaderFile="$WorkDirectory/arm64-elf.header"
	RelocationFile="$WorkDirectory/arm64-elf.relocations"
	SymbolFile="$WorkDirectory/arm64-elf.symbols"
	printf '%s\n' \
		'[ARCH arm64]' \
		'[BITS 64]' \
		'[SECTION .text]' \
		'GLOBAL entry' \
		'EXTERN external_call' \
		'entry:' \
		'CALL external_call' \
		'B.EQ other' \
		'B other' \
		'MOV X0, data_item + 8' \
		'DQ data_item' \
		'NOP' \
		'[SECTION .text.other exec]' \
		'other:' \
		'RET' \
		'[SECTION .data]' \
		'data_item:' \
		'DQ other' > "$SourceFile"
	printf '%s\n' \
		'[ARCH arm64]' \
		'[SECTION .text]' \
		'GLOBAL external_call' \
		'external_call:' \
		'RET' > "$ProviderSource"
	AssembleSuccess arm64-elf -march=arm64 -f elf64 \
		-o "$ObjectFile" "$SourceFile"
	AssembleSuccess arm64-elf-provider -march=arm64 -f elf64 \
		-o "$ProviderObject" "$ProviderSource"
	llvm-readelf -h "$ObjectFile" > "$HeaderFile"
	llvm-readelf -r "$ObjectFile" > "$RelocationFile"
	llvm-readelf -s "$ObjectFile" > "$SymbolFile"
	grep -Fq 'Class:                             ELF64' "$HeaderFile" || \
		Fail "ARM64 ELF class differs"
	grep -Fq 'Machine:                           AArch64' "$HeaderFile" || \
		Fail "ARM64 ELF machine differs"
	for Relocation in R_AARCH64_CALL26 R_AARCH64_CONDBR19 \
		R_AARCH64_JUMP26 R_AARCH64_ADR_PREL_PG_HI21 \
		R_AARCH64_ADD_ABS_LO12_NC R_AARCH64_ABS64; do
		grep -Fq "$Relocation" "$RelocationFile" || \
			Fail "ARM64 ELF relocation is missing: $Relocation"
	done
	grep -Eq '[[:space:]]\$x$' "$SymbolFile" || \
		Fail "ARM64 code mapping symbol is missing"
	grep -Eq '[[:space:]]\$d$' "$SymbolFile" || \
		Fail "ARM64 data mapping symbol is missing"
	LinkSuccess arm64-elf-link ld.lld -m aarch64elf -e entry \
		-o "$LinkedFile" "$ObjectFile" "$ProviderObject"
	Pass "ARM64 ELF object output"
}

CheckARM64Diagnostics()
{
	InvalidSource="$WorkDirectory/arm64-invalid.asm"
	ExternalSource="$WorkDirectory/arm64-external.asm"
	RangeSource="$WorkDirectory/arm64-range.asm"
	AddressSource="$WorkDirectory/arm64-address.asm"
	AddressRangeSource="$WorkDirectory/arm64-address-range.asm"
	printf '%s\n' \
		'[ARCH arm64]' \
		'[BITS 64]' \
		'MOV X0, W1' \
		'ADD SP, SP, X0' \
		'LDR X0, [W1]' \
		'B [target]' \
		'target:' \
		'NOP' > "$InvalidSource"
	AssembleFailure arm64-invalid -f bin \
		-o "$WorkDirectory/arm64-invalid.bin" "$InvalidSource"
	grep -Fq 'ARM64 register widths must match' \
		"$WorkDirectory/arm64-invalid.stderr" || \
		Fail "ARM64 register-width diagnostic differs"
	grep -Fq 'ARM64 register ADD/SUB destination does not accept SP or WSP' \
		"$WorkDirectory/arm64-invalid.stderr" || \
		Fail "ARM64 register-form SP diagnostic differs"
	grep -Fq "ARM64 instruction operands do not match \`LDR'" \
		"$WorkDirectory/arm64-invalid.stderr" || \
		Fail "ARM64 memory-base diagnostic differs"
	grep -Fq "ARM64 instruction operands do not match \`B'" \
		"$WorkDirectory/arm64-invalid.stderr" || \
		Fail "ARM64 direct-branch address diagnostic differs"
	printf '%s\n' \
		'[ARCH arm64]' \
		'EXTERN external_target' \
		'BL external_target' > "$ExternalSource"
	AssembleFailure arm64-external -f bin \
		-o "$WorkDirectory/arm64-external.bin" "$ExternalSource"
	grep -Fq 'binary output format does not support external references' \
		"$WorkDirectory/arm64-external.stderr" || \
		Fail "ARM64 external-reference diagnostic differs"
	printf '%s\n' \
		'[ARCH arm64]' \
		'[SECTION .text]' \
		'B far_target' \
		'[SECTION .far vstart=0x08000000]' \
		'far_target:' \
		'NOP' > "$RangeSource"
	AssembleFailure arm64-range -f bin \
		-o "$WorkDirectory/arm64-range.bin" "$RangeSource"
	grep -Fq 'ARM64 branch target is out of range' \
		"$WorkDirectory/arm64-range.stderr" || \
		Fail "ARM64 branch-range diagnostic differs"
	printf '%s\n' \
		'[ARCH arm64]' \
		'target:' \
		'MOV W0, target' > "$AddressSource"
	AssembleFailure arm64-address -f bin \
		-o "$WorkDirectory/arm64-address.bin" "$AddressSource"
	grep -Fq 'ARM64 label address MOV requires an X general register' \
		"$WorkDirectory/arm64-address.stderr" || \
		Fail "ARM64 label-address register diagnostic differs"
	printf '%s\n' \
		'[ARCH arm64]' \
		'[SECTION .text]' \
		'MOV X0, far_data' \
		'[SECTION .far vstart=0x100000000]' \
		'far_data:' \
		'DQ 0' > "$AddressRangeSource"
	AssembleFailure arm64-address-range -f bin \
		-o "$WorkDirectory/arm64-address-range.bin" "$AddressRangeSource"
	grep -Fq 'ARM64 label address is out of range' \
		"$WorkDirectory/arm64-address-range.stderr" || \
		Fail "ARM64 label-address range diagnostic differs"
	Pass "ARM64 operand diagnostics"
}

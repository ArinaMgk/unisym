#!/bin/bash
# ASCII ShellScript TAB4 LF
# Docutitle: AASM ARM32 Regression Checks
# Copyright: UNISYM, under Apache License 2.0

CheckARM32()
{
	OutputFile="$WorkDirectory/arm32-basic.bin"
	CaseSource="$WorkDirectory/arm32-case.asm"
	CaseOutput="$WorkDirectory/arm32-case.bin"
	Expected='00 f0 20 e3 01 00 a0 e1 78 26 05 e3 34 22 41 e3 00 30 e0 e3 01 40 80 e0 10 50 94 e2 01 61 45 10 02 01 56 e3 06 70 04 e0 62 84 85 e1 06 90 27 e0 04 90 8d e5 04 a0 9d e5 01 80 cd e5 01 b0 dd e5 b2 80 4d e1 b2 c0 5d e1 d1 00 dd e1 f2 10 dd e1 01 c0 dd e5 d1 00 dd e1 b2 80 4d e1 00 00 00 0a 00 00 a0 e3 00 00 00 eb 00 00 00 ea 1e ff 2f e1 3a ff 2f e1 19 ff 2f e1 38 ff 2f e1 1e ff 2f e1'

	AssembleSuccess arm32-basic -march=arm32 -f bin -o "$OutputFile" \
		"$RepositoryDirectory/demo/test/clover/arch-arm32/basic.asm"
	[ "$(NormalizeHex "$OutputFile")" = "$Expected" ] || \
		Fail "ARM32 minimal instruction bytes differ"
	printf '%s\n' \
		'[arch arm32]' \
		'[bits 32]' \
		'mov r0, r1' \
		'add.eq.s r2, r2, r3' \
		'mov.eq r4, 0x12345678' \
		'ret' > "$CaseSource"
	AssembleSuccess arm32-case -f bin -o "$CaseOutput" "$CaseSource"
	[ "$(NormalizeHex "$CaseOutput")" = \
		'01 00 a0 e1 03 20 92 00 78 46 05 03 34 42 41 03 1e ff 2f e1' ] || \
		Fail "ARM32 case-insensitive instruction bytes differ"
	Pass "ARM32 minimal instruction encoding"
}

CheckARM32Fixups()
{
	SourceFile="$WorkDirectory/arm32-fixup.asm"
	OutputFile="$WorkDirectory/arm32-fixup.bin"
	AddressSource="$WorkDirectory/arm32-address-fixup.asm"
	AddressOutput="$WorkDirectory/arm32-address-fixup.bin"
	CallSource="$WorkDirectory/arm32-call-fixup.asm"
	CallOutput="$WorkDirectory/arm32-call-fixup.bin"
	Expected='01 00 00 ea 20 00 00 e3 00 00 40 e3 00 00 00 eb 03 00 00 ea 24 10 00 e3 00 10 40 e3 1e ff 2f e1 00 00 00 00 00 f0 20 e3'
	printf '%s\n' \
		'[ARCH arm32]' \
		'[BITS 32]' \
		'[SECTION .text]' \
		'start:' \
		'B local' \
		'MOV R0, data_label' \
		'local:' \
		'BL worker' \
		'JMP data_code' \
		'worker:' \
		'MOV R1, data_label + 4' \
		'RET' \
		'[SECTION .data]' \
		'data_label:' \
		'DD 0' \
		'data_code:' \
		'NOP' > "$SourceFile"
	AssembleSuccess arm32-fixup -f bin -o "$OutputFile" "$SourceFile"
	[ "$(NormalizeHex "$OutputFile")" = "$Expected" ] || \
		Fail "ARM32 branch and address fixup bytes differ"
	printf '%s\n' \
		'[ARCH arm32]' \
		'[SECTION .text]' \
		'MOV R2, far_data + 0x678' \
		'[SECTION .data vstart=0x12345000]' \
		'far_data:' \
		'DQ 0' > "$AddressSource"
	AssembleSuccess arm32-address-fixup -f bin \
		-o "$AddressOutput" "$AddressSource"
	[ "$(NormalizeHex "$AddressOutput")" = \
		'78 26 05 e3 34 22 41 e3 00 00 00 00 00 00 00 00' ] || \
		Fail "ARM32 split address fixup bytes differ"
	printf '%s\n' \
		'[ARCH arm32]' \
		'[SECTION .text]' \
		'CALL target' \
		'[SECTION .other]' \
		'target:' \
		'RET' > "$CallSource"
	AssembleSuccess arm32-call-fixup -f bin -o "$CallOutput" "$CallSource"
	[ "$(NormalizeHex "$CallOutput")" = 'ff ff ff eb 1e ff 2f e1' ] || \
		Fail "ARM32 cross-section call fixup bytes differ"
	Pass "ARM32 branch and address fixups"
}

CheckARM32Diagnostics()
{
	InvalidSource="$WorkDirectory/arm32-invalid.asm"
	InvalidError="$WorkDirectory/arm32-invalid.stderr"
	ExternalSource="$WorkDirectory/arm32-external.asm"
	RangeSource="$WorkDirectory/arm32-range.asm"
	AddressRangeSource="$WorkDirectory/arm32-address-range.asm"
	printf '%s\n' \
		'[ARCH arm32]' \
		'[BITS 32]' \
		'MOV.S R0, 1' \
		'ADD R15, R0, R1' \
		'LDR R0, [PC]' \
		'LDR R0, [R1 + 4096]' \
		'LDRH R0, [R1 + 256]' \
		'ADD.S.EQ R0, R1, R2' \
		'B [target]' \
		'target:' \
		'NOP' > "$InvalidSource"
	AssembleFailure arm32-invalid -f bin \
		-o "$WorkDirectory/arm32-invalid.bin" "$InvalidSource"
	grep -Fq 'ARM32 immediate MOV does not support .S' "$InvalidError" || \
		Fail "ARM32 immediate-MOV diagnostic differs"
	grep -Fq 'ARM32 data-processing destination does not accept PC' \
		"$InvalidError" || Fail "ARM32 PC destination diagnostic differs"
	grep -Fq 'ARM32 memory base does not accept PC' "$InvalidError" || \
		Fail "ARM32 PC memory-base diagnostic differs"
	[ "$(grep -Fc 'ARM32 load/store offset is out of range' \
		"$InvalidError")" -ge 2 ] || \
		Fail "ARM32 memory-offset diagnostics differ"
	grep -Fq 'ARM32 condition must appear once and before .S' \
		"$InvalidError" || Fail "ARM32 qualifier-order diagnostic differs"
	grep -Fq "ARM32 instruction operands do not match \`B'" \
		"$InvalidError" || Fail "ARM32 direct-branch address diagnostic differs"
	printf '%s\n' \
		'[ARCH arm32]' \
		'EXTERN external_target' \
		'B external_target' > "$ExternalSource"
	AssembleFailure arm32-external -f bin \
		-o "$WorkDirectory/arm32-external.bin" "$ExternalSource"
	grep -Fq 'binary output format does not support external references' \
		"$WorkDirectory/arm32-external.stderr" || \
		Fail "ARM32 external-reference diagnostic differs"
	printf '%s\n' \
		'[ARCH arm32]' \
		'[SECTION .text]' \
		'B far_target' \
		'[SECTION .far vstart=0x02000008]' \
		'far_target:' \
		'NOP' > "$RangeSource"
	AssembleFailure arm32-range -f bin \
		-o "$WorkDirectory/arm32-range.bin" "$RangeSource"
	grep -Fq 'ARM32 branch target is out of range' \
		"$WorkDirectory/arm32-range.stderr" || \
		Fail "ARM32 branch-range diagnostic differs"
	printf '%s\n' \
		'[ARCH arm32]' \
		'[SECTION .text]' \
		'MOV R0, far_data' \
		'[SECTION .far vstart=0x100000000]' \
		'far_data:' \
		'DQ 0' > "$AddressRangeSource"
	AssembleFailure arm32-address-range -f bin \
		-o "$WorkDirectory/arm32-address-range.bin" "$AddressRangeSource"
	grep -Fq 'ARM32 label address is out of range' \
		"$WorkDirectory/arm32-address-range.stderr" || \
		Fail "ARM32 label-address range diagnostic differs"
	Pass "ARM32 operand diagnostics"
}

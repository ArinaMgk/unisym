#!/bin/bash
# ASCII ShellScript TAB4 LF
# Docutitle: AASM RISC-V Regression Checks
# Copyright: UNISYM, under Apache License 2.0

CheckRISCV()
{
	RV32Output="$WorkDirectory/riscv32.bin"
	RV64Output="$WorkDirectory/riscv64.bin"
	MixedSource="$WorkDirectory/mixed-architecture.asm"
	MixedOutput="$WorkDirectory/mixed-architecture.bin"
	IndirectTargetSource="$WorkDirectory/riscv-indirect-target.asm"
	IndirectTargetError="$WorkDirectory/riscv-indirect-target.stderr"
	LoadExtensionSource="$WorkDirectory/riscv-load-extension.asm"
	LoadExtensionOutput="$WorkDirectory/riscv-load-extension.bin"
	SignednessSource="$WorkDirectory/riscv-signedness.asm"
	SignednessOutput="$WorkDirectory/riscv-signedness.bin"
	SignednessNativeSource="$WorkDirectory/riscv-signedness-native.asm"
	SignednessNativeOutput="$WorkDirectory/riscv-signedness-native.bin"
	InvalidSignednessSource="$WorkDirectory/riscv-invalid-signedness.asm"
	InvalidSignednessError="$WorkDirectory/riscv-invalid-signedness.stderr"
	RV32Expected='13 05 10 00 33 05 b5 00 13 06 05 00 a3 01 c1 00 83 06 31 00 03 47 31 00 63 06 b5 00 6f 00 80 00 13 00 00 00 67 80 00 00 b7 50 34 12 17 11 00 00 ef 00 00 08 e7 00 41 00 63 9c 20 06 63 ca 20 06 63 d8 20 06 63 e6 20 06 63 f4 20 06 83 11 82 ff 83 21 82 ff 83 51 82 ff 23 1c 32 fe 23 2c 32 fe 93 22 c3 ff 93 32 c3 ff 93 42 c3 ff 93 62 c3 ff 93 72 c3 ff 93 12 73 00 93 52 73 00 93 52 73 40 b3 03 94 40 b3 13 94 00 b3 23 94 00 b3 33 94 00 b3 43 94 00 b3 53 94 00 b3 53 94 40 b3 63 94 00 b3 73 94 00 0f 00 f0 0f 73 00 00 00 73 00 10 00 13 00 00 00'
	RV64Expected='13 01 01 ff 23 34 11 00 03 35 81 00 83 65 01 00 3b 05 b5 00 1b 55 35 40 63 14 05 00 ef 00 40 00 83 30 81 00 13 01 01 01 67 80 00 00 9b 02 c3 ff 9b 12 73 00 9b 52 73 00 bb 03 94 40 bb 13 94 00 bb 53 94 00 bb 53 94 40 03 35 01 00'

	"$AasmExecutable" -march=riscv -f bin -o "$RV32Output" \
		"$RepositoryDirectory/demo/test/clover/arch-riscv32/basic.asm" \
		>"$WorkDirectory/riscv32.stdout" 2>"$WorkDirectory/riscv32.stderr" || \
		Fail "RISC-V 32-bit input did not assemble"
	[ ! -s "$WorkDirectory/riscv32.stderr" ] || \
		Fail "RISC-V 32-bit input produced diagnostics"
	[ "$(NormalizeHex "$RV32Output")" = "$RV32Expected" ] || \
		Fail "RISC-V 32-bit bytes differ"

	"$AasmExecutable" -march=riscv -f bin -o "$RV64Output" \
		"$RepositoryDirectory/demo/test/clover/arch-riscv64/basic.asm" \
		>"$WorkDirectory/riscv64.stdout" 2>"$WorkDirectory/riscv64.stderr" || \
		Fail "RISC-V 64-bit input did not assemble"
	[ ! -s "$WorkDirectory/riscv64.stderr" ] || \
		Fail "RISC-V 64-bit input produced diagnostics"
	[ "$(NormalizeHex "$RV64Output")" = "$RV64Expected" ] || \
		Fail "RISC-V 64-bit bytes differ"

	printf '%s\n' \
		'[ARCH x86]' \
		'[BITS 64]' \
		'RET' \
		'[ARCH riscv]' \
		'[BITS 32]' \
		'NOP' > "$MixedSource"
	"$AasmExecutable" -f bin -o "$MixedOutput" "$MixedSource" \
		>"$WorkDirectory/mixed-architecture.stdout" \
		2>"$WorkDirectory/mixed-architecture.stderr" || \
		Fail "mixed architecture input did not assemble"
	[ ! -s "$WorkDirectory/mixed-architecture.stderr" ] || \
		Fail "mixed architecture input produced diagnostics"
	[ "$(NormalizeHex "$MixedOutput")" = 'c3 13 00 00 00' ] || \
		Fail "mixed architecture bytes differ"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 32]' \
		'J [target]' \
		'JALR x1, [x2 + 4]' \
		'target:' \
		'NOP' > "$IndirectTargetSource"
	"$AasmExecutable" -f bin -o "$WorkDirectory/riscv-indirect-target.bin" \
		"$IndirectTargetSource" \
		>"$WorkDirectory/riscv-indirect-target.stdout" \
		2>"$IndirectTargetError"
	ResultCode="$?"
	[ "$ResultCode" -ne 0 ] || \
		Fail "RISC-V direct jump accepted a memory operand"
	grep -Fq 'target expression expected' "$IndirectTargetError" || \
		Fail "RISC-V direct jump memory diagnostic differs"
	grep -Fq 'operand 2: indirect target expression expected' \
		"$IndirectTargetError" || \
		Fail "RISC-V indirect jump accepted a memory operand"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'MOV a0, BYTE [sp]' \
		'MOV a1, SIGNED BYTE [sp]' \
		'MOV a2, UNSIGNED BYT4 [sp]' > "$LoadExtensionSource"
	"$AasmExecutable" -f bin -o "$LoadExtensionOutput" \
		"$LoadExtensionSource" \
		>"$WorkDirectory/riscv-load-extension.stdout" \
		2>"$WorkDirectory/riscv-load-extension.stderr" || \
		Fail "RISC-V load extension input did not assemble"
	[ ! -s "$WorkDirectory/riscv-load-extension.stderr" ] || \
		Fail "RISC-V load extension input produced diagnostics"
	[ "$(NormalizeHex "$LoadExtensionOutput")" = \
		'03 45 01 00 83 05 01 00 03 66 01 00' ] || \
		Fail "RISC-V load extension bytes differ"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'UNSIGNED MOV a0, BYTE [sp]' \
		'MOV a1, BYTE SIGNED [sp]' \
		'MOV a2, BYT4 [sp] UNSIGNED' \
		'MOV x13, SIGNED x14' \
		'LB x15, UNSIGNED [sp]' \
		'LBU SIGNED x16, [sp]' \
		'UNSIGNED SLT x5, x6, x7' \
		'SLTU SIGNED x8, x9, x10' \
		'SLTI x11, UNSIGNED x12, 7' \
		'BLT x1, x2, UNSIGNED target' \
		'BGEU x3, x4, target SIGNED' \
		'target:' \
		'MOV SIGNED BYTE [sp], a0' > "$SignednessSource"
	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'MOV a0, BYTE [sp]' \
		'MOV a1, SIGNED BYTE [sp]' \
		'MOV a2, UNSIGNED BYT4 [sp]' \
		'MOV x13, x14' \
		'LBU x15, [sp]' \
		'LB x16, [sp]' \
		'SLTU x5, x6, x7' \
		'SLT x8, x9, x10' \
		'SLTIU x11, x12, 7' \
		'BLTU x1, x2, target' \
		'BGE x3, x4, target' \
		'target:' \
		'MOV BYTE [sp], a0' > "$SignednessNativeSource"
	"$AasmExecutable" -f bin -o "$SignednessOutput" "$SignednessSource" \
		>"$WorkDirectory/riscv-signedness.stdout" \
		2>"$WorkDirectory/riscv-signedness.stderr" || \
		Fail "RISC-V signedness input did not assemble"
	[ ! -s "$WorkDirectory/riscv-signedness.stderr" ] || \
		Fail "RISC-V signedness input produced diagnostics"
	"$AasmExecutable" -f bin -o "$SignednessNativeOutput" \
		"$SignednessNativeSource" \
		>"$WorkDirectory/riscv-signedness-native.stdout" \
		2>"$WorkDirectory/riscv-signedness-native.stderr" || \
		Fail "RISC-V native signedness input did not assemble"
	[ ! -s "$WorkDirectory/riscv-signedness-native.stderr" ] || \
		Fail "RISC-V native signedness input produced diagnostics"
	cmp -s "$SignednessOutput" "$SignednessNativeOutput" || \
		Fail "RISC-V signedness variants differ from native instructions"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'ADD SIGNED x1, x2, x3' \
		'XOR x1, UNSIGNED x2, x3' \
		'SLT SIGNED x1, UNSIGNED x2, x3' > "$InvalidSignednessSource"
	"$AasmExecutable" -f bin \
		-o "$WorkDirectory/riscv-invalid-signedness.bin" \
		"$InvalidSignednessSource" \
		>"$WorkDirectory/riscv-invalid-signedness.stdout" \
		2>"$InvalidSignednessError"
	ResultCode="$?"
	[ "$ResultCode" -ne 0 ] || \
		Fail "RISC-V accepted invalid signedness qualifiers"
	[ "$(grep -Fc 'instruction does not have a signedness variant' \
		"$InvalidSignednessError")" -eq 2 ] || \
		Fail "RISC-V unsupported signedness diagnostics differ"
	grep -Fq 'instruction has multiple signedness qualifiers' \
		"$InvalidSignednessError" || \
		Fail "RISC-V duplicate signedness diagnostic differs"
	Pass "RISC-V encoding and ARCH switching"
}

CheckRISCVELFTarget()
{
	RV32Source="$WorkDirectory/riscv-elf32.asm"
	RV64Source="$WorkDirectory/riscv-elf64.asm"
	RV32Object="$WorkDirectory/riscv-elf32.o"
	RV64Object="$WorkDirectory/riscv-elf64.o"
	RV32Header="$WorkDirectory/riscv-elf32.header"
	RV64Header="$WorkDirectory/riscv-elf64.header"
	InvalidSource="$WorkDirectory/riscv-elf-invalid.asm"
	InvalidError="$WorkDirectory/riscv-elf-invalid.stderr"
	MixedSource="$WorkDirectory/riscv-elf-mixed.asm"
	MixedError="$WorkDirectory/riscv-elf-mixed.stderr"

	printf '%s\n' \
		'[BITS 32]' \
		'SECTION .text' \
		'NOP' > "$RV32Source"
	printf '%s\n' \
		'[BITS 64]' \
		'SECTION .text' \
		'NOP' > "$RV64Source"

	"$AasmExecutable" -march=riscv -f elf32 -o "$RV32Object" "$RV32Source" \
		>"$WorkDirectory/riscv-elf32.stdout" \
		2>"$WorkDirectory/riscv-elf32.stderr" || \
		Fail "RISC-V ELF32 input did not assemble"
	[ ! -s "$WorkDirectory/riscv-elf32.stderr" ] || \
		Fail "RISC-V ELF32 input produced diagnostics"
	"$AasmExecutable" -march=riscv -f elf64 -o "$RV64Object" "$RV64Source" \
		>"$WorkDirectory/riscv-elf64.stdout" \
		2>"$WorkDirectory/riscv-elf64.stderr" || \
		Fail "RISC-V ELF64 input did not assemble"
	[ ! -s "$WorkDirectory/riscv-elf64.stderr" ] || \
		Fail "RISC-V ELF64 input produced diagnostics"

	LC_ALL=C readelf -hW "$RV32Object" > "$RV32Header"
	LC_ALL=C readelf -hW "$RV64Object" > "$RV64Header"
	grep -Fq 'Class:                             ELF32' "$RV32Header" || \
		Fail "RISC-V ELF32 class differs"
	grep -Fq 'Machine:                           RISC-V' "$RV32Header" || \
		Fail "RISC-V ELF32 machine differs"
	grep -Fq 'Flags:                             0x0' "$RV32Header" || \
		Fail "RISC-V ELF32 flags differ"
	grep -Fq 'Class:                             ELF64' "$RV64Header" || \
		Fail "RISC-V ELF64 class differs"
	grep -Fq 'Machine:                           RISC-V' "$RV64Header" || \
		Fail "RISC-V ELF64 machine differs"
	grep -Fq 'Flags:                             0x0' "$RV64Header" || \
		Fail "RISC-V ELF64 flags differ"

	printf '%s\n' '[BITS 32]' 'NOP' > "$InvalidSource"
	"$AasmExecutable" -march=riscv -f elf64 \
		-o "$WorkDirectory/riscv-elf-invalid.o" "$InvalidSource" \
		>"$WorkDirectory/riscv-elf-invalid.stdout" 2>"$InvalidError"
	ResultCode="$?"
	[ "$ResultCode" -ne 0 ] || Fail "ELF64 accepted 32-bit RISC-V code"
	grep -Fq "elf64 output format does not support 32-bit \`riscv' code" \
		"$InvalidError" || Fail "RISC-V ELF bit-width diagnostic differs"

	printf '%s\n' '[ARCH x86]' '[BITS 64]' 'RET' > "$MixedSource"
	"$AasmExecutable" -march=riscv -f elf64 \
		-o "$WorkDirectory/riscv-elf-mixed.o" "$MixedSource" \
		>"$WorkDirectory/riscv-elf-mixed.stdout" 2>"$MixedError"
	ResultCode="$?"
	[ "$ResultCode" -ne 0 ] || Fail "ELF accepted an architecture switch"
	grep -Fq "cannot switch architecture from \`riscv' to \`x86'" \
		"$MixedError" || Fail "ELF architecture-switch diagnostic differs"
	Pass "RISC-V ELF target selection"
}

CheckRISCVRelocation()
{
	RV32Source="$WorkDirectory/riscv-relocation32.asm"
	RV64Source="$WorkDirectory/riscv-relocation64.asm"
	RV32Object="$WorkDirectory/riscv-relocation32.o"
	RV64Object="$WorkDirectory/riscv-relocation64.o"
	RV32Sections="$WorkDirectory/riscv-relocation32.sections"
	RV64Sections="$WorkDirectory/riscv-relocation64.sections"
	RV32Relocations="$WorkDirectory/riscv-relocation32.relocations"
	RV64Relocations="$WorkDirectory/riscv-relocation64.relocations"
	BinaryError="$WorkDirectory/riscv-relocation-binary.stderr"
	CrossSection32Source="$WorkDirectory/riscv-cross-section32.asm"
	CrossSection64Source="$WorkDirectory/riscv-cross-section64.asm"
	CrossSection32Object="$WorkDirectory/riscv-cross-section32.o"
	CrossSection64Object="$WorkDirectory/riscv-cross-section64.o"
	CrossSection32Relocations="$WorkDirectory/riscv-cross-section32.relocations"
	CrossSection64Relocations="$WorkDirectory/riscv-cross-section64.relocations"

	printf '%s\n' \
		'[BITS 32]' \
		'EXTERN external_branch' \
		'EXTERN external_jump' \
		'EXTERN external_data' \
		'SECTION .text' \
		'BEQ x5, x6, external_branch' \
		'JAL x1, external_jump' \
		'SECTION .data' \
		'DD external_data + 4' > "$RV32Source"
	printf '%s\n' \
		'[BITS 64]' \
		'EXTERN external_branch' \
		'EXTERN external_jump' \
		'EXTERN external_data' \
		'SECTION .text' \
		'BNE x7, x8, external_branch' \
		'JAL x1, external_jump' \
		'SECTION .data' \
		'DQ external_data + 8' > "$RV64Source"

	"$AasmExecutable" -march=riscv -f elf32 -o "$RV32Object" "$RV32Source" \
		>"$WorkDirectory/riscv-relocation32.stdout" \
		2>"$WorkDirectory/riscv-relocation32.stderr" || \
		Fail "RISC-V ELF32 relocation input did not assemble"
	[ ! -s "$WorkDirectory/riscv-relocation32.stderr" ] || \
		Fail "RISC-V ELF32 relocation input produced diagnostics"
	"$AasmExecutable" -march=riscv -f elf64 -o "$RV64Object" "$RV64Source" \
		>"$WorkDirectory/riscv-relocation64.stdout" \
		2>"$WorkDirectory/riscv-relocation64.stderr" || \
		Fail "RISC-V ELF64 relocation input did not assemble"
	[ ! -s "$WorkDirectory/riscv-relocation64.stderr" ] || \
		Fail "RISC-V ELF64 relocation input produced diagnostics"

	LC_ALL=C readelf -SW "$RV32Object" > "$RV32Sections"
	LC_ALL=C readelf -SW "$RV64Object" > "$RV64Sections"
	LC_ALL=C readelf -rW "$RV32Object" > "$RV32Relocations"
	LC_ALL=C readelf -rW "$RV64Object" > "$RV64Relocations"
	grep -Eq '[[:space:]]\.rela\.text[[:space:]]+RELA' "$RV32Sections" || \
		Fail "RISC-V ELF32 is missing .rela.text"
	grep -Eq '[[:space:]]\.rela\.data[[:space:]]+RELA' "$RV32Sections" || \
		Fail "RISC-V ELF32 is missing .rela.data"
	grep -Eq '[[:space:]]\.rela\.text[[:space:]]+RELA' "$RV64Sections" || \
		Fail "RISC-V ELF64 is missing .rela.text"
	grep -Eq '[[:space:]]\.rela\.data[[:space:]]+RELA' "$RV64Sections" || \
		Fail "RISC-V ELF64 is missing .rela.data"
	grep -Eq 'R_RISCV_BRANCH[[:space:]]+.*external_branch \+ 0$' \
		"$RV32Relocations" || Fail "RISC-V ELF32 branch relocation differs"
	grep -Eq 'R_RISCV_JAL[[:space:]]+.*external_jump \+ 0$' \
		"$RV32Relocations" || Fail "RISC-V ELF32 jump relocation differs"
	grep -Eq 'R_RISCV_32[[:space:]]+.*external_data \+ 4$' \
		"$RV32Relocations" || Fail "RISC-V ELF32 data relocation differs"
	grep -Eq 'R_RISCV_BRANCH[[:space:]]+.*external_branch \+ 0$' \
		"$RV64Relocations" || Fail "RISC-V ELF64 branch relocation differs"
	grep -Eq 'R_RISCV_JAL[[:space:]]+.*external_jump \+ 0$' \
		"$RV64Relocations" || Fail "RISC-V ELF64 jump relocation differs"
	grep -Eq 'R_RISCV_64[[:space:]]+.*external_data \+ 8$' \
		"$RV64Relocations" || Fail "RISC-V ELF64 data relocation differs"

	"$AasmExecutable" -march=riscv -f bin \
		-o "$WorkDirectory/riscv-relocation-binary.bin" "$RV32Source" \
		>"$WorkDirectory/riscv-relocation-binary.stdout" 2>"$BinaryError"
	ResultCode="$?"
	[ "$ResultCode" -ne 0 ] || Fail "Binary accepted RISC-V relocation"
	[ ! -e "$WorkDirectory/riscv-relocation-binary.bin" ] || \
		Fail "RISC-V Binary relocation left an output file"
	grep -Fq "output format \`bin' does not support RISC-V relocation" \
		"$BinaryError" || Fail "RISC-V Binary relocation diagnostic differs"

	printf '%s\n' \
		'[BITS 32]' \
		'SECTION .text' \
		'BEQ x5, x6, branch_target' \
		'JAL x1, jump_target' \
		'SECTION .branch_target' \
		'DD 0' \
		'branch_target:' \
		'NOP' \
		'SECTION .jump_target' \
		'DD 0' \
		'DD 0' \
		'jump_target:' \
		'NOP' > "$CrossSection32Source"
	printf '%s\n' \
		'[BITS 64]' \
		'SECTION .text' \
		'BNE x7, x8, branch_target' \
		'JAL x1, jump_target' \
		'SECTION .branch_target' \
		'DQ 0' \
		'branch_target:' \
		'NOP' \
		'SECTION .jump_target' \
		'DQ 0' \
		'DQ 0' \
		'jump_target:' \
		'NOP' > "$CrossSection64Source"
	"$AasmExecutable" -march=riscv -f elf32 -o "$CrossSection32Object" \
		"$CrossSection32Source" >"$WorkDirectory/riscv-cross-section32.stdout" \
		2>"$WorkDirectory/riscv-cross-section32.stderr" || \
		Fail "RISC-V ELF32 cross-section input did not assemble"
	[ ! -s "$WorkDirectory/riscv-cross-section32.stderr" ] || \
		Fail "RISC-V ELF32 cross-section input produced diagnostics"
	"$AasmExecutable" -march=riscv -f elf64 -o "$CrossSection64Object" \
		"$CrossSection64Source" >"$WorkDirectory/riscv-cross-section64.stdout" \
		2>"$WorkDirectory/riscv-cross-section64.stderr" || \
		Fail "RISC-V ELF64 cross-section input did not assemble"
	[ ! -s "$WorkDirectory/riscv-cross-section64.stderr" ] || \
		Fail "RISC-V ELF64 cross-section input produced diagnostics"
	LC_ALL=C readelf -rW "$CrossSection32Object" > "$CrossSection32Relocations"
	LC_ALL=C readelf -rW "$CrossSection64Object" > "$CrossSection64Relocations"
	grep -Eq 'R_RISCV_BRANCH[[:space:]]+.*\.branch_target \+ 4$' \
		"$CrossSection32Relocations" || \
		Fail "RISC-V ELF32 cross-section branch relocation differs"
	grep -Eq 'R_RISCV_JAL[[:space:]]+.*\.jump_target \+ 8$' \
		"$CrossSection32Relocations" || \
		Fail "RISC-V ELF32 cross-section jump relocation differs"
	grep -Eq 'R_RISCV_BRANCH[[:space:]]+.*\.branch_target \+ 8$' \
		"$CrossSection64Relocations" || \
		Fail "RISC-V ELF64 cross-section branch relocation differs"
	grep -Eq 'R_RISCV_JAL[[:space:]]+.*\.jump_target \+ 10$' \
		"$CrossSection64Relocations" || \
		Fail "RISC-V ELF64 cross-section jump relocation differs"
	Pass "RISC-V ELF relocations"
}


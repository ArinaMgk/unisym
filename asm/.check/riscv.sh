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
	CompressedSource="$WorkDirectory/riscv-compressed-relocation.asm"
	CompressedObject="$WorkDirectory/riscv-compressed-relocation.o"
	CompressedRelocations="$WorkDirectory/riscv-compressed-relocation.relocations"
	RelaxObject="$WorkDirectory/riscv-branch-relax.o"
	RelaxRelocations="$WorkDirectory/riscv-branch-relax.relocations"
	FormsSource="$WorkDirectory/riscv-relocation-forms.asm"
	Forms32Object="$WorkDirectory/riscv-relocation-forms32.o"
	Forms64Object="$WorkDirectory/riscv-relocation-forms64.o"
	Forms32Relocations="$WorkDirectory/riscv-relocation-forms32.relocations"
	Forms64Relocations="$WorkDirectory/riscv-relocation-forms64.relocations"
	AlignSource="$WorkDirectory/riscv-align-relocation.asm"
	Align32Source="$WorkDirectory/riscv-align-relocation32.asm"
	AlignObject="$WorkDirectory/riscv-align-relocation.o"
	Align32Object="$WorkDirectory/riscv-align-relocation32.o"
	AlignRelocations="$WorkDirectory/riscv-align-relocation.relocations"
	AlignSections="$WorkDirectory/riscv-align-relocation.sections"
	Align32Relocations="$WorkDirectory/riscv-align-relocation32.relocations"
	CompressedAlignSource="$WorkDirectory/riscv-compressed-align.asm"
	CompressedAlignObject="$WorkDirectory/riscv-compressed-align.o"
	CompressedAlignRelocations="$WorkDirectory/riscv-compressed-align.relocations"

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
	AssembleSuccess riscv-branch-relax -march=riscv -mrelax -f elf64 \
		-o "$RelaxObject" "$RV64Source"
	LC_ALL=C readelf -rW "$RelaxObject" > "$RelaxRelocations"
	[ "$(grep -c 'R_RISCV_RELAX' "$RelaxRelocations")" -eq 2 ] || \
		Fail "RISC-V branch and jump relaxation hints differ"
	[ "$(grep -Ec 'R_RISCV_RELAX[[:space:]]+0$' \
		"$RelaxRelocations")" -eq 2 ] || \
		Fail "RISC-V branch and jump relaxation hint records differ"

	printf '%s\n' \
		'EXTERN external_call' \
		'EXTERN external_plt' \
		'EXTERN external_data' \
		'EXTERN external_got' \
		'EXTERN external_store' \
		'SECTION .text' \
		'CALL external_call' \
		'CALL_PLT external_plt' \
		'LUI x5, HIGH external_data + 0x123' \
		'ADDI x5, x5, LOW external_data + 0x123' \
		'got_anchor:' \
		'AUIPC x6, GOT external_got' \
		'LW x8, [x6 + PCREL_LOW got_anchor]' \
		'pcrel_anchor:' \
		'AUIPC x9, PCREL_HIGH external_data + 0x20' \
		'LW x10, [x9 + PCREL_LOW pcrel_anchor]' \
		'LW x7, [x6 + LOW external_data + 8]' \
		'SW x7, [x6 + LOW external_store + 12]' \
		> "$FormsSource"
	AssembleSuccess riscv-relocation-forms32 -march=riscv -f elf32 \
		-o "$Forms32Object" "$FormsSource"
	AssembleSuccess riscv-relocation-forms64 -march=riscv -f elf64 \
		-o "$Forms64Object" "$FormsSource"
	LC_ALL=C readelf -rW "$Forms32Object" > "$Forms32Relocations"
	LC_ALL=C readelf -rW "$Forms64Object" > "$Forms64Relocations"
	for RelocationFile in "$Forms32Relocations" "$Forms64Relocations"; do
		grep -Eq 'R_RISCV_CALL[[:space:]]+.*external_call \+ 0$' \
			"$RelocationFile" || Fail "RISC-V CALL relocation differs"
		grep -Eq 'R_RISCV_CALL_PLT[[:space:]]+.*external_plt \+ 0$' \
			"$RelocationFile" || Fail "RISC-V CALL_PLT relocation differs"
		grep -Eq 'R_RISCV_HI20[[:space:]]+.*external_data \+ 123$' \
			"$RelocationFile" || Fail "RISC-V HI20 relocation differs"
		grep -Eq 'R_RISCV_LO12_I[[:space:]]+.*external_data \+ 123$' \
			"$RelocationFile" || Fail "RISC-V LO12_I relocation differs"
		grep -Eq 'R_RISCV_GOT_HI20[[:space:]]+.*external_got \+ 0$' \
			"$RelocationFile" || Fail "RISC-V GOT_HI20 relocation differs"
		grep -Eq 'R_RISCV_PCREL_LO12_I[[:space:]]+.*got_anchor \+ 0$' \
			"$RelocationFile" || Fail "RISC-V GOT low relocation pair differs"
		grep -Eq 'R_RISCV_PCREL_HI20[[:space:]]+.*external_data \+ 20$' \
			"$RelocationFile" || Fail "RISC-V explicit PCREL_HI20 differs"
		grep -Eq 'R_RISCV_PCREL_LO12_I[[:space:]]+.*pcrel_anchor \+ 0$' \
			"$RelocationFile" || Fail "RISC-V PCREL low relocation pair differs"
		grep -Eq 'R_RISCV_LO12_I[[:space:]]+.*external_data \+ 8$' \
			"$RelocationFile" || Fail "RISC-V load LO12 relocation differs"
		grep -Eq 'R_RISCV_LO12_S[[:space:]]+.*external_store \+ c$' \
			"$RelocationFile" || Fail "RISC-V store LO12 relocation differs"
	done
	AssembleSuccess riscv-relocation-forms-relax -march=riscv -mrelax \
		-f elf64 -o "$WorkDirectory/riscv-relocation-forms-relax.o" \
		"$FormsSource"
	LC_ALL=C readelf -rW \
		"$WorkDirectory/riscv-relocation-forms-relax.o" \
		> "$WorkDirectory/riscv-relocation-forms-relax.relocations"
	[ "$(grep -c 'R_RISCV_RELAX' \
		"$WorkDirectory/riscv-relocation-forms-relax.relocations")" -eq 10 ] || \
		Fail "RISC-V relocation-form relaxation hints differ"

	printf '%s\n' \
		'[BITS 64]' \
		'SECTION .text' \
		'NOP' \
		'NOP' \
		'align 16' \
		'NOP' > "$AlignSource"
	printf '%s\n' \
		'[BITS 32]' \
		'SECTION .text' \
		'NOP' \
		'NOP' \
		'align 16' \
		'NOP' > "$Align32Source"
	AssembleSuccess riscv-align-relocation -march=riscv -mrelax -f elf64 \
		-o "$AlignObject" "$AlignSource"
	AssembleSuccess riscv-align-relocation32 -march=riscv -mrelax -f elf32 \
		-o "$Align32Object" "$Align32Source"
	LC_ALL=C readelf -rW "$AlignObject" > "$AlignRelocations"
	LC_ALL=C readelf -rW "$Align32Object" > "$Align32Relocations"
	LC_ALL=C readelf -SW "$AlignObject" > "$AlignSections"
	grep -Eq 'R_RISCV_ALIGN[[:space:]]+c$' \
		"$AlignRelocations" || Fail "RISC-V ALIGN relocation differs"
	[ "$(grep -c 'R_RISCV_RELAX' "$AlignRelocations")" -eq 0 ] || \
		Fail "RISC-V ALIGN emitted an invalid RELAX hint"
	grep -Eq '[[:space:]]\.text[[:space:]]+PROGBITS.*000018' \
		"$AlignSections" || Fail "RISC-V ALIGN padding size differs"
	grep -Eq 'R_RISCV_ALIGN[[:space:]]+c$' \
		"$Align32Relocations" || \
		Fail "RISC-V ELF32 ALIGN relocation differs"
	printf '%s\n' \
		'[BITS 64]' \
		'[INST ic]' \
		'SECTION .text' \
		'C.NOP' \
		'align 16' \
		'C.NOP' > "$CompressedAlignSource"
	AssembleSuccess riscv-compressed-align -march=riscv -mrelax -f elf64 \
		-o "$CompressedAlignObject" "$CompressedAlignSource"
	LC_ALL=C readelf -rW "$CompressedAlignObject" \
		> "$CompressedAlignRelocations"
	grep -Eq 'R_RISCV_ALIGN[[:space:]]+e$' \
		"$CompressedAlignRelocations" || \
		Fail "RISC-V compressed ALIGN relocation differs"

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

	printf '%s\n' \
		'[BITS 64]' \
		'[INST ic]' \
		'EXTERN external_branch' \
		'EXTERN external_jump' \
		'SECTION .text' \
		'C.BEQZ x8, external_branch' \
		'C.J external_jump' > "$CompressedSource"
	AssembleSuccess riscv-compressed-relocation -march=riscv -f elf64 \
		-o "$CompressedObject" "$CompressedSource"
	LC_ALL=C readelf -rW "$CompressedObject" > "$CompressedRelocations"
	grep -Fq 'R_RISCV_RVC_BRANCH' "$CompressedRelocations" || \
		Fail "RISC-V compressed branch relocation differs"
	grep -Fq 'R_RISCV_RVC_JUMP' "$CompressedRelocations" || \
		Fail "RISC-V compressed jump relocation differs"
	Pass "RISC-V ELF relocations"
}

CheckRISCVExpansion()
{
	RV32Source="$WorkDirectory/riscv-expansion32.asm"
	RV64Source="$WorkDirectory/riscv-expansion64.asm"
	RV32Output="$WorkDirectory/riscv-expansion32.bin"
	RV64Output="$WorkDirectory/riscv-expansion64.bin"
	ELFSource="$WorkDirectory/riscv-expansion-elf.asm"
	ELFObject="$WorkDirectory/riscv-expansion-elf.o"
	ELFRelocations="$WorkDirectory/riscv-expansion-elf.relocations"
	RelaxObject="$WorkDirectory/riscv-expansion-relax.o"
	RelaxRelocations="$WorkDirectory/riscv-expansion-relax.relocations"
	InvalidSource="$WorkDirectory/riscv-expansion-invalid.asm"
	InvalidError="$WorkDirectory/riscv-expansion-invalid.stderr"
	RV32Expected='93 02 70 00 37 53 34 12 13 03 83 67 93 03 f0 ff 17 04 00 00 13 04 84 02 97 04 00 00 83 c4 04 02 17 05 00 00 23 2e 55 00 b7 15 00 00 93 85 45 23 b7 15 00 00 93 85 45 23 00 00 00 00 00 00 00 00'
	RV64Expected='37 35 12 00 1b 05 65 45 13 15 c5 00 13 05 a5 78 13 15 c5 00 13 05 d5 ab 13 15 c5 00 13 05 f5 de b7 35 12 00 9b 85 65 45 93 95 c5 00 93 85 a5 78 93 95 c5 00 93 85 d5 ab 93 95 c5 00 93 85 f5 de 83 e5 05 00 17 06 00 00 13 06 06 02 97 06 00 00 83 a6 46 01 17 07 00 00 23 38 a7 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00'

	printf '%s\n' \
		'[BITS 32]' \
		'SECTION .text' \
		'MOV x5, 7' \
		'MOV x6, 0x12345678' \
		'MOV x7, 0xffffffff' \
		'MOV x8, target' \
		'MOV x9, BYTE [target]' \
		'MOV BYT4 [target + 4], x5, x10' \
		'TIMES 2 MOV x11, 0x1234' \
		'target:' \
		'DD 0' \
		'DD 0' > "$RV32Source"
	printf '%s\n' \
		'[BITS 64]' \
		'SECTION .text' \
		'MOV a0, 0x123456789ABCDEF' \
		'MOV a1, BYT4 [0x123456789ABCDEF]' \
		'MOV a2, target + 8' \
		'MOV a3, SIGNED BYT4 [target + 4]' \
		'MOV BYT8 [target + 8], a0, a4' \
		'target:' \
		'DQ 0' \
		'DQ 0' > "$RV64Source"
	AssembleSuccess riscv-expansion32 -march=riscv -f bin \
		-o "$RV32Output" "$RV32Source"
	AssembleSuccess riscv-expansion64 -march=riscv -f bin \
		-o "$RV64Output" "$RV64Source"
	[ "$(NormalizeHex "$RV32Output")" = "$RV32Expected" ] || \
		Fail "RISC-V 32-bit expansion bytes differ"
	[ "$(NormalizeHex "$RV64Output")" = "$RV64Expected" ] || \
		Fail "RISC-V 64-bit expansion bytes differ"

	printf '%s\n' \
		'[BITS 64]' \
		'EXTERN external_data' \
		'SECTION .text' \
		'MOV a0, external_data + 8' \
		'MOV a1, BYT4 [external_data + 12]' \
		'MOV BYT8 [external_data + 16], a0, a2' \
		'LW a3, [local_data + 4]' \
		'SD a1, [local_data + 8], a4' \
		'SECTION .data' \
		'local_data:' \
		'DQ 0' \
		'DQ 0' > "$ELFSource"
	AssembleSuccess riscv-expansion-elf -march=riscv -f elf64 \
		-o "$ELFObject" "$ELFSource"
	LC_ALL=C readelf -rW "$ELFObject" > "$ELFRelocations"
	[ "$(grep -c 'R_RISCV_RELAX' "$ELFRelocations")" -eq 0 ] || \
		Fail "RISC-V relaxation hints were emitted by default"
	[ "$(grep -c 'R_RISCV_PCREL_HI20' "$ELFRelocations")" -eq 5 ] || \
		Fail "RISC-V expansion high relocations differ"
	[ "$(grep -c 'R_RISCV_PCREL_LO12_I' "$ELFRelocations")" -eq 3 ] || \
		Fail "RISC-V expansion I-type low relocations differ"
	[ "$(grep -c 'R_RISCV_PCREL_LO12_S' "$ELFRelocations")" -eq 2 ] || \
		Fail "RISC-V expansion S-type low relocations differ"
	[ "$(grep -Ec 'R_RISCV_PCREL_LO12_[IS].*\.\.@riscv\.pcrel\..* \+ 0$' \
		"$ELFRelocations")" -eq 5 ] || \
		Fail "RISC-V expansion low relocation anchors differ"
	grep -Eq 'R_RISCV_PCREL_HI20[[:space:]]+.*external_data \+ 8$' \
		"$ELFRelocations" || Fail "RISC-V MOV address addend differs"
	grep -Eq 'R_RISCV_PCREL_HI20[[:space:]]+.*external_data \+ c$' \
		"$ELFRelocations" || Fail "RISC-V load address addend differs"
	grep -Eq 'R_RISCV_PCREL_HI20[[:space:]]+.*external_data \+ 10$' \
		"$ELFRelocations" || Fail "RISC-V store address addend differs"
	AssembleSuccess riscv-expansion-relax -march=riscv -mrelax -f elf64 \
		-o "$RelaxObject" "$ELFSource"
	LC_ALL=C readelf -rW "$RelaxObject" > "$RelaxRelocations"
	[ "$(grep -c 'R_RISCV_RELAX' "$RelaxRelocations")" -eq 10 ] || \
		Fail "RISC-V relaxation hint count differs"
	[ "$(grep -Ec 'R_RISCV_RELAX[[:space:]]+0$' \
		"$RelaxRelocations")" -eq 10 ] || \
		Fail "RISC-V relaxation hint records differ"

	printf '%s\n' \
		'[BITS 64]' \
		'EXTERN external_data' \
		'MOV BYT4 [external_data], a0' \
		'MOV BYT4 [external_data], a0, zero' \
		'SW a0, [external_data], a0' > "$InvalidSource"
	AssembleFailure riscv-expansion-invalid -march=riscv -f elf64 \
		-o "$WorkDirectory/riscv-expansion-invalid.o" "$InvalidSource"
	InvalidError="$WorkDirectory/riscv-expansion-invalid.stderr"
	grep -Fq 'expression-address store requires a temporary register' \
		"$InvalidError" || Fail "RISC-V missing temporary diagnostic differs"
	grep -Fq 'temporary register cannot be zero' "$InvalidError" || \
		Fail "RISC-V zero temporary diagnostic differs"
	grep -Fq 'temporary register overlaps the source' "$InvalidError" || \
		Fail "RISC-V overlapping temporary diagnostic differs"
	Pass "RISC-V multi-instruction expansion"
}

CheckRISCVExtensions()
{
	CPUSource="$WorkDirectory/riscv-extensions-cpu.asm"
	INSTSource="$WorkDirectory/riscv-extensions-inst.asm"
	CPUOutput="$WorkDirectory/riscv-extensions-cpu.bin"
	INSTOutput="$WorkDirectory/riscv-extensions-inst.bin"
	RV32Source="$WorkDirectory/riscv-extensions-rv32.asm"
	RV32Output="$WorkDirectory/riscv-extensions-rv32.bin"
	RV32InvalidSource="$WorkDirectory/riscv-extensions-rv32-invalid.asm"
	DisabledSource="$WorkDirectory/riscv-extensions-disabled.asm"
	FormsSource="$WorkDirectory/riscv-extensions-forms.asm"
	CSRSource="$WorkDirectory/riscv-extensions-csr.asm"
	OrderSource="$WorkDirectory/riscv-extensions-order.asm"
	Expected='33 85 c5 02 b3 56 f7 02 3b 68 18 03 73 95 05 30 73 e6 5f 30 f3 16 f7 3e 73 00 20 30 73 00 20 10 73 00 50 10 73 00 00 12 73 00 b5 12 2f a5 05 14 2f 35 b6 1a 2f 25 b6 86 2f 35 b6 e0'

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[CPU ima_zicsr]' \
		'MUL a0, a1, a2' \
		'DIV UNSIGNED a3, a4, a5' \
		'REMW a6, a7' \
		'CSRRW a0, mstatus, a1' \
		'CSRRSI a2, mtvec, 31' \
		'CSRRW a3, pmpaddr63, a4' \
		'MRET' \
		'SRET' \
		'WFI' \
		'SFENCE.VMA' \
		'SFENCE.VMA a0, a1' \
		'LR.W AQ a0, [a1]' \
		'SC.D RL a0, a1, [a2]' \
		'AQ RL AMOMIN.W a0, a1, [a2]' \
		'AMOMAX.D UNSIGNED a0, a1, [a2]' > "$CPUSource"
	sed 's/\[CPU ima_zicsr\]/[INST ima_zicsr]/' "$CPUSource" > "$INSTSource"
	AssembleSuccess riscv-extensions-cpu -f bin -o "$CPUOutput" "$CPUSource"
	AssembleSuccess riscv-extensions-inst -f bin -o "$INSTOutput" "$INSTSource"
	[ "$(NormalizeHex "$CPUOutput")" = "$Expected" ] || \
		Fail "RISC-V extension bytes differ"
	cmp -s "$CPUOutput" "$INSTOutput" || \
		Fail "RISC-V CPU and INST selections differ"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 32]' \
		'[INST ima_zicsr]' \
		'MUL x5, x6, x7' \
		'DIVU x8, x9, x10' \
		'CSRRW x11, mstatus, x12' \
		'AQ RL AMOADD.W x13, x14, [x15]' > "$RV32Source"
	AssembleSuccess riscv-extensions-rv32 -f bin \
		-o "$RV32Output" "$RV32Source"
	[ "$(NormalizeHex "$RV32Output")" = \
		'b3 02 73 02 33 d4 a4 02 f3 15 06 30 af a6 e7 06' ] || \
		Fail "RISC-V RV32 extension bytes differ"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 32]' \
		'[INST ima]' \
		'MULW x5, x6, x7' \
		'LR.D x5, [x6]' > "$RV32InvalidSource"
	AssembleFailure riscv-extensions-rv32-invalid -f bin \
		-o "$WorkDirectory/riscv-extensions-rv32-invalid.bin" \
		"$RV32InvalidSource"
	[ "$(grep -Fc 'not available in the current bit mode' \
		"$WorkDirectory/riscv-extensions-rv32-invalid.stderr")" -eq 2 ] || \
		Fail "RISC-V RV32 extension diagnostics differ"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[INST i]' \
		'MUL a0, a1, a2' \
		'CSRRW a0, mstatus, a1' \
		'LR.W a0, [a1]' > "$DisabledSource"
	AssembleFailure riscv-extensions-disabled -f bin \
		-o "$WorkDirectory/riscv-extensions-disabled.bin" "$DisabledSource"
	[ "$(grep -Fc 'is disabled by the current instruction set' \
		"$WorkDirectory/riscv-extensions-disabled.stderr")" -eq 3 ] || \
		Fail "RISC-V disabled-extension diagnostics differ"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[INST ima_zicsr]' \
		'LR.W a0, [a1 + 4]' \
		'SC.W a0, a1, [target]' \
		'AQ ADD a0, a1, a2' \
		'target:' \
		'DD 0' > "$FormsSource"
	AssembleFailure riscv-extensions-forms -f bin \
		-o "$WorkDirectory/riscv-extensions-forms.bin" "$FormsSource"
	[ "$(grep -Fc 'atomic register address expected' \
		"$WorkDirectory/riscv-extensions-forms.stderr")" -eq 2 ] || \
		Fail "RISC-V atomic-address diagnostics differ"
	grep -Fq 'does not support atomic ordering qualifiers' \
		"$WorkDirectory/riscv-extensions-forms.stderr" || \
		Fail "RISC-V atomic-ordering diagnostic differs"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[INST i_zicsr]' \
		'CSRRWI a0, mstatus, 32' > "$CSRSource"
	AssembleFailure riscv-extensions-csr -f bin \
		-o "$WorkDirectory/riscv-extensions-csr.bin" "$CSRSource"
	grep -Fq 'CSR immediate is outside the 5-bit range' \
		"$WorkDirectory/riscv-extensions-csr.stderr" || \
		Fail "RISC-V CSR immediate diagnostic differs"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[INST iam]' \
		'ADD a0, a1, a2' > "$OrderSource"
	AssembleFailure riscv-extensions-order -f bin \
		-o "$WorkDirectory/riscv-extensions-order.bin" "$OrderSource"
	grep -Fq "unknown 'cpu' type" \
		"$WorkDirectory/riscv-extensions-order.stderr" || \
		Fail "RISC-V extension-order diagnostic differs"
	Pass "RISC-V instruction-set extensions"
}

CheckRISCVFloatingCompressedVector()
{
	Source="$WorkDirectory/riscv-fdcv.asm"
	Output="$WorkDirectory/riscv-fdcv.bin"
	DisabledSource="$WorkDirectory/riscv-fdcv-disabled.asm"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[INST imafdcv]' \
		'FADD.S f5, f6, f7, rtz' \
		'FSGNJ.D f8, f9, f10' \
		'FCVT.D.S f30, f31' \
		'C.NOP' \
		'NOP' \
		'C.LW x8, [x9 + 68]' \
		'VSETVLI x5, x6, e32, m4, ta, mu' \
		'VADD.VV v4, v5, v6, v0.t' \
		'VLE32.V v21, [x22]' > "$Source"
	AssembleSuccess riscv-fdcv -f bin -o "$Output" "$Source"
	[ "$(NormalizeHex "$Output")" = \
		'd3 12 73 00 53 84 a4 22 53 8f 0f 42 01 00 13 00 00 00 e0 40 d7 72 23 05 57 02 53 00 87 6a 0b 02' ] || \
		Fail "RISC-V F/D/C/V bytes differ"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[INST i]' \
		'FADD.S f1, f2, f3' \
		'C.NOP' \
		'VADD.VV v1, v2, v3' > "$DisabledSource"
	AssembleFailure riscv-fdcv-disabled -f bin \
		-o "$WorkDirectory/riscv-fdcv-disabled.bin" "$DisabledSource"
	[ "$(grep -Fc 'is disabled by the current instruction set' \
		"$WorkDirectory/riscv-fdcv-disabled.stderr")" -eq 3 ] || \
		Fail "RISC-V F/D/C/V extension diagnostics differ"
	Pass "RISC-V floating-point, compressed, and vector encoding"
}

CheckRISCVELFMetadata()
{
	RV32Source="$WorkDirectory/riscv-metadata32.asm"
	RV64Source="$WorkDirectory/riscv-metadata64.asm"
	RV32Object="$WorkDirectory/riscv-metadata32.o"
	RV64Object="$WorkDirectory/riscv-metadata64.o"
	InvalidSource="$WorkDirectory/riscv-abi-invalid.asm"
	ConflictSource="$WorkDirectory/riscv-abi-conflict.asm"

	printf '%s\n' \
		'[BITS 32]' \
		'[INST imafdcv]' \
		'[ABI ilp32d]' \
		'SECTION .text' \
		'C.NOP' > "$RV32Source"
	printf '%s\n' \
		'[BITS 64]' \
		'[INST imafdcv]' \
		'[ABI lp64d]' \
		'SECTION .text' \
		'C.NOP' > "$RV64Source"
	AssembleSuccess riscv-metadata32 -march=riscv -f elf32 \
		-o "$RV32Object" "$RV32Source"
	AssembleSuccess riscv-metadata64 -march=riscv -f elf64 \
		-o "$RV64Object" "$RV64Source"
	LC_ALL=C readelf -hW "$RV32Object" > "$WorkDirectory/riscv-metadata32.header"
	LC_ALL=C readelf -hW "$RV64Object" > "$WorkDirectory/riscv-metadata64.header"
	LC_ALL=C readelf -A "$RV32Object" > "$WorkDirectory/riscv-metadata32.attributes"
	LC_ALL=C readelf -A "$RV64Object" > "$WorkDirectory/riscv-metadata64.attributes"
	grep -Fq 'Flags:                             0x5, RVC, double-float ABI' \
		"$WorkDirectory/riscv-metadata32.header" || \
		Fail "RISC-V ELF32 target flags differ"
	grep -Fq 'Flags:                             0x5, RVC, double-float ABI' \
		"$WorkDirectory/riscv-metadata64.header" || \
		Fail "RISC-V ELF64 target flags differ"
	grep -Fq 'Tag_RISCV_stack_align: 16-bytes' \
		"$WorkDirectory/riscv-metadata32.attributes" || \
		Fail "RISC-V ELF32 stack alignment attribute differs"
	grep -Fq 'Tag_RISCV_arch: "rv32i2p1_m2p0_a2p1_f2p2_d2p2_c2p0_v1p0' \
		"$WorkDirectory/riscv-metadata32.attributes" || \
		Fail "RISC-V ELF32 architecture attribute differs"
	grep -Fq 'Tag_RISCV_arch: "rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0_v1p0' \
		"$WorkDirectory/riscv-metadata64.attributes" || \
		Fail "RISC-V ELF64 architecture attribute differs"

	printf '%s\n' \
		'[BITS 64]' \
		'[INST i]' \
		'[ABI lp64d]' \
		'NOP' > "$InvalidSource"
	AssembleFailure riscv-abi-invalid -march=riscv -f bin \
		-o "$WorkDirectory/riscv-abi-invalid.bin" "$InvalidSource"
	grep -Fq 'abi is incompatible with the current architecture, bit width, or instruction set' \
		"$WorkDirectory/riscv-abi-invalid.stderr" || \
		Fail "RISC-V ABI compatibility diagnostic differs"

	printf '%s\n' \
		'[BITS 64]' \
		'[INST imafd]' \
		'[ABI lp64f]' \
		'NOP' \
		'[ABI lp64d]' \
		'NOP' > "$ConflictSource"
	AssembleFailure riscv-abi-conflict -march=riscv -f elf64 \
		-o "$WorkDirectory/riscv-abi-conflict.o" "$ConflictSource"
	grep -Fq 'RISC-V ELF output cannot contain multiple ABI selections' \
		"$WorkDirectory/riscv-abi-conflict.stderr" || \
		Fail "RISC-V ELF ABI conflict diagnostic differs"
	Pass "RISC-V ELF ABI and attributes"
}


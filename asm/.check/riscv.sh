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

CheckRISCVLongControlFlow()
{
	BranchSource="$WorkDirectory/riscv-long-branch.asm"
	BranchOutput="$WorkDirectory/riscv-long-branch.bin"
	LongSource="$WorkDirectory/riscv-long-control-flow.asm"
	LongOutput="$WorkDirectory/riscv-long-control-flow.bin"
	BackwardSource="$WorkDirectory/riscv-long-backward.asm"
	BackwardOutput="$WorkDirectory/riscv-long-backward.bin"
	CompressedSource="$WorkDirectory/riscv-compressed-upgrade.asm"
	CompressedOutput="$WorkDirectory/riscv-compressed-upgrade.bin"
	CompressedLongSource="$WorkDirectory/riscv-compressed-long.asm"
	CompressedLongOutput="$WorkDirectory/riscv-compressed-long.bin"
	CompressedJalSource="$WorkDirectory/riscv-compressed-jal.asm"
	CompressedJalOutput="$WorkDirectory/riscv-compressed-jal.bin"
	RepeatSource="$WorkDirectory/riscv-long-repeat.asm"
	RepeatOutput="$WorkDirectory/riscv-long-repeat.bin"
	MissingTemporarySource="$WorkDirectory/riscv-long-missing-temporary.asm"
	CrossSectionSource="$WorkDirectory/riscv-long-cross-section.asm"
	CrossSectionObject="$WorkDirectory/riscv-long-cross-section.o"
	CrossSectionRelocations="$WorkDirectory/riscv-long-cross-section.relocations"
	RangeSource="$WorkDirectory/riscv-control-flow-range.asm"
	RangeObject="$WorkDirectory/riscv-control-flow-range.o"
	RangeScript="$WorkDirectory/riscv-control-flow-range.ld"
	RangeLinkOutput="$WorkDirectory/riscv-control-flow-range.elf"
	RangeLinkError="$WorkDirectory/riscv-control-flow-range-link.stderr"
	ShortRangeSource="$WorkDirectory/riscv-short-relocation-range.asm"
	ShortRangeObject="$WorkDirectory/riscv-short-relocation-range.o"
	LocalRangeSource="$WorkDirectory/riscv-local-control-flow-range.asm"
	LocalRangeOutput="$WorkDirectory/riscv-local-control-flow-range.bin"
	ArchitectureSource="$WorkDirectory/riscv-architecture-macro.asm"
	ArchitectureOutput="$WorkDirectory/riscv-architecture-macro.bin"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'BEQ x5, x6, target' \
		'BNE x5, x6, target' \
		'BLT x5, x6, target' \
		'BGE x5, x6, target' \
		'BLTU x5, x6, target' \
		'BGEU x5, x6, target' \
		'TIMES 5000 DB 0' \
		'target:' \
		'NOP' > "$BranchSource"
	AssembleSuccess riscv-long-branch -f bin \
		-o "$BranchOutput" "$BranchSource"
	[ "$(wc -c < "$BranchOutput")" -eq 5052 ] || \
		Fail "RISC-V long branch size differs"
	[ "$(od -An -v -tx1 -N 48 "$BranchOutput" | tr -s '[:space:]' ' ' | \
		sed 's/^ //;s/ $//')" = \
		'63 94 62 00 6f 10 40 3b 63 84 62 00 6f 10 c0 3a 63 d4 62 00 6f 10 40 3a 63 c4 62 00 6f 10 c0 39 63 f4 62 00 6f 10 40 39 63 e4 62 00 6f 10 c0 38' ] || \
		Fail "RISC-V inverted branch sequence differs"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'BEQ x5, x6, far, t0' \
		'J far, t1' \
		'JAL ra, far' \
		'TIMES 1100000 DB 0' \
		'far:' \
		'NOP' > "$LongSource"
	AssembleSuccess riscv-long-control-flow -f bin \
		-o "$LongOutput" "$LongSource"
	[ "$(wc -c < "$LongOutput")" -eq 1100032 ] || \
		Fail "RISC-V long control-flow size differs"
	[ "$(od -An -v -tx1 -N 28 "$LongOutput" | tr -s '[:space:]' ' ' | \
		sed 's/^ //;s/ $//')" = \
		'63 96 62 00 97 d2 10 00 67 80 82 8f 17 d3 10 00 67 00 03 8f 97 d0 10 00 e7 80 80 8e' ] || \
		Fail "RISC-V AUIPC control-flow sequence differs"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'target:' \
		'NOP' \
		'TIMES 5000 DB 0' \
		'BEQ x5, x6, target' > "$BackwardSource"
	AssembleSuccess riscv-long-backward -f bin \
		-o "$BackwardOutput" "$BackwardSource"
	[ "$(wc -c < "$BackwardOutput")" -eq 5012 ] || \
		Fail "RISC-V backward long branch size differs"
	[ "$(od -An -v -tx1 -j 5004 -N 8 "$BackwardOutput" | \
		tr -s '[:space:]' ' ' | sed 's/^ //;s/ $//')" = \
		'63 94 62 00 6f e0 1f c7' ] || \
		Fail "RISC-V backward long branch bytes differ"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[INST ic]' \
		'C.BEQZ x8, medium' \
		'TIMES 500 DB 0' \
		'medium:' \
		'C.BNEZ x9, far' \
		'C.J far' \
		'TIMES 5000 DB 0' \
		'far:' \
		'NOP' > "$CompressedSource"
	AssembleSuccess riscv-compressed-upgrade -f bin \
		-o "$CompressedOutput" "$CompressedSource"
	[ "$(wc -c < "$CompressedOutput")" -eq 5520 ] || \
		Fail "RISC-V compressed upgrade size differs"
	[ "$(od -An -v -tx1 -N 4 "$CompressedOutput" | \
		tr -s '[:space:]' ' ' | sed 's/^ //;s/ $//')" = \
		'63 0c 04 1e' ] || \
		Fail "RISC-V compressed branch direct upgrade differs"
	[ "$(od -An -v -tx1 -j 504 -N 12 "$CompressedOutput" | \
		tr -s '[:space:]' ' ' | sed 's/^ //;s/ $//')" = \
		'63 84 04 00 6f 10 00 39 6f 10 c0 38' ] || \
		Fail "RISC-V compressed control-flow upgrade differs"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[INST ic]' \
		'C.J far, t0' \
		'C.BEQZ x8, far, t1' \
		'TIMES 1100000 DB 0' \
		'far:' \
		'NOP' > "$CompressedLongSource"
	AssembleSuccess riscv-compressed-long -f bin \
		-o "$CompressedLongOutput" "$CompressedLongSource"
	[ "$(wc -c < "$CompressedLongOutput")" -eq 1100024 ] || \
		Fail "RISC-V compressed long control-flow size differs"
	[ "$(od -An -v -tx1 -N 20 "$CompressedLongOutput" | \
		tr -s '[:space:]' ' ' | sed 's/^ //;s/ $//')" = \
		'97 d2 10 00 67 80 42 8f 63 16 04 00 17 d3 10 00 67 00 83 8e' ] || \
		Fail "RISC-V compressed long control-flow bytes differ"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 32]' \
		'[INST ic]' \
		'C.JAL target' \
		'TIMES 3000 DB 0' \
		'target:' \
		'NOP' > "$CompressedJalSource"
	AssembleSuccess riscv-compressed-jal -f bin \
		-o "$CompressedJalOutput" "$CompressedJalSource"
	[ "$(od -An -v -tx1 -N 4 "$CompressedJalOutput" | \
		tr -s '[:space:]' ' ' | sed 's/^ //;s/ $//')" = \
		'ef 00 d0 3b' ] || \
		Fail "RISC-V C.JAL direct upgrade differs"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'TIMES 2 BEQ x1, x2, target' \
		'TIMES 4088 DB 0' \
		'target:' \
		'NOP' > "$RepeatSource"
	AssembleSuccess riscv-long-repeat -f bin \
		-o "$RepeatOutput" "$RepeatSource"
	[ "$(wc -c < "$RepeatOutput")" -eq 4104 ] || \
		Fail "RISC-V repeated branch measurement size differs"
	[ "$(od -An -v -tx1 -N 12 "$RepeatOutput" | \
		tr -s '[:space:]' ' ' | sed 's/^ //;s/ $//')" = \
		'63 94 20 00 6f 10 00 00 e3 8e 20 7e' ] || \
		Fail "RISC-V repeated branch measurement differs"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'J far_jump' \
		'BEQ x1, x2, far_branch' \
		'TIMES 1100000 DB 0' \
		'far_jump:' \
		'NOP' \
		'far_branch:' \
		'NOP' > "$MissingTemporarySource"
	AssembleFailure riscv-long-missing-temporary -f bin \
		-o "$WorkDirectory/riscv-long-missing-temporary.bin" \
		"$MissingTemporarySource"
	grep -Fq 'long jump requires a temporary register' \
		"$WorkDirectory/riscv-long-missing-temporary.stderr" || \
		Fail "RISC-V missing long-jump temporary diagnostic differs"
	grep -Fq 'long branch requires a temporary register' \
		"$WorkDirectory/riscv-long-missing-temporary.stderr" || \
		Fail "RISC-V missing long-branch temporary diagnostic differs"

	printf '%s\n' \
		'[BITS 64]' \
		'[INST ic]' \
		'SECTION .text' \
		'J data_label, t0' \
		'BEQ x1, x2, data_label, t1' \
		'C.J data_label, t2' \
		'C.BNEZ x8, data_label, t3' \
		'SECTION .data' \
		'data_label:' \
		'DQ 0' > "$CrossSectionSource"
	AssembleSuccess riscv-long-cross-section -march=riscv -f elf64 \
		-o "$CrossSectionObject" "$CrossSectionSource"
	LC_ALL=C readelf -rW "$CrossSectionObject" > "$CrossSectionRelocations"
	[ "$(grep -c 'R_RISCV_PCREL_HI20' "$CrossSectionRelocations")" -eq 4 ] || \
		Fail "RISC-V long control-flow high relocation count differs"
	[ "$(grep -c 'R_RISCV_PCREL_LO12_I' "$CrossSectionRelocations")" -eq 4 ] || \
		Fail "RISC-V long control-flow low relocation count differs"
	[ "$(grep -c 'R_RISCV_BRANCH\|R_RISCV_JAL\|R_RISCV_RVC' \
		"$CrossSectionRelocations")" -eq 0 ] || \
		Fail "RISC-V long control-flow retained a short relocation"
	[ "$(grep -Ec 'R_RISCV_PCREL_LO12_I.*\.\.@riscv\.pcrel\..* \+ 0$' \
		"$CrossSectionRelocations")" -eq 4 ] || \
		Fail "RISC-V long control-flow relocation pairing differs"

	printf '%s\n' '[BITS 64]' 'J 0x7ffff7fe, t0' > "$LocalRangeSource"
	AssembleSuccess riscv-local-range-positive -march=riscv -f bin \
		-o "$LocalRangeOutput" "$LocalRangeSource"
	printf '%s\n' '[BITS 64]' 'J -0x80000800, t0' > "$LocalRangeSource"
	AssembleSuccess riscv-local-range-negative -march=riscv -f bin \
		-o "$LocalRangeOutput" "$LocalRangeSource"
	printf '%s\n' '[BITS 64]' 'BEQ x1, x2, 0x7ffff802, t0' \
		> "$LocalRangeSource"
	AssembleSuccess riscv-local-branch-range-positive -march=riscv -f bin \
		-o "$LocalRangeOutput" "$LocalRangeSource"
	printf '%s\n' '[BITS 64]' 'BEQ x1, x2, -0x800007fc, t0' \
		> "$LocalRangeSource"
	AssembleSuccess riscv-local-branch-range-negative -march=riscv -f bin \
		-o "$LocalRangeOutput" "$LocalRangeSource"
	printf '%s\n' '[BITS 64]' 'J 0x7ffff800, t0' > "$LocalRangeSource"
	AssembleFailure riscv-local-range-positive-overflow -march=riscv -f bin \
		-o "$LocalRangeOutput" "$LocalRangeSource"
	grep -Fq 'outside the PC-relative 32-bit range' \
		"$WorkDirectory/riscv-local-range-positive-overflow.stderr" || \
		Fail "RISC-V positive local long-jump overflow diagnostic differs"
	printf '%s\n' '[BITS 64]' 'J -0x80000802, t0' > "$LocalRangeSource"
	AssembleFailure riscv-local-range-negative-overflow -march=riscv -f bin \
		-o "$LocalRangeOutput" "$LocalRangeSource"
	grep -Fq 'outside the PC-relative 32-bit range' \
		"$WorkDirectory/riscv-local-range-negative-overflow.stderr" || \
		Fail "RISC-V negative local long-jump overflow diagnostic differs"
	printf '%s\n' '[BITS 64]' 'BEQ x1, x2, 0x7ffff804, t0' \
		> "$LocalRangeSource"
	AssembleFailure riscv-local-branch-range-positive-overflow -march=riscv \
		-f bin -o "$LocalRangeOutput" "$LocalRangeSource"
	grep -Fq 'outside the PC-relative 32-bit range' \
		"$WorkDirectory/riscv-local-branch-range-positive-overflow.stderr" || \
		Fail "RISC-V positive local long-branch overflow diagnostic differs"
	printf '%s\n' '[BITS 64]' 'BEQ x1, x2, -0x800007fe, t0' \
		> "$LocalRangeSource"
	AssembleFailure riscv-local-branch-range-negative-overflow -march=riscv \
		-f bin -o "$LocalRangeOutput" "$LocalRangeSource"
	grep -Fq 'outside the PC-relative 32-bit range' \
		"$WorkDirectory/riscv-local-branch-range-negative-overflow.stderr" || \
		Fail "RISC-V negative local long-branch overflow diagnostic differs"

	RequireCommand ld.lld
	printf '%s\n' \
		'[BITS 64]' \
		'SECTION .text' \
		'BEQ x1, x2, target, t0' \
		'SECTION .target' \
		'target:' \
		'NOP' > "$RangeSource"
	AssembleSuccess riscv-control-flow-range -march=riscv -f elf64 \
		-o "$RangeObject" "$RangeSource"

	printf '%s\n' \
		'SECTIONS' \
		'{' \
		'  .text 0x1000 : { *(.text) }' \
		'  .target 0x80000802 : { *(.target) }' \
		'}' > "$RangeScript"
	ld.lld -m elf64lriscv --no-relax \
		-T "$RangeScript" -o "$RangeLinkOutput" "$RangeObject" \
		>"$WorkDirectory/riscv-range-positive.stdout" 2>"$RangeLinkError" || \
		Fail "RISC-V positive PC-relative pair boundary did not link"

	printf '%s\n' \
		'SECTIONS' \
		'{' \
		'  .text 0x80000800 : { *(.text) }' \
		'  .target 0x4 : { *(.target) }' \
		'}' > "$RangeScript"
	ld.lld -m elf64lriscv --no-relax \
		-T "$RangeScript" -o "$RangeLinkOutput" "$RangeObject" \
		>"$WorkDirectory/riscv-range-negative.stdout" 2>"$RangeLinkError" || \
		Fail "RISC-V negative PC-relative pair boundary did not link"

	printf '%s\n' \
		'SECTIONS' \
		'{' \
		'  .text 0x1000 : { *(.text) }' \
		'  .target 0x80000804 : { *(.target) }' \
		'}' > "$RangeScript"
	ld.lld -m elf64lriscv --no-relax \
		-T "$RangeScript" -o "$RangeLinkOutput" "$RangeObject" \
		>"$WorkDirectory/riscv-range-positive-overflow.stdout" \
		2>"$RangeLinkError"
	ResultCode="$?"
	[ "$ResultCode" -ne 0 ] || \
		Fail "RISC-V positive PC-relative pair overflow linked"
	grep -Fq 'R_RISCV_PCREL_HI20' "$RangeLinkError" || \
		Fail "RISC-V positive PC-relative overflow diagnostic differs"

	printf '%s\n' \
		'SECTIONS' \
		'{' \
		'  .text 0x80000800 : { *(.text) }' \
		'  .target 0x2 : { *(.target) }' \
		'}' > "$RangeScript"
	ld.lld -m elf64lriscv --no-relax \
		-T "$RangeScript" -o "$RangeLinkOutput" "$RangeObject" \
		>"$WorkDirectory/riscv-range-negative-overflow.stdout" \
		2>"$RangeLinkError"
	ResultCode="$?"
	[ "$ResultCode" -ne 0 ] || \
		Fail "RISC-V negative PC-relative pair overflow linked"
	grep -Fq 'R_RISCV_PCREL_HI20' "$RangeLinkError" || \
		Fail "RISC-V negative PC-relative overflow diagnostic differs"

	printf '%s\n' \
		'[BITS 64]' \
		'SECTION .text' \
		'BEQ x1, x2, target' \
		'SECTION .target' \
		'target:' \
		'NOP' > "$ShortRangeSource"
	AssembleSuccess riscv-short-relocation-range -march=riscv -f elf64 \
		-o "$ShortRangeObject" "$ShortRangeSource"
	printf '%s\n' \
		'SECTIONS' \
		'{' \
		'  .text 0x1000 : { *(.text) }' \
		'  .target 0x3000 : { *(.target) }' \
		'}' > "$RangeScript"
	ld.lld -m elf64lriscv --no-relax \
		-T "$RangeScript" -o "$RangeLinkOutput" "$ShortRangeObject" \
		>"$WorkDirectory/riscv-short-range-link.stdout" \
		2>"$RangeLinkError"
	ResultCode="$?"
	[ "$ResultCode" -ne 0 ] || \
		Fail "out-of-range RISC-V short branch relocation linked"
	grep -Fq 'R_RISCV_BRANCH' "$RangeLinkError" || \
		Fail "RISC-V short branch overflow diagnostic differs"

	printf '%s\n' \
		'[ARCH x86]' \
		'[BITS 64]' \
		'%ifidni __ARCH__,x86' \
		'DB 0x86' \
		'%else' \
		'DB 0' \
		'%endif' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'%ifidni __ARCH__,riscv' \
		'DB 0x52' \
		'%else' \
		'DB 0' \
		'%endif' > "$ArchitectureSource"
	AssembleSuccess riscv-architecture-macro -f bin \
		-o "$ArchitectureOutput" "$ArchitectureSource"
	[ "$(NormalizeHex "$ArchitectureOutput")" = '86 52' ] || \
		Fail "__ARCH__ did not follow the active Archend"

	Pass "RISC-V long branch and jump expansion"
}

CheckRISCVShortRangeLink()
{
	local CheckName="$1"
	local ObjectFile="$2"
	local TextAddress="$3"
	local TargetAddress="$4"
	local ExpectedResult="$5"
	local RelocationName="$6"
	local LinkScript="$WorkDirectory/$CheckName.ld"
	local LinkOutput="$WorkDirectory/$CheckName.elf"

	printf '%s\n' \
		'SECTIONS' \
		'{' \
		"  .text $TextAddress : { *(.text) }" \
		"  .target $TargetAddress : { *(.target) }" \
		'}' > "$LinkScript"
	if [ "$ExpectedResult" = success ]; then
		LinkSuccess "$CheckName" ld.lld -m elf64lriscv --no-relax \
			-T "$LinkScript" -o "$LinkOutput" "$ObjectFile"
	else
		LinkFailure "$CheckName" ld.lld -m elf64lriscv --no-relax \
			-T "$LinkScript" -o "$LinkOutput" "$ObjectFile"
		grep -Fq "$RelocationName" "$WorkDirectory/$CheckName.stderr" || \
			Fail "$CheckName diagnostic differs"
	fi
}

CheckRISCVShortRelocationRange()
{
	local JALSource="$WorkDirectory/riscv-short-jal-range.asm"
	local JALObject="$WorkDirectory/riscv-short-jal-range.o"
	local BranchSource="$WorkDirectory/riscv-short-rvc-branch-range.asm"
	local BranchObject="$WorkDirectory/riscv-short-rvc-branch-range.o"
	local JumpSource="$WorkDirectory/riscv-short-rvc-jump-range.asm"
	local JumpObject="$WorkDirectory/riscv-short-rvc-jump-range.o"

	RequireCommand ld.lld
	printf '%s\n' \
		'[BITS 64]' \
		'SECTION .text' \
		'JAL ra, target' \
		'SECTION .target' \
		'target:' \
		'NOP' > "$JALSource"
	printf '%s\n' \
		'[BITS 64]' \
		'[INST ic]' \
		'SECTION .text' \
		'C.BEQZ x8, target' \
		'SECTION .target' \
		'target:' \
		'C.NOP' > "$BranchSource"
	printf '%s\n' \
		'[BITS 64]' \
		'[INST ic]' \
		'SECTION .text' \
		'C.J target' \
		'SECTION .target' \
		'target:' \
		'C.NOP' > "$JumpSource"

	AssembleSuccess riscv-short-jal-range -march=riscv -f elf64 \
		-o "$JALObject" "$JALSource"
	AssembleSuccess riscv-short-rvc-branch-range -march=riscv -f elf64 \
		-o "$BranchObject" "$BranchSource"
	AssembleSuccess riscv-short-rvc-jump-range -march=riscv -f elf64 \
		-o "$JumpObject" "$JumpSource"

	CheckRISCVShortRangeLink riscv-jal-range-positive "$JALObject" \
		0x1000 0x100ffe success R_RISCV_JAL
	CheckRISCVShortRangeLink riscv-jal-range-negative "$JALObject" \
		0x101000 0x1000 success R_RISCV_JAL
	CheckRISCVShortRangeLink riscv-jal-range-positive-overflow "$JALObject" \
		0x1000 0x101000 failure R_RISCV_JAL
	CheckRISCVShortRangeLink riscv-jal-range-negative-overflow "$JALObject" \
		0x101002 0x1000 failure R_RISCV_JAL

	CheckRISCVShortRangeLink riscv-rvc-branch-range-positive "$BranchObject" \
		0x1000 0x10fe success R_RISCV_RVC_BRANCH
	CheckRISCVShortRangeLink riscv-rvc-branch-range-negative "$BranchObject" \
		0x1100 0x1000 success R_RISCV_RVC_BRANCH
	CheckRISCVShortRangeLink riscv-rvc-branch-range-positive-overflow \
		"$BranchObject" 0x1000 0x1100 failure R_RISCV_RVC_BRANCH
	CheckRISCVShortRangeLink riscv-rvc-branch-range-negative-overflow \
		"$BranchObject" 0x1102 0x1000 failure R_RISCV_RVC_BRANCH

	CheckRISCVShortRangeLink riscv-rvc-jump-range-positive "$JumpObject" \
		0x1000 0x17fe success R_RISCV_RVC_JUMP
	CheckRISCVShortRangeLink riscv-rvc-jump-range-negative "$JumpObject" \
		0x1800 0x1000 success R_RISCV_RVC_JUMP
	CheckRISCVShortRangeLink riscv-rvc-jump-range-positive-overflow \
		"$JumpObject" 0x1000 0x1800 failure R_RISCV_RVC_JUMP
	CheckRISCVShortRangeLink riscv-rvc-jump-range-negative-overflow \
		"$JumpObject" 0x1802 0x1000 failure R_RISCV_RVC_JUMP

	Pass "RISC-V short relocation ranges"
}

CheckRISCVELFLinking()
{
	local RV32Source="$WorkDirectory/riscv-link32.asm"
	local RV64Source="$WorkDirectory/riscv-link64.asm"
	local RV32Object="$WorkDirectory/riscv-link32.o"
	local RV64Object="$WorkDirectory/riscv-link64.o"
	local RV32Output="$WorkDirectory/riscv-link32.elf"
	local RV64Output="$WorkDirectory/riscv-link64.elf"
	local StaticScript="$WorkDirectory/riscv-link-static.ld"
	local RV32Relocations="$WorkDirectory/riscv-link32.relocations"
	local RV64Relocations="$WorkDirectory/riscv-link64.relocations"
	local RV32References="$WorkDirectory/riscv-link32.references"
	local RV64References="$WorkDirectory/riscv-link64.references"
	local DynamicSource="$WorkDirectory/riscv-link-dynamic.asm"
	local DynamicObject="$WorkDirectory/riscv-link-dynamic.o"
	local DynamicOutput="$WorkDirectory/riscv-link-dynamic.so"
	local DynamicSections="$WorkDirectory/riscv-link-dynamic.sections"
	local DynamicRelocations="$WorkDirectory/riscv-link-dynamic.relocations"
	local RelaxSource="$WorkDirectory/riscv-link-relax.asm"
	local RelaxObject="$WorkDirectory/riscv-link-relax.o"
	local RelaxScript="$WorkDirectory/riscv-link-relax.ld"
	local RelaxedOutput="$WorkDirectory/riscv-link-relaxed.elf"
	local UnrelaxedOutput="$WorkDirectory/riscv-link-unrelaxed.elf"
	local RelaxedText="$WorkDirectory/riscv-link-relaxed.text"
	local UnrelaxedText="$WorkDirectory/riscv-link-unrelaxed.text"
	local AlignSource="$WorkDirectory/riscv-link-align.asm"
	local AlignObject="$WorkDirectory/riscv-link-align.o"
	local AlignScript="$WorkDirectory/riscv-link-align.ld"
	local AlignOutput="$WorkDirectory/riscv-link-align.elf"
	local AlignText="$WorkDirectory/riscv-link-align.text"
	local AlignSymbols="$WorkDirectory/riscv-link-align.symbols"
	local AlignSize

	RequireCommand ld.lld
	RequireCommand llvm-objcopy

	printf '%s\n' \
		'[BITS 32]' \
		'SECTION .text' \
		'GLOBAL start' \
		'start:' \
		'CALL function' \
		'load_anchor:' \
		'AUIPC t0, PCREL_HIGH data' \
		'LW t1, [t0 + PCREL_LOW load_anchor]' \
		'store_anchor:' \
		'AUIPC t2, PCREL_HIGH data + 4' \
		'SW t1, [t2 + PCREL_LOW store_anchor]' \
		'got_anchor:' \
		'AUIPC t3, GOT data' \
		'LW t4, [t3 + PCREL_LOW got_anchor]' \
		'RET' \
		'SECTION .target' \
		'function:' \
		'RET' \
		'SECTION .data' \
		'data:' \
		'DD 0x11223344' \
		'DD 0x55667788' \
		'SECTION .refs' \
		'DD data + 4' > "$RV32Source"
	printf '%s\n' \
		'[BITS 64]' \
		'SECTION .text' \
		'GLOBAL start' \
		'start:' \
		'CALL function' \
		'load_anchor:' \
		'AUIPC t0, PCREL_HIGH data' \
		'LD t1, [t0 + PCREL_LOW load_anchor]' \
		'store_anchor:' \
		'AUIPC t2, PCREL_HIGH data + 8' \
		'SD t1, [t2 + PCREL_LOW store_anchor]' \
		'got_anchor:' \
		'AUIPC t3, GOT data' \
		'LD t4, [t3 + PCREL_LOW got_anchor]' \
		'RET' \
		'SECTION .target' \
		'function:' \
		'RET' \
		'SECTION .data' \
		'data:' \
		'DQ 0x1122334455667788' \
		'DQ 0x8877665544332211' \
		'SECTION .refs' \
		'DQ data + 8' > "$RV64Source"
	printf '%s\n' \
		'SECTIONS' \
		'{' \
		'  .text 0x10000 : { *(.text) }' \
		'  .target 0x18000 : { *(.target) }' \
		'  .data 0x20000 : { *(.data) }' \
		'  .refs 0x30000 : { *(.refs) }' \
		'}' > "$StaticScript"

	AssembleSuccess riscv-link32 -march=riscv -f elf32 \
		-o "$RV32Object" "$RV32Source"
	AssembleSuccess riscv-link64 -march=riscv -f elf64 \
		-o "$RV64Object" "$RV64Source"
	LinkSuccess riscv-static-link32 ld.lld -m elf32lriscv --no-relax \
		-e start -T "$StaticScript" -o "$RV32Output" "$RV32Object"
	LinkSuccess riscv-static-link64 ld.lld -m elf64lriscv --no-relax \
		-e start -T "$StaticScript" -o "$RV64Output" "$RV64Object"
	LC_ALL=C readelf -rW "$RV32Output" > "$RV32Relocations"
	LC_ALL=C readelf -rW "$RV64Output" > "$RV64Relocations"
	grep -Fq 'There are no relocations in this file.' "$RV32Relocations" || \
		Fail "RISC-V ELF32 link retained static relocations"
	grep -Fq 'There are no relocations in this file.' "$RV64Relocations" || \
		Fail "RISC-V ELF64 link retained static relocations"
	llvm-objcopy --dump-section .refs="$RV32References" "$RV32Output"
	llvm-objcopy --dump-section .refs="$RV64References" "$RV64Output"
	[ "$(NormalizeHex "$RV32References")" = '04 00 02 00' ] || \
		Fail "RISC-V ELF32 absolute data relocation linked incorrectly"
	[ "$(NormalizeHex "$RV64References")" = \
		'08 00 02 00 00 00 00 00' ] || \
		Fail "RISC-V ELF64 absolute data relocation linked incorrectly"
	LC_ALL=C readelf -SW "$RV32Output" | grep -Eq \
		'[[:space:]]\.got[[:space:]]+PROGBITS' || \
		Fail "RISC-V ELF32 link did not create a GOT"
	LC_ALL=C readelf -SW "$RV64Output" | grep -Eq \
		'[[:space:]]\.got[[:space:]]+PROGBITS' || \
		Fail "RISC-V ELF64 link did not create a GOT"

	printf '%s\n' \
		'[BITS 64]' \
		'EXTERN dynamic_function' \
		'EXTERN dynamic_data' \
		'SECTION .text' \
		'GLOBAL caller' \
		'caller:' \
		'CALL_PLT dynamic_function' \
		'got_anchor:' \
		'AUIPC t0, GOT dynamic_data' \
		'LD t1, [t0 + PCREL_LOW got_anchor]' \
		'RET' > "$DynamicSource"
	AssembleSuccess riscv-dynamic-link -march=riscv -f elf64 \
		-o "$DynamicObject" "$DynamicSource"
	LinkSuccess riscv-shared-link ld.lld -m elf64lriscv -shared \
		-o "$DynamicOutput" "$DynamicObject"
	LC_ALL=C readelf -SW "$DynamicOutput" > "$DynamicSections"
	LC_ALL=C readelf -rW "$DynamicOutput" > "$DynamicRelocations"
	grep -Eq '[[:space:]]\.plt[[:space:]]+PROGBITS' "$DynamicSections" || \
		Fail "RISC-V shared link did not create a PLT"
	grep -Eq '[[:space:]]\.got[[:space:]]+PROGBITS' "$DynamicSections" || \
		Fail "RISC-V shared link did not create a GOT"
	grep -Eq 'R_RISCV_JUMP_SLOT[[:space:]]+.*dynamic_function' \
		"$DynamicRelocations" || \
		Fail "RISC-V CALL_PLT was not consumed by the shared link"
	grep -Eq 'R_RISCV_64[[:space:]]+.*dynamic_data' "$DynamicRelocations" || \
		Fail "RISC-V GOT reference was not consumed by the shared link"

	printf '%s\n' \
		'[BITS 64]' \
		'SECTION .text' \
		'GLOBAL start' \
		'start:' \
		'CALL target' \
		'RET' \
		'SECTION .target' \
		'target:' \
		'RET' > "$RelaxSource"
	printf '%s\n' \
		'SECTIONS' \
		'{' \
		'  .text 0x10000 : { *(.text) }' \
		'  .target 0x11000 : { *(.target) }' \
		'}' > "$RelaxScript"
	AssembleSuccess riscv-link-relax -march=riscv -mrelax -f elf64 \
		-o "$RelaxObject" "$RelaxSource"
	LinkSuccess riscv-link-no-relax ld.lld -m elf64lriscv --no-relax \
		-e start -T "$RelaxScript" -o "$UnrelaxedOutput" "$RelaxObject"
	LinkSuccess riscv-link-with-relax ld.lld -m elf64lriscv --relax \
		-e start -T "$RelaxScript" -o "$RelaxedOutput" "$RelaxObject"
	llvm-objcopy --dump-section .text="$UnrelaxedText" "$UnrelaxedOutput"
	llvm-objcopy --dump-section .text="$RelaxedText" "$RelaxedOutput"
	[ "$(wc -c < "$UnrelaxedText")" -eq 12 ] || \
		Fail "RISC-V unrelaxed CALL size differs"
	[ "$(wc -c < "$RelaxedText")" -eq 8 ] || \
		Fail "RISC-V relaxed CALL size differs"

	printf '%s\n' \
		'[BITS 64]' \
		'SECTION .text' \
		'GLOBAL start' \
		'start:' \
		'NOP' \
		'align 16' \
		'aligned_instruction:' \
		'NOP' > "$AlignSource"
	printf '%s\n' \
		'SECTIONS' \
		'{' \
		'  .text 0x1004 : SUBALIGN(1) { *(.text) }' \
		'}' > "$AlignScript"
	AssembleSuccess riscv-link-align -march=riscv -mrelax -f elf64 \
		-o "$AlignObject" "$AlignSource"
	LinkSuccess riscv-link-align ld.lld -m elf64lriscv --relax \
		-e start -T "$AlignScript" -o "$AlignOutput" "$AlignObject"
	llvm-objcopy --dump-section .text="$AlignText" "$AlignOutput"
	LC_ALL=C readelf -sW "$AlignOutput" > "$AlignSymbols"
	AlignSize="$(wc -c < "$AlignText")"
	[ "$AlignSize" -eq 16 ] || \
		Fail "RISC-V linked ALIGN padding size differs: $AlignSize"
	grep -Eq '0000000000001010[[:space:]].*[[:space:]]aligned_instruction$' \
		"$AlignSymbols" || Fail "RISC-V linked ALIGN address differs"

	Pass "RISC-V ELF end-to-end linking"
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
	FenceSource="$WorkDirectory/riscv-fence.asm"
	FenceOutput="$WorkDirectory/riscv-fence.bin"
	FenceDisabledSource="$WorkDirectory/riscv-fence-disabled.asm"
	FenceInvalidSource="$WorkDirectory/riscv-fence-invalid.asm"
	MultiExtensionOrderSource="$WorkDirectory/riscv-multi-extension-order.asm"
	UnknownExtensionSource="$WorkDirectory/riscv-unknown-extension.asm"
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

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[INST i_zifencei]' \
		'FENCE' \
		'FENCE rw, rw' \
		'FENCE 0, i, orw' \
		'FENCE.I' > "$FenceSource"
	AssembleSuccess riscv-fence -f bin -o "$FenceOutput" "$FenceSource"
	[ "$(NormalizeHex "$FenceOutput")" = \
		'0f 00 f0 0f 0f 00 30 03 0f 00 70 08 0f 10 00 00' ] || \
		Fail "RISC-V FENCE bytes differ"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[INST i]' \
		'FENCE.I' > "$FenceDisabledSource"
	AssembleFailure riscv-fence-disabled -f bin \
		-o "$WorkDirectory/riscv-fence-disabled.bin" "$FenceDisabledSource"
	grep -Fq 'is disabled by the current instruction set' \
		"$WorkDirectory/riscv-fence-disabled.stderr" || \
		Fail "RISC-V Zifencei extension diagnostic differs"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[INST i_zifencei]' \
		'FENCE rr, rw' \
		'FENCE 1, rw, rw' > "$FenceInvalidSource"
	AssembleFailure riscv-fence-invalid -f bin \
		-o "$WorkDirectory/riscv-fence-invalid.bin" "$FenceInvalidSource"
	grep -Fq 'set must contain unique I, O, R, W letters or 0' \
		"$WorkDirectory/riscv-fence-invalid.stderr" || \
		Fail "RISC-V invalid FENCE set diagnostic differs"
	grep -Fq 'mode must be the supported value 0' \
		"$WorkDirectory/riscv-fence-invalid.stderr" || \
		Fail "RISC-V invalid FENCE mode diagnostic differs"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[INST i_zifencei_zicsr]' \
		'NOP' > "$MultiExtensionOrderSource"
	AssembleFailure riscv-multi-extension-order -f bin \
		-o "$WorkDirectory/riscv-multi-extension-order.bin" \
		"$MultiExtensionOrderSource"
	grep -Fq "unknown 'cpu' type" \
		"$WorkDirectory/riscv-multi-extension-order.stderr" || \
		Fail "RISC-V multi-letter extension-order diagnostic differs"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[INST i_zba]' \
		'NOP' > "$UnknownExtensionSource"
	AssembleFailure riscv-unknown-extension -f bin \
		-o "$WorkDirectory/riscv-unknown-extension.bin" "$UnknownExtensionSource"
	grep -Fq "unknown 'cpu' type" \
		"$WorkDirectory/riscv-unknown-extension.stderr" || \
		Fail "RISC-V unknown extension diagnostic differs"
	Pass "RISC-V instruction-set extensions"
}

CheckRISCVFloatingCompressedVector()
{
	Source="$WorkDirectory/riscv-fdcv.asm"
	Output="$WorkDirectory/riscv-fdcv.bin"
	DisabledSource="$WorkDirectory/riscv-fdcv-disabled.asm"
	Compressed64Source="$WorkDirectory/riscv-compressed64.asm"
	Compressed64Output="$WorkDirectory/riscv-compressed64.bin"
	Compressed32Source="$WorkDirectory/riscv-compressed32.asm"
	Compressed32Output="$WorkDirectory/riscv-compressed32.bin"
	CompressedInvalidSource="$WorkDirectory/riscv-compressed-invalid.asm"
	CompressedBitModeSource="$WorkDirectory/riscv-compressed-bit-mode.asm"
	CompressedExtensionSource="$WorkDirectory/riscv-compressed-extension.asm"

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

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[INST imafdc]' \
		'C.ADDI4SPN a0, sp, 16' \
		'C.ADDI a0, -1' \
		'C.ADDIW a0, 1' \
		'C.LI a1, -2' \
		'C.ADDI16SP sp, -16' \
		'C.LUI a2, 1' \
		'C.SRLI a0, 3' \
		'C.SRAI a0, 3' \
		'C.ANDI a0, -2' \
		'C.SUB a0, a1' \
		'C.XOR a0, a1' \
		'C.OR a0, a1' \
		'C.AND a0, a1' \
		'C.SUBW a0, a1' \
		'C.ADDW a0, a1' \
		'C.SLLI a0, 4' \
		'C.LW a0, [a1 + 68]' \
		'C.SW a0, [a1 + 68]' \
		'C.LD a0, [a1 + 24]' \
		'C.SD a0, [a1 + 24]' \
		'C.FLD f8, [a1 + 24]' \
		'C.FSD f8, [a1 + 24]' \
		'C.LWSP a0, [sp + 12]' \
		'C.SWSP a0, [sp + 12]' \
		'C.LDSP a0, [sp + 24]' \
		'C.SDSP a0, [sp + 24]' \
		'C.FLDSP f8, [sp + 24]' \
		'C.FSDSP f8, [sp + 24]' \
		'C.JR a0' \
		'C.JALR a0' \
		'C.EBREAK' > "$Compressed64Source"
	AssembleSuccess riscv-compressed64 -f bin \
		-o "$Compressed64Output" "$Compressed64Source"
	[ "$(NormalizeHex "$Compressed64Output")" = \
		'08 08 7d 15 05 25 f9 55 7d 71 05 66 0d 81 0d 85 79 99 0d 8d 2d 8d 4d 8d 6d 8d 0d 9d 2d 9d 12 05 e8 41 e8 c1 88 6d 88 ed 80 2d 80 ad 32 45 2a c6 62 65 2a ec 62 24 22 ac 02 85 02 95 02 90' ] || \
		Fail "RISC-V RV64 compressed bytes differ"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 32]' \
		'[INST imafdc]' \
		'C.ADDI4SPN a0, sp, 16' \
		'C.FLW f8, [a1 + 68]' \
		'C.FSW f8, [a1 + 68]' \
		'C.FLWSP f8, [sp + 12]' \
		'C.FSWSP f8, [sp + 12]' \
		'C.FLD f9, [a1 + 24]' \
		'C.FSD f9, [a1 + 24]' \
		'C.FLDSP f9, [sp + 24]' \
		'C.FSDSP f9, [sp + 24]' > "$Compressed32Source"
	AssembleSuccess riscv-compressed32 -f bin \
		-o "$Compressed32Output" "$Compressed32Source"
	[ "$(NormalizeHex "$Compressed32Output")" = \
		'08 08 e0 61 e0 e1 32 64 22 e6 84 2d 84 ad e2 24 26 ac' ] || \
		Fail "RISC-V RV32 compressed bytes differ"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 32]' \
		'[INST ic]' \
		'C.ADDI4SPN x5, sp, 4' > "$CompressedInvalidSource"
	AssembleFailure riscv-compressed-invalid -f bin \
		-o "$WorkDirectory/riscv-compressed-invalid.bin" \
		"$CompressedInvalidSource"
	grep -Fq 'C.ADDI4SPN requires x8..x15' \
		"$WorkDirectory/riscv-compressed-invalid.stderr" || \
		Fail "RISC-V compressed operand diagnostic differs"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 32]' \
		'[INST ic]' \
		'C.ADDIW x8, 1' > "$CompressedBitModeSource"
	AssembleFailure riscv-compressed-bit-mode -f bin \
		-o "$WorkDirectory/riscv-compressed-bit-mode.bin" \
		"$CompressedBitModeSource"
	grep -Fq 'is not available in the current bit mode' \
		"$WorkDirectory/riscv-compressed-bit-mode.stderr" || \
		Fail "RISC-V compressed bit-mode diagnostic differs"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 32]' \
		'[INST ic]' \
		'C.FLW f8, [x9]' > "$CompressedExtensionSource"
	AssembleFailure riscv-compressed-extension -f bin \
		-o "$WorkDirectory/riscv-compressed-extension.bin" \
		"$CompressedExtensionSource"
	grep -Fq 'is disabled by the current instruction set' \
		"$WorkDirectory/riscv-compressed-extension.stderr" || \
		Fail "RISC-V compressed combined-extension diagnostic differs"
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
		'[INST imafdcv_zifencei]' \
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
	grep -Fq '_zicsr2p0_zifencei2p0' \
		"$WorkDirectory/riscv-metadata64.attributes" || \
		Fail "RISC-V ELF64 multi-letter attributes differ"

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


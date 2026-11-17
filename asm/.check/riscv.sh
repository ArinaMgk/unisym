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
	DataRelocation32Source="$WorkDirectory/riscv-data-relocation32.asm"
	DataRelocation64Source="$WorkDirectory/riscv-data-relocation64.asm"
	DataRelocation32Object="$WorkDirectory/riscv-data-relocation32.o"
	DataRelocation64Object="$WorkDirectory/riscv-data-relocation64.o"
	DataRelocation32Relocations="$WorkDirectory/riscv-data-relocation32.relocations"
	DataRelocation64Relocations="$WorkDirectory/riscv-data-relocation64.relocations"
	DataRelocationScript="$WorkDirectory/riscv-data-relocation.ld"
	DataRelocation32Output="$WorkDirectory/riscv-data-relocation32.elf"
	DataRelocation64Output="$WorkDirectory/riscv-data-relocation64.elf"
	DataRelocation32Data="$WorkDirectory/riscv-data-relocation32.data"
	DataRelocation64Data="$WorkDirectory/riscv-data-relocation64.data"
	InvalidDataRelocationSource="$WorkDirectory/riscv-data-relocation-invalid.asm"
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
	ArithmeticSource="$WorkDirectory/riscv-arithmetic-relocation.asm"
	Arithmetic32Object="$WorkDirectory/riscv-arithmetic-relocation32.o"
	Arithmetic64Object="$WorkDirectory/riscv-arithmetic-relocation64.o"
	Arithmetic32Relocations="$WorkDirectory/riscv-arithmetic-relocation32.relocations"
	Arithmetic64Relocations="$WorkDirectory/riscv-arithmetic-relocation64.relocations"
	Arithmetic32Data="$WorkDirectory/riscv-arithmetic-relocation32.data"
	Arithmetic64Data="$WorkDirectory/riscv-arithmetic-relocation64.data"
	LabelDifferenceSource="$WorkDirectory/riscv-label-difference.asm"
	LabelDifference32Object="$WorkDirectory/riscv-label-difference32.o"
	LabelDifference64Object="$WorkDirectory/riscv-label-difference64.o"
	LabelDifference32Relocations="$WorkDirectory/riscv-label-difference32.relocations"
	LabelDifference64Relocations="$WorkDirectory/riscv-label-difference64.relocations"
	LabelDifferenceScript="$WorkDirectory/riscv-label-difference.ld"
	LabelDifference32Output="$WorkDirectory/riscv-label-difference32.elf"
	LabelDifference64Output="$WorkDirectory/riscv-label-difference64.elf"
	LabelDifference32Data="$WorkDirectory/riscv-label-difference32.data"
	LabelDifference64Data="$WorkDirectory/riscv-label-difference64.data"
	ULEB32Source="$WorkDirectory/riscv-uleb128-relocation32.asm"
	ULEB64Source="$WorkDirectory/riscv-uleb128-relocation64.asm"
	ULEB32Object="$WorkDirectory/riscv-uleb128-relocation32.o"
	ULEB64Object="$WorkDirectory/riscv-uleb128-relocation64.o"
	ULEB32Relocations="$WorkDirectory/riscv-uleb128-relocation32.relocations"
	ULEB64Relocations="$WorkDirectory/riscv-uleb128-relocation64.relocations"
	ULEBScript="$WorkDirectory/riscv-uleb128-relocation.ld"
	ULEB32Output="$WorkDirectory/riscv-uleb128-relocation32.elf"
	ULEB64Output="$WorkDirectory/riscv-uleb128-relocation64.elf"
	ULEB32Data="$WorkDirectory/riscv-uleb128-relocation32.data"
	ULEB64Data="$WorkDirectory/riscv-uleb128-relocation64.data"
	ULEBNegativeSource="$WorkDirectory/riscv-uleb128-negative.asm"
	ULEBSymbolSource="$WorkDirectory/riscv-uleb128-symbol.asm"
	ULEBTypeSource="$WorkDirectory/riscv-uleb128-type.asm"
	CFI32Source="$WorkDirectory/riscv-cfi-relocation32.asm"
	CFI64Source="$WorkDirectory/riscv-cfi-relocation64.asm"
	CFI32Object="$WorkDirectory/riscv-cfi-relocation32.o"
	CFI64Object="$WorkDirectory/riscv-cfi-relocation64.o"
	CFI32Relocations="$WorkDirectory/riscv-cfi-relocation32.relocations"
	CFI64Relocations="$WorkDirectory/riscv-cfi-relocation64.relocations"
	CFIScript="$WorkDirectory/riscv-cfi-relocation.ld"
	CFI32Output="$WorkDirectory/riscv-cfi-relocation32.elf"
	CFI64Output="$WorkDirectory/riscv-cfi-relocation64.elf"
	CFI32Frame="$WorkDirectory/riscv-cfi-relocation32.frame"
	CFI64Frame="$WorkDirectory/riscv-cfi-relocation64.frame"

	RequireCommand llvm-readelf

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

	for DataRelocationBits in 32 64; do
		case "$DataRelocationBits" in
		32) DataRelocationSource="$DataRelocation32Source" ;;
		64) DataRelocationSource="$DataRelocation64Source" ;;
		esac
		printf '%s\n' \
			"[BITS $DataRelocationBits]" \
			'SECTION .refs' \
			'GLOBAL refs' \
			'refs:' \
			'PCREL32 target + 4' \
			'GOT32 target + 8' \
			'PLT32 function + 12' \
			'SECTION .target' \
			'target:' \
			'DD 0' \
			'SECTION .text' \
			'function:' \
			'RET' > "$DataRelocationSource"
	done
	printf '%s\n' \
		'SECTIONS' \
		'{' \
		'  .refs 0x10000 : { *(.refs) }' \
		'  .target 0x20000 : { *(.target) }' \
		'  .text 0x30000 : { *(.text) }' \
		'  .got 0x40000 : { *(.got) }' \
		'}' > "$DataRelocationScript"
	AssembleSuccess riscv-data-relocation32 -march=riscv -mrelax -f elf32 \
		-o "$DataRelocation32Object" "$DataRelocation32Source"
	AssembleSuccess riscv-data-relocation64 -march=riscv -mrelax -f elf64 \
		-o "$DataRelocation64Object" "$DataRelocation64Source"
	LC_ALL=C llvm-readelf -rW "$DataRelocation32Object" > \
		"$DataRelocation32Relocations"
	LC_ALL=C llvm-readelf -rW "$DataRelocation64Object" > \
		"$DataRelocation64Relocations"
	for RelocationFile in \
		"$DataRelocation32Relocations" "$DataRelocation64Relocations"; do
		grep -Eq 'R_RISCV_32_PCREL[[:space:]]+.*target \+ 4$' \
			"$RelocationFile" || \
			Fail "RISC-V 32_PCREL relocation differs"
		grep -Eq 'R_RISCV_GOT32_PCREL[[:space:]]+.*target \+ 8$' \
			"$RelocationFile" || \
			Fail "RISC-V GOT32_PCREL relocation differs"
		grep -Eq 'R_RISCV_PLT32[[:space:]]+.*function \+ c$' \
			"$RelocationFile" || \
			Fail "RISC-V PLT32 relocation differs"
		[ "$(grep -c 'R_RISCV_RELAX' "$RelocationFile")" -eq 0 ] || \
			Fail "RISC-V 32-bit data relocation emitted a RELAX hint"
	done
	LinkSuccess riscv-data-relocation32-link ld.lld -m elf32lriscv \
		-e refs -T "$DataRelocationScript" -o "$DataRelocation32Output" \
		"$DataRelocation32Object"
	LinkSuccess riscv-data-relocation64-link ld.lld -m elf64lriscv \
		-e refs -T "$DataRelocationScript" -o "$DataRelocation64Output" \
		"$DataRelocation64Object"
	llvm-objcopy --dump-section .refs="$DataRelocation32Data" \
		"$DataRelocation32Output"
	llvm-objcopy --dump-section .refs="$DataRelocation64Data" \
		"$DataRelocation64Output"
	[ "$(NormalizeHex "$DataRelocation32Data")" = \
		'04 00 01 00 08 00 03 00 04 00 02 00' ] || \
		Fail "RISC-V ELF32 linked data relocation values differ"
	[ "$(NormalizeHex "$DataRelocation64Data")" = \
		'04 00 01 00 0c 00 03 00 04 00 02 00' ] || \
		Fail "RISC-V ELF64 linked data relocation values differ"
	printf '%s\n' \
		'[BITS 64]' \
		'PCREL32 7' > "$InvalidDataRelocationSource"
	AssembleFailure riscv-data-relocation-invalid -march=riscv -f elf64 \
		-o "$WorkDirectory/riscv-data-relocation-invalid.o" \
		"$InvalidDataRelocationSource"
	grep -Fq 'expects a symbol plus an addend' \
		"$WorkDirectory/riscv-data-relocation-invalid.stderr" || \
		Fail "RISC-V invalid data relocation diagnostic differs"

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

	printf '%s\n' \
		'EXTERN external_add' \
		'EXTERN external_sub' \
		'SECTION .data' \
		'DB external_add - external_sub + 1' \
		'DW external_add - external_sub + 2' \
		'DD external_add - external_sub + 3' \
		'DQ external_add - external_sub + 4' > "$ArithmeticSource"
	AssembleSuccess riscv-arithmetic-relocation32 -march=riscv -f elf32 \
		-o "$Arithmetic32Object" "$ArithmeticSource"
	AssembleSuccess riscv-arithmetic-relocation64 -march=riscv -f elf64 \
		-o "$Arithmetic64Object" "$ArithmeticSource"
	LC_ALL=C readelf -rW "$Arithmetic32Object" > "$Arithmetic32Relocations"
	LC_ALL=C readelf -rW "$Arithmetic64Object" > "$Arithmetic64Relocations"
	LC_ALL=C readelf -x .data "$Arithmetic32Object" > "$Arithmetic32Data"
	LC_ALL=C readelf -x .data "$Arithmetic64Object" > "$Arithmetic64Data"
	for RelocationFile in \
		"$Arithmetic32Relocations" "$Arithmetic64Relocations"; do
		[ "$(grep -Ec 'R_RISCV_(ADD|SUB)(8|16|32|64)' \
			"$RelocationFile")" -eq 8 ] || \
			Fail "RISC-V arithmetic relocation count differs"
		awk '
			/R_RISCV_ADD(8|16|32|64)/ {
				if (HasPending) Failed = 1
				Pending = $1
				HasPending = 1
				Adds++
				next
			}
			/R_RISCV_SUB(8|16|32|64)/ {
				if (!HasPending || Pending != $1) Failed = 1
				Pending = ""
				HasPending = 0
				Subtracts++
			}
			END {
				if (Failed || HasPending || Adds != 4 || Subtracts != 4) exit 1
			}
		' "$RelocationFile" || \
			Fail "RISC-V arithmetic relocation offsets or order differ"
		for ArithmeticWidth in 8 16 32 64; do
			case "$ArithmeticWidth" in
			8) ArithmeticAddend=1 ;;
			16) ArithmeticAddend=2 ;;
			32) ArithmeticAddend=3 ;;
			64) ArithmeticAddend=4 ;;
			esac
			grep -Eq "R_RISCV_ADD${ArithmeticWidth}[[:space:]]+.*external_add \\+ ${ArithmeticAddend}$" \
				"$RelocationFile" || \
				Fail "RISC-V ADD${ArithmeticWidth} relocation differs"
			grep -Eq "R_RISCV_SUB${ArithmeticWidth}[[:space:]]+.*external_sub \\+ 0$" \
				"$RelocationFile" || \
				Fail "RISC-V SUB${ArithmeticWidth} relocation differs"
		done
	done
	for DataFile in "$Arithmetic32Data" "$Arithmetic64Data"; do
		DataHex="$(awk '
			/^  0x/ {
				for (Column = 2; Column <= NF; Column++)
					if ($Column ~ /^[[:xdigit:]]+$/) printf "%s", tolower($Column)
			}
			END { print "" }
		' "$DataFile")"
		[ "$DataHex" = '000000000000000000000000000000' ] || \
			Fail "RISC-V arithmetic relocation storage is not zero-initialized"
	done

	printf '%s\n' \
		'GLOBAL global_begin' \
		'GLOBAL global_end' \
		'SECTION .text' \
		'global_begin:' \
		'local_begin:' \
		'CALL relax_target' \
		'local_end:' \
		'global_end:' \
		'NOP' \
		'SECTION .data' \
		'DB local_end - local_begin + 1' \
		'DW global_end - global_begin + 2' \
		'DD forward_end - forward_begin + 3' \
		'DQ local_end - local_begin + 4' \
		'SECTION .text' \
		'forward_begin:' \
		'NOP' \
		'NOP' \
		'forward_end:' \
		'NOP' \
		'SECTION .target' \
		'relax_target:' \
		'RET' > "$LabelDifferenceSource"
	printf '%s\n' \
		'SECTIONS' \
		'{' \
		'  .text 0x10000 : { *(.text) }' \
		'  .target 0x11000 : { *(.target) }' \
		'  .data 0x20000 : { *(.data) }' \
		'}' > "$LabelDifferenceScript"
	AssembleSuccess riscv-label-difference32 -march=riscv -mrelax -f elf32 \
		-o "$LabelDifference32Object" "$LabelDifferenceSource"
	AssembleSuccess riscv-label-difference64 -march=riscv -mrelax -f elf64 \
		-o "$LabelDifference64Object" "$LabelDifferenceSource"
	LC_ALL=C readelf -rW "$LabelDifference32Object" > \
		"$LabelDifference32Relocations"
	LC_ALL=C readelf -rW "$LabelDifference64Object" > \
		"$LabelDifference64Relocations"
	for RelocationFile in \
		"$LabelDifference32Relocations" "$LabelDifference64Relocations"; do
		[ "$(grep -Ec 'R_RISCV_(ADD|SUB)(8|16|32|64)' \
			"$RelocationFile")" -eq 8 ] || \
			Fail "RISC-V label difference relocation count differs"
		grep -Eq 'R_RISCV_ADD8[[:space:]]+.*local_end \+ 1$' \
			"$RelocationFile" || \
			Fail "RISC-V local label ADD8 relocation differs"
		grep -Eq 'R_RISCV_SUB8[[:space:]]+.*local_begin \+ 0$' \
			"$RelocationFile" || \
			Fail "RISC-V local label SUB8 relocation differs"
		grep -Eq 'R_RISCV_ADD16[[:space:]]+.*global_end \+ 2$' \
			"$RelocationFile" || \
			Fail "RISC-V global label ADD16 relocation differs"
		grep -Eq 'R_RISCV_SUB16[[:space:]]+.*global_begin \+ 0$' \
			"$RelocationFile" || \
			Fail "RISC-V global label SUB16 relocation differs"
		grep -Eq 'R_RISCV_ADD32[[:space:]]+.*forward_end \+ 3$' \
			"$RelocationFile" || \
			Fail "RISC-V forward label ADD32 relocation differs"
		grep -Eq 'R_RISCV_SUB32[[:space:]]+.*forward_begin \+ 0$' \
			"$RelocationFile" || \
			Fail "RISC-V forward label SUB32 relocation differs"
		grep -Eq 'R_RISCV_ADD64[[:space:]]+.*local_end \+ 4$' \
			"$RelocationFile" || \
			Fail "RISC-V local label ADD64 relocation differs"
		grep -Eq 'R_RISCV_SUB64[[:space:]]+.*local_begin \+ 0$' \
			"$RelocationFile" || \
			Fail "RISC-V local label SUB64 relocation differs"
	done
	LinkSuccess riscv-label-difference32-link ld.lld -m elf32lriscv --relax \
		-e global_begin -T "$LabelDifferenceScript" \
		-o "$LabelDifference32Output" "$LabelDifference32Object"
	LinkSuccess riscv-label-difference64-link ld.lld -m elf64lriscv --relax \
		-e global_begin -T "$LabelDifferenceScript" \
		-o "$LabelDifference64Output" "$LabelDifference64Object"
	llvm-objcopy --dump-section .data="$LabelDifference32Data" \
		"$LabelDifference32Output"
	llvm-objcopy --dump-section .data="$LabelDifference64Data" \
		"$LabelDifference64Output"
	for DataFile in "$LabelDifference32Data" "$LabelDifference64Data"; do
		[ "$(NormalizeHex "$DataFile")" = \
			'05 06 00 0b 00 00 00 08 00 00 00 00 00 00 00' ] || \
			Fail "RISC-V linked label difference data differs"
	done

	for ULEBBits in 32 64; do
		case "$ULEBBits" in
		32) ULEBSource="$ULEB32Source" ;;
		64) ULEBSource="$ULEB64Source" ;;
		esac
		printf '%s\n' \
			"[BITS $ULEBBits]" \
			'SECTION .text' \
			'GLOBAL start' \
			'start:' \
			'begin:' \
			'CALL target' \
			'end:' \
			'wide_begin:' \
			'CALL target' \
			'TIMES 32 NOP' \
			'wide_end:' \
			'SECTION .uleb_data' \
			'ULEB128 0, 127, 128, 624485' \
			'ULEB128 end - begin + 3' \
			'ULEB128 wide_end - wide_begin' \
			'SECTION .target' \
			'target:' \
			'RET' > "$ULEBSource"
	done
	printf '%s\n' \
		'SECTIONS' \
		'{' \
		'  .text 0x10000 : { *(.text) }' \
		'  .target 0x20000 : { *(.target) }' \
		'  .uleb_data 0x30000 : { *(.uleb_data) }' \
		'}' > "$ULEBScript"
	AssembleSuccess riscv-uleb128-relocation32 -march=riscv -mrelax \
		-f elf32 -o "$ULEB32Object" "$ULEB32Source"
	AssembleSuccess riscv-uleb128-relocation64 -march=riscv -mrelax \
		-f elf64 -o "$ULEB64Object" "$ULEB64Source"
	LC_ALL=C readelf -rW "$ULEB32Object" > "$ULEB32Relocations"
	LC_ALL=C readelf -rW "$ULEB64Object" > "$ULEB64Relocations"
	for RelocationFile in "$ULEB32Relocations" "$ULEB64Relocations"; do
		[ "$(grep -c 'R_RISCV_SET_ULEB128' "$RelocationFile")" -eq 2 ] || \
			Fail "RISC-V SET_ULEB128 relocation count differs"
		[ "$(grep -c 'R_RISCV_SUB_ULEB128' "$RelocationFile")" -eq 2 ] || \
			Fail "RISC-V SUB_ULEB128 relocation count differs"
		grep -Eq 'R_RISCV_SET_ULEB128[[:space:]]+.*end \+ 3$' \
			"$RelocationFile" || \
			Fail "RISC-V SET_ULEB128 addend differs"
		[ "$(grep -Ec 'R_RISCV_SUB_ULEB128[[:space:]]+.* \+ 0$' \
			"$RelocationFile")" -eq 2 ] || \
			Fail "RISC-V SUB_ULEB128 addend differs"
		awk '
			/R_RISCV_SET_ULEB128/ {
				if (Pending) Failed = 1
				Pending = $1
				next
			}
			/R_RISCV_SUB_ULEB128/ {
				if (!Pending || Pending != $1) Failed = 1
				Pending = ""
			}
			END { if (Failed || Pending) exit 1 }
		' "$RelocationFile" || \
			Fail "RISC-V ULEB128 relocation offsets or order differ"
	done
	LinkSuccess riscv-uleb128-relocation32-link ld.lld -m elf32lriscv \
		--relax -e start -T "$ULEBScript" -o "$ULEB32Output" "$ULEB32Object"
	LinkSuccess riscv-uleb128-relocation64-link ld.lld -m elf64lriscv \
		--relax -e start -T "$ULEBScript" -o "$ULEB64Output" "$ULEB64Object"
	llvm-objcopy --dump-section .uleb_data="$ULEB32Data" "$ULEB32Output"
	llvm-objcopy --dump-section .uleb_data="$ULEB64Data" "$ULEB64Output"
	for DataFile in "$ULEB32Data" "$ULEB64Data"; do
		[ "$(NormalizeHex "$DataFile")" = \
			'00 7f 80 01 e5 8e 26 07 84 01' ] || \
			Fail "RISC-V linked ULEB128 data differs"
	done
	printf '%s\n' \
		'[BITS 64]' \
		'ULEB128 -1' > "$ULEBNegativeSource"
	printf '%s\n' \
		'[BITS 64]' \
		'EXTERN external_value' \
		'ULEB128 external_value' > "$ULEBSymbolSource"
	printf '%s\n' \
		'[BITS 64]' \
		'ULEB128 "text"' > "$ULEBTypeSource"
	AssembleFailure riscv-uleb128-negative -march=riscv -f elf64 \
		-o "$WorkDirectory/riscv-uleb128-negative.o" "$ULEBNegativeSource"
	AssembleFailure riscv-uleb128-symbol -march=riscv -f elf64 \
		-o "$WorkDirectory/riscv-uleb128-symbol.o" "$ULEBSymbolSource"
	AssembleFailure riscv-uleb128-type -march=riscv -f elf64 \
		-o "$WorkDirectory/riscv-uleb128-type.o" "$ULEBTypeSource"
	grep -Fq 'ULEB128 value cannot be negative' \
		"$WorkDirectory/riscv-uleb128-negative.stderr" || \
		Fail "RISC-V negative ULEB128 diagnostic differs"
	grep -Fq 'ULEB128 only supports constants or symbol differences' \
		"$WorkDirectory/riscv-uleb128-symbol.stderr" || \
		Fail "RISC-V symbolic ULEB128 diagnostic differs"
	grep -Fq 'ULEB128 expects an integer expression' \
		"$WorkDirectory/riscv-uleb128-type.stderr" || \
		Fail "RISC-V ULEB128 type diagnostic differs"

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
	grep -Eq 'R_RISCV_BRANCH[[:space:]]+.*branch_target \+ 0$' \
		"$CrossSection32Relocations" || \
		Fail "RISC-V ELF32 cross-section branch relocation differs"
	grep -Eq 'R_RISCV_JAL[[:space:]]+.*jump_target \+ 0$' \
		"$CrossSection32Relocations" || \
		Fail "RISC-V ELF32 cross-section jump relocation differs"
	grep -Eq 'R_RISCV_BRANCH[[:space:]]+.*branch_target \+ 0$' \
		"$CrossSection64Relocations" || \
		Fail "RISC-V ELF64 cross-section branch relocation differs"
	grep -Eq 'R_RISCV_JAL[[:space:]]+.*jump_target \+ 0$' \
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

	for CFIBits in 32 64; do
		case "$CFIBits" in
		32) CFISource="$CFI32Source" ;;
		64) CFISource="$CFI64Source" ;;
		esac
		printf '%s\n' \
			"[BITS $CFIBits]" \
			'SECTION .text' \
			'GLOBAL start' \
			'start:' \
			'[DEBUG cfi_startproc]' \
			'[DEBUG cfi_def_cfa_offset 16]' \
			'CALL target' \
			'[DEBUG cfi_def_cfa_offset 32]' \
			'TIMES 20 NOP' \
			'[DEBUG cfi_def_cfa_offset 48]' \
			'TIMES 100 NOP' \
			'[DEBUG cfi_def_cfa_offset 64]' \
			'TIMES 17000 NOP' \
			'[DEBUG cfi_def_cfa_offset 80]' \
			'RET' \
			'[DEBUG cfi_endproc]' \
			'second:' \
			'[DEBUG cfi_startproc]' \
			'[DEBUG cfi_def_cfa_offset 96]' \
			'NOP' \
			'[DEBUG cfi_endproc]' \
			'SECTION .target' \
			'target:' \
			'RET' > "$CFISource"
	done
	printf '%s\n' \
		'SECTIONS' \
		'{' \
		'  .text 0x10000 : { *(.text) }' \
		'  .target 0x30000 : { *(.target) }' \
		'}' > "$CFIScript"
	AssembleSuccess riscv-cfi-relocation32 -march=riscv -mrelax -f elf32 \
		-o "$CFI32Object" "$CFI32Source"
	AssembleSuccess riscv-cfi-relocation64 -march=riscv -mrelax -f elf64 \
		-o "$CFI64Object" "$CFI64Source"
	LC_ALL=C readelf -rW "$CFI32Object" > "$CFI32Relocations"
	LC_ALL=C readelf -rW "$CFI64Object" > "$CFI64Relocations"
	for RelocationFile in "$CFI32Relocations" "$CFI64Relocations"; do
		for CFIWidth in 6 8 16 32; do
			[ "$(grep -c "R_RISCV_SET${CFIWidth}" "$RelocationFile")" -eq 1 ] || \
				Fail "RISC-V SET${CFIWidth} CFI relocation differs"
		done
		awk '
			/R_RISCV_SET(6|8|16|32)/ {
				if (Pending) Failed = 1
				Pending = $1
				next
			}
			/R_RISCV_SUB(6|8|16|32)/ && Pending {
				if ($1 != Pending) Failed = 1
				Pending = ""
			}
			END { if (Failed || Pending) exit 1 }
		' "$RelocationFile" || \
			Fail "RISC-V SET/SUB CFI relocation order differs"
	done
	LinkSuccess riscv-cfi-relocation32-link ld.lld -m elf32lriscv --relax \
		-e start -T "$CFIScript" -o "$CFI32Output" "$CFI32Object"
	LinkSuccess riscv-cfi-relocation64-link ld.lld -m elf64lriscv --relax \
		-e start -T "$CFIScript" -o "$CFI64Output" "$CFI64Object"
	LC_ALL=C readelf -wf "$CFI32Output" > "$CFI32Frame"
	LC_ALL=C readelf -wf "$CFI64Output" > "$CFI64Frame"
	for FrameFile in "$CFI32Frame" "$CFI64Frame"; do
		grep -Fq 'DW_CFA_advance_loc: 4 ' "$FrameFile" || \
			Fail "RISC-V relaxed SET6 CFI value differs"
		grep -Fq 'DW_CFA_advance_loc1: 80 ' "$FrameFile" || \
			Fail "RISC-V linked SET8 CFI value differs"
		grep -Fq 'DW_CFA_advance_loc2: 400 ' "$FrameFile" || \
			Fail "RISC-V linked SET16 CFI value differs"
		grep -Fq 'DW_CFA_advance_loc4: 68000 ' "$FrameFile" || \
			Fail "RISC-V linked SET32 CFI value differs"
		[ "$(grep -c 'FDE cie=' "$FrameFile")" -eq 2 ] || \
			Fail "RISC-V multiple CFI procedures differ"
	done
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
		'[INST i_zbc]' \
		'NOP' > "$UnknownExtensionSource"
	AssembleFailure riscv-unknown-extension -f bin \
		-o "$WorkDirectory/riscv-unknown-extension.bin" "$UnknownExtensionSource"
	grep -Fq "unknown 'cpu' type" \
		"$WorkDirectory/riscv-unknown-extension.stderr" || \
		Fail "RISC-V unknown extension diagnostic differs"
	Pass "RISC-V instruction-set extensions"
}

CheckRISCVBitManipulation()
{
	RV32Source="$WorkDirectory/riscv-bitmanip32.asm"
	RV32Output="$WorkDirectory/riscv-bitmanip32.bin"
	RV64Source="$WorkDirectory/riscv-bitmanip64.asm"
	RV64Output="$WorkDirectory/riscv-bitmanip64.bin"
	DisabledSource="$WorkDirectory/riscv-bitmanip-disabled.asm"
	BitModeSource="$WorkDirectory/riscv-bitmanip-bit-mode.asm"
	ImmediateSource="$WorkDirectory/riscv-bitmanip-immediate.asm"
	OrderSource="$WorkDirectory/riscv-bitmanip-order.asm"
	RV32Expected='b3 20 31 20 33 c2 62 20 b3 63 94 20 33 f5 c5 40 b3 66 f7 40 33 c8 28 41 93 10 01 60 93 11 12 60 93 12 23 60 b3 43 94 0a 33 d5 c5 0a b3 66 f7 0a 33 f8 28 0b 93 19 4a 60 93 1a 5b 60 b3 4b 0c 08 b3 1c bd 61 33 de ee 61 93 df f0 61 13 d1 71 28 13 d2 82 69 33 93 83 48 93 14 f5 49 b3 55 d6 48 13 d7 f7 49 33 98 28 69 93 19 fa 69 b3 1a 7b 29 13 9c fc 29 33 dd cd 0f b3 7e ff 0f'
	RV64Expected='bb 00 31 08 3b a2 62 20 bb 43 94 20 3b e5 c5 20 9b 16 f7 0b 9b 17 08 60 9b 18 19 60 9b 19 2a 60 bb 4a 0b 08 bb 1b 9c 61 3b dd cd 61 9b 5e ff 61 93 df 80 6b 13 91 f1 4b 13 d2 f2 4b 13 93 f3 6b 13 94 f4 2b'

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 32]' \
		'[INST i_zba_zbb_zbs_zicond]' \
		'sh1add x1, x2, x3' \
		'sh2add x4, x5, x6' \
		'sh3add x7, x8, x9' \
		'andn x10, x11, x12' \
		'orn x13, x14, x15' \
		'xnor x16, x17, x18' \
		'clz x1, x2' \
		'ctz x3, x4' \
		'cpop x5, x6' \
		'min x7, x8, x9' \
		'minu x10, x11, x12' \
		'max x13, x14, x15' \
		'maxu x16, x17, x18' \
		'sext.b x19, x20' \
		'sext.h x21, x22' \
		'zext.h x23, x24' \
		'rol x25, x26, x27' \
		'ror x28, x29, x30' \
		'rori x31, x1, 31' \
		'orc.b x2, x3' \
		'rev8 x4, x5' \
		'bclr x6, x7, x8' \
		'bclri x9, x10, 31' \
		'bext x11, x12, x13' \
		'bexti x14, x15, 31' \
		'binv x16, x17, x18' \
		'binvi x19, x20, 31' \
		'bset x21, x22, x23' \
		'bseti x24, x25, 31' \
		'czero.eqz x26, x27, x28' \
		'czero.nez x29, x30, x31' > "$RV32Source"
	AssembleSuccess riscv-bitmanip32 -f bin -o "$RV32Output" "$RV32Source"
	[ "$(NormalizeHex "$RV32Output")" = "$RV32Expected" ] || \
		Fail "RISC-V RV32 bit-manipulation bytes differ"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[INST i_zba_zbb_zbs]' \
		'add.uw x1, x2, x3' \
		'sh1add.uw x4, x5, x6' \
		'sh2add.uw x7, x8, x9' \
		'sh3add.uw x10, x11, x12' \
		'slli.uw x13, x14, 63' \
		'clzw x15, x16' \
		'ctzw x17, x18' \
		'cpopw x19, x20' \
		'zext.h x21, x22' \
		'rolw x23, x24, x25' \
		'rorw x26, x27, x28' \
		'roriw x29, x30, 31' \
		'rev8 x31, x1' \
		'bclri x2, x3, 63' \
		'bexti x4, x5, 63' \
		'binvi x6, x7, 63' \
		'bseti x8, x9, 63' > "$RV64Source"
	AssembleSuccess riscv-bitmanip64 -f bin -o "$RV64Output" "$RV64Source"
	[ "$(NormalizeHex "$RV64Output")" = "$RV64Expected" ] || \
		Fail "RISC-V RV64 bit-manipulation bytes differ"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[INST i]' \
		'sh1add x1, x2, x3' \
		'clz x1, x2' \
		'bset x1, x2, x3' \
		'czero.eqz x1, x2, x3' > "$DisabledSource"
	AssembleFailure riscv-bitmanip-disabled -f bin \
		-o "$WorkDirectory/riscv-bitmanip-disabled.bin" "$DisabledSource"
	[ "$(grep -Fc 'is disabled by the current instruction set' \
		"$WorkDirectory/riscv-bitmanip-disabled.stderr")" -eq 4 ] || \
		Fail "RISC-V bit-manipulation extension diagnostics differ"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 32]' \
		'[INST i_zba_zbb]' \
		'add.uw x1, x2, x3' \
		'clzw x1, x2' > "$BitModeSource"
	AssembleFailure riscv-bitmanip-bit-mode -f bin \
		-o "$WorkDirectory/riscv-bitmanip-bit-mode.bin" "$BitModeSource"
	[ "$(grep -Fc 'not available in the current bit mode' \
		"$WorkDirectory/riscv-bitmanip-bit-mode.stderr")" -eq 2 ] || \
		Fail "RISC-V bit-manipulation XLEN diagnostics differ"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 32]' \
		'[INST i_zbb_zbs]' \
		'rori x1, x2, 32' \
		'bseti x1, x2, 32' > "$ImmediateSource"
	AssembleFailure riscv-bitmanip-immediate -f bin \
		-o "$WorkDirectory/riscv-bitmanip-immediate.bin" "$ImmediateSource"
	[ "$(grep -Fc 'shift amount is outside the register width' \
		"$WorkDirectory/riscv-bitmanip-immediate.stderr")" -eq 2 ] || \
		Fail "RISC-V bit-manipulation immediate diagnostics differ"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[INST i_zbb_zba]' \
		'nop' > "$OrderSource"
	AssembleFailure riscv-bitmanip-order -f bin \
		-o "$WorkDirectory/riscv-bitmanip-order.bin" "$OrderSource"
	grep -Fq "unknown 'cpu' type" \
		"$WorkDirectory/riscv-bitmanip-order.stderr" || \
		Fail "RISC-V bit-manipulation extension order diagnostic differs"
	Pass "RISC-V bit-manipulation and conditional extensions"
}

CheckRISCVPseudoInstructions()
{
	RV64Source="$WorkDirectory/riscv-pseudo64.asm"
	RV64Output="$WorkDirectory/riscv-pseudo64.bin"
	RV32Source="$WorkDirectory/riscv-pseudo32.asm"
	RV32Output="$WorkDirectory/riscv-pseudo32.bin"
	NativeSource="$WorkDirectory/riscv-pseudo-native.asm"
	NativeOutput="$WorkDirectory/riscv-pseudo-native.bin"
	InvalidSource="$WorkDirectory/riscv-pseudo-invalid.asm"
	DisabledSource="$WorkDirectory/riscv-pseudo-disabled.asm"
	ImmediateSource="$WorkDirectory/riscv-pseudo-immediate.asm"
	RelocationSource="$WorkDirectory/riscv-pseudo-relocation.asm"
	RelocationObject="$WorkDirectory/riscv-pseudo-relocation.o"
	RV64Expected='37 25 01 00 1b 05 55 34 97 05 00 00 93 85 45 06 17 03 00 00 67 00 c3 05 63 0a 05 04 13 36 15 00 b3 06 a0 40 13 47 f5 ff 9b 07 05 00 13 78 f5 0f 93 18 85 03 93 d8 88 43 13 14 05 03 13 54 04 43 93 14 05 03 93 d4 04 03 13 19 05 02 13 59 09 02 f3 29 00 30 73 90 09 30 73 a0 09 30 73 b0 09 30 73 d0 00 30 73 60 01 30 73 f0 01 30 67 80 00 00'
	RV32Expected='13 95 85 01 13 55 85 41 13 96 06 01 13 56 06 41 13 97 07 01 13 57 07 01'

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[INST i_zicsr]' \
		'start:' \
		'li a0, 0x12345' \
		'la a1, target' \
		'tail target' \
		'beqz a0, target' \
		'seqz a2, a0' \
		'neg a3, a0' \
		'not a4, a0' \
		'sext.w a5, a0' \
		'zext.b a6, a0' \
		'sext.b a7, a0' \
		'sext.h s0, a0' \
		'zext.h s1, a0' \
		'zext.w s2, a0' \
		'csrr s3, mstatus' \
		'csrw mstatus, s3' \
		'csrs mstatus, s3' \
		'csrc mstatus, s3' \
		'csrwi mstatus, 1' \
		'csrsi mstatus, 2' \
		'csrci mstatus, 3' \
		'target:' \
		'ret' > "$RV64Source"
	AssembleSuccess riscv-pseudo64 -f bin -o "$RV64Output" "$RV64Source"
	[ "$(NormalizeHex "$RV64Output")" = "$RV64Expected" ] || \
		Fail "RISC-V RV64 pseudo-instruction bytes differ"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 32]' \
		'[INST i]' \
		'sext.b a0, a1' \
		'sext.h a2, a3' \
		'zext.h a4, a5' > "$RV32Source"
	AssembleSuccess riscv-pseudo32 -f bin -o "$RV32Output" "$RV32Source"
	[ "$(NormalizeHex "$RV32Output")" = "$RV32Expected" ] || \
		Fail "RISC-V RV32 pseudo-instruction bytes differ"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[INST i_zba_zbb]' \
		'sext.b a0, a1' \
		'sext.h a2, a3' \
		'zext.h a4, a5' \
		'zext.w a6, a7' > "$NativeSource"
	AssembleSuccess riscv-pseudo-native -f bin \
		-o "$NativeOutput" "$NativeSource"
	[ "$(NormalizeHex "$NativeOutput")" = \
		'13 95 45 60 13 96 56 60 3b c7 07 08 3b 88 08 08' ] || \
		Fail "RISC-V extension-aware pseudo-instruction bytes differ"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 32]' \
		'[INST i]' \
		'zext.w a0, a1' > "$InvalidSource"
	AssembleFailure riscv-pseudo-invalid -f bin \
		-o "$WorkDirectory/riscv-pseudo-invalid.bin" "$InvalidSource"
	grep -Fq 'integer extension is not available in the current bit mode' \
		"$WorkDirectory/riscv-pseudo-invalid.stderr" || \
		Fail "RISC-V ZEXT.W bit-mode diagnostic differs"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[INST i]' \
		'csrr a0, mstatus' > "$DisabledSource"
	AssembleFailure riscv-pseudo-disabled -f bin \
		-o "$WorkDirectory/riscv-pseudo-disabled.bin" "$DisabledSource"
	grep -Fq 'CSR alias is disabled by the current instruction set' \
		"$WorkDirectory/riscv-pseudo-disabled.stderr" || \
		Fail "RISC-V CSR alias extension diagnostic differs"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[INST i]' \
		'li a0, target' \
		'target:' \
		'nop' > "$ImmediateSource"
	AssembleFailure riscv-pseudo-immediate -f bin \
		-o "$WorkDirectory/riscv-pseudo-immediate.bin" "$ImmediateSource"
	grep -Fq 'LI requires an absolute expression' \
		"$WorkDirectory/riscv-pseudo-immediate.stderr" || \
		Fail "RISC-V LI expression diagnostic differs"

	printf '%s\n' \
		'[ARCH riscv]' \
		'[BITS 64]' \
		'[INST i]' \
		'SECTION .text' \
		'EXTERN object' \
		'EXTERN function' \
		'la a0, object' \
		'tail function' > "$RelocationSource"
	AssembleSuccess riscv-pseudo-relocation -march=riscv -f elf64 \
		-o "$RelocationObject" "$RelocationSource"
	LC_ALL=C readelf -rW "$RelocationObject" > \
		"$WorkDirectory/riscv-pseudo-relocation.relocations"
	grep -Fq 'R_RISCV_PCREL_HI20' \
		"$WorkDirectory/riscv-pseudo-relocation.relocations" || \
		Fail "RISC-V LA high relocation is missing"
	grep -Fq 'R_RISCV_PCREL_LO12_I' \
		"$WorkDirectory/riscv-pseudo-relocation.relocations" || \
		Fail "RISC-V LA low relocation is missing"
	grep -Fq 'R_RISCV_CALL_PLT' \
		"$WorkDirectory/riscv-pseudo-relocation.relocations" || \
		Fail "RISC-V TAIL relocation is missing"
	Pass "RISC-V pseudo-instructions"
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
		'[INST imafdcv_zba_zbb_zbs_zicond_zifencei]' \
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
	grep -Fq '_zba1p0_zbb1p0_zbs1p0_zicond1p0_zicsr2p0_zifencei2p0' \
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


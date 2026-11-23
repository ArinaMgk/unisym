#!/bin/bash
# ASCII ShellScript TAB4 LF
# Docutitle: AASM ARM32 Regression Checks
# Copyright: UNISYM, under Apache License 2.0

CheckARM32()
{
	SourceDirectory="$RepositoryDirectory/demo/test/clover/arch-arm32"
	OutputFile="$WorkDirectory/arm32-basic.bin"
	CaseOutput="$WorkDirectory/arm32-case.bin"
	Expected='00 f0 20 e3 01 00 a0 e1 78 26 05 e3 34 22 41 e3 00 30 e0 e3 01 40 80 e0 10 50 94 e2 01 61 45 10 02 00 61 e0 05 30 a4 e0 08 60 c7 e0 02 01 56 e3 0a 00 19 e1 0c 00 3b e1 06 70 04 e0 62 84 85 e1 06 90 27 e0 91 02 00 e0 95 06 14 e0 04 90 8d e5 04 a0 9d e5 01 80 cd e5 01 b0 dd e5 b2 80 4d e1 b2 c0 5d e1 d1 00 dd e1 f2 10 dd e1 01 c0 dd e5 d1 00 dd e1 b2 80 4d e1 00 00 00 0a 00 00 a0 e3 00 00 00 eb 00 00 00 ea 1e ff 2f e1 3a ff 2f e1 19 ff 2f e1 38 ff 2f e1 1e ff 2f e1'

	AssembleSuccess arm32-basic -march=arm32 -f bin -o "$OutputFile" \
		"$RepositoryDirectory/demo/test/clover/arch-arm32/basic.asm"
	[ "$(NormalizeHex "$OutputFile")" = "$Expected" ] || \
		Fail "ARM32 minimal instruction bytes differ"
	AssembleSuccess arm32-case -f bin -o "$CaseOutput" \
		"$SourceDirectory/case-insensitive.asm"
	[ "$(NormalizeHex "$CaseOutput")" = \
		'01 00 a0 e1 03 20 92 00 78 46 05 03 34 42 41 03 1e ff 2f e1' ] || \
		Fail "ARM32 case-insensitive instruction bytes differ"
	Pass "ARM32 minimal instruction encoding"
}

CheckCortexM7T32()
{
	SourceDirectory="$RepositoryDirectory/demo/test/clover/arch-arm32"
	OutputFile="$WorkDirectory/cortex-m7-t32.bin"
	CaseOutput="$WorkDirectory/cortex-m7-t32-registers.bin"
	AssembleSuccess cortex-m7-t32 -march=arm32 -f bin -o "$OutputFile" \
		"$RepositoryDirectory/demo/test/clover/arch-arm32/cortex-m7.asm"
	[ "$(NormalizeHex "$OutputFile")" = \
		'01 20 01 30 02 39 01 28 08 43 48 60 4a 68 00 d2 05 23 00 bf 70 47' ] || \
		Fail "Cortex-M7 T32 instruction bytes differ"
	AssembleSuccess cortex-m7-t32-registers -f bin -o "$CaseOutput" \
		"$SourceDirectory/cortex-m7-registers.asm"
	Actual="$(NormalizeHex "$CaseOutput")"
	[ "$Actual" = \
		'6a 46 73 46 84 60 c0 f8 18 80 c0 f8 24 b0 94 68 d2 f8 18 80 d2 f8 24 b0 8d 46 18 47' ] || {
		printf 'actual T32 bytes: %s\n' "$Actual" >&2
		Fail "Cortex-M7 T32 high-register save/restore forms differ"
	}
	Pass "Cortex-M7 T32 register transfer and wide load/store encoding"
}

CheckCortexM7Jump()
{
	ObjectFile="$WorkDirectory/cortex-m7-jump.o"
	TextFile="$WorkDirectory/cortex-m7-jump.text"
	SymbolFile="$WorkDirectory/cortex-m7-jump.symbols"
	RequireCommand arm-none-eabi-objcopy
	RequireCommand arm-none-eabi-readelf
	AssembleSuccess cortex-m7-jump -march=arm32 -f elf32 -o "$ObjectFile" \
		"$RepositoryDirectory/lib/asm/arm32/jump.asm"
	arm-none-eabi-objcopy --dump-section .text="$TextFile" "$ObjectFile"
	[ "$(NormalizeHex "$TextFile")" = \
		'71 46 01 60 69 46 41 60 84 60 c5 60 06 61 47 61 c0 f8 18 80 c0 f8 1c 90 c0 f8 20 a0 c0 f8 24 b0 00 20 70 47 02 46 0b 46 00 2b 00 d1 01 23 94 68 d5 68 16 69 57 69 d2 f8 18 80 d2 f8 1c 90 d2 f8 20 a0 d2 f8 24 b0 d2 f8 00 c0 51 68 8d 46 18 46 60 47' ] || \
		Fail "Cortex-M7 jump implementation bytes differ"
	arm-none-eabi-readelf -s "$ObjectFile" > "$SymbolFile"
	grep -Eq '[[:space:]]FUNC[[:space:]]+GLOBAL[[:space:]].*[[:space:]]MarkPoint$' \
		"$SymbolFile" || Fail "MarkPoint is not a global function symbol"
	grep -Eq '[[:space:]]FUNC[[:space:]]+GLOBAL[[:space:]].*[[:space:]]JumpPoint$' \
		"$SymbolFile" || Fail "JumpPoint is not a global function symbol"
	Pass "Cortex-M7 non-local jump implementation encoding"
}

CheckARM32Fixups()
{
	SourceDirectory="$RepositoryDirectory/demo/test/clover/arch-arm32"
	OutputFile="$WorkDirectory/arm32-fixup.bin"
	AddressOutput="$WorkDirectory/arm32-address-fixup.bin"
	CallOutput="$WorkDirectory/arm32-call-fixup.bin"
	Expected='01 00 00 ea 20 00 00 e3 00 00 40 e3 00 00 00 eb 03 00 00 ea 24 10 00 e3 00 10 40 e3 1e ff 2f e1 00 00 00 00 00 f0 20 e3'
	AssembleSuccess arm32-fixup -f bin -o "$OutputFile" \
		"$SourceDirectory/fixup.asm"
	[ "$(NormalizeHex "$OutputFile")" = "$Expected" ] || \
		Fail "ARM32 branch and address fixup bytes differ"
	AssembleSuccess arm32-address-fixup -f bin \
		-o "$AddressOutput" "$SourceDirectory/address-fixup.asm"
	[ "$(NormalizeHex "$AddressOutput")" = \
		'78 26 05 e3 34 22 41 e3 00 00 00 00 00 00 00 00' ] || \
		Fail "ARM32 split address fixup bytes differ"
	AssembleSuccess arm32-call-fixup -f bin -o "$CallOutput" \
		"$SourceDirectory/call-fixup.asm"
	[ "$(NormalizeHex "$CallOutput")" = 'ff ff ff eb 1e ff 2f e1' ] || \
		Fail "ARM32 cross-section call fixup bytes differ"
	Pass "ARM32 branch and address fixups"
}

CheckARM32ELF()
{
	SourceDirectory="$RepositoryDirectory/demo/test/clover/arch-arm32"
	RequireCommand llvm-readelf
	RequireCommand ld.lld
	RequireCommand arm-none-eabi-readelf
	RequireCommand arm-none-eabi-ld
	RequireCommand arm-none-eabi-gcc
	ObjectFile="$WorkDirectory/arm32-elf.o"
	ProviderObject="$WorkDirectory/arm32-elf-provider.o"
	LinkedFile="$WorkDirectory/arm32-elf-linked"
	HeaderFile="$WorkDirectory/arm32-elf.header"
	RelocationFile="$WorkDirectory/arm32-elf.relocations"
	SymbolFile="$WorkDirectory/arm32-elf.symbols"
	AttributeFile="$WorkDirectory/arm32-elf.attributes"
	AssembleSuccess arm32-elf -march=arm32 -f elf32 \
		-o "$ObjectFile" "$SourceDirectory/elf-a32.asm"
	AssembleSuccess arm32-elf-provider -march=arm32 -f elf32 \
		-o "$ProviderObject" "$SourceDirectory/elf-a32-provider.asm"
	llvm-readelf -h "$ObjectFile" > "$HeaderFile"
	llvm-readelf -r "$ObjectFile" > "$RelocationFile"
	llvm-readelf -s "$ObjectFile" > "$SymbolFile"
	llvm-readelf --arch-specific "$ObjectFile" > "$AttributeFile"
	grep -Fq 'Class:                             ELF32' "$HeaderFile" || \
		Fail "ARM32 ELF class differs"
	grep -Fq 'Machine:                           ARM' "$HeaderFile" || \
		Fail "ARM32 ELF machine differs"
	grep -Fq 'Flags:                             0x5000000' "$HeaderFile" || \
		Fail "ARM32 ELF flags differ"
	grep -Fq 'Description: ARM v7' "$AttributeFile" || \
		Fail "ARM32 ELF CPU attribute differs"
	grep -Fq 'Description: Application' "$AttributeFile" || \
		Fail "ARM32 ELF profile attribute differs"
	for Relocation in R_ARM_CALL R_ARM_JUMP24 R_ARM_MOVW_ABS_NC \
		R_ARM_MOVT_ABS R_ARM_ABS32; do
		grep -Fq "$Relocation" "$RelocationFile" || \
			Fail "ARM32 ELF relocation is missing: $Relocation"
	done
	grep -Eq '[[:space:]]\$a$' "$SymbolFile" || \
		Fail "ARM32 code mapping symbol is missing"
	grep -Eq '[[:space:]]\$d$' "$SymbolFile" || \
		Fail "ARM32 data mapping symbol is missing"
	LinkSuccess arm32-elf-link ld.lld -m armelf_linux_eabi -e entry \
		-o "$LinkedFile" "$ObjectFile" "$ProviderObject"
	ThumbObject="$WorkDirectory/arm32-thumb-elf.o"
	ThumbProviderObject="$WorkDirectory/arm32-thumb-provider.o"
	ThumbRelocations="$WorkDirectory/arm32-thumb-relocations"
	ThumbSymbols="$WorkDirectory/arm32-thumb-symbols"
	ThumbAttributes="$WorkDirectory/arm32-thumb-attributes"
	ThumbLinked="$WorkDirectory/arm32-thumb-linked"
	AssembleSuccess arm32-thumb-elf -march=arm32 -f elf32 \
		-o "$ThumbObject" "$SourceDirectory/elf-t32.asm"
	AssembleSuccess arm32-thumb-provider -march=arm32 -f elf32 \
		-o "$ThumbProviderObject" "$SourceDirectory/elf-t32-provider.asm"
	arm-none-eabi-readelf -r "$ThumbObject" > "$ThumbRelocations"
	arm-none-eabi-readelf -s "$ThumbObject" > "$ThumbSymbols"
	arm-none-eabi-readelf -A "$ThumbObject" > "$ThumbAttributes"
	grep -Fq 'R_ARM_THM_JUMP11' "$ThumbRelocations" || \
		Fail "Thumb short-branch relocation is missing"
	grep -Fq 'R_ARM_THM_JUMP8' "$ThumbRelocations" || \
		Fail "Thumb conditional-branch relocation is missing"
	grep -Eq '[[:space:]]\$t$' "$ThumbSymbols" || \
		Fail "Thumb code mapping symbol is missing"
	grep -Fq '00000001     0 FUNC    GLOBAL' "$ThumbSymbols" || \
		Fail "Thumb function symbol state bit is missing"
	grep -Fq 'Tag_CPU_arch: v7E-M' "$ThumbAttributes" || \
		Fail "Cortex-M7 ELF CPU attribute differs"
	grep -Fq 'Tag_THUMB_ISA_use: Thumb-2' "$ThumbAttributes" || \
		Fail "Thumb-2 ELF ISA attribute is missing"
	LinkSuccess arm32-thumb-elf-link arm-none-eabi-ld -Ttext=0x08000000 \
		-e entry -o "$ThumbLinked" "$ThumbObject" "$ThumbProviderObject"
	ThumbCallerSource="$WorkDirectory/arm32-thumb-caller.c"
	ThumbCallerObject="$WorkDirectory/arm32-thumb-caller.o"
	ThumbABOLinked="$WorkDirectory/arm32-thumb-abi-linked.o"
	ThumbJumpObject="$WorkDirectory/arm32-thumb-jump.o"
	AssembleSuccess arm32-thumb-jump-elf -march=arm32 -f elf32 \
		-o "$ThumbJumpObject" \
		"$RepositoryDirectory/lib/asm/arm32/jump.asm"
	printf '%s\n' \
		'extern int MarkPoint(void*);' \
		'extern void JumpPoint(const void*, unsigned);' \
		'int invoke_jump(void* state) {' \
		'  int value = MarkPoint(state);' \
		'  if (value) JumpPoint(state, (unsigned)value);' \
		'  return value;' \
		'}' > "$ThumbCallerSource"
	LinkSuccess arm32-thumb-caller-compile arm-none-eabi-gcc \
		-mcpu=cortex-m7 -mthumb -ffreestanding -c "$ThumbCallerSource" \
		-o "$ThumbCallerObject"
	LinkSuccess arm32-thumb-abi-link arm-none-eabi-ld -r \
		-o "$ThumbABOLinked" "$ThumbCallerObject" \
		"$ThumbJumpObject"
	Pass "ARM32 ELF object output"
}

CheckARM32Diagnostics()
{
	SourceDirectory="$RepositoryDirectory/demo/test/clover/arch-arm32"
	InvalidError="$WorkDirectory/arm32-invalid.stderr"
	AssembleFailure arm32-invalid -f bin \
		-o "$WorkDirectory/arm32-invalid.bin" \
		"$SourceDirectory/invalid.asm"
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
	AssembleFailure arm32-external -f bin \
		-o "$WorkDirectory/arm32-external.bin" \
		"$SourceDirectory/external-reference.asm"
	grep -Fq 'binary output format does not support external references' \
		"$WorkDirectory/arm32-external.stderr" || \
		Fail "ARM32 external-reference diagnostic differs"
	AssembleFailure arm32-range -f bin \
		-o "$WorkDirectory/arm32-range.bin" \
		"$SourceDirectory/branch-range.asm"
	grep -Fq 'ARM32 branch target is out of range' \
		"$WorkDirectory/arm32-range.stderr" || \
		Fail "ARM32 branch-range diagnostic differs"
	AssembleFailure arm32-address-range -f bin \
		-o "$WorkDirectory/arm32-address-range.bin" \
		"$SourceDirectory/address-range.asm"
	grep -Fq 'ARM32 label address is out of range' \
		"$WorkDirectory/arm32-address-range.stderr" || \
		Fail "ARM32 label-address range diagnostic differs"
	Pass "ARM32 operand diagnostics"
}

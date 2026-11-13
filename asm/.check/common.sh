#!/bin/bash
# ASCII ShellScript TAB4 LF
# Docutitle: AASM Common Regression Support
# Copyright: UNISYM, under Apache License 2.0

RepositoryDirectory="$(cd "$ScriptDirectory/.." && pwd)"
TestDirectory="$RepositoryDirectory/demo/test/clover/arch-x86_64"
WorkDirectory="$(mktemp -d "${TMPDIR:-/tmp}/aasm-check.XXXXXX")"

Cleanup()
{
	case "$WorkDirectory" in
	"${TMPDIR:-/tmp}"/aasm-check.*)
		find "$WorkDirectory" -depth -delete
		;;
	esac
}

trap Cleanup EXIT

Fail()
{
	printf '\033[1;31mfailed\033[0m: %s\n' "$1" >&2
	exit 1
}

Pass()
{
	printf '\033[1;32mpassed\033[0m: %s\n' "$1"
}

RequireCommand()
{
	command -v "$1" >/dev/null 2>&1 || Fail "required command not found: $1"
}

if [ -n "${AASM:-}" ]; then
	AasmExecutable="$AASM"
else
	AasmExecutable="$(command -v aasm 2>/dev/null || true)"
fi

[ -n "$AasmExecutable" ] || Fail "set AASM to the built assembler executable"
[ -x "$AasmExecutable" ] || Fail "assembler is not executable: $AasmExecutable"

RequireCommand awk
RequireCommand cmp
RequireCommand diff
RequireCommand find
RequireCommand grep
RequireCommand mktemp
RequireCommand od
RequireCommand readelf
RequireCommand sed
RequireCommand tr
RequireCommand wc

NormalizeHex()
{
	HexText="$(od -An -v -tx1 "$1" | tr -s '[:space:]' ' ' | sed 's/^ //;s/ $//')"
	printf '%s\n' "$HexText"
}

AssembleSuccess()
{
	local CheckName="$1"
	shift
	"$AasmExecutable" "$@" >"$WorkDirectory/$CheckName.stdout" \
		2>"$WorkDirectory/$CheckName.stderr" || {
		sed -n '1,20p' "$WorkDirectory/$CheckName.stderr" >&2
		Fail "$CheckName did not assemble"
	}
	[ ! -s "$WorkDirectory/$CheckName.stderr" ] || {
		sed -n '1,20p' "$WorkDirectory/$CheckName.stderr" >&2
		Fail "$CheckName produced diagnostics"
	}
}

AssembleFailure()
{
	local CheckName="$1"
	local ResultCode
	shift
	"$AasmExecutable" "$@" >"$WorkDirectory/$CheckName.stdout" \
		2>"$WorkDirectory/$CheckName.stderr"
	ResultCode="$?"
	[ "$ResultCode" -ne 0 ] || Fail "$CheckName was accepted"
}

LinkSuccess()
{
	local CheckName="$1"
	shift
	"$@" >"$WorkDirectory/$CheckName.stdout" \
		2>"$WorkDirectory/$CheckName.stderr" || {
		sed -n '1,20p' "$WorkDirectory/$CheckName.stderr" >&2
		Fail "$CheckName did not link"
	}
}

LinkFailure()
{
	local CheckName="$1"
	local ResultCode
	shift
	"$@" >"$WorkDirectory/$CheckName.stdout" \
		2>"$WorkDirectory/$CheckName.stderr"
	ResultCode="$?"
	[ "$ResultCode" -ne 0 ] || Fail "$CheckName linked"
}

CheckPreprocessor()
{
	RawFile="$WorkDirectory/preprocess.raw"
	ActualFile="$WorkDirectory/preprocess.txt"
	ExpectedFile="$WorkDirectory/preprocess.expected"
	printf '%s\n' \
		'[bits 64]' \
		'[section .text]' \
		'db 0x31' \
		'db 0x22' \
		'db 0x22' \
		'db 0x22' > "$ExpectedFile"

	AssembleSuccess preprocess -E -I"$TestDirectory/include/" -o "$RawFile" \
		"$TestDirectory/preprocess.asm"

	sed -e '/^%line /d' -e '/^[[:space:]]*$/d' \
		-e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//' "$RawFile" > "$ActualFile"

	if ! cmp -s "$ExpectedFile" "$ActualFile"; then
		diff -u "$ExpectedFile" "$ActualFile" >&2
		Fail "preprocessor output differs"
	fi
	Pass "preprocessor output"
}

CheckCommonStatements()
{
	SourceFile="$WorkDirectory/common-statements.asm"
	InputFile="$WorkDirectory/common-statements.input"
	OutputFile="$WorkDirectory/common-statements.bin"
	ExpectedFile="$WorkDirectory/common-statements.expected"
	ActualHexFile="$WorkDirectory/common-statements.hex"
	ExpectedHexFile="$WorkDirectory/common-statements.expected.hex"

	printf '\x10\x20\x30\x40\x50' > "$InputFile"
	printf '\x03\x04\x34\x12\x34\x12\x88\x77\x66\x55\x08\x07\x06\x05\x04\x03\x02\x01' > "$ExpectedFile"
	printf '\x54\x00\x00\x00\x00\x00\x00\x00\x00\x00' >> "$ExpectedFile"
	printf '\x4f\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' >> "$ExpectedFile"
	printf '\x59\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' >> "$ExpectedFile"
	printf '\x20\x30\x40' >> "$ExpectedFile"
	printf '%s\n' \
		'bits 64' \
		'section .data' \
		'constant equ 3' \
		'db constant' \
		'eax: equ 4' \
		'db eax' \
		'times 2 dw 0x1234' \
		'dd 0x55667788' \
		'dq 0x0102030405060708' \
		'dt "T"' \
		'do "O"' \
		'dy "Y"' \
		"incbin \"$InputFile\", 1, 3" \
		'section .bss' \
		'resb constant' > "$SourceFile"

	AssembleSuccess common-statements -Ox -f bin -o "$OutputFile" "$SourceFile"

	if ! cmp -s "$ExpectedFile" "$OutputFile"; then
		NormalizeHex "$ExpectedFile" > "$ExpectedHexFile"
		NormalizeHex "$OutputFile" > "$ActualHexFile"
		diff -u "$ExpectedHexFile" "$ActualHexFile" >&2
		Fail "common statement bytes differ"
	fi
	Pass "common statement bytes"
}


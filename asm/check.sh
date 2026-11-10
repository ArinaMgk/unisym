#!/bin/bash
# ASCII ShellScript TAB4 LF
# Docutitle: AASM Regression Check
# Copyright: UNISYM, under Apache License 2.0

set -u

ScriptDirectory="$(cd "$(dirname "$0")" && pwd)"
CheckDirectory="$ScriptDirectory/.check"

. "$CheckDirectory/common.sh"
. "$CheckDirectory/x86.sh"
. "$CheckDirectory/codeend.sh"
. "$CheckDirectory/riscv.sh"

RunX86Checks()
{
	CheckHex basic '48 b8 01 00 00 00 00 00 00 00 48 83 c0 02 48 83 f8 03 75 03 31 c0 c3 b8 01 00 00 00 c3 00 00 00 ef cd ab 89 67 45 23 01' -Ox
	CheckHex encoding '48 b8 88 77 66 55 44 33 22 11 49 89 c0 49 83 c1 7f 49 83 ea ff 31 c0 49 d1 e3 41 54 41 5d 88 18 66 89 18 89 18 48 89 18 db 28 f3 0f 6f 00 c5 fe 6f 00 c3' -Ox
	CheckHex addressing '48 8b 03 48 8b 4d 00 4b 8b 94 ec 34 12 00 00 44 89 46 07 4c 8d 0d 01 00 00 00 c3 11 22 33 44 55 66 77 88' -Ox
	CheckHex symbols '11 01 00 03 00 00 00 12 00 00 00 00 00 00 00 aa aa aa 12 2a' -Ox
	CheckHex data '41 41 53 4d 00 34 12 ef cd ab 89 ef cd ab 89 67 45 23 01 5a 5a 5a 5a cc cc cc cc cc cc cc cc cc' -Ox
	CheckHex prefix 'f0 ff 00 f3 a4 64 48 8b 04 25 20 00 00 00 66 89 d8 67 8b 40 04 c3' -Ox
	CheckHex structure '8b 45 01 00 7a 78 56 34 12' -Ox
	CheckBranch
	CheckRelocation
	CheckBinaryOutputChain
	CheckDebugOutput
	CheckListing
	CheckInvalidAddress
	CheckPrefixOnly
}

RunELFChecks()
{
	CheckRelocation
	CheckDebugOutput
	CheckListing
	CheckRISCVELFTarget
	CheckRISCVRelocation
}

RunRISCVChecks()
{
	CheckRISCV
	CheckRISCVELFTarget
	CheckRISCVRelocation
	CheckRISCVExpansion
}

RunCodeendChecks()
{
	CheckCodeends
	CheckArchends
}

RunCommonChecks()
{
	CheckPreprocessor
	CheckCommonStatements
}

RunAllChecks()
{
	RunX86Checks
	CheckPreprocessor
	RunCodeendChecks
	RunRISCVChecks
	CheckCommonStatements
}

RunCheckGroup()
{
	case "$1" in
	all) RunAllChecks; AllChecks=true ;;
	x86) RunX86Checks ;;
	riscv) RunRISCVChecks ;;
	elf) RunELFChecks ;;
	codeend) RunCodeendChecks ;;
	common) RunCommonChecks ;;
	*) Fail "unknown check group: $1" ;;
	esac
}

AllChecks=false
if [ "$#" -eq 0 ]; then
	set -- all
fi

for CheckGroup in "$@"; do
	RunCheckGroup "$CheckGroup"
done

if [ "$AllChecks" = true ]; then
	printf '\033[1;32mall AASM regression checks passed\033[0m\n'
else
	printf '\033[1;32mselected AASM regression checks passed\033[0m\n'
fi

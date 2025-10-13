# GBK Makefile TAB4 CRLF
# Attribute: 
# LastCheck: RFZ18
# AllAuthor: @dosconio
# ModuTitle: Makefile for utilities of UniSym
# Copyright: ArinaMgk UniSym, Apache License Version 2.0

export PATH := $(ubinpath)/I686/Win32/bin;$(PATH)

INCC_DIR=$(uincpath)/c/
LIBC_DIR=$(ulibpath)/c/
DEST_BIN=$(ubinpath)/I686/Win32/bin
#{TODO} DEST_BIN=$(ubinpath)/WIN32/
DEMO_DIR=../../demo/utilities/
OPT=-O3 -s -D_WinNT -L$(ubinpath) -lw32d
CC32=$(DEST_BIN)/gcc.exe -m32

CSC4=C:/Windows/Microsoft.NET/Framework64/v4.0.30319/csc.exe
PREF=-I${INCC_DIR} -o ${DEST_BIN}/

all:\
args\
cal\
clear\
cpuid\
fdump\
ffset\
ret\
segsel\


args:
	@echo 'MK args'
	@$(CC32) ${DEMO_DIR}args.c $(PREF)args.exe ${OPT}
cal:
	@echo 'MK cal (Calendar)'
	@$(CC32) ${DEMO_DIR}calendar/calendar.c $(PREF)cal.exe ${OPT}
clear:
	@echo 'MK clear'
	@$(CC32) ${DEMO_DIR}clear.c $(PREF)clear.exe ${OPT}
cpuid:
	@echo 'MK cpuid'
	@$(CC32) ${DEMO_DIR}cpuid.c $(PREF)cpuid.exe ${OPT}
fdump:
	@echo 'MK fdump'
	@$(CC32) ${DEMO_DIR}filedump.c ${LIBC_DIR}consio.c $(PREF)fdump.exe ${OPT}
ffset:
	@echo 'MK ffset'
	@$(CC32) ${DEMO_DIR}VirtualDiskCopier/ffset.c $(PREF)ffset.exe ${OPT}
ret:
	@echo 'MK ret'
	@$(CC32) ${DEMO_DIR}ret.c $(PREF)ret.exe ${OPT}

segsel:
	@echo 'MK OSDEV/SegmentSelector powered by .NetFx4:C#'
	@cd ${DEMO_DIR}SegmentSelector && ${CSC4} /nologo /t:winexe /out:SegSel.exe *.cs
	@mv ${DEMO_DIR}SegmentSelector/SegSel.exe ${DEST_BIN}/SegSel.exe

elf:
	@echo MK readelf
	@$(CC32) ${DEMO_DIR}readelf.c ${LIBC_DIR}format/ELF.c $(PREF)readelf.exe ${OPT}

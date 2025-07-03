# GBK Makefile TAB4 CRLF
# Attribute: 
# LastCheck: RFZ18
# AllAuthor: @dosconio
# ModuTitle: Makefile for utilities of UniSym
# Copyright: ArinaMgk UniSym, Apache License Version 2.0

INCC_DIR=$(uincpath)/c/
LIBC_DIR=$(ulibpath)/c/
DEST_BIN=$(ubinpath)/AMD64/Lin64/
DEMO_DIR=../../demo/utilities/
OPT=-O3 -s -D_Linux -L$(ubinpath) -ll64d
CC32=gcc -m64

CSC4=echo 
PREF=-I${INCC_DIR} -o ${DEST_BIN}

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
	@$(CC32) ${DEMO_DIR}args.c $(PREF)args ${OPT}
cal:
	@echo 'MK cal (Calendar)'
	@$(CC32) ${DEMO_DIR}calendar/calendar.c $(PREF)cal ${OPT}
clear:
	@echo 'MK clear'
	@$(CC32) ${DEMO_DIR}clear.c $(PREF)clear ${OPT}

# # #
cpuid:
	@echo 'TD cpuid'

fdump:
	@echo 'MK fdump'
	@$(CC32) ${DEMO_DIR}filedump.c ${LIBC_DIR}consio.c $(PREF)fdump ${OPT}
ffset:
	@echo 'MK ffset'
	@$(CC32) ${DEMO_DIR}VirtualDiskCopier/ffset.c $(PREF)ffset ${OPT}
ret:
	@echo 'MK ret'
	@$(CC32) ${DEMO_DIR}ret.c $(PREF)ret ${OPT}

# # #
segsel:
	@echo 'TD SEGSEL'

elf:
	@echo MK readelf
	@$(CC32) ${DEMO_DIR}readelf.c ${LIBC_DIR}format/ELF.c $(PREF)readelf ${OPT}

install:
#{TODO}

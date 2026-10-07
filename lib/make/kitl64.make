# GBK Makefile TAB4 CRLF
# Attribute: 
# LastCheck: RFZ18
# AllAuthor: @dosconio
# ModuTitle: Makefile for utilities of UniSym
# Copyright: ArinaMgk UniSym, Apache License Version 2.0

INCC_DIR=$(uincpath)/c/
LIBC_DIR=$(ulibpath)/c/
LIBX_DIR=$(ulibpath)/cpp/
DEST_BIN=$(ubinpath)/AMD64/Lin64/
DEMO_DIR=../../demo/utilities/
OPT=-D_Linux -L$(ubinpath) -ll64d -lstdc++
CC32=gcc -m64

CSC4=echo 
PREF=-I${INCC_DIR}

.PHONY: all args cal clear cpuid fdump ffset ret segsel elf

all:\
args\
cal\
clear\
cpuid\
fdump\
ffset\
ret\
segsel\


args: ${DEST_BIN}args
cal: ${DEST_BIN}cal
clear: ${DEST_BIN}clear
fdump: ${DEST_BIN}fdump
ffset: ${DEST_BIN}ffset
ret: ${DEST_BIN}ret
elf: ${DEST_BIN}readelf

${DEST_BIN}args: ${DEMO_DIR}args.c
	@echo 'MK args'
	@$(CC32) ${DEMO_DIR}args.c $(PREF) -o $@ ${OPT}
${DEST_BIN}cal: ${DEMO_DIR}calendar/calendar.c
	@echo 'MK cal (Calendar)'
	@$(CC32) ${DEMO_DIR}calendar/calendar.c $(PREF) -o $@ ${OPT}
${DEST_BIN}clear: ${DEMO_DIR}clear.c
	@echo 'MK clear'
	@$(CC32) ${DEMO_DIR}clear.c $(PREF) -o $@ ${OPT}

# # #
cpuid:
	@echo 'TD cpuid'

${DEST_BIN}fdump: ${DEMO_DIR}filedump.c
	@echo 'MK fdump'
	@$(CC32) ${DEMO_DIR}filedump.c $(PREF) -o $@ ${OPT}
${DEST_BIN}ffset: ${DEMO_DIR}VirtualDiskCopier/ffset.c
	@echo 'MK ffset'
	@$(CC32) ${DEMO_DIR}VirtualDiskCopier/ffset.c $(PREF) -o $@ ${OPT}
${DEST_BIN}ret: ${DEMO_DIR}ret.c
	@echo 'MK ret'
	@$(CC32) ${DEMO_DIR}ret.c $(PREF) -o $@ ${OPT}

# # #
segsel:
	@echo 'TD SEGSEL'

${DEST_BIN}readelf: ${DEMO_DIR}readelf.c ${LIBC_DIR}format/ELF.c
	@echo MK readelf
	@$(CC32) ${DEMO_DIR}readelf.c ${LIBC_DIR}format/ELF.c $(PREF) -o $@ ${OPT}

install:
#{TODO}

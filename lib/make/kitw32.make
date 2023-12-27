# GBK Makefile TAB4 CRLF
# Attribute: 
# LastCheck: RFZ18
# AllAuthor: @dosconio
# ModuTitle: Makefile for kits of UniSym
# Copyright: ArinaMgk UniSym, Apache License Version 2.0

INCC_DIR=../../inc/c/
DEST_BIN=../../../_bin/
LIBC_DIR=../../lib/c/
DEMO_DIR=../../demo/kit/
OPT=-O3 -s -D_WinNT -L../../../_bin/ -lw32d

all:\
args\
cal\
clear\
fdump\
ffset\



args:
	# args
	gcc ${DEMO_DIR}args.c -I${INCC_DIR} -o ${DEST_BIN}args.exe ${OPT}
cal:
	# cal
	gcc ${DEMO_DIR}calendar/calendar.c ${LIBC_DIR}datime/weekday.c -I${INCC_DIR} -o ${DEST_BIN}cal.exe ${OPT}
clear:
	# clear
	gcc ${DEMO_DIR}clear.c -I${INCC_DIR} -o ${DEST_BIN}clear.exe ${OPT}
fdump:
	# fdump
	gcc ${DEMO_DIR}filedump.c ${LIBC_DIR}consio.c -I${INCC_DIR} -o ${DEST_BIN}fdump.exe ${OPT}
ffset:
	# ffset
	gcc ${DEMO_DIR}VirtualDiskCopier/ffset.c -I${INCC_DIR} -o ${DEST_BIN}ffset.exe ${OPT}


outdated:
	# cmd /c move cal.exe ${DEST_BIN}
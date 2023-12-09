INCC_DIR=../../inc/c/
DEST_BIN=../../../_bin/
LIBC_DIR=../../lib/c/
OPT=-O3 -s

all: cal fdump ffset args

cal:
	# calendar
	gcc ../kit/calendar/calendar.c -I${INCC_DIR} -D_WinNT -o ${DEST_BIN}cal.exe ${OPT}
fdump:
	# dump file to screen by visual form
	gcc ../kit/filedump.c ${LIBC_DIR}consio.c -I${INCC_DIR} -D_WinNT -o ${DEST_BIN}fdump.exe ${OPT}
ffset:
	# set part of file from a file.
	gcc ../kit/VirtualDiskCopier/ffset.c ${LIBC_DIR}ustring.c -I${INCC_DIR} -D_WinNT -o ${DEST_BIN}ffset.exe ${OPT}
args:
	# show arguments received from the commands
	gcc ../kit/args.c -I${INCC_DIR} -D_WinNT -o ${DEST_BIN}args.exe ${OPT}


outdated:
	# cmd /c move cal.exe ${DEST_BIN}
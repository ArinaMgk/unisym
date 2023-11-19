INCC_DIR=../inc/c/
DEST_BIN=../../_bin/
calendar:
	gcc ./host/Calendar.c -I${INCC_DIR} -D_WinNT -o cal.exe
	cmd /c move cal.exe ${DEST_BIN}
OP=-D_DEBUG -LE:\PROJ\SVGN\_bin -lw32d -O3 -s

prefix:
	cmd /c call ../../../lib/Script/batch/prefix_chk.bat
strreplace:
	gcc chk_StrReplace.c ${OP} -o chk_StrReplace.exe
	chk_StrReplace.exe
	rm chk_StrReplace.exe
	
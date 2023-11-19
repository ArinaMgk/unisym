sources = ../unisym/c/source/*.c
win64dbg:
	cd ../../_obj/uslc/ && gcc -c $(sources) -O3 -s -lm -D_WinNT -m64 -D_dbg
	ar rcs ../../_lib/c.w64d.lib ../../_obj/uslc/*.o
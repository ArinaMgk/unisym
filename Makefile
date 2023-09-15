sources = ../../unisym/c/source/hstring.c\
	../../unisym/c/source/ustring.c
win64dbg:
	cd ../_obj/uslib/ && gcc -c $(sources) -O3 -s -lm -D_WinNT -m64 -D_dbg
	ar rcs ../_lib/ulib.win64dbg.lib ../_obj/uslib/*.o
lin64dbg:
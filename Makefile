# ASCII Makefile TAB4 CRLF
# Attribute: 
# LastCheck: RFZ09
# AllAuthor: @dosconio
# ModuTitle: Makefile for UniSym
# Copyright: ArinaMgk UniSym, Apache License Version 2.0

# # "Debug" is for _DEBUG but for "-g" option

#{TODO} Split into multiple script for different platform, e.g. `make lin32/gcc` `make win32/msvc`, while:
#{TODO} Makefile Script Generator Script of Perl

dest_bin=../_bin
dest_obj=../_obj
dest_lib=../libc
make_dir=./lib/make/
csrc_dirx=../../unisym/lib/c
# ---- ---- CPL static linkage library ---- ----
CC_32=gcc
CC_64=?
VC_64=E:/software/VS22/VC/Tools/MSVC/14.37.32822/bin/Hostx64/x64/cl.exe
VLNK_64=E:/software/VS22/VC/Tools/MSVC/14.37.32822/bin/Hostx64/x64/lib.exe
VI_64=E:/software/VS22/VC/Tools/MSVC/14.37.32822/include/ -I"C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/ucrt/"
VLIB_64="E:/software/VS22/VC/Tools/MSVC/14.37.32822/lib/x64/" /LIBPATH:"E:/software/VS22/VC/Tools/MSVC/14.37.32822/lib/onecore/x64"
CGW32D_PATH=.

# exclude `.make.c` file
## sors_c=${csrc_dirx}/node/*.c ${csrc_dirx}/auxiliary/toxxxer.c ${csrc_dirx}/debug.c ${csrc_dirx}/crc64.c ${csrc_dirx}/binary.c ${csrc_dirx}/ustring/gstring/MemSet.c ${csrc_dirx}/mcore.c
sors_c=${csrc_dirx}/*.c ${csrc_dirx}/node/*.c ${csrc_dirx}/dnode/*.c ${csrc_dirx}/inode/*.c ${csrc_dirx}/tnode/*.c ${csrc_dirx}/nnode/*.c ${csrc_dirx}/ustring/astring/*.c ${csrc_dirx}/ustring/gstring/*.c ${csrc_dirx}/ustring/chrar/*.c ${csrc_dirx}/datime/*.c ${csrc_dirx}/coear/*.c ${csrc_dirx}/auxiliary/toxxxer.c

#remv_c=*.make.o

all32: CGW32D kitw32 manual

CGW32D:
	## set PATH=%PATH%;${CGW32D_PATH}
	-cd ${dest_obj}/ && mkdir CGWin32D
	cd ${dest_obj}/CGWin32D/ && aasm -f win32 ../../unisym/lib/asm/x86/cpuid.asm -I../../unisym/inc/Kasha/n_ -o./cpuid.a.o
	cd ${dest_obj}/CGWin32D/ && aasm -f win32 ../../unisym/lib/asm/x86/binary.asm -I../../unisym/inc/Kasha/n_ -o./binary.a.o
	$(CC_32) ./lib/c/auxiliary/toxxxer.make.c -o ../_tmp/toxxxer.exe
	cd ../_tmp/ && ./toxxxer.exe
	cd ${dest_obj}/CGWin32D/ && $(CC_32) -c $(sors_c) -D_DEBUG -D_WinNT -D_Win32 -O3
	#cd ${dest_obj}/CGWin32D/ && rm ${remv_c}
	cd ${dest_obj}/CGWin32D/ && ar -rcs ../../_bin/libw32d.a *.o
	cd ${make_dir} && make -f cgw32.make debug
kitw32:
	cd ${make_dir} && make -f kitw32.make all
	# ***\AutoHotkey\Compiler\Ahk2Exe.exe /in %1 /out %2
	ahkcc ./lib/Script/AutoHotkey/Arnscr.ahk ../../../../_bin/arnscr.exe

CVW64D:
	-cd ../_obj/ && mkdir CVWin64D
	cd ../_obj/CVWin64D/ && aasm -f win64 ../../unisym/lib/asm/x64/cpuid.asm -I../../unisym/inc/Kasha/n_ -o./cpuid.a.obj
	cd ../_obj/CVWin64D/ && aasm -f win64 ../../unisym/lib/asm/x64/binary.asm -I../../unisym/inc/Kasha/n_ -o./binary.a.obj
	$(VC_64) ./lib/c/auxiliary/toxxxer.make.c /Fe: ../_tmp/toxxxer_msvc.exe /Fo: ../_tmp/toxxxer_msvc.obj /I$(VI_64) /D_DEBUG /D_WinNT /D_Win64
	cd ../_tmp/ && cmd /C toxxxer_msvc.exe
	cd ../_obj/CVWin64D/ && $(VC_64) /c $(sors_c) /I$(VI_64) /D_DEBUG /D_WinNT /D_Win64
	#cd ../_obj/CVWin64D/ && rm ${remv_c}
	cd ../_obj/CVWin64D/ && $(VLNK_64) /OUT:../../_bin/libw64d.lib *.obj

manual:
	# optional: latex halo.tex && dvipdfmx halo.dvi && rm halo.dvi halo.log halo.aux
	cd  ${dest_bin}/ && xelatex ../unisym/doc/manual/usymdoc.tex && rm usymdoc.log usymdoc.aux

clean:
	-cd ./inc/Python/ && rmdir __pycache__ /S /Q

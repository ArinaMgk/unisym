# ASCII Makefile TAB4 CRLF
# Attribute: 
# LastCheck: RFZ09
# AllAuthor: @dosconio
# ModuTitle: Makefile for UniSym
# Copyright: ArinaMgk UniSym, Apache License Version 2.0

#{TODO} Split into multiple script for different platform, e.g. `make lin32/gcc` `make win32/msvc`
#{TODO} Makefile Script Generator Script of Perl

dest_bin=../_bin
dest_obj=../_obj
dest_lib=../libc
# ---- ---- CPL static linkage library ---- ----
CC_32=gcc
CC_64=?
VC_64=E:/software/VS22/VC/Tools/MSVC/14.37.32822/bin/Hostx64/x64/cl.exe
VLNK_64=E:/software/VS22/VC/Tools/MSVC/14.37.32822/bin/Hostx64/x64/lib.exe
VI_64=E:/software/VS22/VC/Tools/MSVC/14.37.32822/include/ -I"C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/ucrt/"
VLIB_64="E:/software/VS22/VC/Tools/MSVC/14.37.32822/lib/x64/" /LIBPATH:"E:/software/VS22/VC/Tools/MSVC/14.37.32822/lib/onecore/x64"

# exclude `.make.c` file
sors_c=../../unisym/lib/c/node/*.c ../../unisym/lib/c/auxiliary/toxxxer.c ../../unisym/lib/c/debug.c ../../unisym/lib/c/crc64.c ../../unisym/lib/c/binary.c ../../unisym/lib/c/ustring/gstring/MemSet.c ../../unisym/lib/c/mcore.c
#sors_c=../../unisym/lib/c/node/*.c ../../unisym/lib/c/*.c

# "Debug" is for _DEBUG but for "-g" option

CGW32D:
	-cd ../_obj/ && mkdir CGWin32
	cd ../_obj/CGWin32/ && aasm -f win32 ../../unisym/lib/asm/x86/cpuid.asm -I../../unisym/inc/Kasha/n_ -o./cpuid.a.o
	cd ../_obj/CGWin32/ && aasm -f win32 ../../unisym/lib/asm/x86/binary.asm -I../../unisym/inc/Kasha/n_ -o./binary.a.o
	gcc ./lib/c/auxiliary/toxxxer.make.c -o ../_tmp/toxxxer.exe
	cd ../_tmp/ && toxxxer.exe
	cd ../_obj/CGWin32/ && $(CC_32) -c $(sors_c) -D_DEBUG -O3
	cd ../_obj/CGWin32/ && ar -rcs ../../_bin/libw32d.a *.o
	cd ./demo/make/ && make -f cgw32.make debug

# CPL MSVC Win64 Debug
# Use "Developer Command Prompt for VS 2022" run the below script
CVW64D:
	-cd ../_obj/ && mkdir CVWin64Dbg
	cd ../_obj/CVWin64Dbg/ && aasm -f win64 ../../unisym/lib/asm/x64/cpuid.asm -I../../unisym/inc/Kasha/n_ -o./cpuid.a.obj
	cd ../_obj/CVWin64Dbg/ && aasm -f win64 ../../unisym/lib/asm/x64/binary.asm -I../../unisym/inc/Kasha/n_ -o./binary.a.obj
	$(VC_64) ./lib/c/auxiliary/toxxxer.make.c /Fe: ../_tmp/toxxxer_msvc.exe /Fo: ../_tmp/toxxxer_msvc.obj /I$(VI_64)
	cd ../_tmp/ && cmd /C toxxxer_msvc.exe
	cd ../_obj/CVWin64Dbg/ && $(VC_64) /c $(sors_c) -D_DEBUG /I$(VI_64)
	cd ../_obj/CVWin64Dbg/ && $(VLNK_64) /OUT:../../_bin/libw64d.lib *.obj

manual:
	cd  ${dest_bin}/ && xelatex ../unisym/doc/manual/usymdoc.tex
	cd  ${dest_bin}/ && rm usymdoc.log
	cd  ${dest_bin}/ && rm usymdoc.aux
	# optional:
	#  latex halo.tex
	#  dvipdfmx halo.dvi
	#  del *.dvi
kit:
	cd ./demo/make/ && make -f kit.make all
kitx: kit
	# ***\AutoHotkey\Compiler\Ahk2Exe.exe /in %1 /out %2
	ahkcc ./lib/Script/AutoHotkey/Arnscr.ahk ../../../../_bin/arnscr.exe
clean:
	-cd ./inc/Python/ && rmdir __pycache__ /S /Q

# ASCII Makefile TAB4 CRLF
# Attribute: 
# LastCheck: 20240320
# AllAuthor: @dosconio
# ModuTitle: Makefile for UniSym
# Copyright: ArinaMgk UniSym, Apache License Version 2.0

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

# depend [gcc, makefile] [python]

.PHONY: list mx86 cgw32 liblinux cgl32 cgl64

list: # depend [python]
	@python ./lib/Script/Makefile/makemake.py

mx86:
	cd ${make_dir} && make -f cgmx86.make all

cgw32:
	make -f ./lib/make/cgw32.make all

liblinux: cgl32 cgl64 # ELF
cgl32:
	make -f ./lib/make/cgl32.make all
cgl64:
	make -f ./lib/make/cgl64.make all

all32: CGW32D kitw32 manual
kitw32:
	cd ${make_dir} && make -f kitw32.make all
	# ***\AutoHotkey\Compiler\Ahk2Exe.exe /in %1 /out %2
	-ahkcc ./lib/Script/AutoHotkey/Arnscr.ahk ../../../../_bin/arnscr.exe

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

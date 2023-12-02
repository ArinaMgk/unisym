# ASCII Makefile TAB4 CRLF
# Attribute: 
# LastCheck: RFZ02
# AllAuthor: @dosconio
# ModuTitle: Makefile for UniSym
# Copyright: ArinaMgk UniSym, Apache License Version 2.0

dest_bin=../_bin
dest_obj=../_obj
dest_lib=../libc
# ---- ---- CPL static linkage library ---- ----
CC_32=gcc
CC_64=?
VC_64=E:/software/VS22/VC/Tools/MSVC/14.37.32822/bin/Hostx64/x64/cl.exe
VL_64=E:/software/VS22/VC/Tools/MSVC/14.37.32822/bin/Hostx64/x64/lib.exe
VI_64=E:/software/VS22/VC/Tools/MSVC/14.37.32822/include/ -I"C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/ucrt/"

sors_c=../../unisym/lib/c/node/*.c

# CPL GCC Win32 & Linux32
CGWin32Dbg:
	-cd ../_obj/ && mkdir CGWin32
	cd ../_obj/CGWin32/ && $(CC_32) -c $(sors_c) -D_DEBUG
	cd ../_obj/CGWin32/ && ar -rcs ../../_bin/libhost.cpuw32.a *.o
CGLin32:

# CPL MSVC

CVWin64Dbg:
	-cd ../_obj/ && mkdir CVWin64Dbg
	cd ../_obj/CVWin64Dbg/ && $(VC_64) /c $(sors_c) -D_DEBUG -I$(VI_64)
	cd ../_obj/CVWin64Dbg/ && $(VL_64) /OUT:../../_bin/libhost.cpuw64.lib *.obj


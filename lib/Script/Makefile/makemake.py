#!
# ASCII Python-3 TAB4 LF
# Attribute:
import os
import sys
import datetime
# LastCheck: 20240320
__AllAuthor= ["@dosconio"]
__ModuTitle= "Makefile Script Generator Script of Python"
__Copyright= "ArinaMgk UniSym, Apache License Version 2.0"
# Parallel : ./makemake.pl
__BuildTime= datetime.datetime.today()
# Default  : {TEMP: _DEBUG Version}

print(__ModuTitle, ": built at", __BuildTime)

def get_files(whatpath, suffix):
    mfiles = []
    for root, dirs, files in os.walk(whatpath):
        for filename in files:
            if filename.endswith(suffix):
                mfiles.append(os.path.join(root, filename).replace("\\", "/"))
    return mfiles

def get_outfilename(val):
	fname = val.split("/")[-1]
	first_name, file_ext = os.path.splitext(fname)
	return first_name

def set_makefile(fpath, ftext): # UTF-8
	with open(fpath, 'w+b') as fobj:
		fobj.write(bytes(ftext, encoding = "utf8"))

# ---- ---- ---- ---- ---- ---- ---- ----
list_c_tomake = get_files("./lib/c/auxiliary/", ".make.c")
list_asm_file = [
	"../../unisym/lib/asm/<INSSYS>/cpuid.asm",
	"../../unisym/lib/asm/<INSSYS>/binary.asm"
]
list_cpl_file = get_files("./lib/c/", ".c")
list_cpl_file_new = []
#{TEMP} cut special docs
# <board.lib> includes <processor.lib> 
# <processor.lib> includes its special `asm` version
for i in list_cpl_file:
	if (".make.c" in i) or ("/lib/c/processor/" in i) or ("/lib/c/driver/" in i) or ("lib/c/ustring/bstring/" in i): # fixed path for <processor.lib>, <board.lib>
		pass
	else:
		list_cpl_file_new.append(i)
list_cpl_file = list_cpl_file_new
list_cpl_file_new = []
list_cpp_file = get_files("./lib/cpp/", ".cpp")
list_cpp_file_new = []
for i in list_cpp_file:
	if ("/lib/cpp/Device/" in i) :
		pass
	else:
		list_cpp_file_new.append(i)
list_cpp_file = list_cpp_file_new
list_cpp_file_new = []
list_asm_free86 = get_files("./lib/asm/x86", ".asm")

# [General-standing environment Library]
# unisym/lib/make/cgw32.make -> -bin/libw32d.a 
# unisym/lib/make/cgw64.make -> -bin/libw64d.a 
# unisym/lib/make/cvw32.make -> -bin/libw32d.lib
# unisym/lib/make/cvw64.make -> -bin/libw64d.lib
# unisym/lib/make/cgl32.make -> -bin/libl32d.a
# unisym/lib/make/cgl64.make -> -bin/libl64d.a
text_gcc_win32 = "# UNISYM for GCC-Win32 built-" + str(__BuildTime) + '\n'
text_gcc_win64 = "# UNISYM for GCC-Win64 built-" + str(__BuildTime) + '\n'
text_msv_win32 =  "# UNISYM for MSVC-Win32 built-" + str(__BuildTime) + '\n'
text_msv_win64 = "# UNISYM for MSVC-Win64 built-" + str(__BuildTime) + '\n'
text_gcc_lin32 = "# UNISYM for GCC-Lin32 built-" + str(__BuildTime) + '\n'
text_gcc_lin64 = "# UNISYM for GCC-Lin64 built-" + str(__BuildTime) + '\n'
print(text_gcc_win32, text_gcc_win64, text_msv_win32, text_msv_win64, text_gcc_lin32, text_gcc_lin64, sep="")
# ---- make list by win
tmp = """
AASM = aasm
udir = E:/PROJ/SVGN/unisym
"""
text_gcc_win32 += tmp + """
attr = -D_DEBUG -O3 -D_Win32
cplpref=_ugc32_
cpppref=_uxxgc32_
dest_obj=../_obj/CGWin32
CC = E:/PROJ/CoStudioWin64/i686/bin/gcc.exe -m32
CX = E:/PROJ/CoStudioWin64/i686/bin/g++.exe -m32
AR = E:/PROJ/CoStudioWin64/i686/bin/ar.exe
aattr = -fwin32 -I$(udir)/inc/Kasha/n_
dest_abs = ../_bin/libw32d.a
"""
text_gcc_win64 += tmp + """
attr = -D_DEBUG -O3 -D_Win64
cplpref=_ugc64_
cpppref=_uxxgc64_
dest_obj=../_obj/CGWin64
CC = M:/tmp/bin/mingw64/bin/gcc.exe  -m64
CX = M:/tmp/bin/mingw64/bin/g++.exe  -m64
AR = M:/tmp/bin/mingw64/bin/ar.exe
aattr = -fwin64 -I$(udir)/inc/Kasha/n_ 
dest_abs = ../_bin/libw64d.a
"""
text_msv_win32 += tmp + """
attr = /D_DEBUG /D_Win32
cplpref=_uvc32_
cpppref=_uxxvc32_
dest_obj=../_obj/CVWin32
TOOLDIR = E:/software/VS22/VC/Tools/MSVC/14.39.33519
CC = ${TOOLDIR}/bin/Hostx86/x86/cl.exe
CX = ${CC}
AR = ${TOOLDIR}/bin/Hostx86/x86/lib.exe
# contain x86 - x64 can run this
aattr = -fwin32 -I../../unisym/inc/Kasha/n_
dest_abs = ../_bin/libw32d.lib
VLIB_64=/LIBPATH:"${TOOLDIR}/lib/x86/" /LIBPATH:"${TOOLDIR}/lib/onecore/x86" /LIBPATH:"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x86" /LIBPATH:"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/ucrt/x86"
VI_SYS="C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/ucrt/"
VI_64=${TOOLDIR}/include/ /I${VI_SYS} /I"C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/um/" /I"C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/shared/"
"""
text_msv_win64 += tmp + """
attr = /D_DEBUG /D_Win64
cplpref=_uvc64_
cpppref=_uxxvc64_
dest_obj=../_obj/CVWin64
TOOLDIR = E:/software/VS22/VC/Tools/MSVC/14.39.33519
CC = ${TOOLDIR}/bin/Hostx64/x64/cl.exe
CX = ${CC}
AR = ${TOOLDIR}/bin/Hostx64/x64/lib.exe
aattr = -fwin64 -I../../unisym/inc/Kasha/n_
dest_abs = ../_bin/libw64d.lib
VLIB_64=/LIBPATH:"${TOOLDIR}/lib/x64/" /LIBPATH:"${TOOLDIR}/lib/onecore/x64" /LIBPATH:"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64" /LIBPATH:"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/ucrt/x64"
VI_SYS="C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/ucrt/"
VI_64=${TOOLDIR}/include/ /I${VI_SYS} /I"C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/um/" /I"C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/shared/"
"""
tmp = """
CC32 = gcc -m32
CX32 = g++ -m32
CC64 = gcc -m64
CX64 = g++ -m64
AASM = /mnt/hgfs/_bin/ELF64/aasm
attr = -D_DEBUG -D_Linux -O3
aattr = -felf -I$(udir)/inc/Kasha/n_
udir = /mnt/hgfs/unisym
"""
text_gcc_lin32 += tmp + """
cplpref=_ugc32_
cpppref=_uxxgc32_
dest_obj=~/_obj/CGLin32
dest_abs=/mnt/hgfs/_bin/libl32d.a
CC=$(CC32)
CX=$(CX32)
"""
text_gcc_lin64 += tmp + """
cplpref=_ugc64_
cpppref=_uxxgc64_
dest_obj=~/_obj/CGLin64
dest_abs=/mnt/hgfs/_bin/libl64d.a
CC=$(CC64)
CX=$(CX64)
"""
#
tmp = ".PHONY: all\n"+\
	"\nall:\n"+\
	'\t-@E:/software/w64dev/bin/mkdir.exe -p ${dest_obj}\n'+\
	'\t-@E:/software/w64dev/bin/rm.exe -f ${dest_obj}/*\n'
text_gcc_win32 += tmp
text_gcc_win64 += tmp
text_msv_win32 += tmp
text_msv_win64 += tmp
tmp = ".PHONY: all\n"+\
	"\nall:\n"+\
	'\t-@mkdir -p ${dest_obj}\n'+\
	'\t-@rm -f ${dest_obj}/*\n'
text_gcc_lin32 += tmp
text_gcc_lin64 += tmp
for val in list_asm_file:
	# for x86
	fullname = val.replace("<INSSYS>", "x86")
	tmp = '\t' + "cd ${dest_obj}/ && ${AASM} ${aattr} " + fullname + " -o " + get_outfilename(val) + "_a.o" + '\n'
	text_gcc_win32 += tmp
	text_msv_win32 += tmp
	tmp = '\t' + "cd ${dest_obj}/ && ${AASM} ${aattr} " + fullname.replace("../../unisym", "$(udir)") + " -o " + get_outfilename(val) + "_a.o" + '\n'
	text_gcc_lin32 += tmp
	# for x64
	fullname = val.replace("<INSSYS>", "x64")
	tmp = '\t' + "cd ${dest_obj}/ && ${AASM} ${aattr} " + fullname + " -o " + get_outfilename(val) + "_a.o" + '\n'
	text_gcc_win64 += tmp
	text_msv_win64 += tmp
	tmp = '\t' + "cd ${dest_obj}/ && ${AASM} ${aattr} " + fullname.replace("../../unisym", "$(udir)") + " -o " + get_outfilename(val) + "_a.o" + '\n'
	# text_gcc_lin64 += tmp #----elf output format does not support 64-bit code
for val in list_c_tomake:
	file_path = get_outfilename(val)
	tmp = '\t' + "$(CC) " + val + " -o ../_tmp/" + file_path + ".exe" + " && cd ../_tmp/ && ./" + file_path + ".exe\n"
	text_gcc_win32 += tmp
	text_gcc_win64 += tmp
	tmp = '\t' + "$(CC) " + val + " -o ../_tmp/" + file_path + " && ../_tmp/" + file_path + "\n"
	text_gcc_lin32 += tmp
	text_gcc_lin64 += tmp
	tmp = '\t' + "$(CC) " + val + " /Fe: ../_tmp/" + file_path + ".exe" + " /Fo: ../_tmp/" + file_path + ".obj /I${VI_64} ${attr} /link${VLIB_64}" + " && ../_tmp/" + file_path + "&& echo " + file_path + "\n"
	text_msv_win32 += tmp
	text_msv_win64 += tmp
for val in list_cpl_file:
	tmp = '\t' + "$(CC) -c " + val + " -o ${dest_obj}/${cplpref}" + get_outfilename(val) + ".o $(attr)\n"
	text_gcc_win32 += tmp
	text_gcc_win64 += tmp
	text_gcc_lin32 += tmp
	text_gcc_lin64 += tmp
	tmp = '\t' + "$(CC) /c " + val + " /Fo:${dest_obj}/${cplpref}" + get_outfilename(val) + ".obj /I${VI_64} ${attr}" + "\n"
	text_msv_win32 += tmp
	text_msv_win64 += tmp
for val in list_cpp_file:
	tmp = '\t' + "$(CX) -c " + val + " -o ${dest_obj}/${cpppref}" + get_outfilename(val) + ".o $(attr)\n"
	text_gcc_win32 += tmp
	text_gcc_win64 += tmp
	text_gcc_lin32 += tmp
	text_gcc_lin64 += tmp
	tmp = '\t' + "$(CX) /c " + val + " /Fo:${dest_obj}/${cpppref}" + get_outfilename(val) + ".obj /I${VI_64} ${attr}" + "\n"
	text_msv_win32 += tmp
	text_msv_win64 += tmp
tmp = '\t' + "${AR} -rcs ${dest_abs} ${dest_obj}/*" + '\n'
text_gcc_win32 += tmp
text_gcc_win64 += tmp
text_gcc_lin32 += tmp
text_gcc_lin64 += tmp
tmp = '\t' + "${AR} /OUT:${dest_abs} ${dest_obj}/*" + '\n'
text_msv_win32 += tmp
text_msv_win64 += tmp
set_makefile('./lib/make/cgw32.make', text_gcc_win32)
set_makefile('./lib/make/cgw64.make', text_gcc_win64)
set_makefile('./lib/make/cvw32.make', text_msv_win32)
set_makefile('./lib/make/cvw64.make', text_msv_win64)
set_makefile('./lib/make/cgl32.make', text_gcc_lin32)
set_makefile('./lib/make/cgl64.make', text_gcc_lin64)
text_gcc_win32 = text_gcc_win64 = ""
text_msv_win32 = text_msv_win64 = ""
text_gcc_lin32 = text_gcc_lin64 = ""

# unisym/lib/make/cgmx86.make --[Linux Distributes, LF]-> -bin/libmx86.a
# Mecocoa can use Linux format link-library, NOW!
list_gcc_mecocoa_files = []
for val in list_asm_free86:
	list_gcc_mecocoa_files.append("$(asmf) " + val)
list_gcc_mecocoa_files.append("$(CC32) ${libcdir}/driver/i8259A.c")
list_gcc_mecocoa_files.append("$(CC32) ${libcdir}/processor/x86/delay.c -DADDR_CountSeconds=0x524")
list_gcc_mecocoa_files.append("$(CC32) ${libcdir}/task.c")
text_gcc_mecocoa = "# UNISYM for MECOCOA-x86 built-" + str(__BuildTime) + '\n'
print(text_gcc_mecocoa)
text_gcc_mecocoa += ".PHONY: all\n"
text_gcc_mecocoa += """
unidir = /mnt/hgfs/unisym
libcdir = $(unidir)/lib/c
libadir = $(unidir)/lib/asm
asmattr = -I${unidir}/inc/Kasha/n_ -I${unidir}/inc/naasm/n_ -I./include/
asm  = /mnt/hgfs/_bin/ELF64/aasm ${asmattr} #OPT: aasm
asmf = ${asm} -felf
CC32 = gcc -m32 -c -fno-builtin -fleading-underscore -fno-pic\
 -fno-stack-protector -I/mnt/hgfs/unisym/inc/c -D_Linux
"""
text_gcc_mecocoa += "\nall:\n"
text_gcc_mecocoa += '\t' + "-sudo mkdir -m 777 -p ~/_obj/libmx86\n"
text_gcc_mecocoa += '\t' + "-rm -f ~/_obj/libmx86/*.obj\n"
for i in list_gcc_mecocoa_files:
	file_path, file_ext = os.path.splitext(i)
	text_gcc_mecocoa += '\t' + i + " -Dp_i386 -D_MCCAx86 -D_ARC_x86=5 -o ~/_obj/libmx86/" + file_path.split("/")[-1] + ".obj\n"
text_gcc_mecocoa += '\t' + "ar -rcs /mnt/hgfs/_bin/libmx86.a ~/_obj/libmx86/*.obj\n"
with open('./lib/make/cgmx86.make', 'w+b') as fobj:
	fobj.write(bytes(text_gcc_mecocoa, encoding = "utf8")) # do not append line-feed
text_gcc_mecocoa = ""

# f: free-standing environment
# unisym/lib/make/cfcortexm3.make -> ...
#{} ...

#!ASCII Python-3 TAB4 LF
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
list_cpl_file = get_files("./lib/c/", ".c")
list_cpp_file = get_files("./lib/cpp/", ".cpp")
list_asm_free86 = get_files("./lib/asm/x86", ".asm")
list_asm_free64 = get_files("./lib/asm/x64", ".asm")

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
comhead = """
AASM = aasm
aat=-I$(uincpath)/Kasha/n_ -I$(uincpath)/naasm/n_
cplpref=_cc_
cplfile=$(wildcard lib/c/*.c) $(wildcard lib/c/**/*.c) $(wildcard lib/c/**/**/*.c) $(wildcard lib/c/**/**/**/*.c)
cplobjs=$(patsubst %c, %o, $(cplfile))
cpppref=_cx_
cppfile=$(wildcard lib/cpp/*.cpp) $(wildcard lib/cpp/**/*.cpp) $(wildcard lib/cpp/**/**/*.cpp) $(wildcard lib/cpp/**/**/**/*.cpp)
cppobjs=$(patsubst %cpp, %o, $(cppfile))
"""
text_gcc_win32 += comhead + """
attr = -D_DEBUG -O3 -D_Win32
dest_obj=$(uobjpath)/CGWin32
CC = E:/PROJ/CoStudioWin64/i686/bin/gcc.exe -m32
CX = E:/PROJ/CoStudioWin64/i686/bin/g++.exe -m32
AR = E:/PROJ/CoStudioWin64/i686/bin/ar.exe
aattr = -fwin32
dest_abs = ../_bin/libw32d.a
"""
text_gcc_win64 += comhead + """
attr = -D_DEBUG -O3 -D_Win64
dest_obj=$(uobjpath)/CGWin64
CC = M:/tmp/bin/mingw64/bin/gcc.exe  -m64
CX = M:/tmp/bin/mingw64/bin/g++.exe  -m64
AR = M:/tmp/bin/mingw64/bin/ar.exe
aattr = -fwin64
dest_abs = ../_bin/libw64d.a
"""
text_gcc_lin32 += comhead + """
attr = -D_DEBUG -D_Linux -O3
aattr = -felf
dest_obj=$(uobjpath)/CGLin32
dest_abs=$(ubinpath)/libl32d.a
CC=gcc -m32
CX=g++ -m32
"""
text_gcc_lin64 += comhead + """
attr = -D_DEBUG -D_Linux -D__BITS__=64 -O3
aattr = -felf
dest_obj=$(uobjpath)/CGLin64
dest_abs=$(ubinpath)/libl64d.a
CC=gcc -m64
CX=g++ -m64
"""

#  warn4819: Unicode Existing
#  warn4010: Linefeed in Line Comment
#  warn4005: Macro Redefinition
#{TODO}:
text_msv_win32 += comhead + """
attr = /D_DEBUG /D_Win32 /nologo /wd4819 /wd4010 /wd4005
cplpref=_uvc32_
cpppref=_uxxvc32_
dest_obj=$(uobjpath)/CVWin32
TOOLDIR = E:/software/VS22/VC/Tools/MSVC/14.39.33519
CC = ${TOOLDIR}/bin/Hostx86/x86/cl.exe
CX = ${CC}
AR = ${TOOLDIR}/bin/Hostx86/x86/lib.exe
# contain x86 - x64 can run this
aattr = -fwin32
dest_abs = ../_bin/libw32d.lib
VLIB_64=/LIBPATH:"${TOOLDIR}/lib/x86/" /LIBPATH:"${TOOLDIR}/lib/onecore/x86" /LIBPATH:"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x86" /LIBPATH:"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/ucrt/x86"
VI_SYS="C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/ucrt/"
VI_64=${TOOLDIR}/include/ /I${VI_SYS} /I"C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/um/" /I"C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/shared/"
"""
#{TODO}:
text_msv_win64 += comhead + """
attr = /D_DEBUG /D_Win64 /nologo /wd4819 /wd4010 /wd4005
cplpref=_uvc64_
cpppref=_uxxvc64_
dest_obj=$(uobjpath)/CVWin64
TOOLDIR = E:/software/VS22/VC/Tools/MSVC/14.39.33519
CC = ${TOOLDIR}/bin/Hostx64/x64/cl.exe
CX = ${CC}
AR = ${TOOLDIR}/bin/Hostx64/x64/lib.exe
aattr = -fwin64
dest_abs = ../_bin/libw64d.lib
VLIB_64=/LIBPATH:"${TOOLDIR}/lib/x64/" /LIBPATH:"${TOOLDIR}/lib/onecore/x64" /LIBPATH:"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64" /LIBPATH:"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/ucrt/x64"
VI_SYS="C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/ucrt/"
VI_64=${TOOLDIR}/include/ /I${VI_SYS} /I"C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/um/" /I"C:/Program Files (x86)/Windows Kits/10/Include/10.0.19041.0/shared/"
"""

#
tmp = """
.PHONY: all
all: $(asmobjs) $(cplobjs) $(cppobjs)
"""
text_gcc_win32 += tmp
text_gcc_win64 += tmp
text_gcc_lin32 += tmp
text_gcc_lin64 += tmp
tmp = ".PHONY: all\n"+\
	"\nall:\n"+\
	'\t-@mkdir.exe -p ${dest_obj}\n'+\
	'\t-@rm -f ${dest_obj}/*\n'
text_msv_win32 += tmp
text_msv_win64 += tmp

for val in list_asm_free86:
	tmp = '\t@echo AS ' + val.replace("./lib", "lib") + "\n\t@${AASM} ${aattr} ${aat} " + val + " -o ${dest_obj}/_ae_" + get_outfilename(val) + ".o" + '\n'
	text_gcc_win32 += tmp
	text_msv_win32 += tmp
	text_gcc_lin32 += tmp
for val in list_asm_free64:
	tmp = '\t@echo AS ' + val.replace("./lib", "lib") + "\n\t@${AASM} ${aattr} ${aat} " + val + " -o ${dest_obj}/_ax_" + get_outfilename(val) + ".o" + '\n'
	text_gcc_win64 += tmp
	text_msv_win64 += tmp
if True:
	tmp = """
	@echo "CC  = $(CC)"
	@echo "CX  = $(CX)"
	@echo "ATTR= $(attr)"
"""
	text_gcc_win32 += tmp
	text_gcc_win64 += tmp
	text_gcc_lin32 += tmp
	text_gcc_lin64 += tmp
for val in list_cpl_file:
	tmp = "\t@$(CC) /c " + val + " /Fo:${dest_obj}/${cplpref}" + get_outfilename(val) + ".obj /I${VI_64} ${attr}" + "\n"
	text_msv_win32 += tmp
	text_msv_win64 += tmp
for val in list_cpp_file:
	tmp = "\t@$(CX) /c " + val + " /Fo:${dest_obj}/${cpppref}" + get_outfilename(val) + ".obj /I${VI_64} ${attr}" + "\n"
	text_msv_win32 += tmp
	text_msv_win64 += tmp
tmp = '\t@echo AR ${dest_abs} \n\t' + "@${AR} -rcs ${dest_abs} ${dest_obj}/*\n"
text_gcc_win32 += tmp
text_gcc_win64 += tmp
text_gcc_lin32 += tmp
text_gcc_lin64 += tmp
tmp = '\t@echo AR ${dest_abs} \n\t' + "@${AR} /OUT:${dest_abs} ${dest_obj}/* /nologo\n"
text_msv_win32 += tmp
text_msv_win64 += tmp

tmp = """

%.o: %.c
	@echo "CC $(<)" && $(CC) $(attr) -c $< -o $(dest_obj)/$(cplpref)$(notdir $@)
%.o: %.cpp
	@echo "CX $(<)" && $(CX) $(attr) -c $< -o $(dest_obj)/$(cpppref)$(notdir $@)

"""
text_gcc_win32 += tmp
text_gcc_win64 += tmp
text_gcc_lin32 += tmp
text_gcc_lin64 += tmp

set_makefile('./lib/make/cgw32.make', text_gcc_win32)
set_makefile('./lib/make/cgw64.make', text_gcc_win64)
set_makefile('./lib/make/cvw32.make', text_msv_win32)
set_makefile('./lib/make/cvw64.make', text_msv_win64)
set_makefile('./lib/make/cgl32.make', text_gcc_lin32)
set_makefile('./lib/make/cgl64.make', text_gcc_lin64)
text_gcc_win32 = text_gcc_win64 = ""
text_msv_win32 = text_msv_win64 = ""
text_gcc_lin32 = text_gcc_lin64 = ""

#{TO MIX with lin(elf)}
# unisym/lib/make/cgmx86.make --[Linux Distributes, LF]-> -bin/libmx86.a
# Mecocoa can use Linux format link-library, NOW!
list_gcc_mecocoa_files = []
for val in list_asm_free86:
	list_gcc_mecocoa_files.append("$(asmf) " + val)
list_gcc_mecocoa_files.append("$(CC32) ${libcdir}/driver/i8259A.c")
list_gcc_mecocoa_files.append("$(CC32) ${libcdir}/delay.c")
list_gcc_mecocoa_files.append("$(CC32) ${libcdir}/format/ELF.c")
list_gcc_mecocoa_files.append("$(CC32) ${libcdir}/driver/toki/rtclock.c")
list_gcc_mecocoa_files.append("$(CC32) ${libcdir}/driver/toki/PIT.c")
list_gcc_mecocoa_files.append("$(CC32) ${libcdir}/driver/keyboard.c")
list_gcc_mecocoa_files.append("$(CC32) ${libcdir}/task.c")
list_gcc_mecocoa_files.append("$(CC32) ${libcdir}/consio.c")
list_gcc_mecocoa_files.append("$(CC32) ${libcdir}/console/conformat.c")
text_gcc_mecocoa = "# UNISYM for MECOCOA-x86 built-" + str(__BuildTime) + '\n'
print(text_gcc_mecocoa)
text_gcc_mecocoa += ".PHONY: all\n"
text_gcc_mecocoa += """
unidir = /mnt/hgfs/unisym
libcdir = $(unidir)/lib/c
libadir = $(unidir)/lib/asm
asmattr = -I${unidir}/inc/Kasha/n_ -I${unidir}/inc/naasm/n_ -I./include/
asm  = $(ubinpath)/ELF64/aasm ${asmattr} #OPT: aasm
asmf = ${asm} -felf
CC32 = gcc -m32 -c -fno-builtin -fleading-underscore -fno-pic\
 -fno-stack-protector -I$(unidir)/inc/c -D_MCCA=0x8632
"""
text_gcc_mecocoa += "\nall:\n"
text_gcc_mecocoa += '\t' + "-sudo mkdir -m 777 -p $(uobjpath)/libmx86\n"
text_gcc_mecocoa += '\t' + "-rm -f $(uobjpath)/libmx86/*.obj\n"
for i in list_gcc_mecocoa_files:
	file_path, file_ext = os.path.splitext(i)
	text_gcc_mecocoa += '\t' + i + " -Dp_i386 -D_MCCA=0x8632 -o $(uobjpath)/libmx86/mx86_" + file_path.split("/")[-1] + ".obj\n" # for any bit
text_gcc_mecocoa += '\t' + "-rm $(ubinpath)/libmx86.a\n"
text_gcc_mecocoa += '\t' + "ar -rcs $(ubinpath)/libmx86.a $(uobjpath)/libmx86/*.obj\n"
with open('./lib/make/cgmx86.make', 'w+b') as fobj:
	fobj.write(bytes(text_gcc_mecocoa, encoding = "utf8")) # do not append line-feed
text_gcc_mecocoa = ""

# f: free-standing environment
# unisym/lib/make/cfcortexm3.make -> ...
#{} ...

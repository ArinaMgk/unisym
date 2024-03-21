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

# list_c_general = [] #{TODO} Depend   : ../../doc/catalog.csv
list_c_special = []
list_cpp = []


def get_files(whatpath, suffix):
    mfiles = []
    for root, dirs, files in os.walk(whatpath):
        for filename in files:
            if filename.endswith(suffix):
                mfiles.append(os.path.join(root, filename).replace("\\", "/"))
    return mfiles

# ---- ---- ---- ---- ---- ---- ---- ----
list_c_tomake = get_files("./lib/c/auxiliary/", ".make.c")
list_cpl_file = get_files("./lib/c/", ".c")
list_cpl_file_new = []
for i in list_cpl_file:
	if (".make.c" in i) or ("/lib/c/processor/" in i) or ("/lib/c/driver/" in i) or ("lib/c/ustring/bstring/" in i):
		pass
	else:
		list_cpl_file_new.append(i)
list_cpl_file = list_cpl_file_new
list_cpl_file_new = []
list_cpp_file = get_files("./lib/cpp/", ".cpp")

# unisym/lib/make/cgw32.make -> -bin/libw32d.a
text_gcc_win32 = "# UNISYM for MECOCOA built-" + str(__BuildTime) + '\n'
print(text_gcc_win32)
text_gcc_win32 = """
CC32 = gcc -m32
CX32 = g++ -m32
dest_obj=../_obj
"""

text_gcc_win32 += ".PHONY: all\n"
text_gcc_win32 += "\nall:\n"
text_gcc_win32 += '\t' + "-@E:/software/w64dev/bin/mkdir.exe -p ${dest_obj}/CGWin32" + '\n'
text_gcc_win32 += '\t' + "-@E:/software/w64dev/bin/rm.exe -f ${dest_obj}/CGWin32/*" + '\n'
text_gcc_win32 += '\t' + "cd ${dest_obj}/CGWin32/ && aasm -f win32 ../../unisym/lib/asm/x86/cpuid.asm -I../../unisym/inc/Kasha/n_ -o./cpuid.a.o" + '\n'
text_gcc_win32 += '\t' + "cd ${dest_obj}/CGWin32/ && aasm -f win32 ../../unisym/lib/asm/x86/binary.asm -I../../unisym/inc/Kasha/n_ -o./binary.a.o" + '\n'
for val in list_c_tomake:
	fname = val.split("/")[-1]
	file_path, file_ext = os.path.splitext(fname)
	text_gcc_win32 += '\t' + "$(CC32) " + val + " -o ../_tmp/" + file_path + ".exe" + " && ../_tmp/" + file_path + ".exe\n"
for val in list_cpl_file:
	fname = val.split("/")[-1]
	first_name, file_ext = os.path.splitext(fname)
	text_gcc_win32 += '\t' + "$(CC32) -c " + val + " -o ${dest_obj}/CGWin32/_ugc32_" + first_name + ".o -D_DEBUG -D_WinNT -D_Win32 -O3" + '\n'
for val in list_cpp_file:
	fname = val.split("/")[-1]
	first_name, file_ext = os.path.splitext(fname)
	text_gcc_win32 += '\t' + "$(CX32) -c " + val + " -o ${dest_obj}/CGWin32/_uxxgc32_" + first_name + ".o -D_DEBUG -D_WinNT -D_Win32 -O3" + '\n'

text_gcc_win32 += '\t' + "cd ${dest_obj}/CGWin32/ && ar -rcs ../../_bin/libw32.a *" + '\n'
# (use bash or may occur 'ar: *.obj: Invalid argument')
with open('./lib/make/cgw32.make', 'w+b') as fobj:
	fobj.write(bytes(text_gcc_win32, encoding = "utf8"))
text_gcc_win32 = ""

# unisym/lib/make/cgl32.make -> -bin/libw32d.a
text_gcc_lin32 = ""


# unisym/lib/make/cgmx86.make --[Linux Distributes, LF]-> -bin/libmx86.a
list_gcc_mecocoa_files = [
"@$(asmf) ${libadir}/x86/filesys/FAT12.asm",
"@$(asmf) ${libadir}/x86/filefmt/ELF.asm",
"@$(asmf) ${libadir}/x86/disk/floppy.asm",
"@$(asmf) ${libadir}/x86/inst/ioport.asm",
"@$(asmf) ${libadir}/x86/inst/manage.asm",
"@$(asmf) ${libadir}/x86/inst/stack.asm",
"@$(asmf) ${libadir}/x86/inst/interrupt.asm",
"@$(CC32) ${libcdir}/driver/i8259A.c",
"@$(CC32) ${libcdir}/processor/x86/delay.c -DADDR_CountSeconds=0x524"
]
text_gcc_mecocoa = "# UNISYM for MECOCOA built-" + str(__BuildTime) + '\n'
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
text_gcc_mecocoa += '\t' + "-@sudo mkdir -p ~/_obj/libmx86\n"
text_gcc_mecocoa += '\t' + "-@rm -f ~/_obj/libmx86/_u_*.obj\n"
for i in list_gcc_mecocoa_files:
	file_path, file_ext = os.path.splitext(i)
	text_gcc_mecocoa += '\t' + i + " -o ~/_obj/libmx86/_u_" + file_path.split("/")[-1] + ".obj\n"
text_gcc_mecocoa += '\t' + "@ar -rcs /mnt/hgfs/_bin/libmx86.a ~/_obj/libmx86/_u_*.obj\n"
with open('./lib/make/cgmx86.make', 'w+b') as fobj:
	fobj.write(bytes(text_gcc_mecocoa, encoding = "utf8")) # do not append line-feed
text_gcc_mecocoa = ""

# unisym/lib/make/cgmcu.stm32f103ve.make -> ...
#{} ...

# unisym/lib/make/cvw64.make -> -bin/libw64d.a
list_cpl_msvc_file = []
list_cpp_msvc_file = []



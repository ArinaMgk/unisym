#!
# ASCII Python-3 TAB4 LF
# Attribute:
import os
import datetime
# LastCheck: 20240320
__AllAuthor= ["@dosconio"]
__ModuTitle= "Makefile Script Generator Script of Python"
__Copyright= "ArinaMgk UniSym, Apache License Version 2.0"
# Parallel : ./makemake.pl
__BuildTime= datetime.datetime.today()
# Default  : {TEMP: _DEBUG Version}

print(__ModuTitle, ": built at", __BuildTime)

list_c_tomake = []

# list_c_general = [] #{TODO} Depend   : ../../doc/catalog.csv
list_c_special = []
list_cpp = []




# ---- ---- ---- ---- ---- ---- ---- ----
# unisym/lib/make/cgw32.make -> -bin/libw32d.a
list_cpl_gcc_win_file = []
list_cpp_gcc_win_file = []

# unisym/lib/make/cgl32.make -> -bin/libw32d.a
list_cpl_gcc_lin_file = []
list_cpp_gcc_lin_file = []

# unisym/lib/make/cgmx86.make --[Linux Distributes, LF]-> -bin/libmx86.a
list_gcc_mecocoa_files = [
"@$(asmf) ${libadir}/x86/filesys/FAT12.asm",
"@$(asmf) ${libadir}/x86/filefmt/ELF.asm",
"@$(asmf) ${libadir}/x86/disk/floppy.asm",
"@$(asmf) ${libadir}/x86/inst/ioport.asm",
"@$(asmf) ${libadir}/x86/inst/manage.asm",
"@$(asmf) ${libadir}/x86/inst/stack.asm",
"@$(asmf) ${libadir}/x86/inst/interrupt.asm",
"@$(cc32) ${libcdir}/driver/i8259A.c",
"@$(cc32) ${libcdir}/processor/x86/delay.c -DADDR_CountSeconds=0x524"
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
cc32 = gcc -m32 -c -fno-builtin -fleading-underscore -fno-pic\
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



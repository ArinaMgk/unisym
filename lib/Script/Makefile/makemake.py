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

__LibVersion= "0.1"

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
# unisym/lib/make/cgmx86.make -> -bin/libm32d.a (both Kernel+User)
# (Mecocoa User Library is Aococam, ACCM)
# unisym/lib/make/cgl64.make -> -bin/libl64d.a
text_gcc_win32 = "# UNISYM for GCC-Win32 built-" + str(__BuildTime) + '\n'
text_gcc_win64 = "# UNISYM for GCC-Win64 built-" + str(__BuildTime) + '\n'
text_msv_win32 =  "# UNISYM for MSVC-Win32 built-" + str(__BuildTime) + '\n'
text_msv_win64 = "# UNISYM for MSVC-Win64 built-" + str(__BuildTime) + '\n'
text_gcc_lin32 = "# UNISYM for GCC-Lin32 built-" + str(__BuildTime) + '\n'
text_gcc_mecocoa = "# UNISYM for MECOCOA-x86 built-" + str(__BuildTime) + '\n'
# text_gcc_mccar32 = "# UNISYM for MECOCOA-r32 built-" + str(__BuildTime) + '\n' # realized by mcca
text_gcc_lin64 = "# UNISYM for GCC-Lin64 built-" + str(__BuildTime) + '\n'
print(text_gcc_win32, text_gcc_win64, text_msv_win32, text_msv_win64, text_gcc_lin32, text_gcc_lin64, sep="")
print(text_gcc_mecocoa, sep="")

text_gcc_win32 += "ENVIDEN=cgw32" + "\n"
text_gcc_win64 += "ENVIDEN=cgw64" + "\n"
text_gcc_lin32 += "ENVIDEN=cgl32" + "\n"
text_gcc_mecocoa += "ENVIDEN=cgmx86" + "\n"
# text_gcc_mccar32 += "ENVIDEN=cgmr32" + "\n"
text_gcc_lin64 += "ENVIDEN=cgl64" + "\n"
text_msv_win32 += "ENVIDEN=cvw32" + "\n"
text_msv_win64 += "ENVIDEN=cvw64" + "\n"

def apd_gnu_item(text):
	global text_gcc_win32, text_gcc_win64, text_gcc_lin32, text_gcc_mecocoa, text_gcc_lin64
	global text_gcc_mccar32
	text_gcc_win32 += text
	text_gcc_win64 += text
	text_gcc_lin32 += text
	text_gcc_mecocoa += text
	text_gcc_lin64 += text

#[TEMP] ASM cannot make PIC

# ---- make list by win

comhead2 = """
AASM = aasm
aat=-D_MCCA=0x8632
asmpref=_ae_
cplpref=_cc_
cpppref=_cx_

define asm_to_o
$(dest_obj)/$(asmpref)$(notdir $(1:.asm=.o)): $(1)
endef
define c_to_o
$(dest_obj)/$(cplpref)$(notdir $(1:.c=.o)): $(1)
endef
define cpp_to_o
$(dest_obj)/$(cpppref)$(notdir $(1:.cpp=.o)): $(1)
endef

cplfile=$(wildcard lib/c/*.c) $(wildcard lib/c/**/*.c) $(wildcard lib/c/**/**/*.c) $(wildcard lib/c/**/**/**/*.c)
cplobjs=$(addprefix $(dest_obj)/$(cplpref),$(patsubst %c,%o,$(notdir $(cplfile))))
cppfile=$(wildcard lib/cpp/*.cpp) $(wildcard lib/cpp/**/*.cpp) $(wildcard lib/cpp/**/**/*.cpp) $(wildcard lib/cpp/**/**/**/*.cpp)
cppobjs=$(addprefix $(dest_obj)/$(cpppref),$(patsubst %cpp,%o,$(notdir $(cppfile))))
"""


text_gcc_win32 += comhead2 + """
asmfile=$(wildcard lib/asm/x86/*.asm) $(wildcard lib/asm/x86/**/*.asm)
asmobjs=$(addprefix $(dest_obj)/$(asmpref),$(patsubst %asm,%o,$(notdir $(asmfile))))

attr = -D_DEBUG -O3 -D_Win32
dest_obj=$(uobjpath)/CGWin32
BN = $(ubinpath)/I686/Win32/bin
CC = $(BN)/gcc.exe -m32
CX = $(BN)/g++.exe -m32
AR = $(BN)/ar.exe
MKDIR:=$(shell which mkdir.exe)
aattr = -fwin32
dest_abs = $(ubinpath)/libw32d.a
dest_dll = $(ubinpath)/libw32d.so.""" + __LibVersion

text_gcc_win64 += comhead2 + """
asmfile=$(wildcard lib/asm/x64/*.asm) $(wildcard lib/asm/x64/**/*.asm)
asmobjs=$(addprefix $(dest_obj)/$(asmpref),$(patsubst %asm,%o,$(notdir $(asmfile))))

attr = -D_DEBUG -O3 -D_Win64
dest_obj=$(uobjpath)/CGWin64
BN = $(ubinpath)/AMD64/Win64/bin
CC = $(BN)/gcc.exe  -m64
CX = $(BN)/g++.exe  -m64
AR = $(BN)/ar.exe
MKDIR:=$(shell which mkdir.exe)
aattr = -fwin64
dest_abs = $(ubinpath)/libw64d.a
dest_dll = $(ubinpath)/libw64d.so.""" + __LibVersion

text_gcc_lin32 += comhead2 + """
asmfile=$(wildcard lib/asm/x86/*.asm) $(wildcard lib/asm/x86/**/*.asm)
asmobjs=$(addprefix $(dest_obj)/$(asmpref),$(patsubst %asm,%o,$(notdir $(asmfile))))

attr = -D_DEBUG -D_Linux -D__BITS__=32 -O3
aattr = -felf
dest_obj=$(uobjpath)/CGLin32
dest_abs=$(ubinpath)/libl32d.a
CC=gcc -m32
CX=g++ -m32
AR=ar
LD=ld -m elf_i386
dest_dll=$(ubinpath)/libl32d.so.""" + __LibVersion


text_gcc_mecocoa += comhead2 + """
asmfile=$(wildcard lib/asm/x86/*.asm) $(wildcard lib/asm/x86/**/*.asm)
asmobjs=$(addprefix $(dest_obj)/$(asmpref),$(patsubst %asm,%o,$(notdir $(asmfile))))

attr = -D_DEBUG -D_MCCA=0x8632
aattr = -felf
dest_obj=$(uobjpath)/CGMin32
dest_abs=$(ubinpath)/libm32d.a

CXF1=-m32 -mno-red-zone -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-sse4 # -mgeneral-regs-only
CXF2=-fno-stack-protector -fno-pic -fno-exceptions -fno-unwind-tables -fno-builtin
COMWAN = -Wno-builtin-declaration-mismatch
COMFLG=$(CXF1) $(CXF2) -static -nostdlib $(COMWAN) -O0

# -fno-stack-protector: avoid undefined reference to `__stack_chk_fail_local'
# -mno-*sse*          : movdqa-series need more setting

CC=gcc $(COMFLG)
CX=g++ $(COMFLG) -std=c++2a -fno-rtti -Wno-volatile
AR=ar
LD=ld -m elf_i386
dest_dll=$(ubinpath)/libm32d.so.""" + __LibVersion


text_gcc_lin64 += comhead2 + """
asmfile=$(wildcard lib/asm/x64/*.asm) $(wildcard lib/asm/x64/**/*.asm)
asmobjs=$(addprefix $(dest_obj)/$(asmpref),$(patsubst %asm,%o,$(notdir $(asmfile))))

attr = -D_DEBUG -D_Linux -D__BITS__=64 -O3
aattr = -felf64
dest_obj=$(uobjpath)/CGLin64
dest_abs=$(ubinpath)/libl64d.a
CC=gcc -m64
CX=g++ -m64
AR=ar
LD=ld -m elf_x86_64
dest_dll=$(ubinpath)/libl64d.so.""" + __LibVersion

# [Example of XXXXPATH]
# msvcpath=E:/software/VS22/VC/Tools/MSVC/14.39.33519

#  warn4819: Unicode Existing
#  warn4010: Linefeed in Line Comment
#  warn4005: Macro Redefinition
#{TODO}:
text_msv_win32 += comhead2 + """
attr = /D_DEBUG /D_Win32 /nologo /wd4819 /wd4010 /wd4005
cplpref=_uvc_
cpppref=_uxxvc_
dest_obj=$(uobjpath)/CVWin32
CC = ${msvcpath}/bin/Hostx86/x86/cl.exe
CX = ${CC}
AR = ${msvcpath}/bin/Hostx86/x86/lib.exe
MKDIR:=$(shell which mkdir.exe)
# contain x86 - x64 can run this
aattr = -fwin32
dest_abs = $(ubinpath)/libw32d.lib
dest_dll=...
KitWin=C:/Program Files (x86)/Windows Kits/10
SubVer:=$(shell bash -c "ls \\\"$(KitWin)/Include\\\" | head -n 1")
VLIB_64=/LIBPATH:"${msvcpath}/lib/x86/" /LIBPATH:"${msvcpath}/lib/onecore/x86" /LIBPATH:"$(KitWin)/Lib/$(SubVer)/um/x86" /LIBPATH:"$(KitWin)/Lib/$(SubVer)/ucrt/x86"
KitInc=$(KitWin)/Include/$(SubVer)
VI_SYS="$(KitInc)/ucrt/"
VI_64=${msvcpath}/include/ /I${VI_SYS} /I"$(KitInc)/um/" /I"$(KitInc)/shared/"
"""
#{TODO}:
text_msv_win64 += comhead2 + """
attr = /D_DEBUG /D_Win64 /nologo /wd4819 /wd4010 /wd4005
cplpref=_uvc_
cpppref=_uxxvc_
dest_obj=$(uobjpath)/CVWin64
CC = ${msvcpath}/bin/Hostx64/x64/cl.exe
CX = ${CC}
AR = ${msvcpath}/bin/Hostx64/x64/lib.exe
MKDIR:=$(shell which mkdir.exe)
aattr = -fwin64
dest_abs = $(ubinpath)/libw64d.lib
dest_dll = $(ubinpath)/libw64d.so.""" + __LibVersion

text_msv_win64 += """
KitWin=C:/Program Files (x86)/Windows Kits/10
SubVer:=$(shell bash -c "ls \\\"$(KitWin)/Include\\\" | head -n 1")
VLIB_64=/LIBPATH:"${msvcpath}/lib/x64/" /LIBPATH:"${msvcpath}/lib/onecore/x64" /LIBPATH:"$(KitWin)/Lib/$(SubVer)/um/x64" /LIBPATH:"$(KitWin)/Lib/$(SubVer)/ucrt/x64"
KitInc=$(KitWin)/Include/$(SubVer)
VI_SYS="$(KitInc)/ucrt/"
VI_64=${msvcpath}/include/ /I${VI_SYS} /I"$(KitInc)/um/" /I"$(KitInc)/shared/"
"""

#
tmp = """
.PHONY: all
all: $(asmobjs) $(cplobjs) $(cppobjs)
"""
text_gcc_win32 += tmp
text_gcc_win64 += tmp
text_gcc_lin32 += tmp
text_gcc_mecocoa += tmp
text_gcc_lin64 += tmp
tmp = ".PHONY: all\n"+\
	"\nall:\n"+\
	'\t-@$(MKDIR) -p ${dest_obj}\n'+\
	'\t-@$(MKDIR) -p ${dest_obj}-DLL\n'+\
	'\t-@rm -f ${dest_obj}/*\n'+\
	'\t-@rm -f ${dest_obj}-DLL/*\n'
text_msv_win32 += tmp
text_msv_win64 += tmp

# Independent Standard Header
tmp = "\t@$(CC) -E ./lib/c/empty.c $(attr) -o $(ubinpath)/std$(ENVIDEN).h\n"
apd_gnu_item(tmp)
#{TODO} for MSVC


if True:
	tmp = """
	@echo "CC  = $(CC)"
	@echo "CX  = $(CX)"
	@echo "ATTR= $(attr)"
"""
	text_gcc_win32 += tmp
	text_gcc_win64 += tmp
	text_gcc_lin32 += tmp
	text_gcc_mecocoa += tmp
	text_gcc_lin64 += tmp
for val in list_cpl_file:
	tmp = "\t@$(CC) /c " + val + " /Fo:${dest_obj}/${cplpref}" + get_outfilename(val) + ".obj /I${VI_64} ${attr}" + "\n"
	text_msv_win32 += tmp
	text_msv_win64 += tmp
for val in list_cpp_file:
	tmp = "\t@$(CX) /c " + val + " /Fo:${dest_obj}/${cpppref}" + get_outfilename(val) + ".obj /I${VI_64} ${attr}" + "\n"
	text_msv_win32 += tmp
	text_msv_win64 += tmp
tmp = """\t@echo AR ${dest_abs}
\t@${AR} -rcs ${dest_abs} ${dest_obj}/*.o*
\t@echo AR ${dest_dll}
\t@rm -f ${dest_dll}
"""
text_gcc_win32 += tmp
text_gcc_win64 += tmp
text_gcc_mecocoa += tmp
tmp += """\t@${LD} -shared -o ${dest_dll} ${dest_obj}-DLL/* -lc"""
text_gcc_lin32 += tmp
text_gcc_lin64 += tmp
tmp = """\t@echo AR ${dest_abs}
\t@${AR} /OUT:${dest_abs} ${dest_obj}/* /nologo
"""
text_msv_win32 += tmp
text_msv_win64 += tmp



tmp = """
$(foreach src,$(asmfile),$(eval $(call asm_to_o,$(src))))
$(foreach src,$(cplfile),$(eval $(call c_to_o,$(src))))
$(foreach src,$(cppfile),$(eval $(call cpp_to_o,$(src))))

_ae_%.o:
	@echo AS $<
	@${AASM} ${aattr} ${aat} $< -o $@ -MD $(patsubst %.o,%.d,$@)

_cc_%.o:
	@echo "CC $(<)"
	@$(CC) $(attr) -c $< -o $@ -MMD -MF $(patsubst %.o,%.d,$@) -MT $@ || ret 1 "!! Panic When: $(CC) $(attr) -c $< -o $@ -MMD -MF $(patsubst %.o,%.d,$@) -MT $@"

_cx_%.o: 
	@echo "CX $(<)" 
	@$(CX) $(attr) -c $< -o $@ -MMD -MF $(patsubst %.o,%.d,$@) -MT $@ || ret 1 "!! Panic When: $(CX) $(attr) -c $< -o $@ -MMD -MF $(patsubst %.o,%.d,$@) -MT $@"

clean:
	@-rm -rf $(dest_obj)/*
-include $(dest_obj)/*.d

"""
text_gcc_win32 += tmp
text_gcc_win64 += tmp
text_gcc_lin32 += tmp
text_gcc_lin64 += tmp
text_gcc_mecocoa += tmp



set_makefile('./lib/make/cgw32.make', text_gcc_win32)
set_makefile('./lib/make/cgw64.make', text_gcc_win64)
set_makefile('./lib/make/cvw32.make', text_msv_win32)
set_makefile('./lib/make/cvw64.make', text_msv_win64)
set_makefile('./lib/make/cgl32.make', text_gcc_lin32)
set_makefile('./lib/make/cgmx86.make', text_gcc_mecocoa)
set_makefile('./lib/make/cgl64.make', text_gcc_lin64)
text_gcc_win32 = text_gcc_win64 = ""
text_msv_win32 = text_msv_win64 = ""
text_gcc_lin32 = text_gcc_mecocoa = text_gcc_lin64 = ""

# ---- # ---- # MCUDEV # ---- # ---- #

STM32F1 = "# UNISYM for GCC-STM32F1 built-" + str(__BuildTime) + '\n'
STM32F4 = "# UNISYM for GCC-STM32F4 built-" + str(__BuildTime) + '\n'
STM32H7 = "# UNISYM for GCC-STM32H7 built-" + str(__BuildTime) + '\n'
STM32MP13 = "# UNISYM for GCC-STM32MP13 built-" + str(__BuildTime) + '\n'
print(STM32F1, STM32F4, STM32H7, STM32MP13, sep="")

STM32F1   += """IDEN=STM32F1
FLAG=-I$(uincpath) -D_MCU_$(IDEN)x -mcpu=cortex-m3 -mthumb $(FPU) $(FLOAT-ABI)
"""
STM32F4   += """IDEN=STM32F4
FLAG=-I$(uincpath) -D_MCU_$(IDEN)x -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
"""
STM32H7   += """IDEN=STM32H7
FLAG=-I$(uincpath) -D_MCU_$(IDEN)x -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
"""
STM32MP13 += """IDEN=STM32MP13
FLAG=-I$(uincpath) -D_MPU_$(IDEN)  -mcpu=cortex-a7 -mthumb -mfpu=vfpv4-d16   -mfloat-abi=hard
"""
tmp = '''
FLAG+=-fdata-sections -ffunction-sections
PREF=arm-none-eabi-
DEST=$(ubinpath)/lib$(IDEN).a
AS=$(PREF)as
CC=$(PREF)gcc -c
LD=$(PREF)ld
AR=$(PREF)ar
NM=$(PREF)nm
CX=$(PREF)g++ -c
STRIP=$(PREF)strip
OCOPY=$(PREF)objcopy
ODUMP=$(PREF)objdump
RANLIB=$(PREF)ranlib
OPATH=$(uobjpath)/$(IDEN)

cplfile=$(wildcard lib/c/data/**/*.c) $(wildcard lib/c/console/*.c) lib/c/mcore.c
cplobjs=$(patsubst %c, %o, $(cplfile))
cppfile=$(wildcard lib/cpp/*.cpp) $(wildcard lib/cpp/Device/*.cpp) $(wildcard lib/cpp/Device/**/*.cpp) $(wildcard lib/cpp/Device/**/**/*.cpp)
cppfile+= lib/cpp/MCU/$(IDEN).cpp
cppobjs=$(patsubst %cpp, %o, $(cppfile))

all: init $(cplobjs) $(cppobjs)
	@echo AR $(DEST) && ${AR} -rcs $(DEST) $(OPATH)/*
init:
	-@test -d $(OPATH) || mkdir -p $(OPATH)
%.o: %.c
	@echo CC $(<)
	@$(CC) $(FLAG) $< -o $(OPATH)/_g_$(notdir $@) || ret 1 "!! Panic When: $(CC) $(FLAG) $< -o $(OPATH)/_g_$(notdir $@)"	
%.o: %.cpp
	@echo CX $(<)
	@$(CX) $(FLAG) $< -o $(OPATH)/_g_$(notdir $@) || ret 1 "!! Panic When: $(CX) $(FLAG) $< -o $(OPATH)/_g_$(notdir $@)"
'''
STM32F1   += tmp
STM32F4   += tmp
STM32H7   += tmp
STM32MP13 += tmp
with open('./lib/make/cortexm3-Gnu-STM32F1.make', 'w+b') as fobj:
	fobj.write(bytes(STM32F1, encoding = "utf8"))
with open('./lib/make/cortexm4-Gnu-STM32F4.make', 'w+b') as fobj:
	fobj.write(bytes(STM32F4, encoding = "utf8"))
with open('./lib/make/cortexm7-Gnu-STM32H7.make', 'w+b') as fobj:
	fobj.write(bytes(STM32H7, encoding = "utf8"))
with open('./lib/make/cortexa7-Gnu-STM32MP13.make', 'w+b') as fobj:
	fobj.write(bytes(STM32MP13, encoding = "utf8"))

STM32F1 = ""
STM32F4 = ""
STM32H7 = ""
STM32MP13 = ""

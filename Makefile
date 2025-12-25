# ASCII Makefile TAB4 CRLF
# Attribute: 
# LastCheck: 20240320
# AllAuthor: @dosconio
# ModuTitle: Makefile for UniSym
# Copyright: ArinaMgk UniSym, Apache License Version 2.0
# Dependens:
# - TexLive
# - Perl, Python
# - GNU gcc, make
# - gcc-arm-none-eabi


make_dir=./lib/make/
ulibpath?=./lib
uincpath?=./inc
ubinpath?=../_bin
uobjpath?=../_obj

.PHONY: local list manual clean \
	mx86 cgw16 cgw32 cgw64 cvw32 cvw64 cgl32 cgl64\
	\
	malice dotnet rust libnvcc

ciallo:
	@echo Welcome to use UNISYM

local:
	@make --no-print-directory -f ${make_dir}local.make all

list: local# depend [perl python]
	@perl ./lib/Script/Makefile/makemake.pl

native:
	#{TODO}

# ---- [naming style generation 2] ----
# {TODO} cross-compile system, now many native in win64 and lin64
# ELF including (Real16, Flap32 ...)
mx86: x86-EG-MCCA
# COFF with DJGPP
cgw16: i8086-CG-IBMPC
cgw32: x86-CG-Win32
cgw64: x64-CG-Win64
# libmsvc MTd_StaticDebug 
cvw32: x86-CV-Win32
cvw64: x64-CV-Win64
# liblinux ELF
cgl32: x86-EG-Lin32
cgl64: x64-EG-Lin64
cll64: x64-EL-Lin64 # C/C++: AMD64 ELF LLVM Linux(Mode-64)
# mcudev
cgstm32f: cortexm3-EG-STM32F1 cortexm4-EG-STM32F4
cgstm32h: cortexm7-EG-STM32H7
cgstm32mp: cortexa7-Gnu-STM32MP13

# ---- [other hosted-environments] ----

# Nvidia(R) CUDA
libnvcc: 
	#

# ---- [series for interfacial environments] ----

malice: # Magice Standard Library, into .mid middle file
	#

dotnet:
	#

rust:
	cd lib/Rust/unisym && cargo build
	-cp lib/Rust/unisym/target/debug/libunisym.rlib $(ubinpath)/

# ---- [utilities] ----

manual:
	@cd doc && xelatex herepc.tex && mv herepc.pdf ${ubinpath}/${@}.pdf

magice:
	@make -f ${make_dir}magic.make --no-print-directory

test:
	@echo Please use METUTOR to make a check or test for any component

boot:
	@make -f ${make_dir}boot.make --no-print-directory

aasm: # for linux64 [no use O3]
	cc asm/data/*.c asm/about.c asm/aasm*.cpp $(ulibpath)/cpp/fileformat/execute/*.cpp -I$(uincpath) -L$(ubinpath) -ll64d -lstdc++ -D_DEBUG -D_Linux64 -o $(ubinpath)/AMD64/Lin64/aasm -w
	cc asm/data/_*.c asm/data/insnsb.c asm/data/insnsd.c asm/data/regs.c asm/data/regdis.c asm/about.c asm/dasm*.cpp asm/aasm-lib.cpp -I$(uincpath) -L$(ubinpath) -ll64d -lstdc++ -D_DEBUG -D_Linux64 -DNO_STDSCAN -o $(ubinpath)/AMD64/Lin64/dasm -w

# ---- ---- ---- ---- [GENE3 STYLE] ---- ---- ---- ----
# Win and Lin unified by Target Triple Cross-Compiles 
i8086-CG-IBMPC:
	@echo TODO
x86-CG-Win32: list
	-@mkdir.exe -p $(uobjpath)/CGWin32
	-@mkdir.exe -p $(uobjpath)/CGWin32-DLL
	-@rm -f $(uobjpath)/CGWin32/*
	-@rm -f $(ubinpath)/libw32d.a
	make -f ${make_dir}cgw32.make all
	cd ${make_dir} && make -f kitw32.make all
x86-CV-Win32: list
	-@rm -f $(ubinpath)/libw32d.lib
	make -f ${make_dir}cvw32.make all
x86-CM-Win32: #{} cmw32
x86-CL-Win32: #{} clw32

x64-CG-Win64: list
	-@mkdir.exe -p $(uobjpath)/CGWin64
	-@mkdir.exe -p $(uobjpath)/CGWin64-DLL
	-@rm -f $(uobjpath)/CGWin64/*
	-@rm -f $(ubinpath)/libw64d.a
	make -f ${make_dir}cgw64.make all # x86_64-8.1.0-release-posix-seh-rt_v6-rev0.7z
x64-CV-Win64: list
	-@rm -f $(ubinpath)/libw64d.lib
	make -f ${make_dir}cvw64.make all
x64-CM-Win64: #{} cmw64
x64-CL-Win64: #{} clw64

# [Static + Dynamic]
x86-EG-Lin32: list
	-@mkdir -p $(uobjpath)/CGLin32
	-@mkdir -p $(uobjpath)/CGLin32-DLL
	-@rm -f $(uobjpath)/CGLin32/*
	-@rm -f $(ubinpath)/libl32d.a
	make -f ${make_dir}cgl32.make all
# [Static + Dynamic]
x64-EG-Lin64: list
	-@mkdir -p $(uobjpath)/CGLin64
	-@mkdir -p $(uobjpath)/CGLin64-DLL
	-@rm -f $(uobjpath)/CGLin64/*
	-@rm -f $(ubinpath)/libl64d.a
	make -f ${make_dir}cgl64.make all
	cd ${make_dir} && make -f kitl64.make all
x64-EV-Lin64:
x64-EM-Lin64:
x64-EL-Lin64: x64-EG-Lin64
x86-EG-MCCA: list
	-@mkdir -p $(uobjpath)/CGMin32
	#-@mkdir -p $(uobjpath)/CGMin32-DLL
	-@rm -f $(uobjpath)/CGMin32/*
	-@rm -f $(ubinpath)/libm32d.a
	make -f ${make_dir}cgmx86.make all
riscv64-EG-MCCA:
	@echo TODO
i8051-Keil-AT89C5:
	@echo TODO

cortexm0-EG-CW32F003:
	@echo TODO
cortexm0-Keil-CW32F003:
	@echo TODO
cortexm0-EG-CW32F030:
	@echo TODO
cortexm0-Keil-CW32F030:
	@echo TODO

cortexm3-EG-STM32F1: list
	-@mkdir -p $(uobjpath)/STM32F1
	-@rm -f $(uobjpath)/STM32F1/*
	-@rm -f $(ubinpath)/libSTM32F1.a
	@make --no-print-directory -f ${make_dir}cortexm3-Gnu-STM32F1.make all
cortexm3-Keil-STM32F1:
	@echo TODO
cortexm4-EG-STM32F4:
	-@mkdir -p $(uobjpath)/STM32F4
	-@rm -f $(uobjpath)/STM32F4/*
	-@rm -f $(ubinpath)/libSTM32F4.a
	@make --no-print-directory -f ${make_dir}cortexm4-Gnu-STM32F4.make all
cortexm4-Keil-STM32F4:
	@echo TODO
cortexm7-EG-STM32H7:
	-@mkdir -p $(uobjpath)/STM32H7
	-@rm -f $(uobjpath)/STM32H7/*
	-@rm -f $(ubinpath)/libSTM32H7.a
	@make --no-print-directory -f ${make_dir}cortexm7-Gnu-STM32H7.make all
cortexm7-Keil-STM32H7:
	@echo TODO

cortexa7-Gnu-STM32MP13: list
	-@mkdir -p $(uobjpath)/STM32MP13
	-@rm -f $(uobjpath)/STM32MP13/*
	-@rm -f $(ubinpath)/libSTM32MP13.a
	@make --no-print-directory -f ${make_dir}cortexa7-Gnu-STM32MP13.make all
cortexa7-Keil-STM32MP13:
	@echo TODO

clean:
	-cd ./inc/Python/ && rmdir __pycache__ /S /Q
	-cd ./lib/Rust/unisym && cargo clean

# ASCII Makefile TAB4 CRLF
# Attribute: 
# LastCheck: 20240320
# AllAuthor: @dosconio
# ModuTitle: Makefile for UniSym
# Copyright: ArinaMgk UniSym, Apache License Version 2.0

make_dir=./lib/make/

# GENE-2 Style: cXXxx

.PHONY: \
	release-on-win release-on-lin release-on-dos \
	\
	local list list-py3\
	\
	mx86 \
	cgw16 cgw32 cgw64 dllmsvc cvw32 cvw64 libnvcc\
	cgl32 cgl64\
	\
	malice dotnet rust\
	\
	manual kitw32-more\
	\
	test clean

local:
	@clear
	@echo MK Local Data
	@$(CC) ./lib/local/toxxxer.make.c -o $(uobjpath)/toxxxer.make.exe && cd $(uobjpath)/ && ./toxxxer.make.exe

list: local# depend [perl python]
	@perl ./lib/Script/Makefile/makemake.pl

# ---- [usual hosted-environments] ----

mx86: list # including (Real16, Flap32 ...)
	make -f ${make_dir}cgmx86.make all

# COFF 
cgw16: # with DJGPP
	#
cgw32: list
	-@mkdir.exe -p $(uobjpath)/CGWin32
	-@rm -f $(uobjpath)/CGWin32/*
	-@rm -f $(ubinpath)/libw32d.a
	make -f ${make_dir}cgw32.make all
	cd ${make_dir} && make -f kitw32.make all
cgw64: list
	-@mkdir.exe -p $(uobjpath)/CGWin64
	-@rm -f $(uobjpath)/CGWin64/*
	-@rm -f $(ubinpath)/libw64d.a
	make -f ${make_dir}cgw64.make all # x86_64-8.1.0-release-posix-seh-rt_v6-rev0.7z

# libmsvc MTd_StaticDebug 
cvw32: list
	-@rm -f $(ubinpath)/libw32d.lib
	make -f ${make_dir}cvw32.make all
cvw64: list
	-@rm -f $(ubinpath)/libw64d.lib
	make -f ${make_dir}cvw64.make all
#{TODO} Make DLL File

# Nvidia(R) CUDA
libnvcc: 
	#

# liblinux ELF
cgl32: list
	-@mkdir -p $(uobjpath)/CGLin32
	-@rm -f $(uobjpath)/CGLin32/*
	-@rm -f $(ubinpath)/libl32d.a
	make -f ${make_dir}cgl32.make all
cgl64: list
	-@mkdir -p $(uobjpath)/CGLin64
	-@rm -f $(uobjpath)/CGLin64/*
	-@rm -f $(ubinpath)/libl64d.a
	make -f ${make_dir}cgl64.make all
	cd ${make_dir} && make -f kitl64.make all

# ---- [series for interfacial environments] ----

malice: # Magice Standard Library
	#

dotnet:
	#

rust:
	cd lib/Rust/unisym && cargo build
	-cp lib/Rust/unisym/target/debug/libunisym.rlib $(ubinpath)/

# ---- [utilities] ----

manual:
	@cd doc && xelatex herepc.tex && mv herepc.pdf ../../$@
	@echo "Build Manual Finish."

kitw32-more:	
	-ahkcc ./lib/Script/AutoHotkey/Arnscr.ahk ../../../../_bin/arnscr.exe # ***\AutoHotkey\Compiler\Ahk2Exe.exe /in %1 /out %2

MGC_CFLG = -std=c99 -fno-common

magice:
	gcc $(MGC_CFLG) -o $$ubinpath/ELF64/mgc magic/*.c

# ---- [test] ----

test: # "trust"
	cd lib/Rust/unisym && cargo test

test-mgc:
	@cd magic && ./chkmgc.sh

release-on-win: list cgw32 cgw64 cvw32 cvw64 manual #tools...
	#make -C asm
	@echo FI # finish
release-on-lin: list mx86 cgl32 cgl64 rust #tools...
	#make -C asm
	#wel
	@echo FI # finish
release-on-dos: cgw16
	make -C asm win16
	@echo QAQ

# ---- [GENE3 STYLE] ----
i8086-CG-IBMPC:
	@echo TODO
x86-CG-Win32: cgw32
x86-CM-Win32: cvw32
x64-CG-Win64: cgw64
x64-CM-Win64: cvw64
x86-EG-Lin32: cgl32
x64-EG-Lin64: cgl64
x86-EG-MCCA: mx86
riscv64-EG-MCCA:
	@echo TODO
i8051-Keil-AT89C5:
	@echo TODO
cortexm3-EG-STM32F1:
	@echo TODO
cortexm4-EG-STM32F4:
	@echo TODO
cortexm0-EG-CW32F03:
	@echo TODO
cortexm0-EG-CW32F00:
	@echo TODO
cortexm3-Keil-STM32F1:
	@echo TODO
cortexm4-Keil-STM32F4:
	@echo TODO
cortexm0-Keil-CW32F03:
	@echo TODO
cortexm0-Keil-CW32F00:
	@echo TODO

clean:
	-cd ./inc/Python/ && rmdir __pycache__ /S /Q

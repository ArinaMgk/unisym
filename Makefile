# ASCII Makefile TAB4 CRLF
# Attribute: 
# LastCheck: 20240320
# AllAuthor: @dosconio
# ModuTitle: Makefile for UniSym
# Copyright: ArinaMgk UniSym, Apache License Version 2.0

make_dir=./lib/make/

# depend [gcc, makefile] [python]
# (use bash, or try windows-CMD, may occur 'ar: *.obj: Invalid argument' or others...)

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
	manual kitw32 kitw32-more\
	\
	test clean

local:
	@clear
	@echo MK Local Data
	@$(CC) ./lib/local/toxxxer.make.c -o $(uobjpath)/toxxxer.make.exe && cd $(uobjpath)/ && ./toxxxer.make.exe

list: local# depend [perl python]
	@perl ./lib/Script/Makefile/makemake.pl

# ---- [usual hosted-environments] ----

mx86: #[Linux] # including different bitmodes (Real16, Flap32 ...)
	make -f ${make_dir}cgmx86.make all

# COFF 
cgw16: # with DJGPP
	#
cgw32: list
	-@mkdir.exe -p $(uobjpath)/CGWin32
	-@rm -f $(uobjpath)/CGWin32/*
	make -f ${make_dir}cgw32.make all
cgw64: list
	-@mkdir.exe -p $(uobjpath)/CGWin64
	-@rm -f $(uobjpath)/CGWin64/*
	make -f ${make_dir}cgw64.make all # x86_64-8.1.0-release-posix-seh-rt_v6-rev0.7z

# libmsvc MTd_StaticDebug 
cvw32:
	make -f ${make_dir}cvw32.make all
cvw64:
	make -f ${make_dir}cvw64.make all
#{TODO} Make DLL File

# Nvidia(R) CUDA
libnvcc: 
	#

# liblinux ELF
cgl32:
	-@mkdir -p $(uobjpath)/CGLin32
	-@rm -f $(uobjpath)/CGLin32/*
	make -f ${make_dir}cgl32.make all
cgl64:
	-@mkdir -p $(uobjpath)/CGLin64
	-@rm -f $(uobjpath)/CGLin64/*
	make -f ${make_dir}cgl64.make all

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

kitw32: # utility #(x86)
	cd ${make_dir} && make -f kitw32.make all
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

release-on-win: list cgw32 cgw64 cvw32 cvw64 manual kitw32 #tools...
	#make -C asm
	@echo FI # finish
release-on-lin: list mx86 cgl32 cgl64 rust #tools...
	#make -C asm
	#wel
	@echo FI # finish
release-on-dos: cgw16
	make -C asm win16
	@echo QAQ



clean:
	-cd ./inc/Python/ && rmdir __pycache__ /S /Q

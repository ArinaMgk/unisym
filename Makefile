# ASCII Makefile TAB4 CRLF
# Attribute: 
# LastCheck: 20240320
# AllAuthor: @dosconio
# ModuTitle: Makefile for UniSym
# Copyright: ArinaMgk UniSym, Apache License Version 2.0

# Resources: for release version:
# - inc/
# - libs
# - bin/  [optional] (utilities)
# - demo/ [optional]
# - aasm, magice, ...

MSVC_DIR = E:/software/VS22/VC/Tools/MSVC/14.39.33519

dest_bin=../_bin
dest_obj=../_obj
make_dir=./lib/make/

# depend [gcc, makefile] [python]
# (use bash, or try windows-CMD, may occur 'ar: *.obj: Invalid argument' or others...)

.PHONY: \
	list list-py3\
	\
	mx86 \
	libwingcc cgw16 cgw32 cgw64 dllmsvc libmsvc cvw32 cvw64 libnvcc\
	liblinux cgl32 cgl64\
	\
	malice dotnet rust\
	\
	manual kitw32\
	\
	tlwin tllin test clean

list: # depend [perl python]
	@perl ./lib/Script/Makefile/makemake.pl

# ---- [usual hosted-environments] ----

mx86: #[Linux] # including different bitmodes (Real16, Flap32 ...)
	make -f ${make_dir}cgmx86.make all

libwingcc: cgw16 cgw32 cgw64 # COFF 
cgw16: # with DJGPP
	#
cgw32:
	make -f ./lib/make/cgw32.make all
cgw64:
	make -f ./lib/make/cgw64.make all # x86_64-8.1.0-release-posix-seh-rt_v6-rev0.7z

dllmsvc:
	#
libmsvc: cvw32 cvw64# MTd_StaticDebug 
cvw32:
	make -f ./lib/make/cvw32.make all
cvw64:
	make -f ./lib/make/cvw64.make all

libnvcc: # Nvidia(R) CUDA
	#

liblinux: cgl32 cgl64 # ELF
cgl32:
	make -f ./lib/make/cgl32.make all
cgl64:
	make -f ./lib/make/cgl64.make all

# ---- [series for interfacial environments] ----

malice: # Magice Standard Library
	#

dotnet:
	#

rust:
	cd lib/Rust/unisym && cargo build
	-cp lib/Rust/unisym/target/debug/libunisym.rlib $(dest_bin)/

# ---- [utilities] ----

manual: ${dest_bin}/usymdoc.pdf ${dest_bin}/magice.pdf#optional: latex ?.tex && dvipdfmx ?.dvi
	echo "Build Finish."
${dest_bin}/usymdoc.pdf:
	cd  ${dest_bin}/ && xelatex ../unisym/doc/manual/usymdoc.tex && rm usymdoc.log usymdoc.aux 
${dest_bin}/magice.pdf:
	cd  ${dest_bin}/ && xelatex ../unisym/doc/magice/magice.tex && rm magice.log magice.aux magice.out

kitw32: # utility
	cd ${make_dir} && make -f kitw32.make all
	-ahkcc ./lib/Script/AutoHotkey/Arnscr.ahk ../../../../_bin/arnscr.exe # ***\AutoHotkey\Compiler\Ahk2Exe.exe /in %1 /out %2

# ---- [test] ----

tlwin: list cgw32 cgw64 cvw32 cvw64# test lib win
	@echo "Build Finish."
tllin: cgl32 cgl64 mx86# test lib lin
	@echo "Build Finish."

test: # "trust"
	cd lib/Rust/unisym && cargo test

clean:
	-cd ./inc/Python/ && rmdir __pycache__ /S /Q

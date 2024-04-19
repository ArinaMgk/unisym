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

.PHONY: list mx86 cgw32 liblinux cgl32 cgl64  manual winslib linslib

winslib: libwingcc libmsvc
linslib: liblinux mx86

list: # depend [python]
	@python ./lib/Script/Makefile/makemake.py

# ---- [usual host-environments] ----

malice: # Magice Standard Library
	#

mx86:
	make -f ${make_dir}cgmx86.make all

libwingcc: cgw32 cgw64 # COFF 
cgw16: # with DJGPP
	#
cgw32:
	make -f ./lib/make/cgw32.make all
cgw64:
	make -f ./lib/make/cgw64.make all # x86_64-8.1.0-release-posix-seh-rt_v6-rev0.7z

libmsvc: cvw32 cvw64# MTd_StaticDebug 
cvw32:
	make -f ./lib/make/cvw32.make all
cvw64:
	make -f ./lib/make/cvw64.make all

libnvcc: # Nvidia(R) CUDA
	#

liblinux: cgl32 cgl64 # ELF
cgl16:
cgl32:
	make -f ./lib/make/cgl32.make all
cgl64:
	make -f ./lib/make/cgl64.make all

# ---- [series for free-standing environments] ----

unii80386:
	#
unii8051:
	#
uniarmv7m: # including CortexM3
	#

# ---- [series for interfacial environments] ----

dotnet:
	#

# ---- [series for specific board] ----

STM32F103VEx:
	#

# ---- [utilities] ----

kitw32: # utility
	cd ${make_dir} && make -f kitw32.make all
	-ahkcc ./lib/Script/AutoHotkey/Arnscr.ahk ../../../../_bin/arnscr.exe # ***\AutoHotkey\Compiler\Ahk2Exe.exe /in %1 /out %2

manual: ${dest_bin}/usymdoc.pdf ${dest_bin}/magice.pdf#optional: latex ?.tex && dvipdfmx ?.dvi
	echo "Build Finish."
${dest_bin}/usymdoc.pdf:
	cd  ${dest_bin}/ && xelatex ../unisym/doc/manual/usymdoc.tex && rm usymdoc.log usymdoc.aux 
${dest_bin}/magice.pdf:
	cd  ${dest_bin}/ && xelatex ../unisym/doc/magice/magice.tex && rm magice.log magice.aux magice.out

clean:
	-cd ./inc/Python/ && rmdir __pycache__ /S /Q

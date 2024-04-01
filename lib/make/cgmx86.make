# UNISYM for MECOCOA built-2024-04-01 23:50:23.136263
.PHONY: all

unidir = /mnt/hgfs/unisym
libcdir = $(unidir)/lib/c
libadir = $(unidir)/lib/asm
asmattr = -I${unidir}/inc/Kasha/n_ -I${unidir}/inc/naasm/n_ -I./include/
asm  = /mnt/hgfs/_bin/ELF64/aasm ${asmattr} #OPT: aasm
asmf = ${asm} -felf
CC32 = gcc -m32 -c -fno-builtin -fleading-underscore -fno-pic -fno-stack-protector -I/mnt/hgfs/unisym/inc/c -D_Linux

all:
	-sudo mkdir -m 777 -p ~/_obj/libmx86
	-rm -f ~/_obj/libmx86/_u_*.obj
	$(asmf) ./lib/asm/x86/binary.asm -o ~/_obj/libmx86/_u_binary.obj
	$(asmf) ./lib/asm/x86/cpuid.asm -o ~/_obj/libmx86/_u_cpuid.obj
	$(asmf) ./lib/asm/x86/disk/floppy.asm -o ~/_obj/libmx86/_u_floppy.obj
	$(asmf) ./lib/asm/x86/filefmt/ELF.asm -o ~/_obj/libmx86/_u_ELF.obj
	$(asmf) ./lib/asm/x86/filesys/FAT12.asm -o ~/_obj/libmx86/_u_FAT12.obj
	$(asmf) ./lib/asm/x86/inst/interrupt.asm -o ~/_obj/libmx86/_u_interrupt.obj
	$(asmf) ./lib/asm/x86/inst/ioport.asm -o ~/_obj/libmx86/_u_ioport.obj
	$(asmf) ./lib/asm/x86/inst/manage.asm -o ~/_obj/libmx86/_u_manage.obj
	$(asmf) ./lib/asm/x86/inst/stack.asm -o ~/_obj/libmx86/_u_stack.obj
	$(asmf) ./lib/asm/x86/interrupt/i8259A.asm -o ~/_obj/libmx86/_u_i8259A.obj
	$(asmf) ./lib/asm/x86/memory/memory.asm -o ~/_obj/libmx86/_u_memory.obj
	$(asmf) ./lib/asm/x86/string/ustring.asm -o ~/_obj/libmx86/_u_ustring.obj
	$(asmf) ./lib/asm/x86/toki/toki.asm -o ~/_obj/libmx86/_u_toki.obj
	$(CC32) ${libcdir}/driver/i8259A.c -o ~/_obj/libmx86/_u_i8259A.obj
	$(CC32) ${libcdir}/processor/x86/delay.c -DADDR_CountSeconds=0x524 -o ~/_obj/libmx86/_u_delay.obj
	ar -rcs /mnt/hgfs/_bin/libmx86.a ~/_obj/libmx86/_u_*.obj

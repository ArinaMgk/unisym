# ASCII Makefile TAB4 CRLF
# Docutitle: Build Script for Bootloader
# Codifiers: @dosconio: 20241101
# Attribute: 
# Copyright: UNISYM, under Apache License 2.0
# Descriptn: Normal for x86 system 512-bytes

AS=aasm
ASM_FLAG= -I$(uincpath)/Kasha/n_ -I$(uincpath)/naasm/n_ # aasm will auto-inc these in the future
NAM=$(shell echo $(basename $(notdir $<)) | cut -c5- | tr '[:lower:]' '[:upper:]')
DST=boot-x86-$(NAM).bin
TIT=Bootstrap $(DST)
DOING=$(AS) $< $(ASM_FLAG) -o $(ubinpath)/$(DST)

.PHONY: all \
	usual-x86 \
	bootfka bootfx

usual-x86: bootfka bootfx

all: usual-x86

# ---- ---- ---- ---- x86 ---- ---- ---- ---- 

bootfka: ./demo/osdev/bootstrap/bootfka.asm
	@echo AS $(TIT)
	@$(DOING) || echo x_x when $(DOING)

bootfx: ./demo/osdev/bootstrap/bootfx.asm
	@echo AS $(TIT)
	@$(DOING) || echo x_x when $(DOING)

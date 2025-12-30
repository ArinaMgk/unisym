# ASCII Makefile TAB4 CRLF
# Docutitle: Build Script for Bootloader
# Codifiers: @dosconio: 20241101
# Attribute: 
# Copyright: UNISYM, under Apache License 2.0
# Descriptn: Normal for x86 system 512-bytes

AS=aasm
# NAM=$(shell echo $(basename $(notdir $<)) | cut -c5- | tr '[:lower:]' '[:upper:]')
# DST=boot-$(NAM).bin
# TIT=Bootstrap $(DST)
# DOING=$(AS) $< $(ASM_FLAG) -o $(ubinpath)/$(DST) -D_FLOPPY

SRC=$(ulibpath)/../demo/osdev/bootstrap/bootx86.asm

.PHONY: all usual-x86

#{TODO} Floppy and Disk Unified
usual-x86:
	@echo AS Boot x86 Floppy-only
	@$(AS) $(SRC) -o $(ubinpath)/boot-x86.bin -D_FLOPPY

all: usual-x86

#!/bin/bash
# ASCII Makefile TAB4 LF
# Attribute: 
# LastCheck: 
# AllAuthor: @dosconio
# ModuTitle: Makefile for Magice
# Copyright: ArinaMgk UniSym, Apache License Version 2.0

CFLG = -std=c99 -fno-common

.PHONY: all dbg

all: magice.c
	gcc $(CFLG) -o /mnt/hgfs/_bin/ELF64/mgc $<
dbg:
	@./chkmgc.sh

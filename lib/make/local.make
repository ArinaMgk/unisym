# ASCII Makefile TAB4 CRLF
# Docutitle: Build Script for Locale Data
# Codifiers: @dosconio: 20241101
# Attribute: 
# Copyright: UNISYM, under Apache License 2.0
# Descriptn: Normal for x86 system 512-bytes

all:
	@echo MK Local Data
	@$(CC) $(ulibpath)/local/toxxxer.make.c -o $(uobjpath)/toxxxer.make.exe && $(uobjpath)/toxxxer.make.exe


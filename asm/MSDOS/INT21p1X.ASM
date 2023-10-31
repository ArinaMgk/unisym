; ASCII [MASM] for MSDOS/WIN16 (C)(ARINAMGK)UNISYM&DOSCONIO
; INT 21 Partial AH from 10 to 1F

; AH = 10h DOS - CLOSE DISK FILE - DiskFileClose
; AL[00:Fin,FF:NotFound] <<< DskClose <<< FCB=DS:DX
DskClose macro regpara:=<dx>
	push dx
	mov dx, regpara
	mov ah, 010h
	int 21h
	pop dx
endm

; AH = 11h DOS - SEARCH FIRST USING FCB - FCBIndexUsing
; AL[00:Found,FF:NotFound] <<< FCBIndexUsing <<< FCB=DS:DX
; - If file found, FCB is created at DTA address and set up to OPEN or DELETE it.
FCBIndexUsing macro regpara:=<dx>
	push dx
	mov dx, regpara
	mov ah, 011h
	int 21h
	pop dx
endm

; AH = 12h DOS - SEARCH NEXT USING FCB - FCBIndexNext
; AL[00:Found,FF:NotFound] <<< FCBIndexNext <<< FCB=DS:DX
; - If file found, FCB is created at DTA address and set up to OPEN or DELETE it.
FCBIndexNext macro regpara:=<dx>
	push dx
	mov dx, regpara
	mov ah, 012h
	int 21h
	pop dx
endm

; AH = 13h DOS - DELETE FILE via FCB - DskFileDelete
; AL[00:Found,FF:NotFound] <<< DskFileDelete <<< FCB=DS:DX
; - FCB with filename field filled with template for deletion ('?' wildcard allowed)
DskFileDelete macro regpara:=<dx>
	push dx
	mov dx, regpara
	mov ah, 013h
	int 21h
	pop dx
endm

; AH = 14h DOS - SEQUENTIAL DISK FILE READ - DskFileReadSequntial
; AL[00:Fin,01:EOF,02:DTATooSmall,03:Partial Record, EOF] <<< DskFileReadSequntial <<< FCB=DS:DX
DskFileReadSequntial macro regpara:=<dx>
	push dx
	mov dx, regpara
	mov ah, 014h
	int 21h
	pop dx
endm

; AH = 15h DOS - SEQUENTIAL DISK RECORD WRITE - DskFileWriteSequntial
; AL[00:Fin,01:DiskFull,02:DTATooSmall] <<< DskFileWriteSequntial <<< FCB=DS:DX
DskFileWriteSequntial macro regpara:=<dx>
	push dx
	mov dx, regpara
	mov ah, 015h
	int 21h
	pop dx
endm

; AH = 16h DOS - CREATE A DISK FILE - DskFileCreate
; AL[00:Fin,FF:DirFull] <<< DskFileCreate <<< FCB=DS:DX
; - if file already exists, it is truncated to zero length
DskFileCreate macro regpara:=<dx>
	push dx
	mov dx, regpara
	mov ah, 016h
	int 21h
	pop dx
endm

; AH = 17H DOS - RENAME FILE via FCB - DskFileRename
; AL[00:Fin,FF:NotFound] <<< DskFileRename <<< FCB=DS:DX
; - FCB contains new name starting at byte 17h.
DskFileRename macro regpara:=<dx>
	push dx
	mov dx, regpara
	mov ah, 017h
	int 21h
	pop dx
endm

; AH = 18h DOS Internal - UNUSED
; AL=0 <<<

; AH = 19h DOS - GET DEFAULT DISK NUMBER - DskNumberGet
; AL[CrtDrvNo(Letter-'A')] <<< DskNumberGet <<< void
DskNumberGet macro
	mov ah, 019h
	int 21h
endm

; AH = 1Ah DOS - SET DISK TRANSFER AREA ADDRESS - DskDTASet
; void <<< DskDTASet <<< DTA=DS:DX
DskDTASet macro regpara:=<dx>
	push ax
	mov dx, regpara
	mov ah, 01Ah
	int 21h
	pop ax
endm

; AH = 1Bh DOS - ALLOCATION TABLE INFORMATION - DskATIGet
; [DS:pres BX]=(FAT ID byte for default drive) <<< DskATIGet <<< units, sectors, bytes
DskATIGet macro units, sectors, bytes
	push ax
	push cx
	push dx
	mov ah, 01Bh
	int 21h
	mov units, dx; number of allocation units on disk
	mov sectors, al; number of sectors per allocation unit (cluster)
	mov bytes, cx; number of bytes per sector
	pop dx
	pop cx
	pop ax
endm

; AH = 1Ch DOS - ALLOCATION TABLE INFORMATION FOR SPECIFIC DEVICE - DskATIGetSpecific
; [DS:pres BX]=(FAT ID byte for default drive) <<< DskATIGet <<< units, sectors, bytes, drvno
DskATIGet macro units, sectors, bytes, drvno
	push ax
	push cx
	push dx
	mov ah, 01Bh
	int 21h
	mov units, dx; number of allocation units on disk
	mov drvno, al; Drive Number to check
	mov sectors, al; number of sectors per allocation unit (cluster)
	mov bytes, cx; number of bytes per sector
	pop dx
	pop cx
	pop ax
endm

; AH = 1Dh DOS Internal - UNUSED
; AL=0 <<<

; AH = 1Eh DOS Internal - UNUSED
; AL=0 <<<

; AH = 1Fh DOS Internal - GET DEFAULT DRIVE PARAMETER BLOCK - DskDPBGet
; AL[00:Fin,FF:Err] <<< DskDPBGet <<< DPB=BX yo [DS:BX]
; - for DOS 2.x and 3.x, this just invokes function 32h with DL = 0
DskDPBGet macro regpara:=<bx>
	push bx
	mov bx, regpara
	mov ah, 01Fh
	int 21h
	pop bx
endm

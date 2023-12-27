; ASCII [MASM] for MSDOS/WIN16 (C)(ARINAMGK)UNISYM&DOSCONIO
; INT 21 Partial AH from 20 to 2F

; AH = 20h DOS Internal - UNUSED
; AL=0 <<<

; AH = 21h DOS - RANDOM DISK RECORD READ - DskRecordReadRandom
; AL[00:Fin,01:EOF,02:DTATooSmall,03:PartAndEOF] <<< DskRecordReadRandom <<< FCB=DS:DX
DskRecordReadRandom macro regpara:=<dx>
	push dx
	mov dx, regpara
	mov ah, 021h
	int 21h
	pop dx
endm

; AH = 22h DOS - RANDOM DISK RECORD WRITE - DskRecordWriteRandom
; AL[00:Success,01:DiskFull,02:DTATooSmall] <<< DskRecordWriteRandom <<< FCB=DS:DX
DskRecordWriteRandom macro regpara:=<dx>
	push dx
	mov dx, regpara
	mov ah, 022h
	int 21h
	pop dx
endm

; AH = 23h DOS - GET FILE SIZE - DskFileSizeGet
; AL[00:Success,01:FileNotFound] <<< DskFileSizeGet <<< FCB=DS:DX
; - DS:DX = address of unopened FCB with filename and record size fields initialized
; - FCB's random-record field set to number of records (rounded up)
DskFileSizeGet macro regpara:=<dx>
	push dx
	mov dx, regpara
	mov ah, 023h
	int 21h
	pop dx
endm

; AH = 24h DOS - SET RANDOM RECORD FIELD - DskRRFSet
; ref <<< DskRRFSet <<< FCB=DS:DX
; - RRF of FCB is set to be same as Current Block and Current Record.
; - FCB must be OPEN already
DskRRFSet macro regpara:=<dx>
	push dx
	mov dx, regpara
	mov ah, 024h
	int 21h
	pop dx
endm

; AH = 25h DOS - SET INTERRUPT VECTOR - IntVector
; void <<< IntVector <<< AL=INT,DS:DX=NewVector
IntVector macro intno:=<ax>, newvec:=<dx>
	push ax
	push dx
	mov ax, intno
	mov dx, newvec
	mov ah, 025h
	int 21h
	pop dx
	pop ax
endm

;{TODO}

; AH = 26h DOS - CREATE PSP - PSPCreate
;DX = Segment number to set up PSP at
;Return: Current PSP is copied to specified segment
;Note: new PSP is updated with memory size information; INTs 22h, 23h, 24h taken from interrupt vector table

; AH = 27h DOS - RANDOM BLOCK READ
;DS:DX = address of FCB
;CX = number of records to be read
;Return: AL = status
;0 successful read
;1 end of file
;2 data transfer area too small
;3 partial record, EOF

; AH = 28h DOS - RANDOM BLOCK WRITE
;DS:DX = address of FCB
;CX = number of records to be written
;	if zero, truncate file to current random file position
;Return: AL = status
;	0 successful write
;	1 disk full
;	2 data transfer area too small

; AH = 29h DOS - PARSE FILENAME
;DS:SI -> string to parse
;ES:DI -> buffer to fill with unopened FCB
;AL = bit mask to control parsing
;0 = 0: parsing stops if file separator found
;1: leading separator ignored
;1 = 0: drive number in FCB set to default drive if not present
;in string
;1: drive number in FCB not changed
;2 = 0: filename in FCB set to blanks if no filename in string
;1: filename in FCB not changed if string does not contain
;a filename
;3 = 0: extension in FCB set to blanks if no extension in string
;1: extension left unchanged
;Return: AL = 00h: no wildcards in name or extension
;	01h: wildcards appeared
;	FFh: drive specifier invalid
;DS:SI -> first byte after parsed string
;ES:DI buffer filled with unopened FCB

; AH = 2Ah DOS - GET CURRENT DATE
;Return: DL = day
;DH = month
;CX = year
;AL = day of the week (0=Sunday, 1=Monday, etc.)

; AH = 2Bh DOS - SET CURRENT DATE
;DL = day
;DH = month
;CX = year
;Return: AL = 00h if no error
;= FFh if bad value sent to routine
;Note: DOS 3.3 also sets CMOS clock

; AH = 2Bh DESQview - INSTALLATION CHECK
;AL = subfunction (DV v2.00+)
;01h get version
;Return: BX = version (BH = major, BL = minor)
;Note: early copies of v2.00 return 0002h
;02h get shadow buffer info, and start shadowing
;Return: BH = rows in shadow buffer
;BL = columns in shadow buffer
;DX = segment of shadow buffer
;04h get shadow buffer info
;Return: BH = rows in shadow buffer
;BL = columns in shadow buffer
;DX = segment of shadow buffer
;05h stop shadowing
;CX = 4445h ('DE')
;DX = 5351h ('SQ')
;Return: AL = FFh if DESQview not installed
;Note: in DESQview v1.x, there were no subfunctions; this call only identified
;whether or not DESQview was loaded

; AH = 2Ch DOS - GET CURRENT TIME
;Return: CH = hours
;CL = minutes
;DH = seconds
;DL = hundredths of seconds
;Note: time is updated approximately every 5/100 second

; AH = 2Dh DOS - SET CURRENT TIME
;CH = hours
;CL = minutes
;DH = seconds
;DL = hundredths of seconds
;Return: AL = 00h if no error
;= FFh if bad value sent to routine
;Note: DOS 3.3 also sets CMOS clock

; AH = 2Eh DOS - SET VERIFY FLAG
;DL = 0
;AL = 1 VERIFY on 0 VERIFY off

; AH = 2Fh DOS 2+ - GET DISK TRANSFER AREA ADDRESS
;AH = 2Fh
;Return: ES:BX = address of DTA


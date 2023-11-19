; ASCII [MASM] for MSDOS/WIN16 (C)(ARINAMGK)UNISYM&DOSCONIO
; INT 21 Partial AH from 30 to 3F

;{TODO}

; - DOS 2+ - GET DOS VERSION
;AH = 30h
;Return: AL = Major Version number (0 for DOS 1.x)
;AH = Minor Version number
;BH = OEM number
;00h IBM
;16h DEC
;BL:CX = 24-bit user number

; AH = 31h DOS 2+ - TERMINATE BUT STAY RESIDENT
;AL = exit code
;DX = program size, in paragraphs

; AH = 32h DOS Internal - GET DRIVE PARAMETER BLOCK
;DL = drive number
;0 = default, 1 = A, etc.
;Return: AL = 0FFh if invalid drive number, else
;DS:BX -> drive parameter block.
;Structure of DOS Drive Parameter Block:
;	Offset Size Description
;	00h BYTE drive number (0 = A, etc.)
;	01h BYTE unit number within device driver
;	02h WORD number of bytes per sector
;	04h BYTE largest sector number in cluster (one less than sect/clust)
;	05h BYTE log base two of the cluster size
;	06h WORD number of reserved (boot) sectors
;	08h BYTE number of copies of the FAT
;	09h WORD number of root directory entries
;	0Bh WORD first data sector on medium
;	0Dh WORD largest possible cluster number (one more than # data clust)
;	0Fh BYTE number of sectors in one FAT copy
;	10h WORD first sector of root directory
;	12h DWORD address of device driver for this drive
;	16h BYTE media descriptor byte for medium
;	17h BYTE FFh indicates block must be rebuilt (DOS 3.x) 00h indicates block accessed
;	18h DWORD address of next device block, offset = FFFFh indicates last
;	---DOS 2.x only--
;	1Ch WORD starting cluster of current directory (0 = root directory)
;	1Eh 64 BYTEs ASCIZ current directory path string
;	---DOS 3.x--
;	; this was always:
;	1Ch WORD = 0 probably unused, values left from before
;	1Eh WORD = 0FFFFh block was built

; AH = 33h DOS 2+ - EXTENDED CONTROL-BREAK CHECKING
;AL = subfunction
;00h get state
;01h set state
;	DL = 0 for OFF or 1 for ON
;02h internal, called by PRINT.COM (DOS 3.1)
;05h internal, return boot drive in DL (1=A:) (not in DOS 3.1)
;Return: DL = current BREAK setting if AL = 00h
;0 BREAK=OFF
;1 BREAK=ON
;AL = FFh if error

; AH = 34h DOS Internal - RETURN CritSectFlag POINTER
;Return: ES:BX -> 1-byte DOS "Critical Section Flag", also known as InDOS flag
;Notes: when the critical section flag is nonzero, code within DOS is being executed.  It is safe to enter DOS when both the critical section flag and the critical error flag are zero. The critical error flag is the byte after the critical section flag in DOS 2.x, and the byte BEFORE the critical section flag in DOS 3.x (except COMPAQ DOS 3.0, where the critical error lag is located 1AAh bytes BEFORE the critical section flag)

; AH = 35h DOS 2+ - GET INTERRUPT VECTOR
;AL = interrupt number
;Return: ES:BX = value of interrupt vector

; AH = 36h DOS 2+ - GET DISK SPACE
;DL = drive code (0 = default, 1 = A, 2 = B, etc.)
;Return: AX = number of sectors per cluster
;	or 0FFFFh if invalid drive
;BX = number of available clusters
;CX = bytes per sector
;DX = total clusters
;Note: multiply AX * CX * BX for free space on disk
;	multiply AX * CX * DX for total disk space

; AH = 37h DOS Internal - SWITCHAR/AVAILDEV
;AL = subfunction
;0 Read switch character (returns current character in DL)
;1 Set switch character (specify new character in DL)
;2 (DOS 2.x only) Read device availability (as set by function AL=3)
;3 (DOS 2.x only) Set device availability, where:
;	DL = 0 means /DEV/ must preceed device names
;	DL <> 0 means /DEV/ need not preceed device names
;Return: DL = Switch character (if AL=0 or 1)
;	Device availability flag (if AL=2 or 3)
;AL=0FFh means the value in AL was not in the range 0-3.

; AH = 38h DOS 2+ - GET COUNTRY-DEPENDENT INFORMATION--DOS 2.x-
;AL = 0
;get current-country info
;DS:DX = segment:offset of buffer for returned info
;Return: BX = country code
;buffer at DS:DX filled as follows:
;bytes 0-1 = date format
;byte 2    = currency symbol
;byte 3    = 00h
;0 = USA mm dd yy
;1 = Europe dd mm yy
;2 = Japan yy mm dd
;byte 4    = thousands separator char
;byte 5    = 00h
;byte 6    = decimal separator char
;byte 7    = 00h
;bytes 8-1Fh reserved--DOS 3.x-
;AL = 0 for current country
;AL = 01h thru 0FEh for specific country with code <255
;AL = 0FFh for specific country with code >= 255
;BX = 16-bit country code
;DS:DX = segment:offset of buffer for returned info
;DX = 0FFFFh if setting country code, rather than getting info
;Return: (if DX <> 0FFFFh)
;BX = country code
;DS:DX filled in:
;	bytes 0-1 = date format (see above)
;	bytes 2-6 = currency symbol string, ASCIZ
;	byte 7 = thousands seaprator char
;	byte 8 = 00h
;	byte 9 = decimal separator char
;	byte 0Ah
;= 00h
;	byte 0Bh
;	byte 0Ch
;	byte 0Dh
;	byte 0Eh
;= date separator char
;= 00h
;= time separator char
;= 00h
;byte 0Fh
;	byte 10h
;	byte 11h
;	bytes 12h-15h
;	byte 16h
;	byte 17h
;= currency format
;bit 2 = set if currency symbol replaces decimal pt
;bit 1 = number of spaces between value and curr sym
;bit 0 = 0 if currency symbol precedes value 1 if currency symbol follows value
;= number of digits after decimal in currency
;= time format
;bit 0 = 0 if 12-hour clock
;			1 if 24-hour clock
;= address of case map routine (FAR CALL, AL = char)
;= data-list separator char
;= 00h
;	bytes 18h-21h reserved
;	If error:
;CF set
;AX = error code

; AH = 39h DOS 2+ - CREATE A SUBDIRECTORY (MKDIR)
;DS:DX = address of ASCIZ pathname
;Return: CF set on error
;AX = Error Code

; AH = 3Ah DOS 2+ - REMOVE A DIRECTORY ENTRY (RMDIR)
;DS:DX = address of ASCIZ pathname
;Return: CF set on error
;AX = Error Code

; AH = 3Bh DOS 2+ - CHANGE THE CURRENT DIRECTORY (CHDIR)
;DS:DX = address of ASCIZ directory name
;Return: CF set on error
;AX = Error Code

; AH = 3Ch DOS 2+ - CREATE A FILE WITH HANDLE (CREAT) - DskFileCreate
; CF(Err),AX(Errno/Handle) <<< DskFileCreate <<< DS:DX(ASCIZAddr),CX(Attributes)
; Attributes for file
;	bit 0: read-only
;	1: hidden
;	2: system
;	3: volume label
;	4: reserved, must be zero (directory)
;	5: archive bit
;	7: if set, file is shareable under Novell NetWare
DskFileCreate macro Addr:=<DX>, Attr
	push cx
	push dx
	mov dx, Addr
	mov cx, Attr
	mov ah, 03Ch
	int 21h
	pop dx
	pop cx
endm

; AH = 3Dh DOS 2+ - OPEN DISK FILE WITH HANDLE - DskFileOpen
; CF(Err),AX(Errno/Handle) <<< DskFileWrite <<< DS:DX(ASCIZAddr),AL(AccessMode)
; AL:
;	0 = Read Only
;	1 = Write Only
;	2 = Read/Write
;	AL bits 7~3 = file-sharing modes (DOS 3.x)
;	bit 7    = inheritance flag, set for no inheritance
;		bits 6~4 = sharing mode
;			000 compatibility mode
;			001 exclusive (deny all)
;			010 write access denied (deny write)
;			011 read access denied (deny read)
;			100 full access permitted (deny none)
;		bit 3    = reserved, should be zero
DskFileOpen macro Addr:=<DX>, AccMode
	push dx
	mov dx, Addr
	mov al, AccMode
	mov ah, 03Dh
	int 21h
	pop dx
endm

; AH = 3Eh DOS 2+ - CLOSE A FILE WITH HANDLE - DskFileClose
; CF(Err),AX(Errno) <<< DskFileClose <<< BX(Handle)
DskFileClose macro Handle
	push bx
	mov bx, Handle
	mov ah, 03Eh
	int 21h
	pop bx
endm

; AH = 3Fh DOS 2+ - READ FROM FILE WITH HANDLE - DskFileRead
; CF(Err),AX(Errno/BytesRead) <<< DskFileRead <<< DS:DX(Buffer),CX(Bytes),BX(Handle)
DskFileRead macro Buffer, Bytes, Handle
	push bx
	push cx
	push dx
	mov dx, Buffer
	mov cx, Bytes
	mov bx, Handle
	mov ah, 03Fh
	int 21h
	pop dx
	pop cx
	pop bx
endm

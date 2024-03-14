; ASCII [MASM] for MSDOS/WIN16 (C)(ARINAMGK)UNISYM&DOSCONIO
; INT 21 Partial AH from 40 to 4F

; AH = 40h DOS 2+ - WRITE TO FILE WITH HANDLE - DskFileWrite
; CF(Err),AX(Errno/BytofWritten) <<< DskFileWrite <<< DS:DX(Addr),CX(Count),BX(Handle)
; - if CX is zero, no data is written, and the file is truncated or extended to the current position
DskFileWrite macro Addr:=<DX>,Count:=<CX>,Handle:=<BX>
	push cx
	push dx
	push bx
	mov ah, 040h
	mov bx, Handle
	mov cx, Count
	mov dx, Addr
	int 21h
	pop bx
	pop dx
	pop cx
endm

;{TODO}

; AH = 41h DOS 2+ - DELETE A FILE (UNLINK)
;DS:DX -> ASCIZ name of file to delete
;Return: CF set on error
;AX = error code

; AH = 42h DOS 2+ - MOVE FILE READ/WRITE POINTER (LSEEK)
;AL = method value
;0 = offset from beginning of file
;1 = offset from present location
;2 = offset from end of file
;BX = file handle
;CX:DX = offset in bytes
;Return: CF set on error
;AX = error code
;CF clear if successful
;DX:AX = new offset

; AH = 43h DOS 2+ - GET/PUT FILE ATTRIBUTES (CHMOD)
;AL =
;0 get file attributes
;1 put file attributes
;	CX = file attribute bits
;0 = read only
;1 = hidden file
;2 = system file
;3 = volume label
;4 = subdirectory
;5 = written since backup
;8 = shareable (Novell NetWare)
;DS:DX -> ASCIZ file name
;Return: CF set on error
;AX = error code
;CX = file attributes on get

; AX = 4400h DOS 2+ - IOCTL - GET DEVICE INFORMATION
;BX = file or device handle
;Return: CF set on error
;AX = error code
;CF clear if successful
;DX = device info
;If bit 7 set: (character device)
;	bit 0: console input device
;1: console output device
;2: NUL device
;3: CLOCK$ device
;4: device is special
;5: binary (raw) mode
;6: Not EOF
;12: network device (DOS 3.x)
;14: can process IOCTL control strings (func 2-5)
;If bit 7 clear: (file)
;	bits 0-5 are block device number
;6: file has not been written
;12: Network device (DOS 3.x)
;15: file is remote (DOS 3.x)

; AX = 4401h DOS 2+ - IOCTL - SET DEVICE INFORMATION
;BX = device handle
;DH = 0
;DL = device information to set (bits 0-7 from function 0)
;Return: CF set on error
;AX = error code

; AX = 4402h DOS 2+ - IOCTL - READ CHARACTER DEVICE CONTROL STRING
;BX = device handle
;CX = number of bytes to read
;DS:DX -> buffer
;Return: CF set on error
;AX = error code
;CF clear if successful
;AX = number of bytes read

; - AX = 4403h DOS 2+ - IOCTL - WRITE CHARACTER DEVICE CONTROL STRING
;BX = device handle
;CX = number of bytes to write
;DS:DX -> buffer
;Return: CF set on error
;AX = error code
;CF clear if successful
;AX = number of bytes written

; - AX = 4404h DOS 2+ - IOCTL - READ BLOCK DEVICE CONTROL STRING
;BL = drive number (0=default)
;CX = number of bytes to read
;DS:DX -> buffer
;Return: CF set on error
;AX = error code
;CF clear if successful
;AX = number of bytes read

; - AX = 4405h DOS 2+ - IOCTL - WRITE BLOCK DEVICE CONTROL STRING
;BL = drive number (0=default)
;CX = number of bytes to write
;DS:DX -> buffer
;Return: CF set on error
;AX = error code
;CF clear if successful
;AX = number of bytes written

; - AX = 4406h DOS 2+ - IOCTL - GET INPUT STATUS
;BX = file or device handle
;Return: AL = FFh device ready
;	00h device not ready

; - AX = 4407h DOS 2+ - IOCTL - GET OUTPUT STATUS
;BX = file or device handle
;Return: AL = FFh device ready
;	00h device not ready
;Note: for DOS 2.x, files are always ready for output

; - AX = 4408h DOS 3.x - IOCTL - BLOCK DEVICE CHANGEABLE
;BL = drive number (0=default)
;Return: AX = 00h removable
;	01h fixed
;	0Fh invalid drive

; - AX = 4409h DOS 3.x - IOCTL - BLOCK DEVICE LOCAL
;BL = drive number (0=default)
;Return: DX = attribute word, bit 12 set if device is remote

; - AX = 440Ah DOS 3.x - IOCTL - HANDLE LOCAL
;BX = file handle
;Return: DX = attribute word, bit 15 set if file is remote
;Note:   if file is remote, Novell Advanced NetWare 2.0 returns the number of the file server on which the handle is located in CX

; - AX = 440Bh DOS 3.x - IOCTL - SET SHARING RETRY COUNT
;CX = delay (default 1)
;DX = retry count (default 3)
;Return: CF set on error
;AX = error code

; - AX = 440Ch DOS 3.2 - IOCTL - GENERIC
;BX = device handle
;CH = category code
;	00h unknown (DOS 3.3)
;	01h COMn: (DOS 3.3)
;	03h CON (DOS 3.3)
;	05h LPTn:
;CL = function
;	45h set iteration count
;	4Ah select code page
;	4Ch start code-page preparation
;	4Dh end code-page preparation
;	65h get iteration count
;	6Ah query selected code page
;	6Bh query prepare list
;DS:DX -> parameter block
;	for CL=45h WORD  iteration count
;	for CL=4Ah,4Dh,6Ah WORD  length of data
;		WORD  code page ID
;	for CL=4Ch WORD  flags
;		WORD  length of remainder of parameter block
;		WORD  number of code pages following
;		N WORDs code page 1,...,N
;	for CL=6Bh WORD  length of following data
;		WORD  number of hardware code pages
;		N WORDs hardware code pages 1,...,N
;		WORD  number of prepared code pages
;		N WORDs prepared code pages 1,...,N
;Return: CF set on error
;AX = error code

; - AX = 440Dh DOS 3.2 - IOCTL - BLOCK DEVICE REQUEST
;BL = drive number (0=default)
;CH = category code
;	08h disk drive
;CL = function
;	40h set device parameters
;	41h write logical device track
;	42h format and verify logical device track
;	60h get device parameters
;	61h read logical device track
;	62h verify logical device track
;DS:DX -> parameter block
;for functions 40h, 60h
;	BYTE  special functions
;		bit 0 set if function to use current BPB, clear if Device BIOS
;			Parameter Block field contains new default BPB
;		bit 1 set if function to use track layout fields only must be clear if CL=60h
;		bit 2 set if all sectors in track same size (should be set)
;		bits 3-7 reserved
;	BYTE  device type
;		00h  320K/360K disk
;		01h  1.2M disk
;		02h  720K disk
;		03h  single-density 8-inch disk
;		04h  double-density 8-inch disk
;		05h  fixed disk
;		06h  tape drive
;		07h  other type of block device
;	WORD  device attributes
;		bit 0 set if nonremovable medium
;		bit 1 set if door lock supported
;		bits 2-15 reserved
;	WORD  number of cylinders
;	BYTE  media type
;		00h 1.2M disk (default)
;		01h 320K/360K disk
;	31 BYTEs device BPB (see function 53h)
;		WORD  number of sectors per track (start of track layout field)
;		N word pairs: number,size of each sector in track
;	for functions 41h, 61h
;		BYTE  reserved, must be zero
;		WORD  number of disk head
;		WORD  number of disk cylinder
;		WORD  number of first sector to read/write
;		WORD  number of sectors
;		DWORD transfer address
;	for functions 42h, 62h
;		BYTE  reserved, must be zero
;		WORD  number of disk head
;		WORD  number of disk cylinder
;Return: CF set on error
;AX = error code

; - AX = 440Eh DOS 3.2 - IOCTL - GET LOGICAL DRIVE MAP
;BL = drive number (0=default)
;Return:
;CF set on error
;	AX = error code
;CF clear if successful
;	AL = 0 block device has only one logical drive assigned
;	1..26 the last letter used to reference the drive (1=A:,etc)

; - AX = 440Fh DOS 3.2 - IOCTL - SET LOGICAL DRIVE MAP
;BL = physical drive number (0=default)
;Return: CF set on error
;AX = error code
;Note: maps logical drives to physical drives, similar to DOS's treatment of a single physical floppy drive as both A: and B:

; AH = 45h DOS 2+ - CREATE DUPLICATE HANDLE (DUP)
;BX = file handle to duplicate
;Return:
;CF set on error
;	AX = error code
;CF clear if successful
;	AX = new file handle

; AH = 46h DOS 2+ - FORCE DUPLICATE HANDLE (FORCDUP,DUP2)
;BX = Existing file handle
;CX = new file handle
;Return: CF set on error
;AX = error code

; AH = 47h DOS 2+ - GET CURRENT DIRECTORY
;DL = drive (0=default, 1=A, etc.)
;DS:DI points to 64-byte buffer area
;Return: CF set on error
;AX = error code
;Note: the returned path does not include the initial backslash

; AH = 48h DOS 2+ - ALLOCATE MEMORY
;BX = number of 16-byte paragraphs desired
;Return:
;CF set on error
;	AX = error code
;	BX = maximum available
;CF clear if successful
;	AX = segment of allocated memory block

; AH = 49h DOS 2+ - FREE MEMORY
;ES = Segment address of area to be freed
;Return: 
;	CF set on error
;		AX = error code

; AH = 4Ah DOS 2+ - ADJUST MEMORY BLOCK SIZE (SETBLOCK)
;ES = Segment address of block to change
;BX = New size in paragraphs
;Return: CF set on error
;AX = error code
;BX = maximum size possible for the block

; AH = 4Bh DOS 2+ - LOAD OR EXECUTE (EXEC)
;AL = subfunction
;	0 = load and execute program
;	1 = load but do not execute (internal, DOS 3.x & DESQview only)
;	2 = load but do not execute (internal, DOS 2.x only)
;	3 = load overlay; do not create PSP
;DS:DX = filename
;ES:BX = parameter block
;	AL =
;	0: WORD segment of environment (0 = use current)
;		DWORD -> command line
;		DWORD -> FCB 1
;		DWORD -> FCB 2
;	1: WORD segment of environment (0 = use current)
;		DWORD -> command line
;		DWORD -> FCB 1
;		DWORD -> FCB 2
;		DWORD will hold SS:SP on return
;		DWORD will hold program entry point (CS:IP) on return
;	2: WORD segment of environment (0 = use current)
;		DWORD -> command line
;		DWORD -> FCB 1
;		DWORD -> FCB 2
;	3: WORD segment load address
;		WORD segment relocation factor
;Return:
;	CF set on error
;AX = error code
;CF clear if successful
;	if function 1, process ID set to new program's PSP; get with
;	function 62h
;	if function 2, new program's initial stack and entry point
;	returned in registers
;Note: DOS 2.x destroys all registers, including SS:SP
;Structure of .EXE file header:
;	WORD  0x4d, 0x5a signature (sometimes 5Ah, 4Dh)
;	WORD  image size remainder (program size mod 512)
;	WORD  file size in pages (program size div 512)
;	WORD  number of relocation items
;	WORD  header size in paragraphs
;	WORD  minimum extra paragraphs needed
;	WORD  maximum extra paragraphs needed
;	WORD  stack segment
;	WORD  stack offset
;	WORD  word checksum of entire file
;	DWORD initial CS:IP
;	WORD  offset of relocation table
;	WORD  overlay number

; AH = 4Ch DOS 2+ - QUIT WITH EXIT CODE (EXIT)
; void <<< SysQuit <<< AL:excode=0
;Return: never returns
%imacro SysQuit excode:=<0>
	mov ax, 4C00h | excode
	int 21h
%endmacro

; AH = 4Dh DOS 2+ - GET EXIT CODE OF SUBPROGRAM (WAIT)
;Return: AL = exit code of subprogram (functions 31h or 4Ch)
;AH = circumstance which caused termination
;	0 = Terminate/abort
;	1 = Control-C
;	2 = Hard error
;	3 = Terminate and stay resident

; AH = 4Eh DOS 2+ - FIND FIRST ASCIZ (FIND FIRST)
;CX = search attributes
;DS:DX -> ASCIZ filename
;Return: CF set on error
;AX = error code
;[DTA] = data block
;	undocumented fields
;PC-DOS 3.10
;	byte  00h:     drive letter
;	bytes 01h-0Bh: search template
;	byte  0Ch:     search attributes
;DOS 2.x (and DOS 3.x except 3.1???)
;	byte  00h:     search attributes
;	byte  01h:     drive letter
;	bytes 02h-0Ch: search template
;bytes 0Dh-0Eh: entry count within directory
;bytes 0Fh-12h: reserved
;bytes 13h-14h: cluster number of parent directory
;	byte  15h:     attribute of file found
;	bytes 16h-17h: file time
;	bytes 18h-19h: file date
;	bytes 1Ah-1Dh: file size
;	bytes 1Eh-3Ah: ASCIZ filename+extension

; AH = 4Fh DOS 2+ - FIND NEXT ASCIZ (FIND NEXT)
;[DTA] = data block from last AH = 4Eh/4Fh call
;Return: CF set on error
;AX = error code
;[DTA] = data block, see AH = 4Eh above

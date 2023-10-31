; ASCII [MASM] for MSDOS/WIN16 (C)(ARINAMGK)UNISYM&DOSCONIO
; INT 10 Partial AH from 10 to 1F


; - AH = 10h VIDEO - SET PALETTE REGISTERS (Jr, PS, TANDY 1000, EGA, VGA)
;{TODO}
;AL = subfunction
;	00h set palette register
;		BL = palette register to set
;		BH = color value to store
;		(on MCGA, only BX = 0712h is supported)
;	01h set border color register
;		BH = color value to store
;	02h set all palette registers and overscan
;		ES:DX -> 17-byte list
;		bytes 0-15 = values for palette regs. 0-15 byte 16 = value for border color register

; - AX = 1003h VIDEO - TOGGLE INTENSITY/BLINKING BIT (Jr, PS, TANDY 1000, EGA, VGA)
;{TODO}
;BL = 00h enable intensity
;= 01h enable blink

; - AH = 10h VIDEO - GET PALETTE REGISTERS (VGA)
;{TODO}
;AL = subfunction
;07h read individual palette register
;	BL = palette register number
;	Return: BH = palette register value
;08h read overscan (border color) register
;	Return: BH = value
;09h read all palette registers and overscan register
;	ES:DX = buffer address (17 bytes)

; - AH = 10h VIDEO - GET/SET DAC REGISTERS (EGA, VGA/MCGA)
;{TODO}
;AL = subfunction
;	10h set individual DAC register
;		BX = register number
;		CH = new value for green (0-63)
;		CL = new value for blue (0-63)
;		DH = new value for red (0-63)
;	12h set block of DAC registers
;		BX = starting color register
;		CX = number of registers to set
;		ES:DX = Table of 3*CX bytes where each 3 byte
;		group represents one byte each of red, green and blue (0-63)
;	13h select video DAC color page (VGA only)
;		BL = 00h Select paging mode
;		BH = 00h Select 4 blocks of 64
;		BH = 01h Select 16 blocks of 16
;		BL = 01h Select Page
;		BH = page number (00h to 03h) or (00h to 0Fh)
;		(not valid in mode 13h)
;	15h read individual DAC register
;		BL = palette register number
;		Return: DH = red value
;		CH = green value
;		CL = blue value
;	17h read block of DAC registers
;		BX = starting palette register
;		CX = number of palette registers to read
;		ES:DX = buffer (3 * CX bytes in size)
;		Return: CX number of red, green and blue triples in buffer
;	18h *UNDOCUMENTED* set PEL mask
;		BL = new PEL value
;	19h *UNDOCUMENTED* read PEL mask
;		BL = value read
;	1Ah read video DAC color-page state (VGA only)
;		Return: BL = paging mode
;		0 four pages of 64
;		1 sixteen pages of 16
;		BH = current page
;	1Bh perform gray-scale summing
;		BX = starting palette register
;		CX = number of registers to convert

; - AH = 11h VIDEO - TEXT-MODE CHARACTER GENERATOR FUNCTIONS (PS, EGA, VGA)
;{TODO}
;The following functions will cause a mode set, completely resetting the video environment, but without clearing the video buffer
;AL = 00h, 10h: load user-specified patterns
;	ES:BP -> user table
;	CX= count of patterns to store
;	DX= character offset into map 2 block
;	BL= block to load in map 2
;	BH= number of bytes per character pattern
;AL = 01h, 11h: load ROM monochrome patterns (8 by 14)
;	BL= block to load
;AL = 02h, 12h: load ROM 8 by 8 double-dot patterns
;	BL= block to load
;AL = 03h: set block specifier
;	BL= block specifier
;	(EGA/MCGA) bits 0,1 = block selected by chars with attribute bit 3 = 0
;		bits 2,3 = block selected by chars with attribute bit 3 = 1
;	(VGA) bits 0,1,4 = block selected by attribute bit 3 = 0
;	bits 2,3,5 = block selected by attribute bit 3 = 1
;AL = 04h, 14h: load ROM 8x16 character set (VGA)
;	The routines called with AL=1xh are designed to be called only immediately after a mode set and are similar to the routines
;	called with AL=0xh, except that:
;	Page 0 must be active.
;	Bytes/character is recalculated.
;	Max character rows is recalculated.
;	CRT buffer length is recalculated.
;	CRTC registers are reprogrammed as follows:
;		R09 = bytes/char-1 ; max scan line (mode 7 only)
;		R0A = bytes/char-2 ; cursor start
;		R0B = 0 ; cursor end
;		R12 = ((rows+1)*(bytes/char))-1 ; vertical display end
;		R14 = bytes/char ; underline loc
;		(*** BUG: should be 1 less ***)

; - AH = 11h VIDEO - GRAPHICS-MODE CHARACTER GENERATOR FUNCTIONS (PS, EGA, VGA)
;{TODO}
;AL = 20h: set user 8 by 8 graphics characters (INT 1Fh)
;	ES:BP -> user table
;AL = 21h: set user graphics characters
;	ES:BP -> user table
;	CX= bytes per character
;	BL= row specifier
;	0: user set
;	DL = number of rows
;		1: 14 rows
;		2: 25 rows
;		3: 43 rows
;AL = 22h: ROM 8 by 14 set
;	BL = row specifier
;AL = 23h: ROM 8 by 8 double dot
;	BL = row specifier
;AL = 24h: load 8x16 graphics characters (VGA/MCGA)
;	BL = row specifier
;Note: these functions are meant to be called only after a mode set

; - AX = 1103h VIDEO - GET FONT INFORMATION (EGA, MCGA, VGA)
;{TODO}
;BH = pointer specifier
;	0: INT 1Fh pointer
;	1: INT 44h pointer
;	2: ROM 8 by 14 character font pointer
;	3: ROM 8 by 8 double dot font pointer
;	4: ROM 8 by 8 DD font (top half)
;	5: ROM alpha alternate (9 by 14) pointer
;Return: ES:BP = specified pointer
;	CX    = bytes/character
;	DL    = character rows on screen

; - AH = 12h VIDEO - ALTERNATE FUNCTION SELECT (PS, EGA, VGA, MCGA)
;{TODO}
;BL = 10h: return EGA information
;	Return:
;		BH = 0: color mode in effect (3Dx)
;			1: mono mode in effect (3Bx)
;		BL = 0:  64k bytes memory installed
;			1: 128k bytes memory installed
;			2: 192k bytes memory installed
;			3: 256k bytes memory installed
;		CH = feature bits
;		CL = switch settings
;BL = 20h: select alternate print screen routine
;BL = 30h: select vertical resolution for alphanumeric modes (VGA only)
;	AL =
;		00h 200 scan lines
;		01h 350 scan lines
;		02h 400 scan lines
;	Return: AL = 12h if function supported
;BL = 31h: enable/disable default palette loading (VGA/MCGA)
;	AL =
;		00h enable default palette loading
;		01h disable default palette loading
;	Return: AL = 12h if function was supported
;BL = 32h: enable/disable video addressing (VGA/MCGA)
;	AL =
;		00h enable video
;		01h disable video
;	Return: AL = 12h if function was supported
;BL = 33h: enable/disable default gray-scale summing (VGA/MCGA)
;	AL =
;		00h enable gray scale summing
;		01h disable gray scale summing
;	Return: AL = 12h if function was supported
;BL = 34h: enable/disable alphanumeric cursor emulation (VGA only)
;	AL =
;		00h enable cursor emulation
;		01h disable cursor emulation
;	Return: AL = 12h if function was supported
;BL = 35h: PS/2 display-switch interface
;	AL =
;		00h initial adapter video off
;		01h initial planar video on
;		02h switch active video off
;		03h switch inactive video on
;		80h *UNDOCUMENTED* set system board video active flag
;	ES:DX = buffer (128 byte save area if AL = 0, 2 or 3)
;	Return: AL = 12h if function was supported
;BL = 36h: video refresh control (VGA/PS)
;	AL =
;		00h enable refresh
;		01h disable refresh
;	Return: AL = 12h if function supported
;BX = 5500h ??? (used by ATI and TAXAN)
;BX = 5502h ??? (used by ATI and TAXAN)

; - AH = 13h VIDEO - WRITE STRING (AT,XT286,PS,EGA,VGA) - ConPrintFore({TEMP IDEN})
;{TODO}
;AL = mode
;	bit 0: set in order to move cursor after write
;	bit 1: set if string contains alternating characters and attributes, i.e. enable BL input
;BL = attribute if AL bit 1 clear
;BH = display page number
;DH,DL = row,column of starting cursor position
;CX = length of string
;ES:BP -> start of string
; - recognizes CR, LF, BS, and bell

; - AH = 14h VIDEO - LOAD LCD CHARACTER FONT (CONVERTIBLE)
;{TODO}
;AL = 0 load user-specified font
;	ES:DI -> character font
;	BH = number of bytes per character
;	BL =
;		0: load main font (block 0)
;		1: load alternate font (block 1)
;	CX = number of characters to store
;	DX = character offset into RAM font area
;AL = 1 load system rom default font
;	BL =
;		0: load main font (block 0)
;		1: load alternate font (block 1)
;AL = 2 set mapping of LCD high intensity attributes
;	BL =
;		0: ignore high intensity attribute
;		1: map high intensity to underscore
;		2: map high intensity to reverse video
;		3: map high intensity to selected alternate font

; - AH = 15h VIDEO - GET PHYSICAL DISPLAY PARAMETERS (CONVERTIBLE)
;{TODO}
;Return: AX = alternate display adapter type
;			0000h none
;			5140h LCD
;			5153h CGA
;			5151h mono
;	ES:DI -> parameter table
;	word:
;		0: monitor model number
;		1: vertical pixels per meter
;		2: horizontal pixels per meter
;		3: total vertical pixels
;		4: total horizontal pixels
;		5: horizontal pixel separation in micrometers
;		6: vertical pixel separation in micrometers

; - AH = 1Ah VIDEO - DISPLAY COMBINATION (PS,VGA/MCGA)
;{TODO}
;AL = 00h read display combination code
;	Return: BL = active display code
;			BH = alternate display code
;	01h set display combination code
;		BL = active display code
;		BH = alternate display code
;Return: AL = 1Ah if function was supported
;Display combination codes:
;	00h no display
;	01h monochrome adapter w/ monochrome display
;	02h CGA w/ color display
;	03h reserved
;	04h EGA w/ color display
;	05h EGA w/ monochrome display
;	06h PGA w/ color display
;	07h VGA w/ monochrome analog display
;	08h VGA w/ color analog display
;	09h reserved
;	0Ah MCGA w/ digital color display
;	0Bh MCGA w/ monochrome analog display
;	0Ch MCGA w/ color analog display
;	FFh unknown display type

; - AH = 1Bh VIDEO - FUNCTIONALITY/STATE INFORMATION (PS,VGA/MCGA)
;{TODO}
;BX = implementation type
;	= 0000h return funtionality/state information
;ES:DI -> 64 byte buffer
;Return(below): AL = 1Bh if function supported
;ES:DI buffer filled
;	00h address of static funtionality table
;	04h video mode in effect
;	05h number of columns
;	07h length of regen buffer in bytes
;	09h starting address of regen buffer
;	0Bh cursor position for page 0
;	0Dh cursor position for page 1
;	0Fh cursor position for page 2
;	11h cursor position for page 3
;	13h cursor position for page 4
;	15h cursor position for page 5
;	17h cursor position for page 6
;	19h cursor position for page 7
;	1Bh cursor type
;	1Dh active display page
;	1Eh CRTC port address
;	20h current setting of register (3?8)
;	21h current setting of register (3?9)
;	22h number of rows
;	23h bytes/character
;	25h DCC of active display
;	26h DCC of alternate display
;	27h number of colors supported in current mode
;	29h number of pages supported in current mode
;	2Ah number of scan lines active
;		(0,1,2,3) = (200,350,400,480)
;	2Bh primary character block
;	2Ch secondary character block
;	2Dh Miscellaneous flags
;		bit 0 all modes on all displays on
;			1 gray summing on
;			2 monochrome display attached
;			3 default palette loading disabled
;			4 cursor emulation enabled
;			5 0 = intensity; 1 = blinking
;			6 reserved
;			7 reserved
;	2Eh to 30h reserved
;	31h video memory available
;	00h = 64K, 01h = 128K, 02h = 192K, 03h = 256K
;	32h save pointer state flags
;		bit 0 512 character set active
;			1 dynamic save area present
;			2 alpha font override active
;			3 graphics font override active
;			4 palette override active
;			5 DCC override active
;			6 reserved
;			7 reserved
;	33h to 3Fh reserved
;State Functionality Table format (16 bytes)
;	00h modes supported #1
;		bit 0 to bit 7 = 1 modes 0,1,2,3,4,5,6 supported
;	01h modes supported #2
;		bit 0 to bit 7 = 1 modes 8,9,A,B,C,D,E,F supported
;	02h modes supported #3
;		bit 0 to bit 3 = 1 modes 10,11,12,13 supported
;		bit 4 to bit 7 reserved
;	03h to 06h reserved
;	07h scan lines supported
;		bit 0 to bit 2 = 1 if scan lines 200,350,400 supported
;	08h total number of character blocks available in text modes
;	09h maximum number of active character blocks in text modes
;	0Ah miscellaneous function flags #1
;		bit 0 all modes on all displays function supported
;			1 gray summing function supported
;			2 character font loading function supported
;			3 default palette loading enable/disable supported
;			4 cursor emulation function supported
;			5 EGA palette present
;			6 color palette present
;			7 color paging function supported
;	0Bh miscellaneous function flags #2
;		bit 0 light pen supported
;			1 save/restore state function 1Ch supported
;			2 intensity blinking function supported
;			3 Display Combination Code supported
;			4-7 reserved
;	0Ch to 0Dh reserved
;	0Eh Save pointer function flags
;		bit 0 512 character set supported
;			1 dynamic save area supported
;			2 alpha font override supported
;			3 graphics font override supported
;			4 palette override supported
;			5 DCC extension supported
;			6 reserved
;			7 reserved
;	0Fh reserved

; - AH = 1Ch VIDEO - SAVE/RESTORE VIDEO STATE (PS50+,VGA)
;{TODO}
;CX = requested states
;	bit 0 video hardware
;		1 BIOS data areas
;		2 color registers and DAC state
;		3-15 reserved
;AL = 0: return state buffer size
;	Return: BX = number of 64 byte blocks needed
;	1: save video state
;		ES:BX = buffer address
;	2: restore video state
;		ES:BX = buffer address of previously saved state
;Return: AL = 1Ch if function supported

; Next AH maybe 40H

; ASCII [MASM] for MSDOS/WIN16
; INT 21Partial AH from 00 to 0F

; AH = 00h DOS - PROGRAM TERMINATION - TaskTerminate
; void <<< ConTerm <<< void
; - iden. to `int 20h`
ConTerm macro
	mov ah, 000h
	int 21h
endm

; AH = 01h DOS - KEYBOARD INPUT - KeyboardGetCharEcho
; AL(ASCII) <<< KbdGetcEcho <<< void
; - ^C/^Break are checked, and INT 23h executed if read character is echoed to standard output
KbdGetcEcho macro
	mov ah, 001h
	int 21h
endm

; AH = 02h DOS - DISPLAY OUTPUT - StandardPrintChar
; void <<< StdPrintChar <<< Char=DL {,sty=~}
; - ^C/^Break are checked, and INT 23h executed if pressed
StdPrintChar macro regpara:=<dl>
	push dx
	mov dl, regpara
	mov ah, 002h
	int 21h
	pop dx
endm

; AH = 03h DOS - AUX INPUT - AuxiliaryGetChar
; AL(ASCII) <<< AuxGetc <<< void
AuxGetc macro
	mov ah, 003h
	int 21h
endm

; AH = 04h DOS - AUX OUTPUT - AuxiliaryPrintChar
; void <<< AuxPrintChar <<< Char=DL {,sty=~}
AuxPrintChar macro regpara:=<dl>
	push dx
	mov dl, regpara
	mov ah, 004h
	int 21h
	pop dx
endm

; AH = 05h DOS - PRINTER OUTPUT - PrinterPrintChar
; void <<< PrtPrintChar <<< Char=DL {,sty=~}
PrtPrintChar macro regpara:=<dl>
	push dx
	mov dl, regpara
	mov ah, 005h
	int 21h
	pop dx
endm

; AH = 06h DOS - DIRECT CONSOLE I/O CHARACTER OUTPUT - ConsolePrintChar
; void <<< ConPrintChar <<< Char=DL {,sty=~}
ConPrintChar macro regpara:=<dl>
	push dx
	mov dl, regpara
	mov ah, 006h
	int 21h
	pop dx
endm

; AH = 06h DOS - DIRECT CONSOLE I/O CHARACTER INPUT - ConsoleGetCharEcho
; AL(ASCII),ZF(NoChar) <<< ConGetcEcho <<< void
; - Notes: Character is echoed to STDOUT if received. ^C/^Break are NOT checked
ConGetcEcho macro
	push dx
	mov dl, 0FFh
	mov ah, 006h
	int 21h
	pop dx
endm

; AH = 07h DOS - DIRECT STDIN INPUT, NO ECHO - ConsoleGetChar
; AL(ASCII) <<< ConGetc <<< void
; - same as function 06h for input but char not echoed
ConGetc macro
	push dx
	mov dl, 0FFh
	mov ah, 007h
	int 21h
	pop dx
endm


; AH = 08h DOS - KEYBOARD INPUT, NO ECHO - KeyboardGetChar
; AL(ASCII) <<< KbdGetc <<< void
; - same as function 07h, but ^C/^Break are checked
KbdGetc macro
	mov ah, 008h
	int 21h
endm

; AH = 09h DOS - PRINT STRING - ConsolePrintString
; void <<< ConPrint <<< string=DS:DX {,sty=~}
; - string terminated by "$"
; - care: using offset keyword for object label in MASM
; - ^C/^Break checked, and INT 23h called if pressed
ConPrint macro regpara:=<dx>
	push dx
	mov dx, regpara
	mov ah, 009h
	int 21h
	pop dx
endm

; AH = 0Ah DOS - BUFFERED KEYBOARD INPUT - KeyboardGetString
; void <<< KbdGets <<< buffer=DS:DX
; - first byte of buffer must contain maximum length on entry, second byte (DOS set) contains actual length of previous line which may be recalled with
KbdGets macro regpara:=<dx>
	push dx
	mov dx, regpara
	mov ah, 00Ah
	int 21h
	pop dx
endm

; AH = 0Bh DOS - CHECK STANDARD INPUT STATUS - StandardStateOfInput
; AL[00:NoChar,FF:Avail] <<< StdStateIn <<< void
; - ^C/^Break checked, and INT 23h called if pressed
StdStateIn macro
	mov ah, 00Bh
	int 21h
endm

; AH = 0Ch DOS - CLEAR KEYBOARD BUFFER - KeyboardBufferClear
; void <<< KbdFlush <<< AfterCall=AL{1,6,7,8,A}
; - Flushes all typeahead input, then executes function specified by AL (effectively moving it to AH and repeating the INT 21 call). If AL contains a value not in the list above, the keyboard buffer is flushed and no other action is taken.
KbdFlush macro regpara:=<al>
	push ax
	mov al, regpara
	mov ah, 00Ch
	int 21h
	pop ax
endm

; AH = 0Dh DOS - DISK RESET - DiskBufferFlush
; void <<< DskReset <<< void
; - Flushes all disk buffers.
DskReset macro
	mov ah, 00Dh
	int 21h
endm

; AH = 0Eh DOS - SELECT DISK - DiskSelect
; AL(LogicDrv) <<< DskSelect <<< DrvNum=AL[0:A,1:B...]
DskSelect macro regpara:=<al>
	push ax
	mov al, regpara
	mov ah, 00Eh
	int 21h
	pop ax
endm

; AH = 0Fh DOS - OPEN DISK FILE - DiskFileOpen
; AL(00:Found,FF:Not) <<< DskOpen <<< FCB=DS:DX
; - (DOS 3.x) file opened in compatibility mode
DskOpen macro regpara:=<dx>
	push dx
	mov dx, regpara
	mov ah, 00Fh
	int 21h
	pop dx
endm

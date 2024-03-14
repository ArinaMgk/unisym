; Haruno Dosconio, RFB01
; Satisfy the File and Endf format
; RFB01 Revive project Kasha and this file Rehabilitation as DemosEasyBootloader

[CPU 386]

%include "pseudo.a"
%include "video.a"
%include "demos.a"

defineStack16
mov ax,0x07C0
mov ds,ax
mov ax,0xB800
mov es,ax

mov si,str
conPrint si,~
xor ah,ah; KeyRead
int 0x16; KeyRead End
mov ax,0x0100
mov es,ax
mov ds,ax

codefileLoad16 0,0,2; Sector 02 -> ES:0
jmp 0x0100:0x1
cli
hlt
str:
%ifdef _RELEASE
	DB 'Mecocoa(386+) ',__DATE__," : "
%endif
	DB "Press any key to continue...",10,13,0; Mecocoa Style LFCR for new-line
	DB "By @dosconio"
times 510-$+$$ db 0
aRiNa

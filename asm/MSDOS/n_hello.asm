; NASM MSDOS
	ORG 100H
section .text 
	global _start 

_start: 
	MOV AX, CS
	MOV DS, AX
	MOV ES, AX
	MOV SS, AX
	MOV SP, stakendo

	MOV AH, 09H
	MOV DX, msg 
	INT 21H

	MOV AH, 4CH
	INT 21H

section .data 
	stakento: times (20-1) dw 0
	stakendo: dw 0
	msg: DB "Hello, MSDOS!", 0ah, 0dh, "$"

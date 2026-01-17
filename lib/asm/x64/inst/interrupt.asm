
GLOBAL sendEOI

[BITS 64]

section .text

sendEOI:
	MOV EAX, 0xFEE000B0
	MOV [EAX], EAX; the value is not important
RET

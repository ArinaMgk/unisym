; Expected: resolve structure member offsets and emit an initialized structure image.

bits 64

struc RegressionRecord
	.tag: resb 1
	.value: resd 1
endstruc

section .text
	mov eax, [rbp+RegressionRecord.value]

section .data
istruc RegressionRecord
	at RegressionRecord.tag, db 0x7a
	at RegressionRecord.value, dd 0x12345678
iend


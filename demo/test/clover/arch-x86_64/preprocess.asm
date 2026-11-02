; Expected: preprocessing emits one included byte followed by three repeated bytes.

%include "preprocess.inc"

%define SELECT_INCLUDED 1

%macro EMIT_BYTE 1
	db %1
%endmacro

bits 64

section .text
%if SELECT_INCLUDED
	EMIT_BYTE INCLUDED_VALUE
%else
	EMIT_BYTE 0xff
%endif

%rep 3
	db 0x22
%endrep


; ASCII [MASM] for MSDOS/WIN16 (C)(ARINAMGK)UNISYM&DOSCONIO
; ---- conflict with Not-Interrupt Implementation ----
; Prefix of ArnStyleNaming:
;- Dos(Sys) Supervisor Function
;- Kbd      Keyboard Function
;- Con      Console Function
;- Mem      Memory Function
;- Mos      Mouse Function
;- Prt      Printer Function
;- Aux      Auxiliary Function
;- Dsk      Disk Function
;- Timer    Timer Function
;- Task     Task Function
;- Win      Window Function
;- Grf      Graph Function
;- Net      Network Function
;- Com      Communication Function
;- Dev      Device Function
;- Str      String Function
;- Ari      Arithmetic Function
;- Std      Standard I/O Function
;- File     File Function
;- Int      Interrupt Function

; Description:
;- Assume stack set
;- { } FUTURE TODO
;- -wo choose the first by default.

; Abbreviation:
;- DTA: data transfer area
;- DPB: drive parameter block
;- FCB: file control block
;- ATI: allocation table information
;- RRF: random record field
;- PSP: program segment prefix
;- ASCIZ: ASCII string terminated by zero

; Skill
;- Pass in a object label, use `OFFSET(VAR1)` but not `OFFSET VAR1`

include INT21p0X.ASM
; include INT21p1X.ASM
; include INT21p2X.ASM
; include INT21p3X.ASM
; include INT21p4X.ASM
; include INT21p5X.ASM
; include INT21p6X.ASM
; include INT21p7X.ASM
; include INT21p8X.ASM
; include INT21p9X.ASM
; include INT21pAX.ASM
; include INT21pBX.ASM
; include INT21pCX.ASM
; include INT21pDX.ASM
; include INT21pEX.ASM
; include INT21pFX.ASM


; @Dosconio Freq-11.0592MHz ASM-SDAS8051 TAB4 LF
; Copyright @ArinaMgk UNISYM
; STC89Cx
;--------------------------------------------------------
;//{Q} Why the cycle different from that precious in Keil as delay11.c?
;// 202403021100 dosconio: It occurred to me that we can realize this by asm and it proved good.
;// void delayms  (unsigned int  ms);
;// void delayus10(unsigned char us10);
;static void delayms(unsigned int ms)
;{
;	unsigned char i;
;	do
;	{
;		i = 200;
;		while(--i);
;	} while (--ms);
;}
;static void delayus10(unsigned char us10)
;{
;	unsigned char i;
;	do
;	{
;		i = 1;
;		while(--i);
;	} while (--us10);
;}
;--------------------------------------------------------
.module delay11M
.optsdcc -mmcs51 --model-small
;--------------------------------------------------------
; Public variables in this module
	; delay without parameter
	.globl _delay010us
	.globl _delay100us
	.globl _delay500us
	.globl _delay001ms
;--------------------------------------------------------
; special function registers
;--------------------------------------------------------
	.area RSEG    (ABS,DATA)
	.org 0x0000
;--------------------------------------------------------
; special function bits
;--------------------------------------------------------
	.area RSEG    (ABS,DATA)
	.org 0x0000
;--------------------------------------------------------
; overlayable register banks
;--------------------------------------------------------
	.area REG_BANK_0	(REL,OVR,DATA)
	.ds 8
;--------------------------------------------------------
; internal ram data
;--------------------------------------------------------
	.area DSEG    (DATA)
;--------------------------------------------------------
; overlayable items in internal ram 
;--------------------------------------------------------
	.area	OSEG    (OVR,DATA)
;--------------------------------------------------------
; indirectly addressable internal ram data
;--------------------------------------------------------
	.area ISEG    (DATA)
;--------------------------------------------------------
; absolute internal ram data
;--------------------------------------------------------
	.area IABS    (ABS,DATA)
	.area IABS    (ABS,DATA)
;--------------------------------------------------------
; bit data
;--------------------------------------------------------
	.area BSEG    (BIT)
;--------------------------------------------------------
; paged external ram data
;--------------------------------------------------------
	.area PSEG    (PAG,XDATA)
;--------------------------------------------------------
; external ram data
;--------------------------------------------------------
	.area XSEG    (XDATA)
;--------------------------------------------------------
; absolute external ram data
;--------------------------------------------------------
	.area XABS    (ABS,XDATA)
;--------------------------------------------------------
; external initialized ram data
;--------------------------------------------------------
	.area XISEG   (XDATA)
	.area HOME    (CODE)
	.area GSINIT0 (CODE)
	.area GSINIT1 (CODE)
	.area GSINIT2 (CODE)
	.area GSINIT3 (CODE)
	.area GSINIT4 (CODE)
	.area GSINIT5 (CODE)
	.area GSINIT  (CODE)
	.area GSFINAL (CODE)
	.area CSEG    (CODE)
;--------------------------------------------------------
; global & static initialisations
;--------------------------------------------------------
	.area HOME    (CODE)
	.area GSINIT  (CODE)
	.area GSFINAL (CODE)
	.area GSINIT  (CODE)
;--------------------------------------------------------
; Home
;--------------------------------------------------------
	.area HOME    (CODE)
	.area HOME    (CODE)
;--------------------------------------------------------
; code
;--------------------------------------------------------
	.area CSEG    (CODE)
;------------------------------------------------------------
	ar7 = 0x07
	ar6 = 0x06
	ar5 = 0x05
	ar4 = 0x04
	ar3 = 0x03
	ar2 = 0x02
	ar1 = 0x01
	ar0 = 0x00

; seem not like a good fit for multiple cycles

_delay010us:
	NOP
	NOP
	NOP
	NOP
	NOP
	RET

_delay100us:
	PUSH	0x30
	MOV		0x30,#41
	_delay100us_NEXT:
	DJNZ	0x30,_delay100us_NEXT
	POP		0x30
	RET

_delay500us:
	PUSH	0x30
	MOV		0x30,#225
	_delay500us_NEXT:
	DJNZ	0x30,_delay500us_NEXT
	POP		0x30
	RET

_delay001ms:
	NOP
	PUSH	0x30
	PUSH	0x31
	MOV		0x30,#2
	MOV		0x31,#194
	_delay001ms_NEXT:
	DJNZ	0x31,_delay001ms_NEXT
	DJNZ	0x30,_delay001ms_NEXT
	POP		0x31
	POP		0x30
	RET

.area CSEG    (CODE)
.area CONST   (CODE)
.area XINIT   (CODE)
.area CABS    (ABS,CODE)

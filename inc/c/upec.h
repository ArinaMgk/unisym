// UTF-8 C99 TAB4 CRLF
// LastCheck: RFX17
// AllAuthor: @ArinaMgk
// ModuTitle: Unisym Practical EnCoding
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

// ASE, is more suitable for applications than ASCII.
// - constant 0~F
// - constant A~Z, a~z


/* UPEC Origin: Mouchi General Code for decimal RET29 @ArinaMgk
H/L	0	1	2	3	4	5	6	7	8	9	A	B	C	D	E	F
0	XCG	キ	ヨ	ナ	マ	ム	イ	リ	ひ	み	す	よ	お	そ	を	。
1	GRP	の	と	「	」		ー	×	°					ぞ	ち	·
2	SPA	~	@	#	$	`	|	&	'	"	_	=	\	,	!	.
3	ALT	+	-	*	/	%	^	0	1	2	3	4	5	6	7	8
4	9	A	B	C	D	E	F	G	H	I	J	K	L	M	N	O
5	P	Q	R	S	T	U	V	W	X	Y	Z	(	[	{	<	:
6	?	a	b	c	d	e	f	g	h	i	j	k	l	m	n	o
7	p	q	r	s	t	u	v	w	x	y	z	)	]	}	>	;
*/

/* UPEC RFX17 @dosconio
- Consider Keyboard Scan Code
-     (08)~(0F)                 isspace
- 0~9 00010110(16)~00011111(1F) isdigit
- A~Z 00100000(20)~00111111(3F) isupper
- a~z 01100000(60)~01111111(7F) islower
-     01000000(40)~01011111(5F) ispunct
- GRP	の	と	「	」	ー	×	°	ぞ	ち	·
- COL is Color
H/L	0	1	2	3	4	5	6	7	8	9	A	B	C	D	E	F
 |------------------------------------------------------------------
0|	NUL	BAK	NXT	UPP	GRP	COL	USR	UTF	VT	CR	LF	SPA	TAB	PG
1|							0	1	2	3	4	5	6	7	8	9
2|	A	B	C	D	E	F	G	H	I	J	K	L	M	N	O	P	
3|	Q	R	S	T	U	V	W	X	Y	Z		
4|	(	[	{	<	:	'	?	"	_	=	~	+	|	-	\	,	
5|	)	]	}	>	;	`	!	@	#	$	%	^	&	*	/	.
6|	a	b	c	d	e	f	g	h	i	j	k	l	m	n	o	p
7|	q	r	s	t	u	v	w	x	y	z
*/

#define isuspace(c) (c <= 0x10 && c > 0x08)
#define isudigit(c) ((c & 0b11110000) == 0b00010000)
#define isuupper(c) ((c & 0b11100000) == 0b00100000)
#define isulower(c) ((c & 0b11100000) == 0b01100000)
#define isupunct(c) ((c & 0b11100000) == 0b01000000)

#define toulower(c) (isuupper(c)? (c) | 0b01000000 : (c))
#define touupper(c) (isulower(c)? (c) & 0b10111111 : (c))

static char upec2ascii(char upec) {
	if (upec & 0b10000000) return 0x06;
	if (upec >= 0x60)
		return upec - 0x60 + 'a';
	if (upec >= 0x40)
		return (const char[]) {
		'(', '[', '{', '<', ':', '\'', '?', '"',
		'_', '=', '~', '+', '|', '-', '\\', ',',
		')', ']', '}', '>', ';', '`', '!', '@',
		'#', '$', '%', '^', '&', '*', '/', '.'
	} [upec - 0x40] ;
	if (upec >= 0x20)
		return upec - 0x20 + 'A';
	if (upec >= 0x10)
		return upec - 0x16 + '0';
	else return (const char[]) {
		'\0', '\b', '\a', '\0', '\0', '\0', '\0', '\0',
		'\v', '\r', '\n', ' ', '\t', '\f', '\0', '\0'
	} [upec] ;
}

static char ascii2upec(char ascii) {
	if (ascii < 0x20) return (const char[]) {
		// NUL SOH STX ETX EOT ENQ ACK BEL BS  HT  LF  VT  FF  CR  SO  SI
		   0,  0,  0,  0,  0,  0,  0,  2,  0,  12,  10, 8, 13, 9,  0,  0,
		// DLE DC1 DC2 DC3 DC4 NAK SYN ETB CAN EM SUB ESC FS  GS  RS  US
		0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
	} [ascii] ;
	if (ascii < 0x30) return (const char[]) {
		0x0B, 0x56, 0x47, 0x58, 0x59, 0x5A, 0x5C, 0x45,
		// spa ! " # $ % & '
		0x40, 0x50, 0x5D, 0x4B, 0x4F, 0x4D, 0x5F, 0x5E 
		// () * +, -. /
	} [ascii - 0x20] ;
	if (ascii < 0x30 + 10) return ascii - 0x30 + 0x16;
	if (ascii <= 0x40) return (const char[]) {
		0x44, 0x54, 0x43, 0x49, 0x53, 0x46, 0x57
		// : ; < = > ? @
	} [ascii - 0x3A] ;
	if (ascii < 0x5B) return ascii - 'A' + 0x20;
	if (ascii <= 0x60) return  (const char[]) {
		0x41, 0x4E, 0x51, 0x5B, 0x48, 0x55
		// [ \ ] ^ _ `
	} [ascii - 0x5B] ;
	if (ascii < 0x7B) return ascii - 'a' + 0x60;
	return (const char[]) {
		0x42, 0x4C, 0x52, 0x4A, 0
		// { | } ~
	} [ascii - 0x7B] ;
}

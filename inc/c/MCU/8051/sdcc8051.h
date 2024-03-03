// ASCII C-SDCC TAB4 CRLF
// Attribute: <ArnCovenant> <Env> <bin^%> <CPU()> [Allocation]
// LastCheck: 2024Mar01
// AllAuthor: @dosconio
// ModuTitle: 8051 MCU Header for SDCC
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

#ifndef _INC_MCU_8051_SDCC
#define _INC_MCU_8051_SDCC

#define defbyt __sfr  __at
#define defbit __sbit __at

// ---- BYTE Register ----
// defbyt (0x80) p0;
   defbyt (0x81) pSP   ;
   defbyt (0x82) pDPL  ;
   defbyt (0x83) pDPH  ;
   defbyt (0x87) pPCON ;
// defbyt (0x88) pTCON ;
   defbyt (0x89) pTMOD ;
   defbyt (0x8A) pTL0  ;
   defbyt (0x8B) pTL1  ;
   defbyt (0x8C) pTH0  ;
   defbyt (0x8D) pTH1  ;
// defbyt (0x90) p1    ;
// defbyt (0x98) pSCON ;
   defbyt (0x99) pSBUF ;
// defbyt (0xA0) p2    ;
// defbyt (0xA8) pIE   ;
// defbyt (0xB0) p3    ;
// defbyt (0xB8) pIP   ;
// defbyt (0xD0) pPSW  ;
   defbyt (0xE0) pACC  ;
   defbyt (0xF0) pB    ;


// ---- BIT Register ----

defbyt (0x80) p0   ;
defbit (0x80) p0_0 ;
defbit (0x81) p0_1 ;
defbit (0x82) p0_2 ;
defbit (0x83) p0_3 ;
defbit (0x84) p0_4 ;
defbit (0x85) p0_5 ;
defbit (0x86) p0_6 ;
defbit (0x87) p0_7 ;

defbyt (0x88) pTCON    ;
defbit (0x88) pTCON_IT0;
defbit (0x89) pTCON_IE0;
defbit (0x8A) pTCON_IT1;
defbit (0x8B) pTCON_IE1;
defbit (0x8C) pTCON_TR0;
defbit (0x8D) pTCON_TF0;
defbit (0x8E) pTCON_TR1;
defbit (0x8F) pTCON_TF1;

defbyt (0x90) p1   ;
defbit (0x90) p1_0 ;
defbit (0x91) p1_1 ;
defbit (0x92) p1_2 ;
defbit (0x93) p1_3 ;
defbit (0x94) p1_4 ;
defbit (0x95) p1_5 ;
defbit (0x96) p1_6 ;
defbit (0x97) p1_7 ;

defbyt (0x98) pSCON      ;
defbit (0x98) pSCON_RI   ;
defbit (0x99) pSCON_TI   ;
defbit (0x9A) pSCON_RB8  ;
defbit (0x9B) pSCON_TB8  ;
defbit (0x9C) pSCON_REN  ;
defbit (0x9D) pSCON_SM2  ;
defbit (0x9E) pSCON_SM1  ;
defbit (0x9F) pSCON_SM0  ;

defbyt (0xA0) p2   ;
defbit (0xA0) p2_0 ;
defbit (0xA1) p2_1 ;
defbit (0xA2) p2_2 ;
defbit (0xA3) p2_3 ;
defbit (0xA4) p2_4 ;
defbit (0xA5) p2_5 ;
defbit (0xA6) p2_6 ;
defbit (0xA7) p2_7 ;

defbyt (0xA8) pIE      ;
defbit (0xA8) pIE_EX0  ;
defbit (0xA9) pIE_ET0  ;
defbit (0xAA) pIE_EX1  ;
defbit (0xAB) pIE_ET1  ;
defbit (0xAC) pIE_ES   ;
defbit (0xAF) pIE_EA   ;

defbyt (0xB0) p3   ;
//
defbit (0xB0) p3_0 ;
defbit (0xB1) p3_1 ;
defbit (0xB2) p3_2 ;
defbit (0xB3) p3_3 ;
defbit (0xB4) p3_4 ;
defbit (0xB5) p3_5 ;
defbit (0xB6) p3_6 ;
defbit (0xB7) p3_7 ;
//
defbit (0xB0) p3_RXD  ;
defbit (0xB1) p3_TXD  ;
defbit (0xB2) p3_INT0 ;
defbit (0xB3) p3_INT1 ;
defbit (0xB4) p3_T0   ;
defbit (0xB5) p3_T1   ;
defbit (0xB6) p3_WR   ;
defbit (0xB7) p3_RD   ;

defbyt (0xB8) pIP    ;
defbit (0xB8) pIP_PX0;
defbit (0xB9) pIP_PT0;
defbit (0xBA) pIP_PX1;
defbit (0xBB) pIP_PT1;
defbit (0xBC) pIP_PS ;

defbyt (0xD0) pPSW    ;
defbit (0xD0) pPSW_P  ;
defbit (0xD1) pPSW_F1 ;
defbit (0xD2) pPSW_OV ;
defbit (0xD3) pPSW_RS0;
defbit (0xD4) pPSW_RS1;
defbit (0xD5) pPSW_F0 ;
defbit (0xD6) pPSW_AC ;
defbit (0xD7) pPSW_CY ;

/* BIT definitions for bits that are not directly accessible */
#ifdef _INC_MCU_8051_SDCC_ENABLE_UNACCESSIBLE_BITS
/* PCON bits */
#define IDL             0x01
#define PD              0x02
#define GF0             0x04
#define GF1             0x08
#define SMOD            0x80

/* TMOD bits */
#define T0_M0           0x01
#define T0_M1           0x02
#define T0_CT           0x04
#define T0_GATE         0x08
#define T1_M0           0x10
#define T1_M1           0x20
#define T1_CT           0x40
#define T1_GATE         0x80
//
#define T0_MASK         0x0F
#define T1_MASK         0xF0
#endif

/* Interrupt numbers: address = (number * 8) + 3 */
#define _IV_i8051_IE0 0       /* 0x03 external interrupt 0 */
#define _IV_i8051_TF0 1       /* 0x0b timer 0 */
#define _IV_i8051_IE1 2       /* 0x13 external interrupt 1 */
#define _IV_i8051_TF1 3       /* 0x1b timer 1 */
#define _IV_i8051_SI0 4       /* 0x23 serial port 0 */

// ---- ---- ---- ---- todo alice.h ---- ---- ---- ----
// test on 2024Mar02 by sizeof()
typedef unsigned char byte;
typedef unsigned int  word;
typedef unsigned long dword;
typedef signed char sbyte;
typedef signed int  sword;
typedef signed long sdword;

// ---- ---- ---- ---- todo (cons)io.h ---- ---- ---- ----

#define po(x) p##x
#define pi(x,y) p##x##_##y

#define be_cmd 0
#define be_dat 1

#define outpi(pin,elec)  ((pin)=(elec))
#define outpb(port,byte) ((port)=(byte)) 
#define innpi(pin,elec) (pin)
#define innpb(port) (port)

#endif

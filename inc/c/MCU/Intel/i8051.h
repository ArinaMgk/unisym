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

#if !defined(_INC_MCU_8051) && defined(defbyt) && defined(defbit)
#define _INC_MCU_8051

// ---- BYTE Register ----
// defbyt (p0,    0x80)
   defbyt (pSP,   0x81)
   defbyt (pDPL,  0x82)
   defbyt (pDPH,  0x83)
   defbyt (pPCON, 0x87)
// defbyt (pTCON, 0x88)
   defbyt (pTMOD, 0x89)
   defbyt (pTL0,  0x8A)
   defbyt (pTL1,  0x8B)
   defbyt (pTH0,  0x8C)
   defbyt (pTH1,  0x8D)
// defbyt (p1,    0x90)
// defbyt (pSCON, 0x98)
   defbyt (pSBUF, 0x99)
// defbyt (p2,    0xA0)
// defbyt (pIE,   0xA8)
// defbyt (p3,    0xB0)
// defbyt (pIP,   0xB8)
// defbyt (pPSW,  0xD0)
   defbyt (pACC,  0xE0)
   defbyt (pB,    0xF0)


// ---- BIT Register ----

defbyt (p0  , 0x80)
defbit (p0_0, 0x80)
defbit (p0_1, 0x81)
defbit (p0_2, 0x82)
defbit (p0_3, 0x83)
defbit (p0_4, 0x84)
defbit (p0_5, 0x85)
defbit (p0_6, 0x86)
defbit (p0_7, 0x87)

defbyt (pTCON    , 0x88)
defbit (pTCON_IT0, 0x88)
defbit (pTCON_IE0, 0x89)
defbit (pTCON_IT1, 0x8A)
defbit (pTCON_IE1, 0x8B)
defbit (pTCON_TR0, 0x8C)
defbit (pTCON_TF0, 0x8D)
defbit (pTCON_TR1, 0x8E)
defbit (pTCON_TF1, 0x8F)

defbyt (p1  , 0x90)
defbit (p1_0, 0x90)
defbit (p1_1, 0x91)
defbit (p1_2, 0x92)
defbit (p1_3, 0x93)
defbit (p1_4, 0x94)
defbit (p1_5, 0x95)
defbit (p1_6, 0x96)
defbit (p1_7, 0x97)

defbyt (pSCON    , 0x98)
defbit (pSCON_RI , 0x98)
defbit (pSCON_TI , 0x99)
defbit (pSCON_RB8, 0x9A)
defbit (pSCON_TB8, 0x9B)
defbit (pSCON_REN, 0x9C)
defbit (pSCON_SM2, 0x9D)
defbit (pSCON_SM1, 0x9E)
defbit (pSCON_SM0, 0x9F)

defbyt (p2  , 0xA0)
defbit (p2_0, 0xA0)
defbit (p2_1, 0xA1)
defbit (p2_2, 0xA2)
defbit (p2_3, 0xA3)
defbit (p2_4, 0xA4)
defbit (p2_5, 0xA5)
defbit (p2_6, 0xA6)
defbit (p2_7, 0xA7)

defbyt (pIE    , 0xA8)
defbit (pIE_EX0, 0xA8)
defbit (pIE_ET0, 0xA9)
defbit (pIE_EX1, 0xAA)
defbit (pIE_ET1, 0xAB)
defbit (pIE_ES , 0xAC)
defbit (pIE_EA , 0xAF)

defbyt (p3     , 0xB0)
defbit (p3_0   , 0xB0)
defbit (p3_1   , 0xB1)
defbit (p3_2   , 0xB2)
defbit (p3_3   , 0xB3)
defbit (p3_4   , 0xB4)
defbit (p3_5   , 0xB5)
defbit (p3_6   , 0xB6)
defbit (p3_7   , 0xB7)
defbit (p3_RXD , 0xB0)
defbit (p3_TXD , 0xB1)
defbit (p3_INT0, 0xB2)
defbit (p3_INT1, 0xB3)
defbit (p3_T0  , 0xB4)
defbit (p3_T1  , 0xB5)
defbit (p3_WR  , 0xB6)
defbit (p3_RD  , 0xB7)

defbyt (pIP    , 0xB8)
defbit (pIP_PX0, 0xB8)
defbit (pIP_PT0, 0xB9)
defbit (pIP_PX1, 0xBA)
defbit (pIP_PT1, 0xBB)
defbit (pIP_PS , 0xBC)

defbyt (pPSW    , 0xD0)
defbit (pPSW_P  , 0xD0)
defbit (pPSW_F1 , 0xD1)
defbit (pPSW_OV , 0xD2)
defbit (pPSW_RS0, 0xD3)
defbit (pPSW_RS1, 0xD4)
defbit (pPSW_F0 , 0xD5)
defbit (pPSW_AC , 0xD6)
defbit (pPSW_CY , 0xD7)

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

// ---- ---- ---- ---- todo (cons)io.h ---- ---- ---- ----

#define po(x) p##x
#define pi(x,y) p##x##_##y

#define be_cmd 0
#define be_dat 1

#define outpi(pin,elec)  ((pin)=(elec))
#define innpi(pin) (pin)

#ifdef _PORT_SPECIAL
#define outpb(port,byte) ((port)=(byte)) 
#define innpb(port) (port)
#endif

#endif

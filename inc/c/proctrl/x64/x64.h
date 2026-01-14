// ASCII C99 TAB4 CRLF
// Attribute: Little-Endian(Byte, Bit)
// AllAuthor: @ArinaMgk
// ModuTitle: General Header for x64 CPU

#include "../../stdinc.h"
#ifndef _INC_X64
#define _INC_X64

// ---- ATX Board

enum PORT_ATX_X64
#ifdef _INC_CPP
	: uint16
#endif
{
	PORT_PCI_CONFIG_ADDR = 0x0CF8,// dword
	PORT_PCI_CONFIG_DATA = 0x0CFC,// dword
};


// ----

_ESYM_C void OUT_b(uint16 port, uint8 data);
_ESYM_C uint32 IN_b(uint16 port);
_ESYM_C void OUT_w(uint16 port, uint16 data);
_ESYM_C uint32 IN_w(uint16 port);
_ESYM_C void OUT_d(uint16 port, uint32 data);
_ESYM_C uint32 IN_d(uint16 port);

#define outpb OUT_b
#define innpb IN_b
#define outpw OUT_w
#define innpw IN_w
#define outpd OUT_d
#define innpd IN_d

#endif

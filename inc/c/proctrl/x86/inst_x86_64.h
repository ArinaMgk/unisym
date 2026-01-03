// Arina Binary Code Definition Element File \
	Generating from Header to Immediate \
	since RFC

// Format:
// - Lab: Pre0 ... Opc Op0, Op1 ...

#ifndef _AASM_INST
#define _AASM_INST

#define _NOP 0x90 // i.e. _XCHG_AX_AX
#define _XCHG_AX_CX 0x91
#define _XCHG_AX_DX 0x92
#define _XCHG_AX_BX 0x93
#define _XCHG_AX_SP 0x94
#define _XCHG_AX_BP 0x95
#define _XCHG_AX_SI 0x96
#define _XCHG_AX_DI 0x97


#define _MOV_AL_imm 0xB0

#define _MOV_AX_imm 0xB8
#define _MOV_CX_imm 0xB9
#define _MOV_DX_imm 0xBA
#define _MOV_BX_imm 0xBB
#define _MOV_SP_imm 0xBC
#define _MOV_BP_imm 0xBD
#define _MOV_SI_imm 0xBE
#define _MOV_DI_imm 0xBF

#define _MOV_EAX_imm 0xB8

#define _JMP_imm_d 0xE9
#define _JMP_imm_sid 0xEB

#define _BIT_64 0x48 // e.g. _MOV_RAX_imm {0x48, 0xB8, imm64}

#endif // !_AASM_INST

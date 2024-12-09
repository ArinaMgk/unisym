// ASCII C99 TAB4 CRLF
// Attribute: Little-Endian(Byte, Bit)
// LastCheck: 20241207
// AllAuthor: @dosconio
// ModuTitle: General Header for Cortex A7 CPU

#ifndef _INC_ARM_Cortex_A7
#define _INC_ARM_Cortex_A7
#include "../../stdinc.h"

#ifdef _DEV_GCC//{TEMP} GCC only

// Instruction Synchronization Barrier
// - flushes the pipeline in the processor, so that all instructions following the ISB are fetched from cache or memory, after the instruction has been completed.
#define __ISB() _ASM volatile ("isb 0xF":::"memory")

// Data Synchronization Barrier
// - acts as a special kind of Data Memory Barrier. It completes when all explicit memory accesses before this instruction complete.
#define __DSB() _ASM volatile ("dsb 0xF":::"memory")

// Data Memory Barrier
// - ensures the ordering of read and write memory operations issued before and after the barrier.
#define __DMB() _ASM volatile ("dmb 0xF":::"memory")

#define __get_CP(cp, op1, Rt, CRn, CRm, op2) _ASM volatile("MRC p" # cp ", " # op1 ", %0, c" # CRn ", c" # CRm ", " # op2 : "=r" (Rt) : : "memory" )

#define __set_CP(cp, op1, Rt, CRn, CRm, op2) _ASM volatile("MCR p" # cp ", " # op1 ", %0, c" # CRn ", c" # CRm ", " # op2 : : "r" (Rt) : "memory" )

#define __get_CP64(cp, op1, Rt, CRm)         _ASM volatile("MRRC p" # cp ", " # op1 ", %Q0, %R0, c" # CRm  : "=r" (Rt) : : "memory" )

#define __set_CP64(cp, op1, Rt, CRm)         _ASM volatile("MCRR p" # cp ", " # op1 ", %Q0, %R0, c" # CRm  : : "r" (Rt) : "memory" )


#endif

#define statin inline static
// here: L1C PL1 L2C-310
// noth: GIC MMU

// ---- Cache

// CP15 Register SCTLR
#define RESERVED(N, T) T RESERVED##N;
typedef union
{
	struct
	{
		uint32_t M : 1;         //  0  MMU enable
		uint32_t A : 1;         //  1  Alignment check enable
		uint32_t C : 1;         //  2  Cache enable
		RESERVED(0:2, uint32_t)
		uint32_t CP15BEN : 1;   //  5  CP15 barrier enable
		RESERVED(1:1, uint32_t)
		uint32_t B : 1;         //  7  Endianness model
		RESERVED(2:2, uint32_t)
		uint32_t SW : 1;        // 10  SWP and SWPB enable
		uint32_t Z : 1;         // 11  Branch prediction enable
		uint32_t I : 1;         // 12  Instruction cache enable
		uint32_t V : 1;         // 13  Vectors bit
		uint32_t RR : 1;        // 14  Round Robin select
		RESERVED(3:2, uint32_t)
		uint32_t HA : 1;        // 17  Hardware Access flag enable
		RESERVED(4:1, uint32_t)
		uint32_t WXN : 1;       // 19  Write permission implies XN
		uint32_t UWXN : 1;      // 20  Unprivileged write permission implies PL1 XN
		uint32_t FI : 1;        // 21  Fast interrupts configuration enable
		uint32_t U : 1;         // 22  Alignment model
		RESERVED(5:1, uint32_t)
		uint32_t VE : 1;        // 24  Interrupt Vectors Enable
		uint32_t EE : 1;        // 25  Exception Endianness
		RESERVED(6:1, uint32_t)
		uint32_t NMFI : 1;      // 27  Non-maskable FIQ (NMFI) support
		uint32_t TRE : 1;       // 28  TEX remap enable.
		uint32_t AFE : 1;       // 29  Access flag enable
		uint32_t TE : 1;        // 30  Thumb Exception enable
		RESERVED(7:1, uint32_t)
	} b;
	uint32_t w;
} SCTLR_Type;
#undef RESERVED

// System Control Register value
statin uint32 __get_SCTLR(void)
{
	uint32 result;
	__get_CP(15, 0, result, 1, 0, 0);
	return result;
}
statin void __set_SCTLR(uint32 sctlr)
{
	__set_CP(15, 0, sctlr, 1, 0, 0);
}

// Branch Predictor Invalidate All
statin void __set_BPIALL(uint32 value)
{
	__set_CP(15, 0, value, 7, 5, 6);
}

// Instruction Cache Invalidate All
statin void __set_ICIALLU(uint32 value)
{
	__set_CP(15, 0, value, 7, 5, 0);
}

// Data cache clean
statin void __set_DCCMVAC(uint32 value)
{
	__set_CP(15, 0, value, 7, 10, 1);
}

// Data cache invalidate
statin void __set_DCIMVAC(uint32 value)
{
	__set_CP(15, 0, value, 7, 6, 1);
}

// Data cache clean and invalidate
statin void __set_DCCIMVAC(uint32 value)
{
	__set_CP(15, 0, value, 7, 14, 1);
}

//
statin uint32 __get_CSSELR(void)
{
	uint32 result;
	__get_CP(15, 2, result, 0, 0, 0);// __ASM volatile("MRC p15, 2, %0, c0, c0, 0" : "=r"(result) : : "memory");
	return result;
}
statin void __set_CSSELR(uint32 value)
{
	__set_CP(15, 2, value, 0, 0, 0);// __ASM volatile("MCR p15, 2, %0, c0, c0, 0" : : "r"(value) : "memory");
}

//
statin uint32 __get_CCSIDR(void)
{
	uint32 result;
	__get_CP(15, 1, result, 0, 0, 0);// __ASM volatile("MRC p15, 1, %0, c0, c0, 0" : "=r"(result) : : "memory");
	return result;
}

//
statin void __set_DCISW(uint32 value)
{
	__set_CP(15, 0, value, 7, 6, 2);//__ASM volatile("MCR p15, 0, %0, c7, c6, 2" : : "r"(value) : "memory")
}

//
statin void __set_DCCSW(uint32 value)
{
	__set_CP(15, 0, value, 7, 10, 2);// __ASM volatile("MCR p15, 0, %0, c7, c10, 2" : : "r"(value) : "memory")
}

//
statin void __set_DCCISW(uint32 value)
{
	__set_CP(15, 0, value, 7, 14, 2);// __ASM volatile("MCR p15, 0, %0, c7, c14, 2" : : "r"(value) : "memory")
}

//
statin uint32 __get_CLIDR(void)
{
	uint32 result;
	__get_CP(15, 1, result, 0, 0, 1);// __ASM volatile("MRC p15, 1, %0, c0, c0, 1" : "=r"(result) : : "memory");
	return result;
}



// ---- Timer

// Get Count Value
// AKA PL1_GetCurrentValue in core_ca.h
// AKA __get_CNTP_TVAL     in cmsis_cp15.h
statin uint32 PL1_GetCurrentValue(void) {
	uint32 result;
	__get_CP(15, 0, result, 14, 2, 0);
	return result;
}

// Set Count Value
// AKA PL1_SetLoadValue in core_ca.h
// =   __set_CNTP_TVAL  in cmsis_cp15.h + __ISB()
statin void PL1_SetLoadValue(uint32 value) {
	__set_CP(15, 0, value, 14, 2, 0);
	__ISB();
}



#undef statin

#endif

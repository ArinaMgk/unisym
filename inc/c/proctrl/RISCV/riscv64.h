// ASCII GNU-C99 TAB4 CRLF
// Attribute: Little-Endian(Byte, Bit)
// LastCheck: 20240523
// AllAuthor: @dosconio
// ModuTitle: General Header for x86 CPU

#ifndef _INC_RISCV_64
#define _INC_RISCV_64

#ifndef _OPT_RISCV64
	#define _OPT_RISCV64
	#endif
	#include "../../stdinc.h"

// Machine Status Register, mstatus

#define _MSTATUS_MPP_MASK (3L << 11) // previous mode.
#define _MSTATUS_MPP_M    (3L << 11)
#define _MSTATUS_MPP_S    (1L << 11)
#define _MSTATUS_MPP_U    (0L << 11)
#define _MSTATUS_MIE      (1L << 3 ) // machine-mode interrupt enable.

static inline uint64 getMSTATUS() {
	uint64 x;
	asm volatile("csrr %0, mstatus" : "=r"(x));
	return x;
}

static inline void setMSTATUS(uint64 x) {
	asm volatile("csrw mstatus, %0" : : "r"(x));
}

// machine exception program counter
// holds code address to go after exception
static inline void setMEPC(uint64 x) {
	asm volatile("csrw mepc, %0" : : "r"(x));
}

// Supervisor Status Register, sstatus

#define _SSTATUS_SPP  (1L << 8) // Previous mode, 1=Supervisor, 0=User
#define _SSTATUS_SPIE (1L << 5) // Supervisor Previous Interrupt Enable
#define _SSTATUS_UPIE (1L << 4) // User Previous Interrupt Enable
#define _SSTATUS_SIE  (1L << 1) // Supervisor Interrupt Enable
#define _SSTATUS_UIE  (1L << 0) // User Interrupt Enable

static inline uint64 getSSTATUS() {
	uint64 x;
	asm volatile("csrr %0, sstatus" : "=r"(x));
	return x;
}

static inline void setSSTATUS(uint64 x) {
	asm volatile("csrw sstatus, %0" : : "r"(x));
}


// Supervisor Interrupt Pending
static inline uint64 getSIP() {
	uint64 x;
	asm volatile("csrr %0, sip" : "=r"(x));
	return x;
}

static inline void setSIP(uint64 x) {
	asm volatile("csrw sip, %0" : : "r"(x));
}

// Supervisor Interrupt Enable
#define _SIE_SEIE (1L << 9) // external
#define _SIE_STIE (1L << 5) // timer
#define _SIE_SSIE (1L << 1) // software

static inline uint64 getSIE() {
	uint64 x;
	asm volatile("csrr %0, sie" : "=r"(x));
	return x;
}

static inline void setSIE(uint64 x) {
	asm volatile("csrw sie, %0" : : "r"(x));
}

// Machine-mode Interrupt Enable
#define _MIE_MEIE (1L << 11) // external
#define _MIE_MTIE (1L << 7) // timer
#define _MIE_MSIE (1L << 3) // software

static inline uint64 getMIE() {
	uint64 x;
	asm volatile("csrr %0, mie" : "=r"(x));
	return x;
}

static inline void setMIE(uint64 x) {
	asm volatile("csrw mie, %0" : : "r"(x));
}

// machine exception program counter
// holds the code instruction address to go after exception

static inline uint64 getSEPC() {
	uint64 x;
	asm volatile("csrr %0, sepc" : "=r"(x));
	return x;
}

static inline void setSEPC(uint64 x) {
	asm volatile("csrw sepc, %0" : : "r"(x));
}


// Machine Exception Delegation

static inline uint64 getMEDELEG() {
	uint64 x;
	asm volatile("csrr %0, medeleg" : "=r"(x));
	return x;
}

static inline void setMEDELEG(uint64 x) {
	asm volatile("csrw medeleg, %0" : : "r"(x));
}

// Machine Interrupt Delegation
static inline uint64 getMIDELEG() {
	uint64 x;
	asm volatile("csrr %0, mideleg" : "=r"(x));
	return x;
}

static inline void setMIDELEG(uint64 x) {
	asm volatile("csrw mideleg, %0" : : "r"(x));
}

// Supervisor Trap-Vector Base Address
// low two bits are mode.

static inline uint64 getSTVEC() {
	uint64 x;
	asm volatile("csrr %0, stvec" : "=r"(x));
	return x;
}

static inline void setSTVEC(uint64 x) {
	asm volatile("csrw stvec, %0" : : "r"(x));
}

// Machine-mode interrupt vector
static inline void setMTVEC(uint64 x) {
	asm volatile("csrw mtvec, %0" : : "r"(x));
}

// use riscv's sv39 page table scheme.
#define _SATP_SV39 (8L << 60)

#define _MAKE_SATP(pagetable) (_SATP_SV39 | (((uint64)pagetable) >> 12))

// supervisor address translation and protection;
// holds the address of the page table.
static inline uint64 getSATP() {
	uint64 x;
	asm volatile("csrr %0, satp" : "=r"(x));
	return x;
}

static inline void setSATP(uint64 x) {
	asm volatile("csrw satp, %0" : : "r"(x));
}

// Supervisor Scratch register, for early trap handler in trampoline.S.
static inline void setSSCRATCH(uint64 x) {
	asm volatile("csrw sscratch, %0" : : "r"(x));
}

static inline void setMSCRATCH(uint64 x) {
	asm volatile("csrw mscratch, %0" : : "r"(x));
}

// Supervisor Trap Cause
static inline uint64 getSCAUSE() {
	uint64 x;
	asm volatile("csrr %0, scause" : "=r"(x));
	return x;
}

// Supervisor Trap Value
static inline uint64 getSTVAL() {
	uint64 x;
	asm volatile("csrr %0, stval" : "=r"(x));
	return x;
}

// Machine-mode Counter-Enable
static inline uint64 getMCOUNTEREN() {
	uint64 x;
	asm volatile("csrr %0, mcounteren" : "=r"(x));
	return x;
}

static inline void setMCOUNTEREN(uint64 x) {
	asm volatile("csrw mcounteren, %0" : : "r"(x));
}

// machine-mode cycle counter
static inline uint64 getTIME() {
	uint64 x;
	asm volatile("csrr %0, time" : "=r"(x));
	return x;
}

// en/disable device interrupts
static inline void enInterrupt(int enable) {
	if (enable)
		setSSTATUS(getSSTATUS() | _SSTATUS_SIE);
	else
		setSSTATUS(getSSTATUS() & ~_SSTATUS_SIE);
}

static inline int getInterrupt() {
	return (getSSTATUS() & _SSTATUS_SIE) != 0;
}

static inline uint64 getSP() {
	uint64 x;
	asm volatile("mv %0, sp" : "=r"(x));
	return x;
}

// read and write tp, the thread pointer, which holds
// this core's hartid (core number), the index into cpus[].
static inline uint64 getTP() {
	uint64 x;
	asm volatile("mv %0, tp" : "=r"(x));
	return x;
}

static inline void setTP(uint64 x) {
	asm volatile("mv tp, %0" : : "r"(x));
}

static inline uint64 getRA() {
	uint64 x;
	asm volatile("mv %0, ra" : "=r"(x));
	return x;
}

// flush the TLB.
static inline void FlushTLB() {
	// the zero, zero means flush all TLB entries.
	asm volatile("sfence.vma zero, zero");
}


#define _PTE_V (1L << 0) // valid
#define _PTE_R (1L << 1)
#define _PTE_W (1L << 2)
#define _PTE_X (1L << 3)
#define _PTE_U (1L << 4) // 1 -> user can access

// shift a physical address to the right place for a PTE.
#define _PA2PTE(pa) ((((uint64)pa) >> 12) << 10)

#define _PTE2PA(pte) (((pte) >> 10) << 12)

#define _PTE_FLAGS(pte) ((pte)&0x3FF)

// extract the three 9-bit page table indices from a virtual address.
#define _PXMASK 0x1FF // 9 bits
#define _PXSHIFT(level) (PGSHIFT + (9 * (level)))
#define _PX(level, va) ((((uint64)(va)) >> PXSHIFT(level)) & PXMASK)

// one beyond the highest possible virtual address.
// MAXVA is actually one bit less than the max allowed by
// Sv39, to avoid having to sign-extend virtual addresses
// that have the high bit set.
#define _MAXVA (1L << (9 + 9 + 9 + 12 - 1))

typedef uint64 pte_t;
typedef uint64* pagetable_t; // 512 PTEs

//{UNCHK}
static inline uint64 getR0() {
	uint64 x;
	asm volatile("mv %0, x10" : "=r"(x));
	return x;
}

#endif

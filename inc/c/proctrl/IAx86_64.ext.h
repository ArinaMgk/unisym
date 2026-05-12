
struct cpuid_1_0_eax {
	uint32_t stepping       : 4;   // Bits 3:0
	uint32_t model          : 4;   // Bits 7:4
	uint32_t family         : 4;   // Bits 11:8
	uint32_t processor_type : 2;   // Bits 13:12
	uint32_t reserved1      : 2;   // Bits 15:14
	uint32_t extended_model : 4;   // Bits 19:16
	uint32_t extended_family: 8;   // Bits 27:20
	uint32_t reserved2      : 4;   // Bits 31:28
};
struct cpuid_1_0_ebx {
	uint32_t brand_index        : 8;   // Bits 7:0   (brand index, obsolete)
	uint32_t clflush_line_size  : 8;   // Bits 15:8  (CLFLUSH cache line size in 8‑byte units)
	uint32_t initial_apic_id    : 8;   // Bits 23:16 (local APIC physical ID)
	uint32_t max_logical_cpus   : 8;   // Bits 31:24 (max addressable logical CPUs)
};
struct cpuid_1_0_ecx {
	uint32_t sse3            : 1;  // bit 0
	uint32_t pclmulqdq       : 1;  // bit 1
	uint32_t dtes64          : 1;  // bit 2
	uint32_t monitor         : 1;  // bit 3
	uint32_t ds_cpl          : 1;  // bit 4
	uint32_t vmx             : 1;  // bit 5
	uint32_t smx             : 1;  // bit 6
	uint32_t est             : 1;  // bit 7
	uint32_t tm2             : 1;  // bit 8
	uint32_t ssse3           : 1;  // bit 9
	uint32_t cnxt_id         : 1;  // bit 10
	uint32_t sdbg            : 1;  // bit 11
	uint32_t fma             : 1;  // bit 12
	uint32_t cmpxchg16b      : 1;  // bit 13
	uint32_t xtpr            : 1;  // bit 14
	uint32_t pdcm            : 1;  // bit 15
	uint32_t reserved1       : 1;  // bit 16
	uint32_t pcid            : 1;  // bit 17
	uint32_t dca             : 1;  // bit 18
	uint32_t sse4_1          : 1;  // bit 19
	uint32_t sse4_2          : 1;  // bit 20
	uint32_t x2apic          : 1;  // bit 21
	uint32_t movbe           : 1;  // bit 22
	uint32_t popcnt          : 1;  // bit 23
	uint32_t tsc_deadline    : 1;  // bit 24
	uint32_t aesni           : 1;  // bit 25
	uint32_t xsave           : 1;  // bit 26
	uint32_t osxsave         : 1;  // bit 27
	uint32_t avx             : 1;  // bit 28
	uint32_t f16c            : 1;  // bit 29
	uint32_t rdrand          : 1;  // bit 30
	uint32_t reserved2       : 1;  // bit 31
};
struct cpuid_1_0_edx {
	uint32_t fpu             : 1;  // bit 0
	uint32_t vme             : 1;  // bit 1
	uint32_t de              : 1;  // bit 2
	uint32_t pse             : 1;  // bit 3
	uint32_t tsc             : 1;  // bit 4
	uint32_t msr             : 1;  // bit 5
	uint32_t pae             : 1;  // bit 6
	uint32_t mce             : 1;  // bit 7
	uint32_t cx8             : 1;  // bit 8
	uint32_t apic            : 1;  // bit 9
	uint32_t reserved1       : 1;  // bit 10
	uint32_t sep             : 1;  // bit 11
	uint32_t mtrr            : 1;  // bit 12
	uint32_t pge             : 1;  // bit 13
	uint32_t mca             : 1;  // bit 14
	uint32_t cmov            : 1;  // bit 15
	uint32_t pat             : 1;  // bit 16
	uint32_t pse36           : 1;  // bit 17
	uint32_t psn             : 1;  // bit 18 (processor serial number, obsolete)
	uint32_t clfsh           : 1;  // bit 19
	uint32_t reserved2       : 1;  // bit 20
	uint32_t ds              : 1;  // bit 21
	uint32_t acpi            : 1;  // bit 22
	uint32_t mmx             : 1;  // bit 23
	uint32_t fxsr            : 1;  // bit 24
	uint32_t sse             : 1;  // bit 25
	uint32_t sse2            : 1;  // bit 26
	uint32_t ss              : 1;  // bit 27
	uint32_t htt             : 1;  // bit 28 (Hyper‑Threading / multi‑core)
	uint32_t tm              : 1;  // bit 29
	uint32_t ia64            : 1;  // bit 30 (reserved / Itanium)
	uint32_t pbe             : 1;  // bit 31
};


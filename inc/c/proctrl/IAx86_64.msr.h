
#ifdef _INC_CPP
enum class x86MSR : uint32 {
	APIC_BASE = 0x1B,
	FEATURE_CONTROL = 0x3A,

	VMX_BASIC = 0x0480,

	// x2APIC:
	APIC_APICID = 0x0802,
	APIC_VERSION     = 0x0803,
	APIC_TPR         = 0x0808,
	APIC_PPR         = 0x080A,
	APIC_EOI         = 0x080B,
	APIC_LDR         = 0x080D,
	APIC_DFR         = 0x080E,
	APIC_SVR         = 0x080F,// Spurious Vector Register
	APIC_ISR0        = 0x0810,
	APIC_TMR0        = 0x0818,
	APIC_IRR0        = 0x0820,
	APIC_ESR         = 0x0828,
	APIC_LVT_CMCI    = 0x082F,
	APIC_ICR_LOW     = 0x0830,
	APIC_ICR_HIGH    = 0x0831,
	APIC_LVT_TIMER   = 0x0832,
	APIC_LVT_THERMAL = 0x0833,
	APIC_LVT_PMI     = 0x0834,
	APIC_LVT_LINT0   = 0x0835,
	APIC_LVT_LINT1   = 0x0836,
	APIC_LVT_ERROR   = 0x0837,
	APIC_INIT_COUNT  = 0x0838,
	APIC_CUR_COUNT   = 0x0839,
	APIC_DIV_CONF    = 0x083E,
	APIC_SELF_IPI    = 0x083F,
	EFER = 0xC0000080,
	STAR  = 0xC0000081,
	LSTAR = 0xC0000082,
	FMASK = 0xC0000084,
	GS_BASE = 0xC0000101,
	KERNEL_GS_BASE = 0xC0000102,
	VM_CR = 0xC0010114,
	VM_HSAVE_PA = 0xC0010117,// AMD
};
// Model Specific Register
_ESYM_C uint64 getMSR(x86MSR offset);
_ESYM_C void setMSR(x86MSR offset, uint64 value);
#endif


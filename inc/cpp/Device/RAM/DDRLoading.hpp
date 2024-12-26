
#define PREF(x) uni::DDRControl::x
static ::uni::DDRControl ListLoadControl[] = {
	PREF(MSTR),
	PREF(MRCTRL0),
	PREF(MRCTRL1),
	PREF(DERATEEN),
	PREF(DERATEINT),
	PREF(PWRCTL),
	PREF(PWRTMG),
	PREF(HWLPCTL),
	PREF(RFSHCTL0),
	PREF(RFSHCTL3),
	PREF(CRCPARCTL0),
	PREF(ZQCTL0),
	PREF(DFITMG0),
	PREF(DFITMG1),
	PREF(DFILPCFG0),
	PREF(DFIUPD0),
	PREF(DFIUPD1),
	PREF(DFIUPD2),
	PREF(DFIPHYMSTR),
	PREF(ODTMAP),
	PREF(DBG0),
	PREF(DBG1),
	PREF(DBGCMD),
	PREF(POISONCFG),
	PREF(PCCFG),
};
#undef PREF
#define PREF(x) uni::DDRControl::x
// Timing
static ::uni::DDRControl ListLoadTiming[] = {
	PREF(RFSHTMG),
	PREF(DRAMTMG0),
	PREF(DRAMTMG1),
	PREF(DRAMTMG2),
	PREF(DRAMTMG3),
	PREF(DRAMTMG4),
	PREF(DRAMTMG5),
	PREF(DRAMTMG6),
	PREF(DRAMTMG7),
	PREF(DRAMTMG8),
	PREF(DRAMTMG14),
	PREF(ODTCFG),
};
#undef PREF
#define PREF(x) uni::DDRControl::x
// Mapping
static ::uni::DDRControl ListLoadMapping[] = {
	PREF(ADDRMAP1),
	PREF(ADDRMAP2),
	PREF(ADDRMAP3),
	PREF(ADDRMAP4),
	PREF(ADDRMAP5),
	PREF(ADDRMAP6),
	PREF(ADDRMAP9),
	PREF(ADDRMAP10),
	PREF(ADDRMAP11),
};
#undef PREF
#define PREF(x) uni::DDRControl::x
static ::uni::DDRControl ListLoadPerform[] = {
	PREF(SCHED),
	PREF(SCHED1),
	PREF(PERFHPR1),
	PREF(PERFLPR1),
	PREF(PERFWR1),
	PREF(PCFGR_0),
	PREF(PCFGW_0),
	PREF(PCFGQOS0_0),
	PREF(PCFGQOS1_0),
	PREF(PCFGWQOS0_0),
	PREF(PCFGWQOS1_0),
#if 0 // DDR_DUAL_AXI_PORT
	PREF(PCFGR_1),
	PREF(PCFGW_1),
	PREF(PCFGQOS0_1),
	PREF(PCFGQOS1_1),
	PREF(PCFGWQOS0_1),
	PREF(PCFGWQOS1_1),
#endif
};
#undef PREF
#define PREF(x) uni::DDRPhyziks::x
static ::uni::DDRPhyziks ListLoadFizical[] = {
	PREF(PGCR),
	PREF(ACIOCR),
	PREF(DXCCR),
	PREF(DSGCR),
	PREF(DCR),
	PREF(ODTCR),
	PREF(ZQ0CR1),
	PREF(DX0GCR),
	PREF(DX1GCR),
#if 0 // DDR_32BIT_INTERFACE
	PREF(DX2GCR),
	PREF(DX3GCR),
#endif /* DDR_32BIT_INTERFACE */
};
#undef PREF
#define PREF(x) uni::DDRPhyziks::x
static ::uni::DDRPhyziks ListLoadFizTime[] = {
	PREF(PTR0),
	PREF(PTR1),
	PREF(PTR2),
	PREF(DTPR0),
	PREF(DTPR1),
	PREF(DTPR2),
	PREF(MR0),
	PREF(MR1),
	PREF(MR2),
	PREF(MR3),
};
#undef PREF




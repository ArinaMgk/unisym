// INNER INCLUDED

#include "../../unisym"

namespace uni {

	struct DDR_Information {
		rostr name;
		uint32 speed; // kHz
		uint32 size; // Memory size in byte = col * row * width
	};

	struct DDR_Register {
		uint32_t \
			MSTR, MRCTRL0, MRCTRL1, DERATEEN,
			DERATEINT, PWRCTL, PWRTMG, HWLPCTL,
			RFSHCTL0, RFSHCTL3, CRCPARCTL0, ZQCTL0,
			DFITMG0, DFITMG1, DFILPCFG0, DFIUPD0,
			DFIUPD1, DFIUPD2, DFIPHYMSTR, ODTMAP,
			DBG0, DBG1, DBGCMD, POISONCFG,
			PCCFG;
	};

	struct DDR_Timing {
		uint32_t RFSHTMG, DRAMTMG0, DRAMTMG1, DRAMTMG2, DRAMTMG3, DRAMTMG4, DRAMTMG5, DRAMTMG6, DRAMTMG7, DRAMTMG8, DRAMTMG14, ODTCFG;
	};

	// Abandon [0], and use [1~6], [9~11]
	typedef uint32 DDR_Mapping[12];

	struct DDR_Performance {
		uint32_t SCHED, SCHED1, PERFHPR1, PERFLPR1, PERFWR1, PCFGR_0, PCFGW_0, PCFGQOS0_0, PCFGQOS1_0, PCFGWQOS0_0, PCFGWQOS1_0;
		//: DDR_DUAL_AXI_PORT area
		//{TEMP} no-use
		uint32_t PCFGR_1, PCFGW_1, PCFGQOS0_1, PCFGQOS1_1, PCFGWQOS0_1, PCFGWQOS1_1;
	};

	struct DDR_PhyzikRegister {
		uint32_t PGCR, ACIOCR, DXCCR, DSGCR, DCR, ODTCR, ZQ0CR1, DX0GCR, DX1GCR;
		//: DDR_32BIT_INTERFACE
		//{TEMP} no-use
		uint32_t DX2GCR, DX3GCR;
	};

	struct DDR_PhyzikTiming {
		uint32_t PTR0, PTR1, PTR2;
		uint32_t DTPR0, DTPR1, DTPR2;
		uint32_t MR0, MR1, MR2, MR3;
	};








}
namespace uni {
	struct DDRConfig {
		DDR_Information    info;
		DDR_Register       c_reg;
		DDR_Timing         c_timing;
		DDR_Mapping        c_map;
		DDR_Performance    c_perf;
		DDR_PhyzikRegister p_reg;
		DDR_PhyzikTiming   p_timing;
	}; // HAL_DDR_ConfigTypeDef
}

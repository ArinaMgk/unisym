/* Memory mapping of Core Hardware */
#define _SysCtrlSpace_ADDR   (0xE000E000UL)
#define     _SysTick_BASE   (_SysCtrlSpace_ADDR + 0x0010UL)  
#define     _NVIC_BASE      (_SysCtrlSpace_ADDR + 0x0100UL)  
#define     _SCB_BASE       (_SysCtrlSpace_ADDR + 0x0D00UL) 
#define SCB ((struct SysCtrlBlock_Map *)_SCB_BASE)

struct SysCtrlBlock_Map {
	uint32 CPUID;          // 0x000 (R/ )  CPUID Base Register
	uint32 ICSR;           // 0x004 (R/W)  Interrupt Control and State Register
	uint32 VTOR;           // 0x008 (R/W)  Vector Table Offset Register
	uint32 AIRCR;          // 0x00C (R/W)  Application Interrupt and Reset Control Register
	uint32 SCR;            // 0x010 (R/W)  System Control Register
	uint32 CCR;            // 0x014 (R/W)  Configuration Control Register
	uint8  SHP[12U];       // 0x018 (R/W)  System Handlers Priority Registers (4-7, 8-11, 12-15)
	uint32 SHCSR;          // 0x024 (R/W)  System Handler Control and State Register
	uint32 CFSR;           // 0x028 (R/W)  Configurable Fault Status Register
	uint32 HFSR;           // 0x02C (R/W)  HardFault Status Register
	uint32 DFSR;           // 0x030 (R/W)  Debug Fault Status Register
	uint32 MMFAR;          // 0x034 (R/W)  MemManage Fault Address Register
	uint32 BFAR;           // 0x038 (R/W)  BusFault Address Register
	uint32 AFSR;           // 0x03C (R/W)  Auxiliary Fault Status Registe
	uint32 PFR[2U];        // 0x040 (R/ )  Processor Feature Register
	uint32 DFR;            // 0x048 (R/ )  Debug Feature Register
	uint32 ADR;            // 0x04C (R/ )  Auxiliary Feature Register
	uint32 MMFR[4U];       // 0x050 (R/ )  Memory Model Feature Register
	uint32 ISAR[5U];       // 0x060 (R/ )  Instruction Set Attributes Register
	uint32 RESERVED0[5U];
	uint32 CPACR;          // 0x088 (R/W)  Coprocessor Access Control Register
};

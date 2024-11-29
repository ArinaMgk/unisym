
typedef enum class GICDistributor {
	CTLR = 0x000 / 4,          //          (R/W) Distributor Control 
	TYPER = 0x004 / 4,         //          (R/ ) Interrupt Controller Type 
	IIDR = 0x008 / 4,          //          (R/ ) Distributor Implementer Identification 
	//
	STATUSR = 0x010 / 4,       //          (R/W) Error Reporting Status , optional
	//
	SETSPI_NSR = 0x040 / 4,    //          ( /W) Set SPI 
	//
	CLRSPI_NSR = 0x048 / 4,    //          ( /W) Clear SPI 
	//
	SETSPI_SR = 0x050 / 4,     //          ( /W) Set SPI, Secure 
	//
	CLRSPI_SR = 0x058 / 4,     //          ( /W) Clear SPI, Secure 
	//
	IGROUPR = 0x080 / 4,       // u32[ 32] (R/W) Interrupt Group 
	ISENABLER = 0x100 / 4,     // u32[ 32] (R/W) Interrupt Set-Enable 
	ICENABLER = 0x180 / 4,     // u32[ 32] (R/W) Interrupt Clear-Enable 
	ISPENDR = 0x200 / 4,       // u32[ 32] (R/W) Interrupt Set-Pending 
	ICPENDR = 0x280 / 4,       // u32[ 32] (R/W) Interrupt Clear-Pending 
	ISACTIVER = 0x300 / 4,     // u32[ 32] (R/W) Interrupt Set-Active 
	ICACTIVER = 0x380 / 4,     // u32[ 32] (R/W) Interrupt Clear-Active 
	IPRIORITYR = 0x400 / 4,    // u32[255] (R/W) Interrupt Priority 
	//
	ITARGETSR = 0x800 / 4,     // u32[255] (R/W) Interrupt Targets 
	//
	ICFGR = 0xC00 / 4,         // u32[ 64] (R/W) Interrupt Configuration 
	IGRPMODR = 0xD00 / 4,      // u32[ 32] (R/W) Interrupt Group Modifier 
	//
	NSACR = 0xE00 / 4,         // u32[ 64] (R/W) Non-secure Access Control 
	SGIR = 0xF00 / 4,          //          ( /W) Software Generated Interrupt 
	//
	CPENDSGIR = 0xF10 / 4,     // u32[  4] (R/W) SGI Clear-Pending 
	SPENDSGIR = 0xF20 / 4,     // u32[  4] (R/W) SGI Set-Pending 
	//
	IROUTER = 0x6100 / 4,      // u64[988] (R/W) Interrupt Routing 
} GICReg;

enum class GICInterface {
	  // 0x00X ~ 0x02X
	CTLR,    // (R/W) CPU Interface Control 
	PMR,     // (R/W) Interrupt Priority Mask 
	BPR,     // (R/W) Binary Point 
	IAR,     // (R/ ) Interrupt Acknowledge 
	EOIR,    // ( /W) End Of Interrupt 
	RPR,     // (R/ ) Running Priority 
	HPPIR,   // (R/ ) Highest Priority Pending Interrupt 
	ABPR,    // (R/W) Aliased Binary Point 
	AIAR,    // (R/ ) Aliased Interrupt Acknowledge 
	AEOIR,   // ( /W) Aliased End Of Interrupt 
	AHPPIR,  // (R/ ) Aliased Highest Priority Pending Interrupt 
	STATUSR, // (R/W) Error Reporting Status (optional)
	// ...
	APR = 0x0D0 / 4,      // (R/W) [4] Active Priority 
	NSAPR = 0x0E0 / 4,    // (R/W) [4] Non-secure Active Priority 
	IIDR = 0x0FC / 4,     // (R/ ) CPU Interface Identification 
	DIR = 0x1000 / 4,     // ( /W) Deactivate Interrupt 
};



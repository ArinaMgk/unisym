
#ifndef _INC_DEV_PREDEF_FMC
#define _INC_DEV_PREDEF_FMC

#if defined(_MCU_STM32H7x)

#include "../../../cpp/MCU/_ADDRESS/ADDR-STM32.h"
#include "../../../cpp/Device/RCC/RCC"

namespace uni {

	// FMC Bank5/6 SDRAM controller register block (word offsets, relative to FMC_Bank5_6_R_BASE)
	// AKA FMC_Bank5_6_TypeDef
	enum class FMC_SDRAMReg : byte {
		SDCR1 = 0, SDCR2,        // SDRAM Control 1/2   (+0x140/+0x144)
		SDTR1, SDTR2,            // SDRAM Timing 1/2    (+0x148/+0x14C)
		SDCMR,                   // SDRAM Command Mode  (+0x150)
		SDRTR,                   // SDRAM Refresh Timer (+0x154)
		SDSR                     // SDRAM Status        (+0x158)
	};

	// ---- SDCR fields (semantic field values, written via Reference::maset/setof) ----
	enum class SDRAMBank     : byte { Bank1 = 0, Bank2 = 1 };                 // Init.SDBank, also SDCR[2]/SDTR[2] index
	enum class SDRAMWidth    : byte { Bits8 = 0, Bits16 = 1, Bits32 = 2 };    // Read/Write access width (log2 bytes)
	enum class SDRAMColumn   : byte { C8 = 0, C9 = 1, C10 = 2, C11 = 3 };     // NC
	enum class SDRAMRow      : byte { R11 = 0, R12 = 1, R13 = 2 };            // NR
	enum class SDRAMDataWidth: byte { W8 = 0, W16 = 1, W32 = 2 };             // MWID
	enum class SDRAMBankNum  : byte { Two = 0, Four = 1 };                    // NB
	enum class SDRAMCas      : byte { CL1 = 1, CL2 = 2, CL3 = 3 };            // CAS (1-based)
	enum class SDRAMClock    : byte { Disable = 0, C2 = 2, C3 = 3 };          // SDCLK (H7 field: /2 = value 2, see HAL FMC_SDRAM_CLOCK_PERIOD_2 = 0x800)
	enum class SDRAMReadPipe : byte { NoDelay = 0, One = 1, Two = 2 };        // RPIPE

	// ---- SDCMR ----
	enum class SDRAMCmd    : stduint { Normal = 0, ClockConfigEnable = 1, PALL = 2,
		AutoRefresh = 3, LoadMode = 4, SelfRefresh = 5, PowerDown = 6 };      // AKA FMC_SDRAM_CMD_*
	enum class SDRAMTarget : stduint { Bank1 = 0x10, Bank2 = 0x08, Both = 0x18 }; // CTB1/CTB2

	// ---- AKA HAL_SDRAM_StateTypeDef ----
	enum class SDRAMState : byte { Reset, Ready, Busy, Error, WriteProtected, Precharged };

	// ---- config structures (AKA FMC_SDRAM_InitTypeDef / TimingTypeDef / CommandTypeDef) ----
	struct SDRAMInit {
		SDRAMBank bank = SDRAMBank::Bank1;
		SDRAMColumn column = SDRAMColumn::C8;
		SDRAMRow row = SDRAMRow::R12;
		SDRAMDataWidth dataWidth = SDRAMDataWidth::W16;
		SDRAMBankNum bankNum = SDRAMBankNum::Four;
		SDRAMCas cas = SDRAMCas::CL3;
		bool writeProtection = false;
		SDRAMClock clockPeriod = SDRAMClock::C2;
		bool readBurst = false;
		SDRAMReadPipe readPipe = SDRAMReadPipe::NoDelay;
	};
	struct SDRAMTiming {
		byte loadToActive = 2;      // TMRD
		byte exitSelfRefresh = 7;   // TXSR
		byte selfRefreshTime = 4;   // TRAS
		byte rowCycle = 7;          // TRC
		byte writeRecovery = 2;     // TWR
		byte rpDelay = 2;           // TRP
		byte rcdDelay = 2;          // TRCD
	};
	struct SDRAMCommand {
		SDRAMCmd mode = SDRAMCmd::Normal;
		SDRAMTarget target = SDRAMTarget::Bank1;
		byte autoRefreshNumber = 1; // NRFS (1..16, register stores value-1)
		uint16 modeRegister = 0;    // MRD (13b)
	};

	// ---- field bit positions ----
	#define _SDRAM_SDCR_POS_NC      0
	#define _SDRAM_SDCR_POS_NR      2
	#define _SDRAM_SDCR_POS_MWID    4
	#define _SDRAM_SDCR_POS_NB      6
	#define _SDRAM_SDCR_POS_CAS     7
	#define _SDRAM_SDCR_POS_WP      9
	#define _SDRAM_SDCR_POS_SDCLK   10
	#define _SDRAM_SDCR_POS_RBURST  12  // SDCR1 only on H7
	#define _SDRAM_SDCR_POS_RPIPE   13  // SDCR1 only on H7

	#define _SDRAM_SDTR_POS_TMRD    0
	#define _SDRAM_SDTR_POS_TXSR    4
	#define _SDRAM_SDTR_POS_TRAS    8
	#define _SDRAM_SDTR_POS_TRC     12
	#define _SDRAM_SDTR_POS_TWR     16
	#define _SDRAM_SDTR_POS_TRP     20
	#define _SDRAM_SDTR_POS_TRCD    24

	#define _SDRAM_SDCMR_POS_MODE   0
	#define _SDRAM_SDCMR_POS_CTB2   3
	#define _SDRAM_SDCMR_POS_CTB1   4
	#define _SDRAM_SDCMR_POS_NRFS   5
	#define _SDRAM_SDCMR_POS_MRD    9

	#define _SDRAM_SDRTR_POS_CRE    0
	#define _SDRAM_SDRTR_POS_COUNT  1
	#define _SDRAM_SDRTR_POS_REIE   14

	#define _SDRAM_SDSR_POS_RE      0
	#define _SDRAM_SDSR_POS_MODES1  1
	#define _SDRAM_SDSR_POS_MODES2  3
	#define _SDRAM_SDSR_POS_BUSY    5

	#define _FMC_BCR1_FMCEN         31  // FMC global enable (BTCR[0] bit31)
	#define _FMC_BCR1_POS_BMAP      24  // 2b bank mapping (BTCR[0])

	// FMC bank mapping modes (AKA FMC_SWAPBMAP_*)
	#define _FMC_SWAPBMAP_DISABLE    0x00000000U
	#define _FMC_SWAPBMAP_SDRAM_SRAM 0x01000000U // BMAP_0
	#define _FMC_SWAPBMAP_SDRAMB2    0x02000000U // BMAP_1

	// ---- NORSRAM Bank1~4 (SRAM/PSRAM) ----
	// AKA FMC_Bank1_TypeDef (BTCR[0..7]) + FMC_Bank1E_TypeDef (BWTR[0..6])
	// register index: bank*2 = BCR, BTR = bank*2+1, BWTR uses the same bank*2 index

	enum class SRAMBank     : byte { Bank1 = 0, Bank2 = 1, Bank3 = 2, Bank4 = 3 }; // AKA FMC_NORSRAM_BANK* (register index = bank*2)
	enum class SRAMMemory   : byte { SRAM = 0, PSRAM = 1 };                 // MTYP (NOR is out of SRAM driver scope)
	enum class SRAMDataWidth: byte { W8 = 0, W16 = 1, W32 = 2 };            // MWID (external bus width)
	enum class SRAMAccess   : byte { ModeA = 0, ModeB = 1, ModeC = 2, ModeD = 3 }; // ACCMOD (asynchronous access mode)
	enum class SRAMWidth    : byte { Bits8 = 0, Bits16 = 1, Bits32 = 2 };   // Read/Write access width (log2 bytes)
	enum class SRAMState    : byte { Reset, Ready, Busy, Error, Protected };// AKA HAL_SRAM_StateTypeDef

	// ---- AKA FMC_NORSRAM_InitTypeDef (trimmed to async SRAM fields) ----
	struct SRAMInit {
		SRAMBank bank = SRAMBank::Bank1;
		bool dataAddressMux = false;            // MUXEN
		SRAMMemory memory = SRAMMemory::SRAM;   // MTYP
		SRAMDataWidth dataWidth = SRAMDataWidth::W16; // MWID
		bool writeOperation = true;             // WREN
		bool extendedMode = false;              // EXTMOD
	};
	// ---- AKA FMC_NORSRAM_TimingTypeDef (trimmed to async SRAM fields) ----
	struct SRAMTiming {
		byte addressSetup = 0;       // ADDSET
		byte addressHold = 1;        // ADDHLD
		byte dataSetup = 1;          // DATAST
		byte busTurnAround = 0;      // BUSTURN
		SRAMAccess accessMode = SRAMAccess::ModeA; // ACCMOD
	};

	// ---- BCR bit positions ----
	#define _SRAM_BCR_POS_MBKEN      0
	#define _SRAM_BCR_POS_MUXEN      1
	#define _SRAM_BCR_POS_MTYP       2   // 2b
	#define _SRAM_BCR_POS_MWID       4   // 2b
	#define _SRAM_BCR_POS_FACCEN     6
	#define _SRAM_BCR_POS_BURSTEN    8
	#define _SRAM_BCR_POS_WAITPOL    9
	#define _SRAM_BCR_POS_WAITCFG    11
	#define _SRAM_BCR_POS_WREN       12
	#define _SRAM_BCR_POS_WAITEN     13
	#define _SRAM_BCR_POS_EXTMOD     14
	#define _SRAM_BCR_POS_ASYNCWAIT  15
	#define _SRAM_BCR_POS_CPSIZE     16  // 3b
	#define _SRAM_BCR_POS_CBURSTRW   19
	#define _SRAM_BCR_POS_CCLKEN     20
	#define _SRAM_BCR_POS_WFDIS      21

	// ---- BTR/BWTR bit positions ----
	#define _SRAM_BTR_POS_ADDSET     0   // 4b
	#define _SRAM_BTR_POS_ADDHLD     4   // 4b
	#define _SRAM_BTR_POS_DATAST     8   // 8b
	#define _SRAM_BTR_POS_BUSTURN    16  // 4b
	#define _SRAM_BTR_POS_CLKDIV     20  // 4b (sync-only, always 0 for async SRAM)
	#define _SRAM_BTR_POS_DATLAT     24  // 4b (sync-only, always 0 for async SRAM)
	#define _SRAM_BTR_POS_ACCMOD     28  // 2b

}

#endif // _MCU_STM32H7x

#endif // _INC_DEV_PREDEF_FMC

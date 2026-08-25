
#ifndef _INC_DEV_PREDEF_QSPI
#define _INC_DEV_PREDEF_QSPI

#if defined(_MCU_STM32H7x)
#include "../../../cpp/MCU/_ADDRESS/ADDR-STM32.h"
#include "../../../cpp/Device/RCC/RCC"
#include "../../../cpp/Device/MDMA"
#endif

///
/// QSPI REG (H7 only; Quad SPI, AKA QUADSPI)
///
#if defined(_MCU_STM32H7x)
namespace uni {

	// AKA QUADSPI_TypeDef register map (word offsets from 0x52005000)
	enum class QSPIReg : unsigned {
		CR = 0x00 / 4,   // control
		DCR = 0x04 / 4,  // device configuration
		SR = 0x08 / 4,   // status
		FCR = 0x0C / 4,  // flag clear
		DLR = 0x10 / 4,  // data length
		CCR = 0x14 / 4,  // communication configuration
		AR = 0x18 / 4,   // address
		ABR = 0x1C / 4,  // alternate bytes
		DR = 0x20 / 4,   // data
		PSMKR = 0x24 / 4,// polling status mask
		PSMAR = 0x28 / 4,// polling status match
		PIR = 0x2C / 4,  // polling interval
		LPTR = 0x30 / 4, // low power timeout
	};

	// ---- QUADSPI_CR bits ----
	#define _QSPI_CR_POS_EN       0
	#define _QSPI_CR_POS_ABORT    1
	#define _QSPI_CR_POS_DMAEN    2
	#define _QSPI_CR_POS_TCEN     3
	#define _QSPI_CR_POS_SSHIFT   4
	#define _QSPI_CR_POS_DFM      6
	#define _QSPI_CR_POS_FSEL     7
	#define _QSPI_CR_POS_FTHRES   8  // 4b
	#define _QSPI_CR_POS_TEIE     16
	#define _QSPI_CR_POS_TCIE     17
	#define _QSPI_CR_POS_FTIE     18
	#define _QSPI_CR_POS_SMIE     19
	#define _QSPI_CR_POS_TOIE     20
	#define _QSPI_CR_POS_APMS     22
	#define _QSPI_CR_POS_PMM      23
	#define _QSPI_CR_POS_PRESCALER 24 // 8b

	// ---- QUADSPI_DCR bits ----
	#define _QSPI_DCR_POS_CKMODE  0
	#define _QSPI_DCR_POS_CSHT    8  // 3b
	#define _QSPI_DCR_POS_FSIZE   16 // 5b

	// ---- QUADSPI_SR bits ----
	#define _QSPI_SR_POS_TEF      0
	#define _QSPI_SR_POS_TCF      1
	#define _QSPI_SR_POS_FTF      2
	#define _QSPI_SR_POS_SMF      3
	#define _QSPI_SR_POS_TOF      4
	#define _QSPI_SR_POS_BUSY     5
	#define _QSPI_SR_POS_FLEVEL   8  // 6b

	// ---- QUADSPI_FCR bits ----
	#define _QSPI_FCR_POS_CTEF    0
	#define _QSPI_FCR_POS_CTCF    1
	#define _QSPI_FCR_POS_CSMF    3
	#define _QSPI_FCR_POS_CTOF    4

	// ---- QUADSPI_DLR bits ----
	#define _QSPI_DLR_POS_DL      0  // 32b

	// ---- QUADSPI_CCR bits ----
	#define _QSPI_CCR_POS_INSTRUCTION 0  // 8b
	#define _QSPI_CCR_POS_IMODE    8  // 2b
	#define _QSPI_CCR_POS_ADMODE   10 // 2b
	#define _QSPI_CCR_POS_ADSIZE   12 // 2b
	#define _QSPI_CCR_POS_ABMODE   14 // 2b
	#define _QSPI_CCR_POS_ABSIZE   16 // 2b
	#define _QSPI_CCR_POS_DCYC     18 // 5b
	#define _QSPI_CCR_POS_DMODE    24 // 2b
	#define _QSPI_CCR_POS_FMODE    26 // 2b
	#define _QSPI_CCR_POS_SIOO     28
	#define _QSPI_CCR_POS_DHHC     30
	#define _QSPI_CCR_POS_DDRM     31

	// ---- QUADSPI_PIR bits ----
	#define _QSPI_PIR_POS_INTERVAL 0  // 16b

	// ---- QUADSPI_LPTR bits ----
	#define _QSPI_LPTR_POS_TIMEOUT 0  // 16b

	// ---- error codes (AKA HAL_QSPI_ERROR_*) ----
	#define _QSPI_ERROR_NONE          0x00000000U
	#define _QSPI_ERROR_TIMEOUT       0x00000001U
	#define _QSPI_ERROR_TRANSFER      0x00000002U
	#define _QSPI_ERROR_DMA           0x00000004U
	#define _QSPI_ERROR_INVALID_PARAM 0x00000008U

	// ---- limits (AKA IS_QSPI_*) ----
	#define _QSPI_CLOCK_PRESCALER_MAX 0xFF
	#define _QSPI_FIFO_THRESHOLD_MAX  32
	#define _QSPI_FLASH_SIZE_MAX      31
	#define _QSPI_INSTRUCTION_MAX     0xFF
	#define _QSPI_DUMMY_CYCLES_MAX    31
	#define _QSPI_INTERVAL_MAX        0xFFFF
	#define _QSPI_TIMEOUT_PERIOD_MAX  0xFFFF

	// ===== enum class constants (no inner prefix/suffix) =====

	// AKA QSPI_SampleShifting
	enum class QSPISampleShift : unsigned {
		None = 0,          // no shift
		HalfCycle = 1,     // 1/2 clock cycle shift (CR.SSHIFT)
	};

	// AKA QSPI_ChipSelectHighTime (DCR.CSHT)
	enum class QSPICSHighTime : unsigned {
		Cycle1 = 0, Cycle2 = 1, Cycle3 = 2, Cycle4 = 3,
		Cycle5 = 4, Cycle6 = 5, Cycle7 = 6, Cycle8 = 7,
	};

	// AKA QSPI_ClockMode (DCR.CKMODE)
	enum class QSPIClockMode : unsigned {
		Mode0 = 0,   // clk low while nCS released
		Mode3 = 1,   // clk high while nCS released
	};

	// AKA QSPI_Flash_Select (CR.FSEL)
	enum class QSPIFlashSelect : unsigned {
		Flash1 = 0,
		Flash2 = 1,
	};

	// AKA QSPI_DualFlash_Mode (CR.DFM)
	enum class QSPIDualFlash : unsigned {
		Disable = 0,
		Enable = 1,
	};

	// AKA QSPI_AddressSize (CCR.ADSIZE)
	enum class QSPIAddressSize : unsigned {
		_8 = 0, _16 = 1, _24 = 2, _32 = 3,
	};

	// AKA QSPI_AlternateBytesSize (CCR.ABSIZE)
	enum class QSPIAlternateBytesSize : unsigned {
		_8 = 0, _16 = 1, _24 = 2, _32 = 3,
	};

	// AKA QSPI_InstructionMode (CCR.IMODE)
	enum class QSPIInstrMode : unsigned {
		None = 0, Line1 = 1, Line2 = 2, Line4 = 3,
	};

	// AKA QSPI_AddressMode (CCR.ADMODE)
	enum class QSPIAddrMode : unsigned {
		None = 0, Line1 = 1, Line2 = 2, Line4 = 3,
	};

	// AKA QSPI_AlternateBytesMode (CCR.ABMODE)
	enum class QSPIAlternateMode : unsigned {
		None = 0, Line1 = 1, Line2 = 2, Line4 = 3,
	};

	// AKA QSPI_DataMode (CCR.DMODE)
	enum class QSPIDataMode : unsigned {
		None = 0, Line1 = 1, Line2 = 2, Line4 = 3,
	};

	// AKA QSPI_DdrMode (CCR.DDRM)
	enum class QSPIDdrMode : unsigned {
		Disable = 0,
		Enable = 1,
	};

	// AKA QSPI_DdrHoldHalfCycle (CCR.DHHC)
	enum class QSPIDdrHold : unsigned {
		AnalogDelay = 0,
		HalfClockDelay = 1,
	};

	// AKA QSPI_SIOOMode (CCR.SIOO)
	enum class QSPISiooMode : unsigned {
		EveryCmd = 0,
		FirstCmd = 1,
	};

	// AKA QSPI_MatchMode (CR.PMM)
	enum class QSPIMatchMode : unsigned {
		And = 0,
		Or = 1,
	};

	// AKA QSPI_AutomaticStop (CR.APMS)
	enum class QSPIAutoStop : unsigned {
		Disable = 0,
		Enable = 1,
	};

	// AKA QSPI_TimeOutActivation (CR.TCEN)
	enum class QSPITimeoutActivation : unsigned {
		Disable = 0,
		Enable = 1,
	};

	// AKA HAL_QSPI_StateTypeDef
	enum class QSPIState : byte {
		Reset = 0x00,
		Ready = 0x01,
		Busy = 0x02,
		BusyIndirectTx = 0x12,
		BusyIndirectRx = 0x22,
		BusyAutoPolling = 0x42,
		BusyMemMapped = 0x82,
		Abort = 0x08,
		Error = 0x04,
	};

}
#endif // _MCU_STM32H7x

#endif

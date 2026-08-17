
#ifndef _INC_DEV_PREDEF_I2S
#define _INC_DEV_PREDEF_I2S

#if defined(_MCU_STM32)
#include "../../../cpp/MCU/_ADDRESS/ADDR-STM32.h"
#include "../../../cpp/Device/RCC/RCC"
#endif

///
/// I2S config enums (modern SPI/I2S IP: STM32H7 / STM32MP13)
///
#if defined(_MCU_STM32)
namespace uni {

	// AKA I2S_MODE_* (abstract: direction is selected by the transfer method,
	//                 mapped to I2SCFG[2:0] inside the driver)
	enum class I2SMode_E : byte {
		Slave = 0,
		Master = 1,
	};
	// AKA I2S_STANDARD_* (I2SCFGR.I2SSTD + PCMSYNC; encoding identical on H7 & MP13)
	enum class I2SStandard_E : stduint {
		Philips = 0x00000000,
		MSB = 0x00000010,
		LSB = 0x00000020,
		PCM_Short = 0x00000030,
		PCM_Long = 0x000000B0,
	};
	// AKA I2S_DATAFORMAT_* (abstract: H7 and MP13 use different register encodings,
	//                       see canMode() platform mapping)
	enum class I2SDataFormat_E : byte {
		Bits16 = 0,
		Bits16Extended = 1,
		Bits24 = 2,
		Bits32 = 3,
	};
	// AKA I2S_CPOL_* (I2SCFGR.CKPOL bit11, identical)
	enum class I2SClockPolarity_E : stduint {
		Low = 0x00000000,
		High = 0x00000800,
	};
	// AKA I2S_FIRSTBIT_* (CFG2.LSBFRST bit23, identical)
	enum class I2SFirstBit_E : stduint {
		MSB = 0x00000000,
		LSB = 0x00800000,
	};
	// AKA I2S_WS_INVERSION_* (abstract: WSINV bit is H7=12 / MP13=13)
	enum class I2SWSInversion_E : byte {
		Disable = 0,
		Enable = 1,
	};
	// AKA I2S_DATA_24BIT_ALIGNMENT_* (abstract: DATFMT is H7 2-bit@12 / MP13 1-bit@14)
	enum class I2SData24BitAlignment_E : byte {
		Right = 0,
		Left = 1,
	};
	// AKA I2S_MASTER_KEEP_IO_STATE_* (CFG2.AFCNTR bit31, identical)
	enum class I2SMasterKeepIO_E : stduint {
		Disable = 0x00000000,
		Enable = 0x80000000,
	};
	// AKA I2S_MCLKOUTPUT_* (I2SCFGR.MCKOE bit25, identical)
	enum class I2SMCLKOutput_E : stduint {
		Disable = 0x00000000,
		Enable = 0x02000000,
	};
#if defined(_MCU_STM32H7x)
	// AKA I2S_IO_SWAP_* (CFG2.IOSWP bit15, H7 only)
	enum class I2SIOSwap_E : stduint {
		Disable = 0x00000000,
		Enable = 0x00008000,
	};
	// AKA I2S_FIFO_THRESHOLD_* (CFG1.FTHLV, H7 only)
	enum class I2SFifoThreshold_E : stduint {
		Data1 = 0x00000000, Data2 = 0x00000020, Data3 = 0x00000040, Data4 = 0x00000060,
		Data5 = 0x00000080, Data6 = 0x000000A0, Data7 = 0x000000C0, Data8 = 0x000000E0,
	};
	// AKA I2S_SLAVE_EXTEND_FRE_DETECTION_* (abstract: I2SCFGR.FIXCH, H7 only)
	enum class I2SSlaveExtendFREDetection_E : byte {
		Disable = 0,
		Enable = 1,
	};
#endif

}
#endif // I2S REG

// I2S error codes (unisym unified encoding; NOT copied from either HAL,
// because HAL_I2S_ERROR_* bit assignments differ between H7 and MP13)
// AKA HAL_I2S_ERROR_*
#define _I2S_ERR_NONE      0x00
#define _I2S_ERR_UDR       0x01
#define _I2S_ERR_OVR       0x02
#define _I2S_ERR_FRE       0x04
#define _I2S_ERR_DMA       0x08
#define _I2S_ERR_TIMEOUT   0x10
#define _I2S_ERR_PRESCALER 0x20
#define _I2S_ERR_NOT_SUPPORTED 0x40
#define _I2S_ERR_NO_OGT    0x80
#define _I2S_TIMEOUT_VALUE 0x1FFFFFFUL

#endif

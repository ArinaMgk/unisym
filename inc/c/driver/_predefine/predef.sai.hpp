
#ifndef _INC_DEV_PREDEF_SAI
#define _INC_DEV_PREDEF_SAI

#if defined(_MCU_STM32)
#include "../../../cpp/MCU/_ADDRESS/ADDR-STM32.h"
#include "../../../cpp/Device/RCC/RCC"
#endif

///
/// SAI config enums (Serial Audio Interface: STM32H7 / STM32MP13, identical IP)
///
#if defined(_MCU_STM32)
namespace uni {

	// AKA SAI_AUDIOMODE_* (CR1.MODE[1:0])
	enum class SAIAudioMode_E : stduint {
		MasterTx = 0x00000000,
		MasterRx = 0x00000001,
		SlaveTx = 0x00000002,
		SlaveRx = 0x00000003,
	};
	// AKA SAI_PROTOCOL_* (abstract: Free/Spdif/Ac97 are CR1.PRTCFG values;
	//                        I2S/Msb/Lsb/Pcm trigger InitProtocol auto frame/slot config)
	enum class SAIProtocol_E : byte {
		Free = 0, Spdif = 1, Ac97 = 2,
		I2S = 3, MsbJustified = 4, LsbJustified = 5,
		PcmLong = 6, PcmShort = 7,
	};
	// AKA SAI_DATASIZE_* (CR1.DS[2:0])
	enum class SAIDataSize_E : stduint {
		Data8 = 0x00000040, Data10 = 0x00000060, Data16 = 0x00000080,
		Data20 = 0x000000A0, Data24 = 0x000000C0, Data32 = 0x000000E0,
	};
	// AKA SAI_FIRSTBIT_* (CR1.LSBFIRST)
	enum class SAIFirstBit_E : stduint {
		MSB = 0x00000000,
		LSB = 0x00000100,
	};
	// AKA SAI_CLOCKSTROBING_* (abstract: CKSTR computed per AudioMode in canMode)
	enum class SAIClockStrobing_E : byte {
		FallingEdge = 0,
		RisingEdge = 1,
	};
	// AKA SAI_SYNCHRONOUS_* (abstract: mapped to CR1.SYNCEN + GCR in canMode)
	enum class SAISynchro_E : byte {
		Asynchronous = 0,
		Synchronous = 1,
		SynchronousExtSai1 = 2,
		SynchronousExtSai2 = 3,
	};
	// AKA SAI_SYNCEXT_* (abstract: mapped to GCR.SYNCOUT in canMode)
	enum class SAISynchroExt_E : byte {
		Disable = 0,
		OutBlockA = 1,
		OutBlockB = 2,
	};
	// AKA SAI_OUTPUTDRIVE_* (CR1.OUTDRIV)
	enum class SAIOutputDrive_E : stduint {
		Disable = 0x00000000,
		Enable = 0x00002000,
	};
	// AKA SAI_MASTERDIVIDER_* (CR1.NOMCK)
	enum class SAINoDivider_E : stduint {
		Divider = 0x00000000,
		NoDivider = 0x00080000,
	};
	// AKA SAI_FIFOTHRESHOLD_* (CR2.FTH[2:0])
	enum class SAIFifoThreshold_E : stduint {
		Empty = 0x00000000, Quarter = 0x00000001, Half = 0x00000002,
		ThreeQuarter = 0x00000003, Full = 0x00000004,
	};
	// AKA SAI_MONOMODE (CR1.MONO)
	enum class SAIMonoStereo_E : stduint {
		Stereo = 0x00000000,
		Mono = 0x00001000,
	};
	// AKA SAI_COMPANDINGMODE_* (CR2.COMP)
	enum class SAICompanding_E : stduint {
		None = 0x00000000,
		UlLaw = 0x00008000,
		ALaw = 0x0000C000,
	};
	// AKA SAI_TRISTATE_* (CR2.TRIS)
	enum class SAITriState_E : stduint {
		Disable = 0x00000000,
		Enable = 0x00000010,
	};
	// AKA SAI_FS_DEFINITION_* (FRCR.FSDEF)
	enum class SAIFSDefinition_E : stduint {
		StartFrame = 0x00000000,
		ChannelIdentification = 0x00010000,
	};
	// AKA SAI_FS_POLARITY_* (FRCR.FSPOL)
	enum class SAIFSPolarity_E : stduint {
		ActiveLow = 0x00000000,
		ActiveHigh = 0x00020000,
	};
	// AKA SAI_FS_OFFSET_* (FRCR.FSOFF)
	enum class SAIFSOffset_E : stduint {
		FirstBit = 0x00000000,
		BeforeFirstBit = 0x00040000,
	};
	// AKA SAI_SLOTSIZE_* (SLOTR.SLOTSZ[1:0])
	enum class SAISlotSize_E : stduint {
		DataSize = 0x00000000,
		B16 = 0x00000040,
		B32 = 0x00000080,
	};
#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
	// AKA SAI_MCK_OVERSAMPLING_* (CR1.OSR)
	enum class SAIMckOverSampling_E : stduint {
		Disable = 0x00000000,
		Enable = 0x04000000,
	};
	// AKA SAI_PDM_CLOCK*_ENABLE (PDMCR.CKENx)
	enum class SAIClockEnable_E : stduint {
		Disable = 0x00000000,
		Clock1 = 0x00000100, Clock2 = 0x00000200,
		Clock3 = 0x00000400, Clock4 = 0x00000800,
	};
#endif
#if defined(_MPU_STM32MP13)
	// AKA SAI_MCK_OUTPUT_* (CR1.MCKEN, MP13 only)
	enum class SAIMckOutput_E : stduint {
		Disable = 0x00000000,
		Enable = 0x08000000,
	};
#endif

	// block register offsets (relative to block base: SAI_BASE + 0x004(A) / + 0x024(B))
	enum class SAIReg {
		CR1 = 0x00, CR2 = 0x04, FRCR = 0x08, SLOTR = 0x0C,
		IMR = 0x10, SR = 0x14, CLRFR = 0x18, DR = 0x1C,
	};
	// peripheral-level register offsets (relative to SAI_BASE)
	enum class SAIGlobalReg {
		GCR = 0x00, PDMCR = 0x44, PDMDLY = 0x48,
	};

}
#endif // SAI REG

// SAI error codes (unisym unified encoding; NOT copied from HAL bit layout)
// AKA HAL_SAI_ERROR_*
#define _SAI_ERR_NONE      0x000
#define _SAI_ERR_OVR       0x001
#define _SAI_ERR_UDR       0x002
#define _SAI_ERR_AFSDET    0x004
#define _SAI_ERR_LFSDET    0x008
#define _SAI_ERR_CNREADY   0x010
#define _SAI_ERR_WCKCFG    0x020
#define _SAI_ERR_TIMEOUT   0x040
#define _SAI_ERR_DMA       0x080
#define _SAI_ERR_NOT_SUPPORTED 0x100
#define _SAI_TIMEOUT_VALUE 0x1FFFFFFUL

#endif


#ifndef _INC_DEV_PREDEF_SPI
#define _INC_DEV_PREDEF_SPI

#if defined(_MCU_STM32)
#include "../../../cpp/MCU/_ADDRESS/ADDR-STM32.h"
#include "../../../cpp/Device/RCC/RCC"
#endif

///
/// SPI REG (modern SPI IP: STM32H7 / STM32MP13)
///
#if defined(_MCU_STM32)
namespace uni {

	enum class SPIReg {
	#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
		CR1 = 0x00,
		CR2 = 0x04,
		CFG1 = 0x08,
		CFG2 = 0x0C,
		IER = 0x10,
		SR = 0x14,
		IFCR = 0x18,
		TXDR = 0x20,
		RXDR = 0x30,
		CRCPOLY = 0x40,
		TXCRC = 0x44,
		RXCRC = 0x48,
		UDRDR = 0x4C,
		I2SCFGR = 0x50,
	#endif
	};

	// AKA SPI_Mode (CFG2.MASTER)
	enum class SPIMode_E : stduint {
		Slave = 0x00000000,
		Master = 0x00400000,
	};
	// AKA SPI_Direction (CFG2.COMM)
	enum class SPIDirection_E : stduint {
		TwoLine = 0x00000000,       // full-duplex
		TwoLineTxOnly = 0x00020000, // 2 lines, transmit only
		TwoLineRxOnly = 0x00040000, // 2 lines, receive only
		OneLine = 0x00060000,       // 1 line (half-duplex)
	};
	// AKA SPI_Data_Size (CFG1.DSIZE = bits - 1)
	enum class SPIDataSize_E : stduint {
		Bits4 = 3, Bits5, Bits6, Bits7, Bits8, Bits9, Bits10, Bits11,
		Bits12, Bits13, Bits14, Bits15, Bits16, Bits17, Bits18, Bits19,
		Bits20, Bits21, Bits22, Bits23, Bits24, Bits25, Bits26, Bits27,
		Bits28, Bits29, Bits30, Bits31, Bits32,
	};
	// AKA SPI_Clock_Polarity (CFG2.CPOL)
	enum class SPIClockPolarity_E : stduint {
		Low = 0x00000000,
		High = 0x02000000,
	};
	// AKA SPI_Clock_Phase (CFG2.CPHA)
	enum class SPIClockPhase_E : stduint {
		Edge1 = 0x00000000,
		Edge2 = 0x01000000,
	};
	// AKA SPI_Slave_Select_Management (CFG2.SSM/SSOE)
	enum class SPINss_E : stduint {
		HardInput = 0x00000000,
		HardOutput = 0x20000000,
		Soft = 0x04000000,
	};
	// AKA SPI_BaudRate_Prescaler (CFG1.MBR)
	enum class SPIBaudRate_E : stduint {
		Div2 = 0x00000000, Div4 = 0x10000000, Div8 = 0x20000000,
		Div16 = 0x30000000, Div32 = 0x40000000, Div64 = 0x50000000,
		Div128 = 0x60000000, Div256 = 0x70000000,
	};
	// AKA SPI_MSB_LSB_Transmission (CFG2.LSBFRST)
	enum class SPIFirstBit_E : stduint {
		MSB = 0x00000000,
		LSB = 0x00800000,
	};
	// AKA SPI_TI_Mode (CFG2.SP_0)
	enum class SPITIMode_E : stduint {
		Disable = 0x00000000,
		Enable = 0x00080000,
	};
	// AKA SPI_CRC_Calculation (CFG1.CRCEN)
	enum class SPICrc_E : stduint {
		Disable = 0x00000000,
		Enable = 0x00400000,
	};
	// AKA SPI_CRC_length (CFG1.CRCSIZE)
	enum class SPICrcLength_E : stduint {
		DataSize = 0x00000000,
		Bits4 = 0x00030000, Bits5, Bits6, Bits7, Bits8, Bits9, Bits10, Bits11,
		Bits12, Bits13, Bits14, Bits15, Bits16, Bits17, Bits18, Bits19,
		Bits20, Bits21, Bits22, Bits23, Bits24, Bits25, Bits26, Bits27,
		Bits28, Bits29, Bits30, Bits31, Bits32,
	};
	// AKA SPI_CRC_Calculation_Initialization_Pattern
	enum class SPICrcInitPattern_E : stduint {
		AllZero = 0x00000000,
		AllOne = 0x00000001,
	};
	// AKA SPI_NSSP_Mode (CFG2.SSOM)
	enum class SPINssPulse_E : stduint {
		Disable = 0x00000000,
		Enable = 0x40000000,
	};
	// AKA SPI_NSS_Polarity (CFG2.SSIOP)
	enum class SPINssPolarity_E : stduint {
		Low = 0x00000000,
		High = 0x10000000,
	};
	// AKA SPI_Fifo_Threshold (CFG1.FTHLV)
	enum class SPIFifoThreshold_E : stduint {
		Data1 = 0x00000000, Data2 = 0x00000020, Data3 = 0x00000040, Data4 = 0x00000060,
		Data5 = 0x00000080, Data6 = 0x000000A0, Data7 = 0x000000C0, Data8 = 0x000000E0,
		Data9 = 0x00000100, Data10 = 0x00000120, Data11 = 0x00000140, Data12 = 0x00000160,
		Data13 = 0x00000180, Data14 = 0x000001A0, Data15 = 0x000001C0, Data16 = 0x000001E0,
	};
	// AKA SPI_Master_SS_Idleness (CFG2.MSSI)
	enum class SPIMasterSSIdle_E : stduint {
		Cycle0, Cycle1, Cycle2, Cycle3, Cycle4, Cycle5, Cycle6, Cycle7,
		Cycle8, Cycle9, Cycle10, Cycle11, Cycle12, Cycle13, Cycle14, Cycle15,
	};
	// AKA SPI_Master_InterData_Idleness (CFG2.MIDI)
	enum class SPIMasterInterDataIdle_E : stduint {
		Cycle0 = 0x00000000, Cycle1 = 0x00000010, Cycle2 = 0x00000020, Cycle3 = 0x00000030,
		Cycle4 = 0x00000040, Cycle5 = 0x00000050, Cycle6 = 0x00000060, Cycle7 = 0x00000070,
		Cycle8 = 0x00000080, Cycle9 = 0x00000090, Cycle10 = 0x000000A0, Cycle11 = 0x000000B0,
		Cycle12 = 0x000000C0, Cycle13 = 0x000000D0, Cycle14 = 0x000000E0, Cycle15 = 0x000000F0,
	};
	// AKA SPI_Master_RX_AutoSuspend (CR1.MASRX)
	enum class SPIMasterRxAutoSusp_E : stduint {
		Disable = 0x00000000,
		Enable = 0x00000100,
	};
	// AKA SPI_Master_Keep_IO_State (CFG2.AFCNTR)
	enum class SPIMasterKeepIO_E : stduint {
		Disable = 0x00000000,
		Enable = 0x80000000,
	};
	// AKA SPI_IO_Swap (CFG2.IOSWP)
	enum class SPIIOSwap_E : stduint {
		Disable = 0x00000000,
		Enable = 0x00008000,
	};
	// AKA SPI_Underrun_Detection (CFG1.UDRDET)
	enum class SPIUnderrunDetect_E : stduint {
		BeginDataFrame = 0x00000000,
		EndDataFrame = 0x00000800,
		BeginActiveNss = 0x00001000,
	};
	// AKA SPI_Underrun_Behaviour (CFG1.UDRCFG)
	enum class SPIUnderrunBehav_E : stduint {
		RegisterPattern = 0x00000000,
		LastReceived = 0x00000200,
		LastTransmitted = 0x00000400,
	};

}

#endif // SPI REG

#endif

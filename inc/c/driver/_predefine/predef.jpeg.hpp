
#ifndef _INC_DEV_PREDEF_JPEG
#define _INC_DEV_PREDEF_JPEG

#if defined(_MCU_STM32H7x)
#include "../../../cpp/MCU/_ADDRESS/ADDR-STM32.h"
#include "../../../cpp/Device/RCC/RCC"
#include "../../../cpp/Device/MDMA"
#endif

///
/// JPEG REG (H7 only; JPEG codec, AKA JPGDEC)
///
#if defined(_MCU_STM32H7x)
namespace uni {

	// AKA JPEG_TypeDef register map (word offsets from _JPEG_ADDR)
	enum class JPEGReg : unsigned {
		CONFR0 = 0x00 / 4, // 0x00: start/stop + partial MCU
		CONFR1 = 0x04 / 4, // 0x04: NF/DE/COLORSPACE/NS/HDR/YSIZE
		CONFR2 = 0x08 / 4, // 0x08: NMCU
		CONFR3 = 0x0C / 4, // 0x0C: XSIZE
		CONFR4 = 0x10 / 4, // 0x10: component 0 (HD/HA/QT/NB/VSF/HSF)
		CONFR5 = 0x14 / 4, // 0x14: component 1
		CONFR6 = 0x18 / 4, // 0x18: component 2
		CONFR7 = 0x1C / 4, // 0x1C: component 3
		// 0x20-0x2C reserved
		CR  = 0x30 / 4,    // 0x30: control
		SR  = 0x34 / 4,    // 0x34: status
		CFR = 0x38 / 4,    // 0x38: clear flag
		// 0x3C reserved
		DIR = 0x40 / 4,    // 0x40: data input register (FIFO)
		DOR = 0x44 / 4,    // 0x44: data output register (FIFO)
		// 0x48-0x4C reserved
		QMEM0 = 0x50 / 4,  // 0x50: quantization table 0 (16 words)
		QMEM1 = 0x90 / 4,  // 0x90: quantization table 1
		QMEM2 = 0xD0 / 4,  // 0xD0: quantization table 2
		QMEM3 = 0x110 / 4, // 0x110: quantization table 3
		HUFFMIN  = 0x150 / 4, // 0x150: Huffman Min (16 words)
		HUFFBASE = 0x190 / 4, // 0x190: Huffman Base (32 words)
		HUFFSYMB = 0x210 / 4, // 0x210: Huffman Symb (84 words)
		DHTMEM   = 0x360 / 4, // 0x360: DHT memory (103 words)
		// 0x4FC reserved
		HUFFENC_AC0 = 0x500 / 4, // 0x500: encoder AC Huffman table 0 (88 words)
		HUFFENC_AC1 = 0x660 / 4, // 0x660: encoder AC Huffman table 1 (88 words)
		HUFFENC_DC0 = 0x7C0 / 4, // 0x7C0: encoder DC Huffman table 0 (8 words)
		HUFFENC_DC1 = 0x7E0 / 4, // 0x7E0: encoder DC Huffman table 1 (8 words)
	};

	// AKA JPEG_ColorSpace (CONFR1.COLORSPACE[5:4])
	enum class JPEGColorSpace : unsigned {
		Gray = 0,  // 1 component
		YCbCr = 1, // 3 components (default)
		RGB = 2,   // 3 components, 4:4:4 only
		CMYK = 3,  // 4 components
	};

	// AKA HAL_JPEG_STATETypeDef
	enum class JPEGState : byte {
		Reset = 0,
		Ready,
		Busy,
		BusyEncoding,
		BusyDecoding,
		Timeout,
		Error,
	};

	// AKA HAL_JPEG_ERROR_* (bitmask)
	#define _JPEG_ERROR_NONE        0x00000000U
	#define _JPEG_ERROR_HUFF_TABLE  0x00000001U
	#define _JPEG_ERROR_QUANT_TABLE 0x00000002U
	#define _JPEG_ERROR_DMA         0x00000004U
	#define _JPEG_ERROR_TIMEOUT     0x00000008U

	// AKA JPEG_PAUSE_RESUME_* selection
	#define JPEG_PAUSE_INPUT         0x00000001U
	#define JPEG_PAUSE_OUTPUT        0x00000002U
	#define JPEG_PAUSE_INPUT_OUTPUT  0x00000003U

	// JPEG_CR bits
	#define _JPEG_CR_POS_JCEN   0
	#define _JPEG_CR_POS_IFTIE  1  // input FIFO threshold IT enable
	#define _JPEG_CR_POS_IFNFIE 2  // input FIFO not full IT enable
	#define _JPEG_CR_POS_OFTIE  3  // output FIFO threshold IT enable
	#define _JPEG_CR_POS_OFNEIE 4  // output FIFO not empty IT enable
	#define _JPEG_CR_POS_EOCIE  5  // end of conversion IT enable
	#define _JPEG_CR_POS_HPDIE  6  // header parsing done IT enable
	#define _JPEG_CR_POS_IFF    13 // input FIFO flush
	#define _JPEG_CR_POS_OFF    14 // output FIFO flush

	// JPEG_SR bits
	#define _JPEG_SR_POS_IFTF  1  // input FIFO below threshold
	#define _JPEG_SR_POS_IFNFF 2  // input FIFO not full
	#define _JPEG_SR_POS_OFTF  3  // output FIFO reached threshold
	#define _JPEG_SR_POS_OFNEF 4  // output FIFO not empty
	#define _JPEG_SR_POS_EOCF  5  // end of conversion
	#define _JPEG_SR_POS_HPDF  6  // header parsing done
	#define _JPEG_SR_POS_COF   7  // codec operation ongoing

	// JPEG_CFR bits
	#define _JPEG_CFR_POS_CEOCF 4 // clear EOCF
	#define _JPEG_CFR_POS_CHPDF 5 // clear HPDF

	// JPEG_CONFR1 bits
	#define _JPEG_CONFR1_POS_NF   0 // 2b number of components
	#define _JPEG_CONFR1_POS_DE   3 // decoding enable
	#define _JPEG_CONFR1_POS_COLORSPACE 4 // 2b
	#define _JPEG_CONFR1_POS_NS   6 // 2b number of components in scan
	#define _JPEG_CONFR1_POS_HDR  8 // header processing on/off
	#define _JPEG_CONFR1_POS_YSIZE 16 // 16b image height

	// JPEG_CONFR4-7 component config bits (identical layout)
	#define _JPEG_CONFRx_POS_HD  0
	#define _JPEG_CONFRx_POS_HA  1
	#define _JPEG_CONFRx_POS_QT  2 // 2b
	#define _JPEG_CONFRx_POS_NB  4 // 4b
	#define _JPEG_CONFRx_POS_VSF 8 // 4b
	#define _JPEG_CONFRx_POS_HSF 12 // 4b

	// ---- internal context bits (AKA JPEG_CONTEXT_*) ----
	#define _JPEG_CONTEXT_ENCODE        0x00000001U
	#define _JPEG_CONTEXT_DECODE        0x00000002U
	#define _JPEG_CONTEXT_OPERATION_MASK 0x00000003U
	#define _JPEG_CONTEXT_POLLING       0x00000004U
	#define _JPEG_CONTEXT_IT            0x00000008U
	#define _JPEG_CONTEXT_DMA           0x0000000CU
	#define _JPEG_CONTEXT_METHOD_MASK   0x0000000CU
	#define _JPEG_CONTEXT_CONF_ENCODING 0x00000100U
	#define _JPEG_CONTEXT_PAUSE_INPUT   0x00001000U
	#define _JPEG_CONTEXT_PAUSE_OUTPUT  0x00002000U
	#define _JPEG_CONTEXT_CUSTOM_TABLES 0x00004000U
	#define _JPEG_CONTEXT_ENDING_DMA    0x00008000U

}
#endif // _MCU_STM32H7x

#endif

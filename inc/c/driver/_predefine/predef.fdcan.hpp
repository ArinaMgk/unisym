// ASCII CPL TAB4 LF
// Docutitle: [Device] FDCAN
// Developer: @dosconio
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#ifndef _INC_DEV_PREDEF_FDCAN
#define _INC_DEV_PREDEF_FDCAN

#if defined(_MCU_STM32H7x)
#include "../../../cpp/MCU/_ADDRESS/ADDR-STM32.h"
#include "../../../cpp/Device/RCC/RCC"
#include "../../../cpp/interrupt"

#define FDCAN1_BASE          (D2_APB1PERIPH_BASE + 0xA000)
#define FDCAN2_BASE          (D2_APB1PERIPH_BASE + 0xA400)
#define FDCAN_CCU_BASE       (D2_APB1PERIPH_BASE + 0xA800)
#define SRAMCAN_BASE         (D2_APB1PERIPH_BASE + 0xAC00)
#define FDCAN_RAM_END        0x4000B5FC // Last address of the Message RAM

/// FDCAN REG
namespace uni {
	namespace FdcanReg {
		typedef enum {
			// : FDCAN core registers (offset 0x000-0x0FC)
			CREL = 0x00, ENDN = 0x04, _RSV1 = 0x08, DBTP = 0x0C,
			TEST = 0x10, RWD = 0x14, CCCR = 0x18, NBTP = 0x1C,
			TSCC = 0x20, TSCV = 0x24, TOCC = 0x28, TOCV = 0x2C,
			_RSV2 = 0x30, _RSV3 = 0x34, _RSV4 = 0x38, _RSV5 = 0x3C,
			ECR = 0x40, PSR = 0x44, TDCR = 0x48, _RSV6 = 0x4C,
			IR = 0x50, IE = 0x54, ILS = 0x58, ILE = 0x5C,
			_RSV7 = 0x60, _RSV8 = 0x64, _RSV9 = 0x68, _RSV10 = 0x6C,
			_RSV11 = 0x70, _RSV12 = 0x74, _RSV13 = 0x78, _RSV14 = 0x7C,
			GFC = 0x80, SIDFC = 0x84, XIDFC = 0x88, _RSV15 = 0x8C,
			XIDAM = 0x90, HPMS = 0x94, NDAT1 = 0x98, NDAT2 = 0x9C,
			RXF0C = 0xA0, RXF0S = 0xA4, RXF0A = 0xA8, RXBC = 0xAC,
			RXF1C = 0xB0, RXF1S = 0xB4, RXF1A = 0xB8, RXESC = 0xBC,
			TXBC = 0xC0, TXFQS = 0xC4, TXESC = 0xC8, TXBRP = 0xCC,
			TXBAR = 0xD0, TXBCR = 0xD4, TXBTO = 0xD8, TXBCF = 0xDC,
			TXBTIE = 0xE0, TXBCIE = 0xE4, _RSV16 = 0xE8, _RSV17 = 0xEC,
			TXEFC = 0xF0, TXEFS = 0xF4, TXEFA = 0xF8, _RSV18 = 0xFC,
			// : TT registers (offset 0x100-0x300, base + 0x100)
			TTTMC = 0x100, TTRMC = 0x104, TTOCF = 0x108, TTMLM = 0x10C,
			TURCF = 0x110, TTOCN = 0x114, TTGTP = 0x118, TTTMK = 0x11C,
			TTIR = 0x120, TTIE = 0x124, TTILS = 0x128, TTOST = 0x12C,
			TURNA = 0x130, TTLGT = 0x134, TTCTC = 0x138, TTCPT = 0x13C,
			TTCSM = 0x140, _RSV19 = 0x144, _RSV20 = 0x148, _RSV21 = 0x14C,
			TTTS = 0x300,
		} FdcanRegType;
	}
	namespace FdcanCcuReg {
		typedef enum {
			CREL = 0x00, CCFG = 0x04, CSTAT = 0x08, CWD = 0x0C,
			IR = 0x10, IE = 0x14,
		} FdcanCcuRegType;
	}

	// ---- bit positions (FDCAN core) ----
	#define _FDCAN_CCCR_POSI_INIT   0
	#define _FDCAN_CCCR_POSI_CCE    1
	#define _FDCAN_CCCR_POSI_ASM    2
	#define _FDCAN_CCCR_POSI_CSA    3
	#define _FDCAN_CCCR_POSI_CSR    4
	#define _FDCAN_CCCR_POSI_MON    5
	#define _FDCAN_CCCR_POSI_DAR    6
	#define _FDCAN_CCCR_POSI_TEST   7
	#define _FDCAN_CCCR_POSI_FDOE   8
	#define _FDCAN_CCCR_POSI_BRSE   9
	#define _FDCAN_CCCR_POSI_PXHD   12
	#define _FDCAN_CCCR_POSI_EFBI   13
	#define _FDCAN_CCCR_POSI_TXP    14
	#define _FDCAN_CCCR_POSI_NISO   15
	#define _FDCAN_TEST_POSI_LBCK   4
	#define _FDCAN_TEST_POSI_TX     5
	#define _FDCAN_TEST_POSI_RX     7
	#define _FDCAN_RWD_POSI_WDC     0
	#define _FDCAN_RWD_POSI_WDV     4
	#define _FDCAN_NBTP_POSI_NTSEG2 0
	#define _FDCAN_NBTP_POSI_NTSEG1 8
	#define _FDCAN_NBTP_POSI_NBRP   16
	#define _FDCAN_NBTP_POSI_NSJW   25
	#define _FDCAN_DBTP_POSI_DSJW   0
	#define _FDCAN_DBTP_POSI_DTSEG2 4
	#define _FDCAN_DBTP_POSI_DTSEG1 8
	#define _FDCAN_DBTP_POSI_DBRP   16
	#define _FDCAN_DBTP_POSI_TDC    9
	#define _FDCAN_TSCC_POSI_TSS    0
	#define _FDCAN_TSCC_POSI_TCP    16
	#define _FDCAN_TOCC_POSI_ETOC   0
	#define _FDCAN_TOCC_POSI_TOS    1
	#define _FDCAN_TOCC_POSI_TOP    16
	#define _FDCAN_ECR_POSI_TEC     0
	#define _FDCAN_ECR_POSI_REC     8
	#define _FDCAN_ECR_POSI_RP      15
	#define _FDCAN_ECR_POSI_CEL     16
	#define _FDCAN_PSR_POSI_LEC     0
	#define _FDCAN_PSR_POSI_ACT     3
	#define _FDCAN_PSR_POSI_EP      5
	#define _FDCAN_PSR_POSI_EW      6
	#define _FDCAN_PSR_POSI_BO      7
	#define _FDCAN_PSR_POSI_DLEC    8
	#define _FDCAN_PSR_POSI_RESI    11
	#define _FDCAN_PSR_POSI_RBRS    12
	#define _FDCAN_PSR_POSI_REDL    13
	#define _FDCAN_PSR_POSI_PXE     14
	#define _FDCAN_PSR_POSI_TDCV    16
	#define _FDCAN_TDCR_POSI_TDCF   0
	#define _FDCAN_TDCR_POSI_TDCO   8
	#define _FDCAN_GFC_POSI_RRFE    0
	#define _FDCAN_GFC_POSI_RRFS    1
	#define _FDCAN_GFC_POSI_ANFE    2
	#define _FDCAN_GFC_POSI_ANFS    4
	#define _FDCAN_SIDFC_POSI_FLSSA 2
	#define _FDCAN_SIDFC_POSI_LSS   16
	#define _FDCAN_XIDFC_POSI_FLESA 2
	#define _FDCAN_XIDFC_POSI_LSE   16
	#define _FDCAN_XIDAM_POSI_EIDM  0
	#define _FDCAN_HPMS_POSI_BIDX   0
	#define _FDCAN_HPMS_POSI_MSI    6
	#define _FDCAN_HPMS_POSI_FIDX   8
	#define _FDCAN_HPMS_POSI_FLST   15
	#define _FDCAN_RXF0C_POSI_F0SA  2
	#define _FDCAN_RXF0C_POSI_F0S   16
	#define _FDCAN_RXF0C_POSI_F0WM  24
	#define _FDCAN_RXF0C_POSI_F0OM  31
	#define _FDCAN_RXF0S_POSI_F0FL  0
	#define _FDCAN_RXF0S_POSI_F0GI  8
	#define _FDCAN_RXF0S_POSI_F0PI  16
	#define _FDCAN_RXF0S_POSI_F0F   24
	#define _FDCAN_RXF0S_POSI_RF0L  25
	#define _FDCAN_RXF0A_POSI_F0AI  0
	#define _FDCAN_RXBC_POSI_RBSA   2
	#define _FDCAN_RXF1C_POSI_F1SA  2
	#define _FDCAN_RXF1C_POSI_F1S   16
	#define _FDCAN_RXF1C_POSI_F1WM  24
	#define _FDCAN_RXF1C_POSI_F1OM  31
	#define _FDCAN_RXF1S_POSI_F1FL  0
	#define _FDCAN_RXF1S_POSI_F1GI  8
	#define _FDCAN_RXF1S_POSI_F1PI  16
	#define _FDCAN_RXF1S_POSI_F1F   24
	#define _FDCAN_RXF1S_POSI_RF1L  25
	#define _FDCAN_RXF1A_POSI_F1AI  0
	#define _FDCAN_RXESC_POSI_F0DS  0
	#define _FDCAN_RXESC_POSI_F1DS  4
	#define _FDCAN_RXESC_POSI_RBDS  8
	#define _FDCAN_TXBC_POSI_TBSA   2
	#define _FDCAN_TXBC_POSI_NDTB   16
	#define _FDCAN_TXBC_POSI_TFQS   24
	#define _FDCAN_TXBC_POSI_TFQM   30
	#define _FDCAN_TXFQS_POSI_TFFL  0
	#define _FDCAN_TXFQS_POSI_TFGI  8
	#define _FDCAN_TXFQS_POSI_TFQPI 16
	#define _FDCAN_TXFQS_POSI_TFQF  21
	#define _FDCAN_TXESC_POSI_TBDS  0
	#define _FDCAN_TXEFC_POSI_EFSA  2
	#define _FDCAN_TXEFC_POSI_EFS   16
	#define _FDCAN_TXEFC_POSI_EFWM  24
	#define _FDCAN_TXEFS_POSI_EFFL  0
	#define _FDCAN_TXEFS_POSI_EFGI  8
	#define _FDCAN_TXEFS_POSI_EFPI  16
	#define _FDCAN_TXEFS_POSI_EFF   24
	#define _FDCAN_TXEFS_POSI_TEFL  25
	#define _FDCAN_TXEFA_POSI_EFAI  0
	// interrupt bits (IR / IE / ILS share the same positions)
	#define _FDCAN_IR_POSI_RF0N  0
	#define _FDCAN_IR_POSI_RF0W  1
	#define _FDCAN_IR_POSI_RF0F  2
	#define _FDCAN_IR_POSI_RF0L  3
	#define _FDCAN_IR_POSI_RF1N  4
	#define _FDCAN_IR_POSI_RF1W  5
	#define _FDCAN_IR_POSI_RF1F  6
	#define _FDCAN_IR_POSI_RF1L  7
	#define _FDCAN_IR_POSI_HPM   8
	#define _FDCAN_IR_POSI_TC    9
	#define _FDCAN_IR_POSI_TCF   10
	#define _FDCAN_IR_POSI_TFE   11
	#define _FDCAN_IR_POSI_TEFN  12
	#define _FDCAN_IR_POSI_TEFW  13
	#define _FDCAN_IR_POSI_TEFF  14
	#define _FDCAN_IR_POSI_TEFL  15
	#define _FDCAN_IR_POSI_TSW   16
	#define _FDCAN_IR_POSI_MRAF  17
	#define _FDCAN_IR_POSI_TOO   18
	#define _FDCAN_IR_POSI_DRX   19
	#define _FDCAN_IR_POSI_ELO   22
	#define _FDCAN_IR_POSI_EP    23
	#define _FDCAN_IR_POSI_EW    24
	#define _FDCAN_IR_POSI_BO    25
	#define _FDCAN_IR_POSI_WDI   26
	#define _FDCAN_IR_POSI_PEA   27
	#define _FDCAN_IR_POSI_PED   28
	#define _FDCAN_IR_POSI_ARA   29
	#define _FDCAN_ILE_POSI_EINT0 0
	#define _FDCAN_ILE_POSI_EINT1 1

	// ---- bit positions (TT) ----
	#define _FDCAN_TTTMC_POSI_TMSA 2
	#define _FDCAN_TTTMC_POSI_TME  16
	#define _FDCAN_TTRMC_POSI_RID  0
	#define _FDCAN_TTRMC_POSI_XTD  30
	#define _FDCAN_TTRMC_POSI_RMPS 31
	#define _FDCAN_TTOCF_POSI_OM   0
	#define _FDCAN_TTOCF_POSI_GEN  3
	#define _FDCAN_TTOCF_POSI_TM   4
	#define _FDCAN_TTOCF_POSI_LDSDL 5
	#define _FDCAN_TTOCF_POSI_IRTO 8
	#define _FDCAN_TTOCF_POSI_EECS 15
	#define _FDCAN_TTOCF_POSI_AWL  16
	#define _FDCAN_TTOCF_POSI_EGTF 24
	#define _FDCAN_TTOCF_POSI_ECC  25
	#define _FDCAN_TTOCF_POSI_EVTP 26
	#define _FDCAN_TTMLM_POSI_CCM  0
	#define _FDCAN_TTMLM_POSI_CSS  6
	#define _FDCAN_TTMLM_POSI_TXEW 8
	#define _FDCAN_TTMLM_POSI_ENTT 16
	#define _FDCAN_TURCF_POSI_NCL  0
	#define _FDCAN_TURCF_POSI_DC   16
	#define _FDCAN_TURCF_POSI_ELT  31
	#define _FDCAN_TTOCN_POSI_SGT  0
	#define _FDCAN_TTOCN_POSI_ECS  1
	#define _FDCAN_TTOCN_POSI_SWP  2
	#define _FDCAN_TTOCN_POSI_SWS  3
	#define _FDCAN_TTOCN_POSI_RTIE 5
	#define _FDCAN_TTOCN_POSI_TMC  6
	#define _FDCAN_TTOCN_POSI_TTIE 8
	#define _FDCAN_TTOCN_POSI_GCS  9
	#define _FDCAN_TTOCN_POSI_FGP  10
	#define _FDCAN_TTOCN_POSI_TMG  11
	#define _FDCAN_TTOCN_POSI_NIG  12
	#define _FDCAN_TTOCN_POSI_ESCN 13
	#define _FDCAN_TTOCN_POSI_LCKC 15
	#define _FDCAN_TTGTP_POSI_TP   0
	#define _FDCAN_TTGTP_POSI_CTP  16
	#define _FDCAN_TTTMK_POSI_TM   0
	#define _FDCAN_TTTMK_POSI_TICC 16
	#define _FDCAN_TTTMK_POSI_LCKM 31
	#define _FDCAN_TTIR_POSI_SBC  0
	#define _FDCAN_TTIR_POSI_SMC  1
	#define _FDCAN_TTIR_POSI_CSM  2
	#define _FDCAN_TTIR_POSI_SOG  3
	#define _FDCAN_TTIR_POSI_RTMI 4
	#define _FDCAN_TTIR_POSI_TTMI 5
	#define _FDCAN_TTIR_POSI_SWE  6
	#define _FDCAN_TTIR_POSI_GTW  7
	#define _FDCAN_TTIR_POSI_GTD  8
	#define _FDCAN_TTIR_POSI_GTE  9
	#define _FDCAN_TTIR_POSI_TXU  10
	#define _FDCAN_TTIR_POSI_TXO  11
	#define _FDCAN_TTIR_POSI_SE1  12
	#define _FDCAN_TTIR_POSI_SE2  13
	#define _FDCAN_TTIR_POSI_ELC  14
	#define _FDCAN_TTIR_POSI_IWT  15
	#define _FDCAN_TTIR_POSI_WT   16
	#define _FDCAN_TTIR_POSI_AW   17
	#define _FDCAN_TTIR_POSI_CER  18
	#define _FDCAN_TTOST_POSI_EL   0
	#define _FDCAN_TTOST_POSI_MS   2
	#define _FDCAN_TTOST_POSI_SYS  4
	#define _FDCAN_TTOST_POSI_QGTP 6
	#define _FDCAN_TTOST_POSI_QCS  7
	#define _FDCAN_TTOST_POSI_RTO  8
	#define _FDCAN_TTOST_POSI_WGTD 22
	#define _FDCAN_TTOST_POSI_GFI  23
	#define _FDCAN_TTOST_POSI_TMP  24
	#define _FDCAN_TTOST_POSI_GSI  27
	#define _FDCAN_TTOST_POSI_WFE  28
	#define _FDCAN_TTOST_POSI_AWE  29
	#define _FDCAN_TTOST_POSI_WECS 30
	#define _FDCAN_TTOST_POSI_SPL  31
	#define _FDCAN_TURNA_POSI_NAV  0
	#define _FDCAN_TTLGT_POSI_LT   0
	#define _FDCAN_TTLGT_POSI_GT   16
	#define _FDCAN_TTCTC_POSI_CT   0
	#define _FDCAN_TTCTC_POSI_CC   16
	#define _FDCAN_TTCPT_POSI_CCV  0
	#define _FDCAN_TTCPT_POSI_SWV  16
	#define _FDCAN_TTCSM_POSI_CSM  0
	#define _FDCAN_TTTS_POSI_SWTSEL 0
	#define _FDCAN_TTTS_POSI_EVTSEL 4

	// ---- bit positions (CCU) ----
	#define _FDCANCCU_CCFG_POSI_TQBT 0
	#define _FDCANCCU_CCFG_POSI_BCC  6
	#define _FDCANCCU_CCFG_POSI_CFL  7
	#define _FDCANCCU_CCFG_POSI_OCPM 8
	#define _FDCANCCU_CCFG_POSI_CDIV 16
	#define _FDCANCCU_CCFG_POSI_SWR  31
	#define _FDCANCCU_CSTAT_POSI_OCPC 0
	#define _FDCANCCU_CSTAT_POSI_TQC  18
	#define _FDCANCCU_CSTAT_POSI_CALS 30
	#define _FDCANCCU_CWD_POSI_WDC  0
	#define _FDCANCCU_CWD_POSI_WDV  16
	#define _FDCANCCU_IR_POSI_CWE   0
	#define _FDCANCCU_IR_POSI_CSC   1
	#define _FDCANCCU_IE_POSI_CWEE  0
	#define _FDCANCCU_IE_POSI_CSCE  1

	// ---- element masks (Tx/Rx header words in Message RAM) ----
	#define _FDCAN_ELM_MASK_STDID 0x1FFC0000
	#define _FDCAN_ELM_MASK_EXTID 0x1FFFFFFF
	#define _FDCAN_ELM_MASK_RTR   0x20000000
	#define _FDCAN_ELM_MASK_XTD   0x40000000
	#define _FDCAN_ELM_MASK_ESI   0x80000000
	#define _FDCAN_ELM_MASK_TS    0x0000FFFF
	#define _FDCAN_ELM_MASK_DLC   0x000F0000
	#define _FDCAN_ELM_MASK_BRS   0x00100000
	#define _FDCAN_ELM_MASK_FDF   0x00200000
	#define _FDCAN_ELM_MASK_EFC   0x00800000
	#define _FDCAN_ELM_MASK_MM    0xFF000000
	#define _FDCAN_ELM_MASK_FIDX  0x7F000000
	#define _FDCAN_ELM_MASK_ANMF  0x80000000
	#define _FDCAN_ELM_MASK_ET    0x00C00000

	// DLC code -> byte count (AKA HAL DLCtoBytes)
	static const byte _FdcanDlcToBytes[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64 };

	// ---- error codes (AKA HAL_FDCAN_ErrorCode) ----
	#define ERR_FDCAN_NONE            0x00000000
	#define ERR_FDCAN_TIMEOUT         0x00000001
	#define ERR_FDCAN_NOT_INITIALIZED 0x00000002
	#define ERR_FDCAN_NOT_READY       0x00000004
	#define ERR_FDCAN_NOT_STARTED     0x00000008
	#define ERR_FDCAN_NOT_SUPPORTED   0x00000010
	#define ERR_FDCAN_PARAM           0x00000020
	#define ERR_FDCAN_PENDING         0x00000040
	#define ERR_FDCAN_RAM_ACCESS      0x00000080
	#define ERR_FDCAN_LOG_OVERFLOW    0x00400000 // IR.ELO
	#define ERR_FDCAN_RAM_WDG         0x04000000 // IR.WDI
	#define ERR_FDCAN_PROTOCOL_ARBT   0x08000000 // IR.PEA
	#define ERR_FDCAN_PROTOCOL_DATA   0x10000000 // IR.PED
	#define ERR_FDCAN_RESERVED_AREA   0x20000000 // IR.ARA

	// FDCAN kernel clock enable (H7: APB1HENR.FDCANEN, bit 8, shared by FDCAN1/2)
	#define _RCC_APB1HENR_POSI_FDCANEN 8

} // namespace uni
#endif // _MCU_STM32H7x
#endif // _INC_DEV_PREDEF_FDCAN

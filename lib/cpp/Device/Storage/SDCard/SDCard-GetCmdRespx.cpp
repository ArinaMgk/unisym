// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Stroage) SDCard
// Codifiers: @dosconio: 20250107
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

#include "../../../../../inc/cpp/Device/SD.hpp"
#include "../../../../../inc/cpp/Device/Storage/SD-PARA.h"
#include "../../../../../inc/cpp/Device/RCC/RCC"
#include "../../../../../inc/cpp/Device/GPIO"
#include "../../../../../inc/cpp/Device/SysTick"
#include "../../../../../inc/c/driver/interrupt/GIC.h"
#include "../../../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"

#define bitmatch(bits,mask) (((bits) & (mask)) == (mask))

//[refer]
// - stm32mp13xx_hal_sd.c
// - stm32mp13xx_hal_sd_ex.c
// - stm32mp13xx_ll_sdmmc.c

#define SDMMC_MAX_VOLT_TRIAL               ((uint32_t)0x0000FFFFU)

#define SDMMC_MAX_TRIAL                    ((uint32_t)0x0000FFFFU)

#define SD_INIT_FREQ           400000U // Initialization phase : 400 kHz max
#define SD_NORMAL_SPEED_FREQ 25000000U // Normal speed phase : 25 MHz max
#define SD_HIGH_SPEED_FREQ   50000000U // High speed phase : 50 MHz max

#define SDMMC_CMDTIMEOUT                   ((uint32_t)5000U)      // Command send and response timeout
#define SDMMC_MAXERASETIMEOUT              ((uint32_t)63000U)     // Max erase Timeout 63 s
#define SDMMC_STOPTRANSFERTIMEOUT          ((uint32_t)100000000U) // Timeout for STOP TRANSMISSION command

// setby { SDMMC_GetCmdResp7 }
static bool timeout_not_flag = false;


namespace uni {
#if defined(_MPU_STM32MP13)

	// Return the command index of last command for which response received
	inline static uint8 SDMMC_GetCommandResponse(const SecureDigitalCard_t& sd) {
		return sd[SDReg::RESPCMD];
	}
	// Return the response received from the card for the last command
	inline static uint32 SDMMC_GetResponse(const SecureDigitalCard_t& sd, uint32 response) {
		if (--response >= 4) return 0;// 1 ~ 4
		return (&sd[SDReg::RESP1])[response];
	}

	// Checks for error conditions for R1 response
	bool SecureDigitalCard_t::SDMMC_GetCmdResp1(uint8 SD_CMD, uint32 Timeout, uint32* feedback) const {
		// 8 is the number of required instructions cycles for the below loop statement. The SDMMC_CMDTIMEOUT is expressed in ms
		stduint timeout = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U / SysTickHz);
		uint32 sta_reg;
		do {
			asrtret(timeout_not_flag = timeout--);
			asserv(feedback)[nil] = SDMMC_ERROR_TIMEOUT;
			sta_reg = self[SDReg::STA] & 0x200045;// !(CCRCFAIL | CMDREND | CTIMEOUT | BUSYD0END) || (CMDACT=CPSMACT)
		} while (!sta_reg || self[SDReg::STA].bitof(13));
		if (self[SDReg::STA].bitof(2))// STAR.CTIMEOUT
		{
			// Card is not SD V2.0 compliant
			self[SDReg::STA].setof(2, false);
			return false;
		}
		if (self[SDReg::STA].bitof(0))// STAR.CCRCFAIL
		{
			// Card is not SD V2.0 compliant
			self[SDReg::STA].setof(0, false);
			return false;
		}
		self[SDReg::ICR] = _IMM(_IMM1S(21) | 0b11000101);// __SDMMC_CLEAR_FLAG: SDMMC_STATIC_CMD_FLAGS = FLAG_CCRCFAIL | FLAG_CTIMEOUT | FLAG_CMDREND | FLAG_CMDSENT | FLAG_BUSYD0END
		// Check response received is of desired command
		if (SDMMC_GetCommandResponse(self) != SD_CMD) {
			asserv(feedback)[nil] = SDMMC_ERROR_CMD_CRC_FAIL; return false;
		}
		// have received response, retrieve it for analysis
		uint32 response_r1 = SDMMC_GetResponse(self, 1);
		if ((response_r1 & SDMMC_OCR_ERRORBITS) == 0) {
			asserv(feedback)[nil] = SDMMC_ERROR_NONE; return true;
		}
		if (bitmatch(response_r1, SDMMC_OCR_ADDR_OUT_OF_RANGE)) {
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
		}
		else if (bitmatch(response_r1, SDMMC_OCR_ADDR_MISALIGNED)) {
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_MISALIGNED;
		}
		else if (bitmatch(response_r1, SDMMC_OCR_BLOCK_LEN_ERR)) {
			asserv(feedback)[nil] = SDMMC_ERROR_BLOCK_LEN_ERR;
		}
		else if (bitmatch(response_r1, SDMMC_OCR_ERASE_SEQ_ERR)) {
			asserv(feedback)[nil] = SDMMC_ERROR_ERASE_SEQ_ERR;
		}
		else if (bitmatch(response_r1, SDMMC_OCR_BAD_ERASE_PARAM)) {
			asserv(feedback)[nil] = SDMMC_ERROR_BAD_ERASE_PARAM;
		}
		else if (bitmatch(response_r1, SDMMC_OCR_WRITE_PROT_VIOLATION)) {
			asserv(feedback)[nil] = SDMMC_ERROR_WRITE_PROT_VIOLATION;
		}
		else if (bitmatch(response_r1, SDMMC_OCR_LOCK_UNLOCK_FAILED)) {
			asserv(feedback)[nil] = SDMMC_ERROR_LOCK_UNLOCK_FAILED;
		}
		else if (bitmatch(response_r1, SDMMC_OCR_COM_CRC_FAILED)) {
			asserv(feedback)[nil] = SDMMC_ERROR_COM_CRC_FAILED;
		}
		else if (bitmatch(response_r1, SDMMC_OCR_ILLEGAL_CMD)) {
			asserv(feedback)[nil] = SDMMC_ERROR_ILLEGAL_CMD;
		}
		else if (bitmatch(response_r1, SDMMC_OCR_CARD_ECC_FAILED)) {
			asserv(feedback)[nil] = SDMMC_ERROR_CARD_ECC_FAILED;
		}
		else if (bitmatch(response_r1, SDMMC_OCR_CC_ERROR)) {
			asserv(feedback)[nil] = SDMMC_ERROR_CC_ERR;
		}
		else if (bitmatch(response_r1, SDMMC_OCR_STREAM_READ_UNDERRUN)) {
			asserv(feedback)[nil] = SDMMC_ERROR_STREAM_READ_UNDERRUN;
		}
		else if (bitmatch(response_r1, SDMMC_OCR_STREAM_WRITE_OVERRUN)) {
			asserv(feedback)[nil] = SDMMC_ERROR_STREAM_WRITE_OVERRUN;
		}
		else if (bitmatch(response_r1, SDMMC_OCR_CID_CSD_OVERWRITE)) {
			asserv(feedback)[nil] = SDMMC_ERROR_CID_CSD_OVERWRITE;
		}
		else if (bitmatch(response_r1, SDMMC_OCR_WP_ERASE_SKIP)) {
			asserv(feedback)[nil] = SDMMC_ERROR_WP_ERASE_SKIP;
		}
		else if (bitmatch(response_r1, SDMMC_OCR_CARD_ECC_DISABLED)) {
			asserv(feedback)[nil] = SDMMC_ERROR_CARD_ECC_DISABLED;
		}
		else if (bitmatch(response_r1, SDMMC_OCR_ERASE_RESET)) {
			asserv(feedback)[nil] = SDMMC_ERROR_ERASE_RESET;
		}
		else if (bitmatch(response_r1, SDMMC_OCR_AKE_SEQ_ERROR)) {
			asserv(feedback)[nil] = SDMMC_ERROR_AKE_SEQ_ERR;
		}
		else {
			asserv(feedback)[nil] = SDMMC_ERROR_GENERAL_UNKNOWN_ERR;
		}
		return false;
	}
	// Checks for error conditions for R2 (CID or CSD) response
	bool SecureDigitalCard_t::SDMMC_GetCmdResp2(uint32* feedback) const {
		// 8 is the number of required instructions cycles for the below loop statement. The SDMMC_CMDTIMEOUT is expressed in ms
		stduint timeout = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U / SysTickHz);
		uint32 sta_reg;
		do {
			asrtret(timeout_not_flag = timeout--);
			asserv(feedback)[nil] = SDMMC_ERROR_TIMEOUT;
			sta_reg = self[SDReg::STA] & 0x45;// !(CCRCFAIL | CMDREND | CTIMEOUT) || (CMDACT=CPSMACT)
		} while (!sta_reg || self[SDReg::STA].bitof(13));
		if (self[SDReg::STA].bitof(2))// STAR.CTIMEOUT
		{
			// Card is not SD V2.0 compliant
			self[SDReg::STA].setof(2, false);
			asserv(feedback)[nil] = SDMMC_ERROR_CMD_RSP_TIMEOUT;
			return false;
		}
		if (self[SDReg::STA].bitof(0))// STAR.CCRCFAIL
		{
			// Card is not SD V2.0 compliant
			self[SDReg::STA].setof(0, false);
			asserv(feedback)[nil] = SDMMC_ERROR_CMD_CRC_FAIL;
			return false;
		}
		else {
			self[SDReg::ICR] = _IMM(_IMM1S(21) | 0b11000101);// __SDMMC_CLEAR_FLAG: SDMMC_STATIC_CMD_FLAGS = FLAG_CCRCFAIL | FLAG_CTIMEOUT | FLAG_CMDREND | FLAG_CMDSENT | FLAG_BUSYD0END
		}
		asserv(feedback)[nil] = SDMMC_ERROR_NONE;
		return true;
	}
	// Checks for error conditions for R3 (OCR) response
	bool SecureDigitalCard_t::SDMMC_GetCmdResp3(uint32* feedback) const {
		// 8 is the number of required instructions cycles for the below loop statement. The SDMMC_CMDTIMEOUT is expressed in ms
		stduint timeout = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U / SysTickHz);
		uint32 sta_reg;
		do {
			asrtret(timeout_not_flag = timeout--);
			asserv(feedback)[nil] = SDMMC_ERROR_TIMEOUT;
			sta_reg = self[SDReg::STA] & 0x45;// !(CCRCFAIL | CMDREND | CTIMEOUT) || (CMDACT=CPSMACT)
		} while (!sta_reg || self[SDReg::STA].bitof(13));
		if (self[SDReg::STA].bitof(2))// STAR.CTIMEOUT
		{
			// Card is not SD V2.0 compliant
			self[SDReg::STA].setof(2, false);
			return false;
		}
		else {
			self[SDReg::ICR] = _IMM(_IMM1S(21) | 0b11000101);// __SDMMC_CLEAR_FLAG: SDMMC_STATIC_CMD_FLAGS = FLAG_CCRCFAIL | FLAG_CTIMEOUT | FLAG_CMDREND | FLAG_CMDSENT | FLAG_BUSYD0END
		}
		asserv(feedback)[nil] = SDMMC_ERROR_NONE;
		return true;
	}
	// Checks for error conditions for R6 (RCA) response
	bool SecureDigitalCard_t::SDMMC_GetCmdResp6(uint8 SD_CMD, uint16* pRCA, uint32* feedback) const {
		// 8 is the number of required instructions cycles for the below loop statement. The SDMMC_CMDTIMEOUT is expressed in ms
		stduint timeout = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U / SysTickHz);
		uint32 sta_reg;

		do {
			asrtret(timeout_not_flag = timeout--);
			asserv(feedback)[nil] = SDMMC_ERROR_TIMEOUT;
			sta_reg = self[SDReg::STA] & 0x45;// !(CCRCFAIL | CMDREND | CTIMEOUT) || (CMDACT=CPSMACT)
		} while (!sta_reg || self[SDReg::STA].bitof(13));
		if (self[SDReg::STA].bitof(2))// STAR.CTIMEOUT
		{
			// Card is not SD V2.0 compliant
			self[SDReg::STA].setof(2, false);
			asserv(feedback)[nil] = SDMMC_ERROR_CMD_RSP_TIMEOUT;
			return false;
		}
		if (self[SDReg::STA].bitof(0))// STAR.CCRCFAIL
		{
			// Card is not SD V2.0 compliant
			self[SDReg::STA].setof(0, false);
			asserv(feedback)[nil] = SDMMC_ERROR_CMD_CRC_FAIL;
			return false;
		}
		uint8 cmd_resp = SDMMC_GetCommandResponse(self);
		// Check response received is of desired command
		if (cmd_resp != SD_CMD) {
			asserv(feedback)[nil] = SDMMC_ERROR_CMD_CRC_FAIL; return false;
		}
		self[SDReg::ICR] = (_IMM1S(21) | _IMM(0b11000101));// __SDMMC_CLEAR_FLAG: SDMMC_STATIC_CMD_FLAGS = FLAG_CCRCFAIL | FLAG_CTIMEOUT | FLAG_CMDREND | FLAG_CMDSENT | FLAG_BUSYD0END
		uint32 response_r1 = self[SDReg::RESP1];//SDMMC_GetResponse(self, 1);
		if ((response_r1 & (SDMMC_R6_GENERAL_UNKNOWN_ERROR | SDMMC_R6_ILLEGAL_CMD | SDMMC_R6_COM_CRC_FAILED)) == 0) {
			*pRCA = (uint16_t)(response_r1 >> 16);
			asserv(feedback)[nil] = SDMMC_ERROR_NONE; return true;
		}
		if (bitmatch(response_r1, SDMMC_R6_ILLEGAL_CMD)) {
			asserv(feedback)[nil] = SDMMC_ERROR_ILLEGAL_CMD;
		}
		if (bitmatch(response_r1, SDMMC_R6_COM_CRC_FAILED)) {
			asserv(feedback)[nil] = SDMMC_ERROR_COM_CRC_FAILED;
		}
		else {
			asserv(feedback)[nil] = SDMMC_ERROR_GENERAL_UNKNOWN_ERR;
		}
		return false;
	}
	// Checks for error conditions for R7 response.
	bool SecureDigitalCard_t::SDMMC_GetCmdResp7() const {
		// 8 is the number of required instructions cycles for the below loop statement. The SDMMC_CMDTIMEOUT is expressed in ms
		stduint timeout = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U / SysTickHz);
		uint32 sta_reg;
		do {
			asrtret(timeout_not_flag = timeout--);
			sta_reg = self[SDReg::STA] & 0x45;// !(CCRCFAIL | CMDREND | CTIMEOUT) || (CMDACT=CPSMACT)
		} while (!sta_reg || self[SDReg::STA].bitof(13));
		if (self[SDReg::STA].bitof(2))// STAR.CTIMEOUT
		{
			// Card is not SD V2.0 compliant
			self[SDReg::STA].setof(2, false);
			return false;
		}
		if (self[SDReg::STA].bitof(0))// STAR.CCRCFAIL
		{
			// Card is not SD V2.0 compliant
			self[SDReg::STA].setof(0, false);
			return false;
		}
		if (self[SDReg::STA].bitof(6))// STAR.CMDREND
			self[SDReg::STA].setof(6, false);
		return true;
	}

#endif
}

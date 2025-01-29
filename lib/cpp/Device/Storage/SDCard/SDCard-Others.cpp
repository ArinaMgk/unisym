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
#define statin inline static

//[refer]
// - stm32mp13xx_hal_sd.c
// - stm32mp13xx_hal_sd_ex.c
// - stm32mp13xx_ll_sdmmc.c

#define BLOCKSIZE 512

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

extern "C" {
	uint16 SD_InitCard_sd_rca = 0;
}

namespace uni {
#if defined(_MPU_STM32MP13)

	// Return the command index of last command for which response received
	statin uint8 SDMMC_GetCommandResponse(const SecureDigitalCard_t& sd) {
		return sd[SDReg::RESPCMD];
	}
	// Return the response received from the card for the last command
	statin uint32 SDMMC_GetResponse(const SecureDigitalCard_t& sd, uint32 response) {
		if (--response >= 4) return 0;// 1 ~ 4
		return (&sd[SDReg::RESP1])[response];
	}
	// Read data(word) from Rx FIFO in blocking mode(polling)
	// <=> SDMMC_GetFIFOCount
	statin uint32 SDMMC_ReadFIFO(const SecureDigitalCard_t& sd) {
		return sd[SDReg::FIFO_Start];
	}
	// Write data (word) to Tx FIFO in blocking mode (polling)
	statin void SDMMC_WriteFIFO(const SecureDigitalCard_t& sd, uint32* data) {
		sd[SDReg::FIFO_Start] = *data;
	}
	// Set SDMMC Power state to Power-Cycle
	statin void SDMMC_PowerState_Cycle(const SecureDigitalCard_t& sd) {
		sd[SDReg::POWER].setof(1, true);// PWRCTRL b1
	}
	// Set SDMMC Power state to OFF
	statin void SDMMC_PowerState_OFF(const SecureDigitalCard_t& sd) {
		sd[SDReg::POWER].maset(0, 2, nil);// PWRCTRL
	}
	// number of remaining data bytes to be transferred
	statin uint32 SDMMC_GetDataCounter(const SecureDigitalCard_t& sd) {
		return sd[SDReg::DCOUNT];
	}
	// Sets one of the two options of inserting read wait interval. Read Wait control by stopping SDMMCCLK or using SDMMC_DATA2
	statin void SDMMC_SetSDMMCReadWaitMode(const SecureDigitalCard_t& sd, bool clk_else_data2) {
		sd[SDReg::DCTRL].setof(10, clk_else_data2);// RWMOD
	}

	//[RET]
	// - 0x00: Power OFF
	// - 0x02: Power UP
	// - 0x03: Power ON
	statin uint32 SDMMC_GetPowerState(const SecureDigitalCard_t& sd)
	{
		return  sd[SDReg::POWER].masof(0, 2);// PWRCTRL
	}

	// 


	void SecureDigitalCard_t::setClockEdge(bool clock_edge) const {
		self[SDReg::CLKCR].setof(16, !clock_edge);// NEGEDGE
	}
	void SecureDigitalCard_t::setClockPowerSave(bool powersave_enable) const {
		self[SDReg::CLKCR].setof(12, powersave_enable);// PWRSAV
	}
	void SecureDigitalCard_t::setBusWide(byte bus_width) const {
		if (bus_width == 1)
			self[SDReg::CLKCR].maset(4, 2, 0b00);// WIDBUS
		else if (bus_width == 4)
			self[SDReg::CLKCR].maset(4, 2, 0b01);// WIDBUS
		else if (bus_width == 8)
			self[SDReg::CLKCR].maset(4, 2, 0b10);// WIDBUS
	}
	void SecureDigitalCard_t::setHardwareFlowControl(bool hardflow_control) const {
		self[SDReg::CLKCR].setof(17, hardflow_control);// HWFC_EN
	}
	void SecureDigitalCard_t::setClockDiv(stduint Init_ClockDiv) const {
		self[SDReg::CLKCR].maset(0, 10, Init_ClockDiv);// CLKDIV
	}

	bool SecureDigitalCard_t::SD_PowerON(uint32* feedback) {
		uint32 error_state;
		// CMD0: GO_IDLE_STATE
		asrtret(SDMMC_CmdGoIdleState());

		// CMD8: SEND_IF_COND: Command available only on V2.0 cards
		CARD_V2X_else_V1X = SDMMC_CmdOperCond();
		if (!CARD_V2X_else_V1X) {
			asrtret(SDMMC_CmdGoIdleState());// CMD0: GO_IDLE_STATE
		}
		if (CARD_V2X_else_V1X) {
			// SEND CMD55 APP_CMD with RCA as 0
			if (!SDMMC_CmdAppCommand(0, NULL)) {
				asserv(feedback)[0] = SDMMC_ERROR_UNSUPPORTED_FEATURE;
				return false;
			}
		}
		// SD CARD
		// Send ACMD41 SD_APP_OP_COND with Argument 0x80100000
		uint32 validvoltage = 0U;
		uint32 response = 0U;
		uint32 count = 0U;
		while ((count < SDMMC_MAX_VOLT_TRIAL) && (validvoltage == 0U)) {
			// SEND CMD55 APP_CMD with RCA as 0
			if (!SDMMC_CmdAppCommand(0, feedback)) {
				return false;
			}
			// Send CMD41
			if (!SDMMC_CmdAppOperCommand(SDMMC_VOLTAGE_WINDOW_SD | SDMMC_HIGH_CAPACITY | SD_SWITCH_1_8V_CAPACITY, &error_state)) {
				asserv(feedback)[0] = SDMMC_ERROR_UNSUPPORTED_FEATURE;
				return false;
			}
			// Get command response
			response = SDMMC_GetResponse(self, 1);
			// Get operating voltage
			validvoltage = (((response >> 31U) == 1U) ? 1U : 0U);
			count++;
		}
		if (count >= SDMMC_MAX_VOLT_TRIAL) {
			asserv(feedback)[0] = SDMMC_ERROR_INVALID_VOLTRANGE;
			return false;
		}

		// Set default card type
		CardType = CardType_E::SDSC;
		if ((response & SDMMC_HIGH_CAPACITY) == SDMMC_HIGH_CAPACITY)
		{
			CardType = CardType_E::SDHC_SDXC;
		#if (USE_SD_TRANSCEIVER != 0U)//{TEMP} no-trans
			if (hsd->Init.TranceiverPresent == SDMMC_TRANSCEIVER_PRESENT)
			{
				if ((response & SD_SWITCH_1_8V_CAPACITY) == SD_SWITCH_1_8V_CAPACITY)
				{
					hsd->SdCard.CardSpeed = CARD_ULTRA_HIGH_SPEED;
					// Start switching procedue
					hsd->Instance->POWER |= SDMMC_POWER_VSWITCHEN;
					// Send CMD11 to switch 1.8V mode
					errorstate = SDMMC_CmdVoltageSwitch(hsd->Instance);
					if (errorstate != HAL_SD_ERROR_NONE) return errorstate;
					// Check to CKSTOP
					while ((hsd->Instance->STA & SDMMC_FLAG_CKSTOP) != SDMMC_FLAG_CKSTOP)
					{
						if ((HAL_GetTick() - tickstart) >= SDMMC_DATATIMEOUT)
						{
							return HAL_SD_ERROR_TIMEOUT;
						}
					}
					// Clear CKSTOP Flag
					hsd->Instance->ICR = SDMMC_FLAG_CKSTOP;
					// Check to BusyD0
					if ((hsd->Instance->STA & SDMMC_FLAG_BUSYD0) != SDMMC_FLAG_BUSYD0)
					{
					// Error when activate Voltage Switch in SDMMC Peripheral
						return SDMMC_ERROR_UNSUPPORTED_FEATURE;
					}
					else
					{
					// Enable Transceiver Switch PIN
					#if defined (USE_HAL_SD_REGISTER_CALLBACKS) && (USE_HAL_SD_REGISTER_CALLBACKS == 1U)
						hsd->DriveTransceiver_1_8V_Callback(SET);
					#else
						HAL_SD_DriveTransceiver_1_8V_Callback(SET);
					#endif // USE_HAL_SD_REGISTER_CALLBACKS */
							// Switch ready
						hsd->Instance->POWER |= SDMMC_POWER_VSWITCH;
						// Check VSWEND Flag
						while ((hsd->Instance->STA & SDMMC_FLAG_VSWEND) != SDMMC_FLAG_VSWEND)
						{
							if ((HAL_GetTick() - tickstart) >= SDMMC_DATATIMEOUT)
							{
								return HAL_SD_ERROR_TIMEOUT;
							}
						}
						// Clear VSWEND Flag
						hsd->Instance->ICR = SDMMC_FLAG_VSWEND;
						// Check BusyD0 status
						if ((hsd->Instance->STA & SDMMC_FLAG_BUSYD0) == SDMMC_FLAG_BUSYD0)
						{
							return HAL_SD_ERROR_INVALID_VOLTRANGE;// Error when enabling 1.8V mode
						}
						// Switch to 1.8V OK
						// Disable VSWITCH FLAG from SDMMC Peripheral
						hsd->Instance->POWER = 0x13U;
						// Clean Status flags
						hsd->Instance->ICR = 0xFFFFFFFFU;
					}
				}
			}
		#endif // USE_SD_TRANSCEIVER
		}
		asserv(feedback)[0] = SDMMC_ERROR_NONE;
		return true;
	}
	
	bool SecureDigitalCard_t::SD_InitCard(uint32* feedback) {
		uint16& sd_rca = SD_InitCard_sd_rca;// = 0
		uint32 tickstart = SysTick::getTick();
		if (!SDMMC_GetPowerState(self)) {// power off
			asserv(feedback)[0] = SDMMC_ERROR_REQUEST_NOT_APPLICABLE;
			return false;
		}
		//
		if (CardType != CardType_E::SECURED) {
			// Send CMD2 ALL_SEND_CID
			asrtret(SDMMC_CmdSendCID(feedback));
			// Get Card identification number data
			for0(i, 4) CID[i] = SDMMC_GetResponse(self, i + 1);
		}
		//
		if (CardType != CardType_E::SECURED) {
			// Send CMD3 SET_REL_ADDR with argument 0
			// SD Card publishes its RCA
			while (sd_rca == 0) {
				asrtret(SDMMC_CmdSetRelAdd(&sd_rca, feedback));
				if ((SysTick::getTick() - tickstart) >= SDMMC_CMDTIMEOUT) {
					asserv(feedback)[0] = SDMMC_ERROR_TIMEOUT;
					return timeout_not_flag = false;
				}
			}
		}

		if (CardType != CardType_E::SECURED) {
			CardInfo.RelCardAdd = sd_rca;// Get the SD card RCA
			// Send CMD9 SEND_CSD with argument as card's RCA
			asrtret(SDMMC_CmdSendCSD(CardInfo.RelCardAdd << 16, feedback));
			// Get Card Specific Data
			for0(i, 4) CSD[i] = SDMMC_GetResponse(self, i + 1);
		}

		CardInfo.Class = SDMMC_GetResponse(self, 2) >> 20;

		// Get CSD parameters
		HAL_SD_CardCSDTypeDef CSD;
		if (!HAL_SD_GetCardCSD(&CSD)) {
			asserv(feedback)[0] = SDMMC_ERROR_UNSUPPORTED_FEATURE;
			return false;
		}
		// Select the Card
		asrtret(SDMMC_CmdSelDesel(CardInfo.RelCardAdd << 16, feedback));
		// All cards are initialized
		timeout_not_flag = true;
		asserv(feedback)[0] = SDMMC_ERROR_NONE;
		return true;
	}
	bool SecureDigitalCard_t::SDMMC_CmdBlockLength(uint32 BlockSize) const {
		SDMMC_SendCommand(BlockSize, SDMMC_CMD_SET_BLOCKLEN, 0b01, WaitForInterrupt_E::None, true);
		asrtret(SDMMC_GetCmdResp1(SDMMC_CMD_SET_BLOCKLEN, SDMMC_CMDTIMEOUT, NULL));
		return true;
	}

	bool SecureDigitalCard_t::SDMMC_CmdGoIdleState() const {
		CmdInitType cit;
		cit.argument = 0;
		cit.CmdIndex = 0;// SDMMC_CMD_GO_IDLE_STATE;
		cit.Response = 0;// SDMMC_RESPONSE_NO;
		cit.WaitForInterrupt = WaitForInterrupt_E::None;
		cit.CPSM = true;// enable
		asrtret(SDMMC_SendCommand(cit));// >>> SDMMCx->ARG(cit.argument)
		asrtret(SDMMC_GetCmdError());
		return true;
	}
	bool SecureDigitalCard_t::SDMMC_SendCommand(const CmdInitType& cit) const {
		Reflocal(cmd) = self[SDReg::CMD];
		asrtret(cit.CmdIndex < 0x40U);
		asrtret(cit.Response <= 0b11);
		cmd.maset(0, 6, cit.CmdIndex);// CMDINDEX
		cmd.maset(8, 2, cit.Response);// WAITRESP 0b{00 no 01 short 11 long}
		cmd.maset(10, 2, _IMM(cit.WaitForInterrupt));// WAITRESP
		cmd.setof(12, cit.CPSM);// CPSMEN
		cmd.setof(16, false);// CMDSUSPEND
		self[SDReg::ARG] = cit.argument;
		self[SDReg::CMD] = cmd;
		return true;
	}
	bool SecureDigitalCard_t::SDMMC_SendCommand(stduint argument, stduint cmdindex, byte response, WaitForInterrupt_E waitforinterrupt, bool cpsm) const {
		Reflocal(cmd) = self[SDReg::CMD];
		asrtret(cmdindex < 0x40U);
		asrtret(response <= 0b11);
		cmd.maset(0, 6, cmdindex);// CMDINDEX
		cmd.maset(8, 2, response);// WAITRESP
		cmd.maset(10, 2, _IMM(waitforinterrupt));// WAITRESP
		cmd.setof(12, cpsm);// CPSMEN
		cmd.setof(16, false);// CMDSUSPEND
		self[SDReg::ARG] = argument;
		self[SDReg::CMD] = cmd;
		return true;
	}
	bool SecureDigitalCard_t::SDMMC_GetCmdError() const {
		stduint timeout = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U / SysTickHz);
		do asrtret(timeout--) while (!self[SDReg::STA].bitof(7));// STAR.CMDSENT
		self[SDReg::ICR] = _IMM(_IMM1S(21) | 0b11000101);// __SDMMC_CLEAR_FLAG: SDMMC_STATIC_CMD_FLAGS = FLAG_CCRCFAIL | FLAG_CTIMEOUT | FLAG_CMDREND | FLAG_CMDSENT | FLAG_BUSYD0END
		return true;
	}
	bool SecureDigitalCard_t::SDMMC_CmdOperCond() const {
		// Send CMD8 to verify SD card interface operating condition
		// Argument: - [31:12]: Reserved (shall be set to '0')
		// - [11:8]: Supply Voltage (VHS) 0x1 (Range: 2.7-3.6 V)
		// - [7:0]: Check Pattern (recommended 0xAA)
		// CMD Response: R7
		CmdInitType cit;
		cit.argument = 0x000001AAU;// SDMMC_CHECK_PATTERN;
		cit.CmdIndex = SDMMC_CMD_HS_SEND_EXT_CSD;
		cit.Response = 1;// SDMMC_RESPONSE_SHORT;
		cit.WaitForInterrupt = WaitForInterrupt_E::None;
		cit.CPSM = true;
		asrtret(SDMMC_SendCommand(cit));
		asrtret(SDMMC_GetCmdResp7());
		return true;
	}

	bool SecureDigitalCard_t::HAL_SD_GetCardCSD(HAL_SD_CardCSDTypeDef* pCSD) {
		pCSD->CSDStruct = (uint8_t)((CSD[0] & 0xC0000000U) >> 30U);
		pCSD->SysSpecVersion = (uint8_t)((CSD[0] & 0x3C000000U) >> 26U);
		pCSD->Reserved1 = (uint8_t)((CSD[0] & 0x03000000U) >> 24U);
		pCSD->TAAC = (uint8_t)((CSD[0] & 0x00FF0000U) >> 16U);
		pCSD->NSAC = (uint8_t)((CSD[0] & 0x0000FF00U) >> 8U);
		pCSD->MaxBusClkFrec = (uint8_t)(CSD[0] & 0x000000FFU);
		pCSD->CardComdClasses = (uint16_t)((CSD[1] & 0xFFF00000U) >> 20U);
		pCSD->RdBlockLen = (uint8_t)((CSD[1] & 0x000F0000U) >> 16U);
		pCSD->PartBlockRead = (uint8_t)((CSD[1] & 0x00008000U) >> 15U);
		pCSD->WrBlockMisalign = (uint8_t)((CSD[1] & 0x00004000U) >> 14U);
		pCSD->RdBlockMisalign = (uint8_t)((CSD[1] & 0x00002000U) >> 13U);
		pCSD->DSRImpl = (uint8_t)((CSD[1] & 0x00001000U) >> 12U);
		pCSD->Reserved2 = 0U;
		if (CardType == CardType_E::SDSC)
		{
			pCSD->DeviceSize = (((CSD[1] & 0x000003FFU) << 2U) | ((CSD[2] & 0xC0000000U) >> 30U));
			pCSD->MaxRdCurrentVDDMin = (uint8_t)((CSD[2] & 0x38000000U) >> 27U);
			pCSD->MaxRdCurrentVDDMax = (uint8_t)((CSD[2] & 0x07000000U) >> 24U);
			pCSD->MaxWrCurrentVDDMin = (uint8_t)((CSD[2] & 0x00E00000U) >> 21U);
			pCSD->MaxWrCurrentVDDMax = (uint8_t)((CSD[2] & 0x001C0000U) >> 18U);
			pCSD->DeviceSizeMul = (uint8_t)((CSD[2] & 0x00038000U) >> 15U);
			CardInfo.BlockNbr = (pCSD->DeviceSize + 1U);
			CardInfo.BlockNbr *= (1UL << ((pCSD->DeviceSizeMul & 0x07U) + 2U));
			CardInfo.BlockSize = (1UL << (pCSD->RdBlockLen & 0x0FU));
			CardInfo.LogBlockNbr = (CardInfo.BlockNbr) * ((CardInfo.BlockSize) / BLOCKSIZE);
			CardInfo.LogBlockSize = BLOCKSIZE;
		}
		else if (CardType == CardType_E::SDHC_SDXC)
		{
		  /* Byte 7 */
			pCSD->DeviceSize = (((CSD[1] & 0x0000003FU) << 16U) | ((CSD[2] & 0xFFFF0000U) >> 16U));
			CardInfo.BlockNbr = ((pCSD->DeviceSize + 1U) * 1024U);
			CardInfo.LogBlockNbr = CardInfo.BlockNbr;
			CardInfo.BlockSize = BLOCKSIZE;
			CardInfo.LogBlockSize = CardInfo.BlockSize;
		}
		else
		{
			// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS)
			self[SDReg::ICR] = (_IMM1S(0U)) | (_IMM1S(1U)) | (_IMM1S(2U)) |
				(_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) |
				(_IMM1S(6U)) | (_IMM1S(7U)) | (_IMM1S(8U)) |
				(_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) |
				(_IMM1S(21U)) | (_IMM1S(22U)) | (_IMM1S(23U)) |
				(_IMM1S(24U)) | (_IMM1S(25U)) | (_IMM1S(26U)) |
				(_IMM1S(27U)) | (_IMM1S(28U));
			//{} hsd->ErrorCode |= HAL_SD_ERROR_UNSUPPORTED_FEATURE;
			return false;
		}
		pCSD->EraseGrSize = (uint8_t)((CSD[2] & 0x00004000U) >> 14U);
		pCSD->EraseGrMul = (uint8_t)((CSD[2] & 0x00003F80U) >> 7U);
		pCSD->WrProtectGrSize = (uint8_t)(CSD[2] & 0x0000007FU);
		pCSD->WrProtectGrEnable = (uint8_t)((CSD[3] & 0x80000000U) >> 31U);
		pCSD->ManDeflECC = (uint8_t)((CSD[3] & 0x60000000U) >> 29U);
		pCSD->WrSpeedFact = (uint8_t)((CSD[3] & 0x1C000000U) >> 26U);
		pCSD->MaxWrBlockLen = (uint8_t)((CSD[3] & 0x03C00000U) >> 22U);
		pCSD->WriteBlockPaPartial = (uint8_t)((CSD[3] & 0x00200000U) >> 21U);
		pCSD->Reserved3 = 0;
		pCSD->ContentProtectAppli = (uint8_t)((CSD[3] & 0x00010000U) >> 16U);
		pCSD->FileFormatGroup = (uint8_t)((CSD[3] & 0x00008000U) >> 15U);
		pCSD->CopyFlag = (uint8_t)((CSD[3] & 0x00004000U) >> 14U);
		pCSD->PermWrProtect = (uint8_t)((CSD[3] & 0x00002000U) >> 13U);
		pCSD->TempWrProtect = (uint8_t)((CSD[3] & 0x00001000U) >> 12U);
		pCSD->FileFormat = (uint8_t)((CSD[3] & 0x00000C00U) >> 10U);
		pCSD->ECC = (uint8_t)((CSD[3] & 0x00000300U) >> 8U);
		pCSD->CSD_CRC = (uint8_t)((CSD[3] & 0x000000FEU) >> 1U);
		pCSD->Reserved4 = 1;
		return true;
	}
	extern "C" void f_0();

	bool SecureDigitalCard_t::SDMMC_CmdSendCID(uint32* feedback) const {
		CmdInitType cit;
		cit.argument = 0;
		cit.CmdIndex = SDMMC_CMD_ALL_SEND_CID;
		cit.Response = 0b11;// SDMMC_RESPONSE_LONG;
		cit.WaitForInterrupt = WaitForInterrupt_E::None;
		cit.CPSM = true;// enable
		asrtret(SDMMC_SendCommand(cit));// >>> SDMMCx->ARG(cit.argument)
		asrtret(SDMMC_GetCmdResp2(feedback));
		return true;
	}
	bool SecureDigitalCard_t::SDMMC_CmdSendCSD(uint32 Argument, uint32* feedback) const {
		// Send CMD9 SEND_CSD
		CmdInitType cit;
		cit.argument = Argument;
		cit.CmdIndex = SDMMC_CMD_SEND_CSD;
		cit.Response = 0b11;// SDMMC_RESPONSE_LONG;
		cit.WaitForInterrupt = WaitForInterrupt_E::None;
		cit.CPSM = true;// enable
		asrtret(SDMMC_SendCommand(cit));// >>> SDMMCx->ARG(cit.argument)
		asrtret(SDMMC_GetCmdResp2(feedback));
		return true;
	}

	bool SecureDigitalCard_t::SDMMC_CmdSetRelAdd(uint16* pRCA, uint32* feedback) const {
		asrtret(SDMMC_SendCommand(0, SDMMC_CMD_SET_REL_ADDR, 0b01));
		asrtret(SDMMC_GetCmdResp6(SDMMC_CMD_SET_REL_ADDR, pRCA, feedback));
		return true;
	}

	uint32 SecureDigitalCard_t::SDMMC_CmdReadSingleBlock(uint32 ReadAdd, uint32* feedback) {
		SDMMC_SendCommand(ReadAdd, SDMMC_CMD_READ_SINGLE_BLOCK, 0b01);
		return SDMMC_GetCmdResp1(SDMMC_CMD_READ_SINGLE_BLOCK, SDMMC_CMDTIMEOUT, feedback);
	}
	uint32 SecureDigitalCard_t::SDMMC_CmdReadMultiBlock(uint32 ReadAdd, uint32* feedback) {
		SDMMC_SendCommand(ReadAdd, SDMMC_CMD_READ_MULT_BLOCK, 0b01);
		return SDMMC_GetCmdResp1(SDMMC_CMD_READ_MULT_BLOCK, SDMMC_CMDTIMEOUT, feedback);
	}

	uint32 SecureDigitalCard_t::SDMMC_CmdWriteSingleBlock(uint32 ReadAdd, uint32* feedback) {
		SDMMC_SendCommand(ReadAdd, SDMMC_CMD_WRITE_SINGLE_BLOCK, 0b01);
		return SDMMC_GetCmdResp1(SDMMC_CMD_WRITE_SINGLE_BLOCK, SDMMC_CMDTIMEOUT, feedback);
	}
	uint32 SecureDigitalCard_t::SDMMC_CmdWriteMultiBlock(uint32 ReadAdd, uint32* feedback) {
		SDMMC_SendCommand(ReadAdd, SDMMC_CMD_WRITE_MULT_BLOCK, 0b01);
		return SDMMC_GetCmdResp1(SDMMC_CMD_WRITE_MULT_BLOCK, SDMMC_CMDTIMEOUT, feedback);
	}

	// Send the Start Address Erase command (not) for SD and check the response
	bool SecureDigitalCard_t::SDMMC_CmdSDEraseStartAdd(uint32 StartAdd, uint32* feedback) {
		SDMMC_SendCommand(StartAdd, SDMMC_CMD_SD_ERASE_GRP_START, 0b01);
		return SDMMC_GetCmdResp1(SDMMC_CMD_SD_ERASE_GRP_START, SDMMC_CMDTIMEOUT, feedback);
	}
	bool SecureDigitalCard_t::SDMMC_CmdEraseStartAdd(uint32 StartAdd, uint32* feedback) {
		SDMMC_SendCommand(StartAdd, SDMMC_CMD_ERASE_GRP_START, 0b01);
		return SDMMC_GetCmdResp1(SDMMC_CMD_ERASE_GRP_START, SDMMC_CMDTIMEOUT, feedback);
	}

	// Send the End Address Erase command (not) for SD and check the response
	bool SecureDigitalCard_t::SDMMC_CmdSDEraseEndAdd(uint32 EndAdd, uint32* feedback) {
		SDMMC_SendCommand(EndAdd, SDMMC_CMD_SD_ERASE_GRP_END, 0b01);
		return SDMMC_GetCmdResp1(SDMMC_CMD_SD_ERASE_GRP_END, SDMMC_CMDTIMEOUT, feedback);
	}
	bool SecureDigitalCard_t::SDMMC_CmdEraseEndAdd(uint32 EndAdd, uint32* feedback) {
		SDMMC_SendCommand(EndAdd, SDMMC_CMD_ERASE_GRP_END, 0b01);
		return SDMMC_GetCmdResp1(SDMMC_CMD_ERASE_GRP_END, SDMMC_CMDTIMEOUT, feedback);
	}

	bool SecureDigitalCard_t::SDMMC_CmdErase(uint32 EraseType, uint32* feedback) {
		SDMMC_SendCommand(EraseType, SDMMC_CMD_ERASE, 0b01);
		return SDMMC_GetCmdResp1(SDMMC_CMD_ERASE, SDMMC_MAXERASETIMEOUT, feedback);
	}

	// Send the Stop Transfer command and check the response
	bool SecureDigitalCard_t::SDMMC_CmdStopTransfer(uint32* feedback) {
		// Send CMD12 STOP_TRANSMISSION
		self[SDReg::CMD].setof(7);// __SDMMC_CMDSTOP_ENABLE: CMDSTOP
		self[SDReg::CMD].rstof(6);// __SDMMC_CMDTRANS_DISABLE: SDMMC_CMD_CMDTRANS
		SDMMC_SendCommand(nil, SDMMC_CMD_STOP_TRANSMISSION, 0b01);
		SDMMC_GetCmdResp1(SDMMC_CMD_STOP_TRANSMISSION, SDMMC_STOPTRANSFERTIMEOUT, feedback);
		self[SDReg::CMD].setof(7, false);// __SDMMC_CMDSTOP_DISABLE
		// Ignore Address Out Of Range Error, Not relevant at end of memory
		if (*feedback == SDMMC_ERROR_ADDR_OUT_OF_RANGE) {
			*feedback = SDMMC_ERROR_NONE;
		}
		return *feedback == SDMMC_ERROR_NONE;
	}

	// Send the Bus Width command and check the response
	bool SecureDigitalCard_t::SDMMC_CmdBusWidth(uint32 BusWidth, uint32* feedback) {
		SDMMC_SendCommand(BusWidth, SDMMC_CMD_APP_SD_SET_BUSWIDTH, 0b01);
		return SDMMC_GetCmdResp1(SDMMC_CMD_APP_SD_SET_BUSWIDTH, SDMMC_CMDTIMEOUT, feedback);
	}


	bool SecureDigitalCard_t::SDMMC_CmdSelDesel(uint32 Addr, uint32* feedback) const {
		// Send CMD7 SDMMC_SEL_DESEL_CARD
		SDMMC_SendCommand(Addr, SDMMC_CMD_SEL_DESEL_CARD, 0x01, WaitForInterrupt_E::None, true);
		asrtret(SDMMC_GetCmdResp1(SDMMC_CMD_SEL_DESEL_CARD, SDMMC_CMDTIMEOUT, feedback));
		return true;
	}

	// Send the Send SCR command and check the response
	bool SecureDigitalCard_t::SDMMC_CmdSendSCR(uint32* feedback) {
		SDMMC_SendCommand(nil, SDMMC_CMD_SD_APP_SEND_SCR, 0b01);
		return SDMMC_GetCmdResp1(SDMMC_CMD_SD_APP_SEND_SCR, SDMMC_CMDTIMEOUT, feedback);
	}

	// Send the Set Relative Address command to MMC card (not SD card)
	bool SecureDigitalCard_t::SDMMC_CmdSetRelAddMmc(uint16 RCA, uint32* feedback) {
		SDMMC_SendCommand(RCA << 16, SDMMC_CMD_SET_REL_ADDR, 0b01);
		return SDMMC_GetCmdResp1(SDMMC_CMD_SET_REL_ADDR, SDMMC_CMDTIMEOUT, feedback);
	}

	// Send the Sleep command to MMC card (not SD card)
	uint32 SecureDigitalCard_t::SDMMC_CmdSleepMmc(uint32 Argument, uint32* feedback) {
		SDMMC_SendCommand(Argument, SDMMC_CMD_MMC_SLEEP_AWAKE, 0b01);
		return SDMMC_GetCmdResp1(SDMMC_CMD_MMC_SLEEP_AWAKE, SDMMC_CMDTIMEOUT, feedback);
	}

	// Send the Status command and check the response
	bool SecureDigitalCard_t::SDMMC_CmdSendStatus(uint32 Argument, uint32* feedback) {
		SDMMC_SendCommand(Argument, SDMMC_CMD_SEND_STATUS, 0b01);
		return SDMMC_GetCmdResp1(SDMMC_CMD_SEND_STATUS, SDMMC_CMDTIMEOUT, feedback);
	}

	// Send the Status register command and check the response
	bool SecureDigitalCard_t::SDMMC_CmdStatusRegister(uint32* feedback) {
		SDMMC_SendCommand(nil, SDMMC_CMD_SD_APP_STATUS, 0b01);
		return SDMMC_GetCmdResp1(SDMMC_CMD_SD_APP_STATUS, SDMMC_CMDTIMEOUT, feedback);
	}

	// Sends host capacity support information and activates the card's initialization process. Send SDMMC_CMD_SEND_OP_COND command
	bool SecureDigitalCard_t::SDMMC_CmdOpCondition(uint32 Argument, uint32* feedback) {
		SDMMC_SendCommand(Argument, SDMMC_CMD_SEND_OP_COND, 0b01);
		return SDMMC_GetCmdResp3(feedback);
	}

	// Checks switchable function and switch card function. SDMMC_CMD_HS_SWITCH command
	bool SecureDigitalCard_t::SDMMC_CmdSwitch(uint32 Argument, uint32* feedback) {
		SDMMC_SendCommand(Argument,// SDMMC_SDR25_SWITCH_PATTERN
			SDMMC_CMD_HS_SWITCH, 0b01);
		return SDMMC_GetCmdResp1(SDMMC_CMD_HS_SWITCH, SDMMC_CMDTIMEOUT, feedback);
	}

	// Send the command asking the accessed card to send its operating condition register (OCR)
	bool SecureDigitalCard_t::SDMMC_CmdVoltageSwitch(uint32* feedback) {
		SDMMC_SendCommand(nil, SDMMC_CMD_VOLTAGE_SWITCH, 0b01);
		return SDMMC_GetCmdResp1(SDMMC_CMD_VOLTAGE_SWITCH, SDMMC_CMDTIMEOUT, feedback);
	}

	// Send the Send EXT_CSD command and check the response
	bool SecureDigitalCard_t::SDMMC_CmdSendEXTCSD(uint32 Argument, uint32* feedback) {
		SDMMC_SendCommand(Argument, SDMMC_CMD_HS_SEND_EXT_CSD, 0b01);
		return SDMMC_GetCmdResp1(SDMMC_CMD_HS_SEND_EXT_CSD, SDMMC_CMDTIMEOUT, feedback);
	}





















	// Send the Application command to verify that that the next command is an application specific com-mand rather than a standard command and check the response.
	bool SecureDigitalCard_t::SDMMC_CmdAppCommand(uint32 Argument, uint32* feedback) const {
		CmdInitType cit;
		cit.argument = Argument;
		cit.CmdIndex = SDMMC_CMD_APP_CMD;
		cit.Response = 1;// SDMMC_RESPONSE_SHORT;
		cit.WaitForInterrupt = WaitForInterrupt_E::None;
		cit.CPSM = true;
		SDMMC_SendCommand(cit);
		// Check for error conditions
		// If there is a HAL_ERROR, it is a MMC card, else it is a SD card: SD card 2.0 (voltage range mismatch) or SD card 1.x
		uint32 error_state;
		SDMMC_GetCmdResp1(SDMMC_CMD_APP_CMD, SDMMC_CMDTIMEOUT, &error_state);
		asserv(feedback)[nil] = error_state;
		return error_state == SDMMC_ERROR_NONE;
	}
	// Send the command asking the accessed card to send its operating condition register (OCR)
	bool SecureDigitalCard_t::SDMMC_CmdAppOperCommand(uint32 Argument, uint32* feedback) const {
		CmdInitType cit;
		cit.argument = Argument;
		cit.CmdIndex = SDMMC_CMD_SD_APP_OP_COND;
		cit.Response = 1;// SDMMC_RESPONSE_SHORT;
		cit.WaitForInterrupt = WaitForInterrupt_E::None;
		cit.CPSM = true;
		SDMMC_SendCommand(cit);
		uint32 error_state;
		SDMMC_GetCmdResp3(&error_state);
		asserv(feedback)[nil] = error_state;
		return error_state == SDMMC_ERROR_NONE;
	}

	// Configure the SDMMC data path according to the specified parameters in the SDMMC_DataInitTypeDef.
	bool SecureDigitalCard_t::SDMMC_ConfigData(const SDMMC_DataInitTypeDef& Data) {
		asrtret(Data.DataLength <= 0x01FFFFFFU);
		asrtret(Data.DataBlockSize < 0b1111);// SDMMC_DATABLOCK_SIZE_xB x=2**DataBlockSize
		self[SDReg::DTIMER] = Data.DataTimeOut;
		self[SDReg::DLEN] = Data.DataLength;
		Reflocal(data_ctrl) = self[SDReg::DCTRL];
		data_ctrl.setof(0, (Data.DPSM));
		data_ctrl.setof(1, _IMM(Data.TransferDir));
		data_ctrl.maset(2, 2, _IMM(Data.TransferMode));
		data_ctrl.maset(4, 4, Data.DataBlockSize);// DBLOCKSIZE
		self[SDReg::DCTRL] = data_ctrl;
		return true;
	}


	bool SecureDigitalCard_t::SD_SendSDStatus(uint32* pSDstatus, uint32* feedback) {
		uint32 tickstart = SysTick::getTick();

		_TODO
			
	}






#endif
}

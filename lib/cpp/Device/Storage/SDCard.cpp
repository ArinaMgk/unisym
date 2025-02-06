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

#include "../../../../inc/cpp/Device/SD.hpp"
#include "../../../../inc/cpp/Device/Storage/SD-PARA.h"
#include "../../../../inc/cpp/Device/RCC/RCC"
#include "../../../../inc/cpp/Device/GPIO"
#include "../../../../inc/cpp/Device/SysTick"
#include "../../../../inc/c/driver/interrupt/GIC.h"
#include "../../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"

#define BLOCKSIZE 512
static const _TEMP stduint block_bytes { BLOCKSIZE };

#define bitmatch(bits,mask) (((bits) & (mask)) == (mask))
#define statin inline static

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

namespace uni {
#if defined(_MPU_STM32MP13)
	static const uint32 _REFADDR_SDMMC[] = { nil,
		AHB6_PERIPH_BASE + 0x5000UL,
		AHB6_PERIPH_BASE + 0x7000UL,
	};

	SecureDigitalCard_t SDCard1(1);

	Reference SecureDigitalCard_t::operator[](SDReg idx) const {
		return _REFADDR_SDMMC[getID()] + _IMMx4(idx);
	}

	SDMMC_CLKSRC SecureDigitalCard_t::getClockSource() const {
		return (SDMMC_CLKSRC)RCC[RCCReg::SDMMC12CKSELR].masof(3 * (getID() - 1), 3);
	}
	void SecureDigitalCard_t::setClockSource(SDMMC_CLKSRC clk_src) const {
		RCC[RCCReg::SDMMC12CKSELR].maset(3 * (getID() - 1), 3, _IMM(clk_src));
	}
	stduint SecureDigitalCard_t::getFrequency() const {
		stduint frequency = 0;
		switch (getClockSource()) {
		case SDMMC_CLKSRC::HCLK6:
			// AKA HAL_RCC_GetHCLK5/6Freq = HAL_RCC_GetACLKFreq
			if (true) // AKA HAL_RCC_GetACLKFreq
			{
				using namespace RCCReg;
				uint32 axidiv = RCC[AXIDIVR].masof(0, 3) & 0b011;// AXIDIV
				frequency = RCC.AXIS.getFrequency() / (axidiv + 1);
			}
			break;
		case SDMMC_CLKSRC::PLL3:
			frequency = RCC.PLL3.getFrequencyR();
			break;
		case SDMMC_CLKSRC::PLL4:
			frequency = RCC.PLL4.getFrequencyP();
			break;
		case SDMMC_CLKSRC::HSI:
			frequency = RCC.HSI.getFrequency();
			break;
		default: break;
		}
		return frequency;
	}

	bool SecureDigitalCard_t::Read(stduint BlockIden, void* Dest) {
		// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_DATA_FLAGS)
		self[SDReg::ICR] = (_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) | (_IMM1S(8U)) | (_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) | (_IMM1S(27U)) | (_IMM1S(28U));
		//[1] (? How to Use)
		// Slice LinkNodeConf{
		// 	.address = _IMM(Dest),// not roleaddr
		// 	.length = block_bytes,
		// };
		// SDMMC_DMALinkNode pLinkNode = { 0 };
		// SDMMC_DMALinkedList Read_LinkedList = { 0 };
		// SDMMC_DMALinkedList_BuildNode(&pLinkNode, &LinkNodeConf);
		// SDMMC_DMALinkedList_InsertNode(&Read_LinkedList, NULL, &pLinkNode);
		// SDMMC_DMALinkedList_EnableCircularMode(&Read_LinkedList, false);
		// HAL_SDEx_DMALinkedList_ReadBlocks(&Read_LinkedList, BlockIden, 1);
		//[2]
		HAL_SD_ReadBlocks((uint8_t*)Dest, BlockIden, 1, 1000, NULL);
		while (HAL_SD_GetCardState() != HAL_SD_CardStateTypeDef::TRANSFER);
		return true;
	}

	bool SecureDigitalCard_t::Write(stduint BlockIden, const void* Sors) {
		// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_DATA_FLAGS)
		self[SDReg::ICR] = (_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) | (_IMM1S(8U)) | (_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) | (_IMM1S(27U)) | (_IMM1S(28U));
		//[1] single node case (? Unchk)
		// HAL_SDEx_DMALinkedList_BuildNode(&pLinkNode[0], &LinkNodeConf);
		// HAL_SDEx_DMALinkedList_InsertNode(&Write_LinkedList, NULL, &pLinkNode[0]);
		// /*Enable Circular Linked list mode */
		// HAL_SDEx_DMALinkedList_EnableCircularMode(&Write_LinkedList);
		// TxLnkLstBufCplt = 0;
		// TxNodeBufCplt = 0;
		// /* Write 24MB  on uSD card using DMA transfer */
		// HAL_SDEx_DMALinkedList_WriteBlocks(&hsd1, &Write_LinkedList, DATA_ADDRESS, DATA_SIZE / BLOCKSIZE);
		// // (here) HAL_SDEx_DMALinkedList_LockNode .. R/W Operations for Buffer .. HAL_SDEx_DMALinkedList_UnlockNode
		// /* Remove write nodes, remove node on the right then left */
		// HAL_SDEx_DMALinkedList_RemoveNode(&Write_LinkedList, &pLinkNode[0]);
		//[2]
		stduint timeo = 1000;// assume 1kHz SysTick
		HAL_SD_WriteBlocks((uint8_t*)Sors, BlockIden, 1, timeo, NULL);
		while (HAL_SD_GetCardState() != HAL_SD_CardStateTypeDef::TRANSFER);
		return true;
	}


#endif
}

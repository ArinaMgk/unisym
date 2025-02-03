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

#define statin inline static
#define bitmatch(bits,mask) (((bits) & (mask)) == (mask))
#define whenis(x) case(x):// when is

// SDMMC_Internal_DMA_Mode  SDMMC Internal DMA Mode
#define SDMMC_DISABLE_IDMA              ((uint32)0x00000000)
#define SDMMC_ENABLE_IDMA_SINGLE_BUFF   (SDMMC_IDMA_IDMAEN)
#define SDMMC_ENABLE_IDMA_DOUBLE_BUFF0  (SDMMC_IDMA_IDMAEN | SDMMC_IDMA_IDMABMODE)
#define SDMMC_ENABLE_IDMA_DOUBLE_BUFF1  (SDMMC_IDMA_IDMAEN | SDMMC_IDMA_IDMABMODE | SDMMC_IDMA_IDMABACT)

#define BLOCKSIZE 512

namespace uni {
#if defined(_MPU_STM32MP13)

	bool SecureDigitalCard_t::SDMMC_DMALinkedList_InsertNode(SDMMC_DMALinkedList* pLinkedList, SDMMC_DMALinkNode* pPrevNode, SDMMC_DMALinkNode* pNode)
	{
		uint32 link_list_offset;
		uint32 node_address = (uint32)pNode;
		if (pLinkedList->NodesCounter == 0U)// as First Node
		{
			pLinkedList->pHeadNode = pNode;
			pLinkedList->pTailNode = pNode;
			pLinkedList->NodesCounter = 1U;
		}
		else if (pPrevNode == pLinkedList->pTailNode) {
			if (pNode <= pLinkedList->pHeadNode)// (STMic. Rule) Node Address should greater than Head Node Address
				return false;
			// Last Node
			// SDMMC_IDMALAR_ULA
			Reference(&pNode->IDMALAR).setof(31, 0);//{unchk}
			// link Prev node with new one
			Reference(&pPrevNode->IDMALAR).setof(31, 1);
			Reference(&pPrevNode->IDMALAR).maset(2, 14, node_address - (uint32)pLinkedList->pHeadNode);// IDMALA
			pLinkedList->NodesCounter++;
			pLinkedList->pTailNode = pNode;
		}
		else {
			if (pNode <= pLinkedList->pHeadNode)// (STMic. Rule) Node Address should greater than Head Node Address
				return false;

			// link New node with Next one
			link_list_offset = pNode->IDMALAR;
			Reference(&pNode->IDMALAR).maset(2, 14, link_list_offset);// IDMALA //{unchk}
			// link Prev node with new one
			Reference(&pPrevNode->IDMALAR).setof(31, 1);
			Reference(&pPrevNode->IDMALAR).maset(2, 14, node_address - (uint32)pLinkedList->pHeadNode);// IDMALA
			pLinkedList->NodesCounter++;
		}
		return true;
	}

	bool SecureDigitalCard_t::SDMMC_DMALinkedList_RemoveNode(SDMMC_DMALinkedList* pLinkedList, SDMMC_DMALinkNode* pNode)
	{
		uint32 count = 0U;
		uint32 linked_list_offset;
		SDMMC_DMALinkNode* prev_node = NULL;
		SDMMC_DMALinkNode* curr_node;
		// First Node
		if (pLinkedList->NodesCounter == 0U)
			return false;
		else {
			curr_node = pLinkedList->pHeadNode;
			while ((curr_node != pNode) && (count <= pLinkedList->NodesCounter))
			{
				prev_node = curr_node;
				Stdfield IDMALA(&curr_node->IDMALAR, 2, 14);
				curr_node = (SDMMC_DMALinkNode*)(IDMALA + pLinkedList->pHeadNode);
				count++;
			}
			if ((count == 0U) || (count > pLinkedList->NodesCounter))// Node not found in the linked list
				return false;
			pLinkedList->NodesCounter--;

			if (pLinkedList->NodesCounter == 0U)
			{
				pLinkedList->pHeadNode = 0U;
				pLinkedList->pTailNode = 0U;
			}
			else {
				// link prev node with next one
				linked_list_offset = curr_node->IDMALAR;
				Stdfield(&prev_node->IDMALAR, 2, 14) = linked_list_offset;// IDMALA
				// Configure the new Link Node registers
				pNode->IDMALAR |= linked_list_offset;
				pLinkedList->pTailNode = prev_node;
			}
		}
		return true;
	}

	bool SecureDigitalCard_t::HAL_SDEx_DMALinkedList_ReadBlocks(SDMMC_DMALinkedList* pLinkedList, uint32 BlockAddr, uint32 NumberOfBlocks, uint32* feedback) {
		SDMMC_DataInitTypeDef config;
		uint32 errorstate;
		uint32 DmaBase0_reg;
		uint32 DmaBase1_reg;
		uint32 add = BlockAddr;

		if ((add + NumberOfBlocks) > (CardInfo.LogBlockNbr))
		{
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}

		self[SDReg::IDMABASER] = pLinkedList->pHeadNode->IDMABASER;
		self[SDReg::IDMABSIZE] = pLinkedList->pHeadNode->IDMABSIZE;

		self[SDReg::IDMABAR] = pLinkedList->pHeadNode;
		self[SDReg::IDMALAR] = _IMM1S(29) | _IMM1S(30) | _IMM1S(31)
			// SDMMC_IDMALAR_ABR | SDMMC_IDMALAR_ULS | SDMMC_IDMALAR_ULA
			| sizeof(SDMMC_DMALinkNode); // Initial configuration

		DmaBase0_reg = self[SDReg::IDMABASER];
		DmaBase1_reg = self[SDReg::IDMABAR];

		if ((self[SDReg::IDMABSIZE] == 0U) || (DmaBase0_reg == 0U) || (DmaBase1_reg == 0U))
		{
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}

		// Initialize data control register
		self[SDReg::DCTRL] = 0;
		// Clear old Flags
		// __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_DATA_FLAGS)
		self[SDReg::ICR] = (_IMM1S(1U)) | (_IMM1S(3U)) | (_IMM1S(4U)) | (_IMM1S(5U)) | (_IMM1S(8U)) | (_IMM1S(9U)) | (_IMM1S(10U)) | (_IMM1S(11U)) | (_IMM1S(27U)) | (_IMM1S(28U));

		asserv(feedback)[nil] = SDMMC_ERROR_NONE;

		if (CardType != CardType_E::SDHC_SDXC)
			add *= 512U;

		// Configure the SD DPSM (Data Path State Machine)
		config.DataTimeOut = SDMMC_DATATIMEOUT;
		config.DataLength = BLOCKSIZE * NumberOfBlocks;
		config.DataBlockSize = 9; // 2**9 == 512B
		config.TransferDir = SDMMC_DataTransferDir::toSDMMC;
		config.TransferMode = SDMMC_DataTransferMode::Block;
		config.DPSM = false;
		(void)SDMMC_ConfigData(config);

		self[SDReg::DCTRL].setof(13);// FIFORST

		self[SDReg::CMD].setof(6); // CMDTRANS __SDMMC_CMDTRANS_ENABLE(hsd->Instance);

		self[SDReg::IDMACTRL] = 0b11; // SDMMC_ENABLE_IDMA_DOUBLE_BUFF0;

		// Read Blocks in DMA mode
		//{} self.Context = (SD_CONTEXT_READ_MULTIPLE_BLOCK | SD_CONTEXT_DMA);

		asrtret(SDMMC_CmdReadMultiBlock(add, feedback));

		// __HAL_SD_ENABLE_IT(hsd, (SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT | SDMMC_IT_RXOVERR | SDMMC_IT_DATAEND | SDMMC_IT_IDMABTC));
		self[SDReg::MASK] |= ((((0x1UL << (1U)) | (0x1UL << (3U)) | (0x1UL << (5U)) | (0x1UL << (8U)) | (0x1UL << (28U)))));

		return true;
	}




	bool SecureDigitalCard_t::HAL_SDEx_DMALinkedList_WriteBlocks(SDMMC_DMALinkedList* pLinkedList, uint32 BlockAddr, uint32 NumberOfBlocks, uint32* feedback) {
		SDMMC_DataInitTypeDef config;
		uint32 errorstate;
		uint32 DmaBase0_reg;
		uint32 DmaBase1_reg;
		uint32 add = BlockAddr;

		if ((add + NumberOfBlocks) > (CardInfo.LogBlockNbr))
		{
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}

		self[SDReg::IDMABASER] = pLinkedList->pHeadNode->IDMABASER;
		self[SDReg::IDMABSIZE] = pLinkedList->pHeadNode->IDMABSIZE;

		self[SDReg::IDMABAR] = pLinkedList->pHeadNode;
		self[SDReg::IDMALAR] = _IMM1S(29) | _IMM1S(30) | _IMM1S(31)
			// SDMMC_IDMALAR_ABR | SDMMC_IDMALAR_ULS | SDMMC_IDMALAR_ULA
			| sizeof(SDMMC_DMALinkNode); // Initial configuration

		DmaBase0_reg = self[SDReg::IDMABASER];
		DmaBase1_reg = self[SDReg::IDMABAR];

		if ((self[SDReg::IDMABSIZE] == 0U) || (DmaBase0_reg == 0U) || (DmaBase1_reg == 0U))
		{
			asserv(feedback)[nil] = SDMMC_ERROR_ADDR_OUT_OF_RANGE;
			return false;
		}

		/* Initialize data control register */
		self[SDReg::DCTRL] = 0;

		asserv(feedback)[nil] = SDMMC_ERROR_NONE;

		if (CardType != CardType_E ::SDHC_SDXC)
			add *= 512U;

		/* Configure the SD DPSM (Data Path State Machine) */
		config.DataTimeOut = SDMMC_DATATIMEOUT;
		config.DataLength = BLOCKSIZE * NumberOfBlocks;
		config.DataBlockSize = 9; // 2**9 == 512B
		config.TransferDir = SDMMC_DataTransferDir::toCard;
		config.TransferMode = SDMMC_DataTransferMode::Block;
		config.DPSM = false;
		(void)SDMMC_ConfigData(config);

		self[SDReg::CMD].setof(6); // CMDTRANS __SDMMC_CMDTRANS_ENABLE(hsd->Instance);

		self[SDReg::IDMACTRL] = 0b11; // SDMMC_ENABLE_IDMA_DOUBLE_BUFF0;

		// Write Blocks in DMA mode
		//{} self.Context = (SD_CONTEXT_WRITE_MULTIPLE_BLOCK | SD_CONTEXT_DMA);

		asrtret(SDMMC_CmdWriteMultiBlock(add, feedback));

		// __HAL_SD_ENABLE_IT(hsd, (SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT | SDMMC_IT_TXUNDERR | SDMMC_IT_DATAEND | SDMMC_IT_IDMABTC));
		self[SDReg::MASK] |= ((((0x1UL << (1U)) | (0x1UL << (3U)) | (0x1UL << (4U)) | (0x1UL << (8U)) | (0x1UL << (28U)))));

		return true;
	}





#endif
}

// ASCII CPL TAB4 LF
// Docutitle: [Device] Sony/Philips Digital Interface Receiver
// Codifiers: @ArinaMgk
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

#ifndef _INC_DEV_PREDEF_SPDIFRX
#define _INC_DEV_PREDEF_SPDIFRX

#include "../../stdinc.h"

#if defined(_MCU_STM32H7x)
#define _SPDIFRX_BASE 0x40004000U
#define _SPDIFRX_RCC_POS_ENABLE 16
#define _SPDIFRX_RCC_POS_RESET 16
#define _SPDIFRX_DMA_REQUEST_DATA 93U
#define _SPDIFRX_DMA_REQUEST_CONTROL 94U

#define _SPDIFRX_CR_POS_ENABLE 0
#define _SPDIFRX_CR_POS_RXDMAEN 2
#define _SPDIFRX_CR_POS_RXSTEO 3
#define _SPDIFRX_CR_POS_DRFMT 4
#define _SPDIFRX_CR_POS_PMSK 6
#define _SPDIFRX_CR_POS_VMSK 7
#define _SPDIFRX_CR_POS_CUMSK 8
#define _SPDIFRX_CR_POS_PTMSK 9
#define _SPDIFRX_CR_POS_CBDMAEN 10
#define _SPDIFRX_CR_POS_CHSEL 11
#define _SPDIFRX_CR_POS_NBTR 12
#define _SPDIFRX_CR_POS_WFA 14
#define _SPDIFRX_CR_POS_INSEL 16
#define _SPDIFRX_CR_POS_CKSEN 20
#define _SPDIFRX_CR_POS_CKSBKPEN 21

#define _SPDIFRX_SR_RXNE (1U << 0)
#define _SPDIFRX_SR_CSRNE (1U << 1)
#define _SPDIFRX_SR_PERR (1U << 2)
#define _SPDIFRX_SR_OVR (1U << 3)
#define _SPDIFRX_SR_SBD (1U << 4)
#define _SPDIFRX_SR_SYNCD (1U << 5)
#define _SPDIFRX_SR_FERR (1U << 6)
#define _SPDIFRX_SR_SERR (1U << 7)
#define _SPDIFRX_SR_TERR (1U << 8)
#define _SPDIFRX_IMR_IFEIE (1U << 6)

#define _SPDIFRX_LINK_ERRORS (_SPDIFRX_SR_FERR | _SPDIFRX_SR_SERR | _SPDIFRX_SR_TERR)
#define _SPDIFRX_IFCR_CLEARABLE (_SPDIFRX_SR_PERR | _SPDIFRX_SR_OVR | _SPDIFRX_SR_SBD | _SPDIFRX_SR_SYNCD)

namespace uni {
	enum class SPDIFRXReg : byte { CR, IMR, SR, IFCR, DR, CSR, DIR };
	enum class SPDIFRXHardwareState : byte { Idle = 0, Synchronizing = 1, Receiving = 3 };
}
#endif
#endif

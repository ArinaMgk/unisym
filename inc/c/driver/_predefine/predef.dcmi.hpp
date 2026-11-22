// ASCII CPL TAB4 LF
// Docutitle: [Device] Digital Camera Interface
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

#ifndef _INC_DEV_PREDEF_DCMI
#define _INC_DEV_PREDEF_DCMI

#include "../../stdinc.h"

#if defined(_MCU_STM32H7x)
#define _DCMI_BASE 0x48020000U
#define _DCMI_DMA_REQUEST 75U
#define _DCMI_TIMEOUT_VALUE 1000U
#define _DCMI_CR_POS_CAPTURE 0
#define _DCMI_CR_POS_CM 1
#define _DCMI_CR_POS_CROP 2
#define _DCMI_CR_POS_JPEG 3
#define _DCMI_CR_POS_ESS 4
#define _DCMI_CR_POS_PCKPOL 5
#define _DCMI_CR_POS_HSPOL 6
#define _DCMI_CR_POS_VSPOL 7
#define _DCMI_CR_POS_FCRC 8
#define _DCMI_CR_POS_EDM 10
#define _DCMI_CR_POS_ENABLE 14
#define _DCMI_CR_POS_BSM 16
#define _DCMI_CR_POS_OEBS 18
#define _DCMI_CR_POS_LSM 19
#define _DCMI_CR_POS_OELS 20
#define _DCMI_SR_POS_HSYNC 0
#define _DCMI_SR_POS_VSYNC 1
#define _DCMI_SR_POS_FNE 2
#define _DCMI_IT_POS_FRAME 0
#define _DCMI_IT_POS_OVR 1
#define _DCMI_IT_POS_ERR 2
#define _DCMI_IT_POS_VSYNC 3
#define _DCMI_IT_POS_LINE 4
#define _DCMI_IT_ALL 0x1FU
#define _DCMI_IT_CAPTURE 0x1EU
#define _DCMI_ESCR_POS_FSC 0
#define _DCMI_ESCR_POS_LSC 8
#define _DCMI_ESCR_POS_LEC 16
#define _DCMI_ESCR_POS_FEC 24
#define _DCMI_ESUR_POS_FSU 0
#define _DCMI_ESUR_POS_LSU 8
#define _DCMI_ESUR_POS_LEU 16
#define _DCMI_ESUR_POS_FEU 24
#define _DCMI_CWSTRT_POS_HOFFCNT 0
#define _DCMI_CWSTRT_POS_VST 16
#define _DCMI_CWSIZE_POS_CAPCNT 0
#define _DCMI_CWSIZE_POS_VLINE 16
#define _DCMI_WINDOW_COORDINATE 0x3FFFU
#define _DCMI_WINDOW_HEIGHT 0x1FFFU
#define _DCMI_RCC_POS_ENABLE 0
#define _DCMI_RCC_POS_RESET 0
#define _DCMI_ERROR_NONE 0x00000000U
#define _DCMI_ERROR_OVR 0x00000001U
#define _DCMI_ERROR_SYNC 0x00000002U
#define _DCMI_ERROR_TIMEOUT 0x00000020U
#define _DCMI_ERROR_DMA 0x00000040U

namespace uni {
	enum class DCMIReg : byte {
		CR, SR, RISR, IER, MISR, ICR, ESCR, ESUR, CWSTRTR, CWSIZER, DR
	};
}
#endif
#endif

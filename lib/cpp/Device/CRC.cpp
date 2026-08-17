// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) CRC
// Codifiers: @dosconio: 20241213 ~ <Last-check> 
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

#include "../../../inc/cpp/Device/CRC"

#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)

#include "../../../inc/cpp/Device/RCC/RCC"
#include "../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"

namespace uni {

#if defined(_MCU_STM32H7x)
	#define _CRC_ADDR (D3_AHB1PERIPH_BASE + 0x4C00)    // 0x58024C00
	#define _CRC_RCC_EN_POS 19                          // RCC_AHB4ENR_CRCEN
#elif defined(_MPU_STM32MP13)
	#define _CRC_ADDR (AHB6_PERIPH_BASE + 0x9000)       // 0x58009000
	#define _CRC_RCC_EN_POS 20                          // RCC_MP_AHB6ENSETR_CRC1EN
#endif

	// CR bits (identical on H7/MP13, from RM0433/RM0475)
	#define _CRC_CR_RESET_Pos     0
	#define _CRC_CR_POLYSIZE_Pos  3
	#define _CRC_CR_REV_IN_Pos    5
	#define _CRC_CR_REV_OUT_Pos   7

	// default polynomial / init value (AKA DEFAULT_CRC32_POLY / DEFAULT_CRC_INITVALUE)
	#define _CRC_DEFAULT_POLY      0x04C11DB7
	#define _CRC_DEFAULT_INITVALUE 0xFFFFFFFF

	Reference CRC_t::operator[](CRCReg idx) const {
		return Reference(_CRC_ADDR + _IMMx4(idx));
	}

	// AKA __HAL_RCC_CRC_CLK_ENABLE / __HAL_RCC_CRC_CLK_DISABLE
	bool CRC_t::enClock(bool ena) const {
	#if defined(_MCU_STM32H7x)
		RCC[RCCReg::AHB4ENR].setof(_CRC_RCC_EN_POS, ena);
	#elif defined(_MPU_STM32MP13)
		RCCReg::RCCReg rcc_reg = ena ? RCCReg::MP_NS_AHB6ENSETR : RCCReg::MP_NS_AHB6ENCLRR;
		RCC[rcc_reg] = _IMM1S(_CRC_RCC_EN_POS);
	#endif
		return true;
	}

	// AKA HAL_CRC_Init (default path: DefaultPolynomialUse=ENABLE, DefaultInitValueUse=ENABLE,
	//   InputDataInversionMode=NONE, OutputDataInversionMode=DISABLE)
	bool CRC_t::setMode() const {
		enClock();
		self[CRCReg::POL] = _CRC_DEFAULT_POLY;
		self[CRCReg::CR].maset(_CRC_CR_POLYSIZE_Pos, 2, (stduint)CRCPolyLength::_32bit);
		self[CRCReg::INIT] = _CRC_DEFAULT_INITVALUE;
		self[CRCReg::CR].maset(_CRC_CR_REV_IN_Pos, 2, (stduint)CRCInversion::None);
		self[CRCReg::CR].setof(_CRC_CR_REV_OUT_Pos, false);
		return true;
	}

	// AKA HAL_CRC_DeInit
	bool CRC_t::canMode() const {
		self[CRCReg::CR].setof(_CRC_CR_RESET_Pos, true);    // __HAL_CRC_DR_RESET
		self[CRCReg::IDR] = 0;                              // __HAL_CRC_SET_IDR(0)
		enClock(false);                                     // HAL_CRC_MspDeInit
		return true;
	}

	// AKA HAL_CRCEx_Polynomial_Set
	bool CRC_t::setPolynomial(stduint polynomial, CRCPolyLength length) const {
		stduint msb = 31;                                    // degree of the second-highest polynomial term (e.g. X^7+X^6+X^5+X^2+1 -> msb = 6)
		stduint degree_limit = 0;
		while (((polynomial & ((stduint)0x1 << msb)) == 0) && (msb-- > 0)) {}
		switch (length) {
			case CRCPolyLength::_7bit:  degree_limit = 7;  break;
			case CRCPolyLength::_8bit:  degree_limit = 8;  break;
			case CRCPolyLength::_16bit: degree_limit = 16; break;
			case CRCPolyLength::_32bit: degree_limit = 32; break;
			default: return false;
		}
		if (msb >= degree_limit) return false;
		self[CRCReg::POL] = polynomial;
		self[CRCReg::CR].maset(_CRC_CR_POLYSIZE_Pos, 2, (stduint)length);
		return true;
	}

	// AKA __HAL_CRC_INITIALCRCVALUE_CONFIG
	bool CRC_t::setInitValue(stduint value) const {
		self[CRCReg::INIT] = value;
		return true;
	}

	// AKA HAL_CRCEx_Input_Data_Reverse + Output_Data_Reverse
	bool CRC_t::setInversion(CRCInversion input, bool output_reverse) const {
		self[CRCReg::CR].maset(_CRC_CR_REV_IN_Pos, 2, (stduint)input);
		self[CRCReg::CR].setof(_CRC_CR_REV_OUT_Pos, output_reverse);
		return true;
	}

	// AKA HAL_CRC_Calculate
	stduint CRC_t::Calculate(pureptr_t buf, stduint leng, CRCInputFormat format) const {
		return _compute(buf, leng, format, true);
	}

	// AKA HAL_CRC_Accumulate
	stduint CRC_t::Accumulate(pureptr_t buf, stduint leng, CRCInputFormat format) const {
		return _compute(buf, leng, format, false);
	}

	stduint CRC_t::_compute(pureptr_t buf, stduint leng, CRCInputFormat format, bool reset) const {
		stduint temp = 0;
		stduint i = 0;
		if (reset)
			self[CRCReg::CR].setof(_CRC_CR_RESET_Pos, true);    // __HAL_CRC_DR_RESET
		switch (format) {
			case CRCInputFormat::Word: {
				const stduint* p = (const stduint*)buf;
				for (i = 0; i < leng; i++)
					self[CRCReg::DR] = p[i];
				temp = self[CRCReg::DR];
				break;
			}
			case CRCInputFormat::Byte: {
				// AKA CRC_Handle_8: pack 4 bytes big-endian into one word write; tail 1/2/3 bytes via sub-word writes
				const byte* p = (const byte*)buf;
				for (i = 0; i < leng / 4; i++)
					self[CRCReg::DR] = ((stduint)p[4*i] << 24) | ((stduint)p[4*i+1] << 16)
						| ((stduint)p[4*i+2] << 8) | (stduint)p[4*i+3];
				if (leng % 4 == 1)
					Reference_T<byte>(_CRC_ADDR) = p[4*i];
				if (leng % 4 == 2)
					Reference_T<word>(_CRC_ADDR) = (word)(((word)p[4*i] << 8) | (word)p[4*i+1]);
				if (leng % 4 == 3) {
					Reference_T<word>(_CRC_ADDR) = (word)(((word)p[4*i] << 8) | (word)p[4*i+1]);
					Reference_T<byte>(_CRC_ADDR) = p[4*i+2];
				}
				temp = self[CRCReg::DR];
				break;
			}
			case CRCInputFormat::Halfword: {
				// AKA CRC_Handle_16: pack 2 halfwords into one word write; odd tail halfword via a sub-word write
				const word* p = (const word*)buf;
				for (i = 0; i < leng / 2; i++)
					self[CRCReg::DR] = ((stduint)p[2*i] << 16) | (stduint)p[2*i+1];
				if (leng % 2 != 0)
					Reference_T<word>(_CRC_ADDR) = p[2*i];
				temp = self[CRCReg::DR];
				break;
			}
			default:
				break;
		}
		return temp;
	}

	// AKA __HAL_CRC_SET_IDR
	void CRC_t::setIDR(stduint value) const {
		self[CRCReg::IDR] = value;
	}

	// AKA __HAL_CRC_GET_IDR
	stduint CRC_t::getIDR() const {
		return self[CRCReg::IDR] & 0xFFFFFFFF;
	}

	CRC_t CRC;

}

#endif

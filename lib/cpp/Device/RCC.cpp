// ASCII CPP TAB4 CRLF
// Docutitle: Reset and Clock Control
// Datecheck: 20240420 ~ <Last-check>
// Developer: @dosconio
// Attribute: <ArnCovenant> <Environment> <Platform>
// Reference: <Reference>
// Dependens: <Dependence>
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

#include "../../../inc/cpp/Device/RCC/RCC"

namespace uni {
	bool RCCAHB::setMode(uint8 divexpo, bool usingPCLK1, bool usingPCLK2) {
		// "Set the highest APBx dividers in order to ensure that we do not go through a non-spec phase whatever we decrease or increase HCLK"
		Reference Cfgreg(_RCC_CFGR);
		if (usingPCLK1) Cfgreg |= 0x00000700;
		if (usingPCLK2) Cfgreg |= 0x00000700 << 3;
		// Set the new HCLK clock divider
		if (divexpo >= 9) return false;
		if (divexpo) divexpo = (divexpo - 1) | 0x8;// zero => zero-self
		Cfgreg |= divexpo << 4;// MGK number!
		return true;
	}
	//
	bool RCCSystemClock::setMode(SysclkSource::RCCSysclockSource source) {
		bool state;
		switch (source) {
		case SysclkSource::HSI:
			state = RCCOscillatorHSI::isReady();
			break;
		case SysclkSource::HSE:
			state = RCCOscillatorHSE::isReady();
			break;
		case SysclkSource::PLL:
			state = RCCPLL::isReady();
			break;
		default:
			state = false;
			break;
		}
		if (!state) return false;
		setSource(source);
		while (getSource() != source);
		return true;
	}
	//
	RCC_t RCC;
}


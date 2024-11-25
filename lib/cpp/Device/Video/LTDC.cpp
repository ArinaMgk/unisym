// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Deivce) LTDC
// Codifiers: @dosconio: 20241123 ~ <Last-check> 
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

#include "../../../../inc/cpp/Device/LTDC"
#include "../../../../inc/cpp/Device/RCC/RCC"
#include "../../../../inc/cpp/Device/RCC/RCCAddress"
#if defined(_MPU_STM32MP13)


namespace uni {
	LTDC_t LTDC;

	void LTDC_t::enClock(bool ena) {
		RCC[ena ? RCCReg::MP_NS_APB4ENSETR : RCCReg::MP_NS_APB4ENCLRR] = 0x00000001;
	}
	
	bool LTDC_t::setMode(Color color) {
		enClock();
		//: List and Check parameter
		stduint VerticalSync = vertical.sync_len - 1;
		stduint HorizontalSync = horizontal.sync_len - 1;
		stduint AccumulatedVBP = VerticalSync + vertical.back_porch;
		stduint AccumulatedHBP = HorizontalSync + horizontal.back_porch;
		stduint AccumulatedActiveH = AccumulatedVBP + vertical.active_len;
		stduint AccumulatedActiveW = AccumulatedHBP + horizontal.active_len;
		stduint TotalHeigh = AccumulatedActiveH + vertical.front_porch;
		stduint TotalWidth = AccumulatedActiveW + horizontal.front_porch;
		//: Apply and Enable
		self[LTDCReg::GCR].setof(31, HSPolarity);
		self[LTDCReg::GCR].setof(30, VSPolarity);
		self[LTDCReg::GCR].setof(29, DEPolarity);
		self[LTDCReg::GCR].setof(28, PCPolarity);
		self[LTDCReg::SSCR].maset(0, 12, VerticalSync);// VSH
		self[LTDCReg::SSCR].maset(16, 12, HorizontalSync);// HSW
		self[LTDCReg::BPCR].maset(0, 12, AccumulatedVBP);// AVBP
		self[LTDCReg::BPCR].maset(16, 12, AccumulatedHBP);// AHBP
		self[LTDCReg::AWCR].maset(0, 12, AccumulatedActiveH);// AAH
		self[LTDCReg::AWCR].maset(16, 12, AccumulatedActiveW);// AAW
		self[LTDCReg::TWCR].maset(0, 12, TotalHeigh);// TOTALH
		self[LTDCReg::TWCR].maset(16, 12, TotalWidth);// TOTALW
		self[LTDCReg::BCCR].maset(0x00, 8, color.b);
		self[LTDCReg::BCCR].maset(0x08, 8, color.g);
		self[LTDCReg::BCCR].maset(0x10, 8, color.r);
		self[LTDCReg::EDCR].maset(25, 3, nil);//{TODO} Configure the output to YCbCr 422: Enable, the CCIR hard-wired coefficients, chrominance order
		self[LTDCReg::FUTR].maset(0, 16, nil);//{TODO} THRE, Configure the Fifo Underrun Threshold register
		//: Enable the Transfer Error and FIFO underrun interrupts
		// AKA __HAL_LTDC_ENABLE_IT(hltdc, LTDC_IT_TE | LTDC_IT_FU)
		self[LTDCReg::IER].setof(1);// LTDC_IER_FUWIE
		self[LTDCReg::IER].setof(2);// LTDC_IER_TERRIE
		// AKA __HAL_LTDC_ENABLE_SECURE_IT(hltdc, LTDC_IT_TE | LTDC_IT_FU)
		self[LTDCReg::IER2].setof(1);// LTDC_IER_FUWIE
		self[LTDCReg::IER2].setof(2);// LTDC_IER_TERRIE
		enAble();
		return true;
	}

	void LTDC_t::enAble(bool ena) {
		self[LTDCReg::GCR].setof(0, ena);// LTDCEN
	}
	
	stduint LTDC_t::getFrequency() {
		return RCC.PLL4.getFrequencyQ();
	}

}

#endif

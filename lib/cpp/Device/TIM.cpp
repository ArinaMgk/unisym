// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) TIM
// Codifiers: @dosconio: 20240422 ~ <Last-check> 
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

#include "../../inc/cpp/Device/TIM"

namespace uni {
#if 0
#elif defined(_MCU_STM32F10x)
//#undef TIM

	void TIM_t::setInterrupt(Handler_t fn) {
		FUNC_TIMx[getID()] = fn;
	}

	static Request_t TIM_Request_list[16] = {
		(Request_t)0, (Request_t)0, (Request_t)0, (Request_t)0,
		(Request_t)0, (Request_t)0, IRQ_TIM6, IRQ_TIM7,
	};
	void TIM_t::setInterruptPriority(byte preempt, byte sub_priority) {
		NVIC.setPriority(TIM_Request_list[getID()], preempt, sub_priority);
	}
	//{TODO}
	void TIM_t::enInterrupt(bool enable) {
		if (enable)
		{
			NVIC.setAble(TIM_Request_list[(*this).getID()]);
			//aka HAL_TIM_Base_Start_IT
			{
				(*this)[TimReg::DIER] |= 1;// TIM_IT_UPDATE
				if (getID() <= 5 || getID() == 8) //aka IS_TIM_SLAVE_INSTANCE, true for TIM 1/2/3/4/5/8
				{
					/* TODO (why so?)
					tmpsmcr = htim->Instance->SMCR & TIM_SMCR_SMS;
					if (!IS_TIM_SLAVEMODE_TRIGGER_ENABLED(tmpsmcr))
					{
						__HAL_TIM_ENABLE(htim);
					}
					*/
				}
				else enAble();
			}
		}
		else _TODO;
	}

	void TIM_t::ConfigMaster(_TEMP stduint master_output_triggerm, bool master_slave_enable) {
		// Compatible with `HAL_StatusTypeDef HAL_TIMEx_MasterConfigSynchronization(TIM_HandleTypeDef *htim, TIM_MasterConfigTypeDef * sMasterConfig)`
		(*this)[TimReg::CR2] &= ~(7U << 4); // Reset MMS Bits
		(*this)[TimReg::CR2] |= master_output_triggerm; // Select the TRGO source
		(*this)[TimReg::SMCR].setof(_TIM_SMCR_POS_MSM, master_slave_enable);
	}
	
	TIM_B TIM6(0x40001000, 4, 6);
	TIM_B TIM7(0x40001400, 5, 7);



#endif
}



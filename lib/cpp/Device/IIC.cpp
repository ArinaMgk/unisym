// UTF-8 C/C++11 TAB4 CRLF
// Docutitle: (Protocol) Inter-Integrated Circuit, I2C
// Codifiers: @dosconio: 20240429~;
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

#include "../../../inc/cpp/Device/IIC"

namespace uni {
#if defined(_SUPPORT_GPIO)

	void IIC_SOFT::SendStart(void) {
		if (push_pull) SDA.setMode(GPIOMode::OUT_PushPull);
		SDA = true;
		SCL = true;
		asserv(func_delay)();
		SDA = false;
		asserv(func_delay)();
		SCL = false;
	}

	void IIC_SOFT::SendStop(void) {
		if (push_pull) SDA.setMode(GPIOMode::OUT_PushPull);
		SCL = false;// opt?
		SDA = false;
		asserv(func_delay)();
		SCL = true;
		SDA = true;
		asserv(func_delay)();
	}

	bool IIC_SOFT::WaitAcknowledge() {
		if (push_pull) SDA.setMode(GPIOMode::OUT_PushPull);
		byte timespan = 0;
		SDA = true;
		asserv(func_delay)();
		if (push_pull) SDA.setMode(GPIOMode::IN_Floating);
		SCL = true;
		asserv(func_delay)();
		while (SDA) {
			if (++timespan) {
				SendStop();
				return last_ack_accepted = false;
			}
			asserv(func_delay)();
		}
		SCL = false;
		return last_ack_accepted = true;
	}
	void IIC_SOFT::SendAcknowledge(bool ack) {
		if (push_pull) SDA.setMode(GPIOMode::OUT_PushPull);
		SCL = false;
		SDA = !ack;
		asserv(func_delay)();
		SCL = true;
		asserv(func_delay)();
		SCL = false;
	}

	void IIC_SOFT::Send(byte txt, bool auto_wait_ack) {
		if (push_pull) SDA.setMode(GPIOMode::OUT_PushPull);
		SCL = false;
		for0(i, _BYTE_BITS_) {
			SDA = txt & 0x80;
			txt <<= 1;
			asserv(func_delay)();// necessary delay
			SCL = true;
			asserv(func_delay)();
			SCL = false;
			asserv(func_delay)();
		}
		if (auto_wait_ack) WaitAcknowledge();
	}

	byte IIC_SOFT::ReadByte(bool feedback, bool ack) {
		if (push_pull) SDA.setMode(GPIOMode::IN_Floating);
		byte res = 0;
		for0(i, _BYTE_BITS_) {
			SCL = false;
			asserv(func_delay)();
			SCL = true;
			res <<= 1;
			if (SDA) res++;
			asserv(func_delay)();
		}
		SCL = false;// dosconio fix
		if (feedback) SendAcknowledge(ack);
		return res;
	}


#endif
}

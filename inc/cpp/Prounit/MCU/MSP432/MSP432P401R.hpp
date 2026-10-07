// ASCII C99 TAB4 CRLF
// LastCheck: RFG19
// AllAuthor: @dosconio
// ModuTitle: ARNCOVE KEIL MSP432P401R 8080 16-BIT
/*
	Copyright 2023 ArinaMgk & dosconio

	Licensed under the Apache License, Version 2.0 (the "License") while
	Dosconio-Framex is licensed under Mulan PSL v2.;
	you may not use this file except in compliance with the License.
	You may obtain copies of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html
	http://license.coscl.org.cn/MulanPSL2 

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#if defined MCU_MSP432P401R || defined __MSP432P401R__

// HRNSTYLE for MSP432P401R Official Equipment
#define po(x) GPIO_PORT_P##x
#define pi(y) GPIO_PIN##y
#define PinSet(p,xdp) (xdp? \
	GPIO_setOutputHighOnPin(p): \
	GPIO_setOutputLowOnPin (p))
#define PinGet(port,pin) (P##port##IN & BIT##pin)

#define outpi(port,pin,xdp) (xdp? \
	GPIO_setOutputHighOnPin(port, 1<<pin): \
	GPIO_setOutputLowOnPin (port, 1<<pin))

inline static outpw(uint16_t da)
{
	GPIO_setOutputHighOnPin(po(9), (da & 0xFF00) >> 8);// HIGH
	GPIO_setOutputHighOnPin(po(7), (da & 0x00FF));// LOW
	// SHOT IDEA YO RFG211534
	GPIO_setOutputLowOnPin(po(9), 0xFF & ~((da & 0xFF00) >> 8));
	GPIO_setOutputLowOnPin(po(7), 0xFF & ~(da & 0x00FF));
}


#endif

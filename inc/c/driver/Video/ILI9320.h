// ASCII CPP TAB4 CRLF
// Docutitle: (Driver) Video ILI9320
// Codifiers: @dosconio: 20240723
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
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

#ifndef _INC_Driver_ILI9320
#define _INC_Driver_ILI9320
#include "../../stdinc.h"
#ifdef _INC_CPP

#include "../../../cpp/Device/Video"
#include "../../../cpp/Device/GPIO"

namespace uni {
	// FreePins: The Pins may be from different Ports.
	class ILI9320_FreePins : public VideoControlInterface {
		uint16 DeviceType;
		byte lines;
		//
		GPIO_Pin* dat[16];
		GPIO_Pin WR;// write
		GPIO_Pin RD;// read
		GPIO_Pin RS;// 
		GPIO_Pin RST;// 
		GPIO_Pin CS;// 
		// SPI:
		GPIO_Pin CL;
		GPIO_Pin DA;
		//
		stduint xlim, ylim;
	protected:
		void setOutput(uint16 val) {
			if (lines == 4 || lines == 8 || lines == 16); else return;
			for0(i, lines) {
				dat[i]->operator=(val&1);
				val >>= 1;
			}
		}
		word getInnput(void) {
			word res = 0;
			// omit assert
			for0r(i, lines) {
				res |= bool(*dat[i]) ? 1 : 0;
				res <<= 1;
			}
			return res;
		}

		//aka ili9320_WriteIndex
		void SendRegisterIndex(word val) {
			RS = 0;
			RD = 1;
			Send(val);
			RS.Toggle();
		}

		void SetSheet(const Rectangle& rect) {
			SetCursor(rect.getVertex());
			setRegister(0x0050, rect.x);
			setRegister(0x0052, rect.x);
			setRegister(0x0051, rect.x + rect.width);
			setRegister(0x0053, rect.y + rect.height);
		}

		virtual void SetCursor(const Point& disp);
		virtual Point GetCursor();
		virtual void DrawPoint(const Point& disp, Color* color);
		virtual void DrawRectangle(const DisplayRectangle& rect);
		virtual void DrawFont(const Point& disp, const DisplayFont& font);
		virtual Color GetColor(Point p);
	public:
		void (*func_delay_us)(stduint us);
		ILI9320_FreePins(stduint lines, GPIO_Pin** dat, GPIO_Pin WR, GPIO_Pin RD, GPIO_Pin RS, GPIO_Pin RST, GPIO_Pin CS, GPIO_Pin CL, GPIO_Pin DA) :
			WR(WR), RD(RD), RS(RS), RST(RST), CS(CS), CL(CL), DA(DA) {
			func_delay_us = 0;
			self.lines = lines;
			if (lines > 4) { // not SPI
				for0(i, lines) self.dat[i] = dat[i];
			}
		}
		
		void setMode(byte lines);

		//aka ili9320_WriteData 向控制器指定地址写入数据，调用前需先写寄存器地址，内部函数
		void Send(word data) {
			switch (lines) {
			case 16:
				setOutput(data);
				WR = 0; WR.Toggle();
				break;
			case 8:
				setOutput(data >> 8);
				WR = 0; WR.Toggle();
				setOutput(data & 0xFF);
				WR = 0; WR.Toggle();
				break;
			case 4:
				//{TODO} Soft-SPI
				for0(i, bitsof(word)) {
					CL = 0;
					DA = bool(data & 0x8000);
					CL.Toggle();
					data <<= 1;
				}
				break;
			default:
				break;
			}
		}


		
		//aka ili9320_ReadData
		word Read(void) {
			/************************************************************************
			 ** nCS       ----\__________________________________________/-------  **
			 ** RS        ------\____________/-----------------------------------  **
			 ** nRD       -------------------------\_____/-----------------------  **
			 ** nWR       --------\_______/--------------------------------------  **
			 ** DB[0:15]  ---------[index]----------[data]-----------------------  **
			 **                                                                    **
			 ** nCS       ----\__________________________________________/-------  **
			 ** RS        ------\______________________/---------------------------**
			 ** nRD       ----------------------------------\_____/\_____/-------  **
			 ** nWR       --------\_______/\_______/-------------------------------**
			 ** DB[0:7]   ---------[0x00]---[index]---------[dataH][dataL]---------**
			 ************************************************************************/
			word res = 0;
			switch (lines) {
			case 16:
				for0(i, lines) dat[i]->setMode(GPIOMode::IN_Floating);
				RD = 0;
				res = getInnput();
				RD.Toggle();
				for0(i, lines) dat[i]->setMode(GPIOMode::OUT_PushPull);
				break;
			case 8:
				for0(i, lines) dat[i]->setMode(GPIOMode::IN_Floating);
				RD = 0;
				res = getInnput() << 8;
				RD.Toggle();
				RD.Toggle();
				res |= getInnput();
				RD.Toggle();
				for0(i, lines) dat[i]->setMode(GPIOMode::OUT_PushPull);
				break;
			case 4:
				//{TODO} Soft-SPI
				_TODO
				break;
			default:
				break;
			}
			return res;
		}

		void Reset() {
			// keep Tres 1ms
			RST = 1;
			asserv(func_delay_us)(500);
			RST.Toggle();
			asserv(func_delay_us)(500);
			RST.Toggle();
		}

		void setRegister(word idx, word val) {
			/************************************************************************
			 ** nCS       ----\__________________________________________/-------  **
			 ** RS        ------\____________/-----------------------------------  **
			 ** nRD       -------------------------------------------------------  **
			 ** nWR       --------\_______/--------\_____/-----------------------  **
			 ** DB[0:15]  ---------[index]----------[data]-----------------------  **
			 **                                                                    **
			 ** nCS       ----\__________________________________________/-------  **
			 ** RS        ------\______________________/---------------------------**
			 ** nRD       -------------------------------------------------------  **
			 ** nWR       --------\_______/\_______/--------\_____/\_____/---------**
			 ** DB[0:7]   ---------[0x00]---[index]---------[dataH][dataL]---------**
			 ************************************************************************/
			CS = 0;
			RS = 0;
			RD = 1;
			Send(idx);
			RS.Toggle();
			Send(val);
			CS.Toggle();
		}

		//aka ili9320_ReadRegister
		word getRegister(word idx) {
			word res;
			CS = 0;
			SendRegisterIndex(idx);
			res = Read();
			CS.Toggle();
			return res;
		}
		
		void setBacklight(bool lit_on) {
			_TODO
		}

		void Clear(Color color = Color::Black) {
			DrawRectangle(Rectangle(Point(0, 0), Size2(xlim, ylim), color, true));
		}

	};

	
}

#endif

#endif

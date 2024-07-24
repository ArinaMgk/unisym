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

#include "../../../../inc/c/driver/Video/ILI9320.h"

#ifdef _INC_CPP

namespace uni {

	struct ili_datat {
		word regidx, datas;
		ili_datat(word r, word d) : regidx(r), datas(d) {}
	};
	static ili_datat _ili_squ_1[] = {
			ili_datat(0x00e7, 0x0010),
			ili_datat(0x0000, 0x0001), //start internal osc
			ili_datat(0x0001, 0x0100),
			ili_datat(0x0002, 0x0700), //power on sequence                     
			ili_datat(0x0003, (1 << 12) | (1 << 5) | (1 << 4)), 	//65K 
			ili_datat(0x0004, 0x0000),
			ili_datat(0x0008, 0x0207),
			ili_datat(0x0009, 0x0000),
			ili_datat(0x000a, 0x0000), //display setting         
			ili_datat(0x000c, 0x0001), //display setting          
			ili_datat(0x000d, 0x0000), //0f3c          
			ili_datat(0x000f, 0x0000),
	};
	static ili_datat _ili_squ_9919H[] = {
			// POWER ON &RESET DISPLAY OFF
			ili_datat(0x28, 0x0006),
			ili_datat(0x00, 0x0001),
			ili_datat(0x10, 0x0000),
			ili_datat(0x01, 0x72ef),
			ili_datat(0x02, 0x0600),
			ili_datat(0x03, 0x6a38),
			ili_datat(0x11, 0x6874), //70
			//  RAM WRITE DATA MASK
			ili_datat(0x0f, 0x0000),
			//  RAM WRITE DATA MASK
			ili_datat(0x0b, 0x5308),
			ili_datat(0x0c, 0x0003),
			ili_datat(0x0d, 0x000a),
			ili_datat(0x0e, 0x2e00), //0030
			ili_datat(0x1e, 0x00be),
			ili_datat(0x25, 0x8000),
			ili_datat(0x26, 0x7800),
			ili_datat(0x27, 0x0078),
			ili_datat(0x4e, 0x0000),
			ili_datat(0x4f, 0x0000),
			ili_datat(0x12, 0x08d9),
			// Adjust the Gamma Curve
			ili_datat(0x30, 0x0000), //0007
			ili_datat(0x31, 0x0104), //0203
			ili_datat(0x32, 0x0100), //0001
			ili_datat(0x33, 0x0305), //0007
			ili_datat(0x34, 0x0505), //0007
			ili_datat(0x35, 0x0305), //0407
			ili_datat(0x36, 0x0707), //0407
			ili_datat(0x37, 0x0300), //0607
			ili_datat(0x3a, 0x1200), //0106
			ili_datat(0x3b, 0x0800),
			ili_datat(0x07, 0x0033),
	};

	
	Point ILI9320_FreePins::GetCursor() {
		_TODO return Point();
	}
	void ILI9320_FreePins::SetCursor(const Point& disp) {
		if (DeviceType == 0x8989) {
			setRegister(0x004e, disp.y);// line
			setRegister(0x004f, 0x13f - disp.x);// column
		}
		else if (DeviceType == 0x9919) {
			setRegister(0x004e, disp.x);// line
			setRegister(0x004f, disp.y);
		}
		else {
			setRegister(0x0020, disp.y);// line
			setRegister(0x0021, 0x13f - disp.x);
		}
	}
	void ILI9320_FreePins::DrawPoint(const Point& disp, Color* color) {
		if ((disp.x > xlim) || (disp.y > ylim)) return;
		SetCursor(disp);
		CS = 0;
		SendRegisterIndex(0x0022);
		RS = 1;
		Send(color->ToRGB565());//>???????????????? is it 565 desga?
		CS.Toggle();
	}
	void ILI9320_FreePins::DrawRectangle(const DisplayRectangle& rect) {
		//{UNCHK}
		if (rect.filled) {
			SetCursor(rect.getVertex());
			CS = 0;
			SendRegisterIndex(0x0022);
			RS = 1;
			for0(i, xlim * ylim) Send(rect.color.ToRGB565());// ????????? RGB565 ですが？
			CS.Toggle();
		}
		else _TODO;
	}
	void ILI9320_FreePins::DrawFont(const Point& disp, const DisplayFont& font) {
		_TODO
	}

	//aka ili9320_GetPoint
	Color ILI9320_FreePins::GetColor(Point p) {
		uint16 tmp;
		SetCursor(p);
		CS = 0;
		SendRegisterIndex(0x0022);
		RD = 0;
		tmp = Read();// dummy
		RD.Toggle();
		RD.Toggle();
		tmp = Read();
		RD.Toggle();
		CS.Toggle();
		return Color::FromBGR565(tmp);
	}


	void ILI9320_FreePins::setMode(byte lines) {
		self.lines = lines;

		_TEMP xlim = 320;
		_TEMP ylim = 240;

		_TEMP;
		WR.setMode(GPIOMode::OUT_PushPull);
		RD.setMode(GPIOMode::OUT_PushPull);
		RS.setMode(GPIOMode::OUT_PushPull);
		RST.setMode(GPIOMode::OUT_PushPull);
		CS.setMode(GPIOMode::OUT_PushPull);
		for0(i, lines) self.dat[i]->setMode(GPIOMode::OUT_PushPull);
		
		Send(0xFFFF);
		RST = 1;
		WR = 1;
		CS = 1;
		RS = 1;
		RD = 1;
		Reset(); asserv(func_delay_us)(500);
		setRegister(0x0000, 0x0001); asserv(func_delay_us)(5000);
		DeviceType = getRegister(0x0000);
		// _TEMP DeviceType = 0x9919;
		if (DeviceType == 0x9919 || DeviceType == 0x3232) {
			for0a(i, _ili_squ_9919H) 
				setRegister(_ili_squ_9919H[i].regidx, _ili_squ_9919H[i].datas);//{} this func can accept squ_struct
		}
		_TODO// more device

		func_delay_us(5000);
		Clear();
	}
	
}

#endif

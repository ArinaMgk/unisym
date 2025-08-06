// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant yo Host
// LastCheck: 2023 Nov 16
// AllAuthor: @ArinaMgk since RFR30; @dosconio
// ModuTitle: Console Input and Output
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

#include "../../inc/c/consio.h"
#if !defined(_MCCA)
#include <stdio.h>
#endif

#if defined(_WinNT) || defined(_Linux) ||  defined(_MCCA)
namespace uni {
	HostConsole Console;

	int HostConsole::out(const char* str, stduint len) {
		if (!count_mode) outtxt(str, len);
		out_count += len;
		return len;
	}
	int HostConsole::inn() {
	#if defined(_WinNT) || defined(_Linux)
		return _TEMP getchar();
	#else
		return _TEMP 0;
	#endif
	}

}
#endif
#if defined(_MCCA) && _MCCA==0x8632

namespace uni {
	//[sync(outtxt)]
	int BareConsole::out(const char* str, stduint len) {
		const stduint _BytesPerLine = unit * area_total.x;
		const stduint _Offset = unit * area_show.y * area_total.x;// Top Area
		volatile byte* _VideoBuf0 = (volatile byte*)vga_loc_addr;
		if (!count_mode)// outtxt(str, len);
		{
			word posi = last_curposi * unit;
			byte chr;
			// MIN(len, StrLength(str));
			for0(i, len) {
				chr = (byte)*str++;
				switch (chr)
				{
				case (byte)'\xFF':// 20240217-ALICE's FF Method
					attr = *str++;
					attr_enable = (attr != (byte)'\xFF');
					_LIMIT--;
					break;
				case '\r':
					posi -= posi % _BytesPerLine; //= posi / _BytesPerLine * _BytesPerLine;
					break;
				case '\n':// down
					posi += _BytesPerLine;
					break;
				case '\b':// left
					posi -= 2;
					break;
				case '\x01':// next
					posi += 2;
					break;
				case '\x02':// up
					posi -= _BytesPerLine;
					break;
				default:
					_VideoBuf0[posi++] = chr;//{}
					if (attr_enable)
						_VideoBuf0[posi++] = attr;//{}
					else
						posi++;
					break;
				}
				if (!chr) break;
				// posi includes the crtline (aka. begin-addr)
				//while (posi - unit * area_total.x * crtline >= _Offset + _BytesPerLine * area_show.height)
				while (posi >= _BytesPerLine * (topline + area_total.y))
				{
					Scroll(+1, posi);
				}
				if (auto_incbegaddr && posi / _BytesPerLine == topline + crtline + area_show.height) {
					setStartLine(++crtline);
				}
			}
			last_curposi = posi / 2;
			auto po = curget();
			if (Ranglin(po, topline * area_total.x, area_total.x * area_total.y))
				curset(last_curposi);
			_crt_out_cnt += len;
		}
		out_count += len;
		return len;
	}
	int BareConsole::inn() {

		return _TODO - 1;
	}
	void BareConsole::Scroll(stdsint lines, word& posi) {
		//{} The fact may be ContinPage + MemAbsolute
		const stduint _BytesPerLine = unit * area_total.x;
		volatile byte* _VideoBuf = (volatile byte*)vga_loc_addr + topline * _BytesPerLine;
		//__asm("cli");
		//{TODO} 
		if (!lines) return;
		// lines > 0 :
		//MIN(lines, _LinesPerScreen);
		word* sors = (word*)(_VideoBuf + _BytesPerLine * lines);// Add for Pointer!
		word* dest = (word*)_VideoBuf;
		forp(dest, area_total.x * (area_total.y - lines))* dest = *sors++;
		forp(dest, area_total.x * lines)
			* dest = 0x0720;//{TEMP} the new lines are of 'white on black' color
		posi -= _BytesPerLine * lines;//{TEMP}! assume lines always equals 1
		stat_lines += lines;
	}

	void BareConsole::setStartPosition(word begposi) {
		outpb(CRT_CR_AR, CRT_CDR_StartAddressHigh);
		outpb(CRT_CR_DR, begposi >> 8);
		outpb(CRT_CR_AR, CRT_CDR_StartAddressLow);
		outpb(CRT_CR_DR, begposi & 0xFF);
	}

}

extern uni::BareConsole* BCONS0;
void outtxt(const char* str, stduint len) {
	asserv (BCONS0)->out(str, len);
}

#endif
#if 0

size_t ConScanLine(char* innbuf, size_t limit) {
	char ch;
	while (i < limit && std::cin.get(ch) && ch != '\n')
		innbuf[i++] = ch;
	innbuf[i] = 0;
}

#endif

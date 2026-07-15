// ASCII CPP TAB4 CRLF
// Docutitle: (Module) Video
// Codifiers: @dosconio: 20240513
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

#include "../../../inc/cpp/Device/_Video.hpp"
#include "../../../inc/cpp/reference"
#include "../../../inc/c/data.h"

namespace {
	inline uint32 PackFramebufferColor(const uni::FramebufferInfo& fbi, uni::Color color) {
		switch (fbi.format) {
		case uni::PixelFormat::ARGB8888:
			return color.val;
		case uni::PixelFormat::ABGR8888:
			return (_IMM(color.r) << 0)
				| (_IMM(color.g) << 8)
				| (_IMM(color.b) << 16)
				| (_IMM(color.a) << 24);
		default:
			return color.val;
		}
	}

	inline uni::Color UnpackFramebufferColor(const uni::FramebufferInfo& fbi, uint32 val) {
		switch (fbi.format) {
		case uni::PixelFormat::ARGB8888:
			return uni::cast<uni::Color>(val);
		case uni::PixelFormat::ABGR8888: {
			uni::Color color;
			color.r = (val >> 0) & 0xFF;
			color.g = (val >> 8) & 0xFF;
			color.b = (val >> 16) & 0xFF;
			color.a = (val >> 24) & 0xFF;
			return color;
		}
		default:
			return uni::cast<uni::Color>(val);
		}
	}
}

namespace uni {
	uint32& ScreenBridge::Locate32(const Point& disp) const {
		return *(uint32*)((byte*)fbi->physical_range.address + disp.y * fbi->pitch + disp.x * 4);
	}

	void ScreenBridge::SetCursor(const Point&) const {}

	Point ScreenBridge::GetCursor() const {
		return Point(0, 0);
	}

	void ScreenBridge::DrawPoint(const Point& disp, Color color) const {
		if (!fbi) return;
		Locate32(disp) = PackFramebufferColor(*fbi, color);
	}

	void ScreenBridge::DrawRectangle(const Rectangle& rect) const {
		if (!fbi) return;
		const uint32 val = PackFramebufferColor(*fbi, rect.color);
		uint32* p = &Locate32(rect.getVertex());
		for0(y, rect.height) {
			for0(x, rect.width) p[x] = val;
			p = (uint32*)((byte*)p + fbi->pitch);
		}
	}

	void ScreenBridge::DrawFont(const Point&, const DisplayFont&, const String&) const {}

	Color ScreenBridge::GetColor(Point p) const {
		if (!fbi) return Color();
		return UnpackFramebufferColor(*fbi, Locate32(p));
	}

	void ScreenBridge::DrawPoints(const Rectangle& rect, const Color* base) const {
		if (!fbi || !base) return;
		uint32* p = &Locate32(rect.getVertex());
		const Color* pbase = base + rect.y * fbi->screen_size.x + rect.x;
		for0(y, rect.height) {
			for0(x, rect.width) p[x] = PackFramebufferColor(*fbi, pbase[x]);
			p = (uint32*)((byte*)p + fbi->pitch);
			pbase += fbi->screen_size.x;
		}
	}
}



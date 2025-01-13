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
#include "../../../../inc/cpp/Device/DDR"
#include "../../../../inc/cpp/Device/RCC/RCC"
#include "../../../../inc/cpp/Device/RCC/RCCAddress"
#if defined(_MPU_STM32MP13)

#define lt(x) getParent()[LTDCReg::x]
#define ly(x) self[LTDCLayerReg::x]

namespace uni {
	LTDC_t LTDC;

	static Point cursor[2];
	
	static LTDC_LAYER_t layers[2] = { LTDC_LAYER_t(1), LTDC_LAYER_t(2) };

	static stduint dbgs;
	//

	LTDC_LAYER_t::LayerPara* LTDC_LAYER_t::layer_param_refer(LTDC_LAYER_t::LayerPara* para) {
		para->roleaddr = (pureptr_t)DDR.getRoleress();
		para->window = Rectangle(Point(0, 0), Size2(800, 480));
		para->image_size = Size2(800, 480);
		para->factor1_mode = true;
		para->factor2_mode = true;
		para->backcolor = Color::Black;
		para->pixel_format = PixelFormat::RGB565;
		para->Alpha0 = 0;
		return para;
	}

	bool LTDC_LAYER_t::setMode_sub(LayerPara& param) const {
		uint32 tmp,
			stride, PSIZE = 0U,
			ALEN = 0U, APOS = 0U, RLEN = 0U, RPOS = 0U, BLEN = 0U, BPOS = 0U, GLEN = 0U, GPOS = 0U;
		stduint win_x0 = param.window.getVertex().x;
		stduint win_x1 = param.window.getVertexOpposite().x;
		stduint win_y0 = param.window.getVertex().y;
		stduint win_y1 = param.window.getVertexOpposite().y;
		{
			Tdsfield AHBP((pureptr_t)&lt(BPCR), 16, 12);
			Tdsfield AVBP((pureptr_t)&lt(BPCR), 0, 12);
			stduint vir_WHPCR = 0;
			Tdsfield WHSPPOS((pureptr_t)&vir_WHPCR, 16, 12);// part &ly(WHPCR)
			Tdsfield WHSTPOS((pureptr_t)&vir_WHPCR, 0, 12);// part &ly(WHPCR)
			stduint vir_WVPCR = 0;
			Tdsfield WVSPPOS((pureptr_t)&vir_WVPCR, 16, 12);// part &ly(WVPCR)
			Tdsfield WVSTPOS((pureptr_t)&vir_WVPCR, 0, 12);// part &ly(WVPCR)
			// Configure the horizontal start and stop position
			WHSPPOS = win_x1 + AHBP;// after once, &ly(WHPCR) will not be changed instantly
			WHSTPOS = win_x0 + AHBP + 1;
			ly(WHPCR) = vir_WHPCR;
			// Configure the vertical start and stop position
			WVSPPOS = win_y1 + AVBP;
			WVSTPOS = win_y0 + AVBP + 1;
			ly(WVPCR) = vir_WVPCR;
		}
		{
			// Configure the default color values
			ly(DCCR) = uint32(param.backcolor) & _TEMP 0xFFFFFF;
			// Specifies the constant alpha value
			Tdsfield CONSTA((pureptr_t)&ly(CACR), 0, 8);
			CONSTA = param.backcolor.a;// use backcolor as Alpha
		}
		// Specifies the pixel format
		{
			switch (param.pixel_format) {
			case PixelFormat::ARGB1555:
				PSIZE = 2U;
				ALEN = 1U; APOS = 15U; RLEN = 5U; RPOS = 10U; GLEN = 5U; GPOS = 5U; BLEN = 5U; BPOS = 0U;
				break;
			case PixelFormat::ARGB4444:
				PSIZE = 2U;
				ALEN = 4U; APOS = 12U; RLEN = 4U; RPOS = 8U; GLEN = 4U; GPOS = 4U; BLEN = 4U; BPOS = 0U;
				break;
			case PixelFormat::L8:
				PSIZE = 1U;
				ALEN = 0U; APOS = 0U; RLEN = 8U; RPOS = 0U; GLEN = 8U; GPOS = 0U; BLEN = 8U; BPOS = 0U;
				break;
			case PixelFormat::AL44:
				PSIZE = 1U;
				ALEN = 4U; APOS = 4U; RLEN = 4U; RPOS = 0U; GLEN = 4U; GPOS = 0U; BLEN = 4U; BPOS = 0U;
				break;
			case PixelFormat::AL88:
				PSIZE = 2U;
				ALEN = 8U; APOS = 8U; RLEN = 8U; RPOS = 0U; GLEN = 8U; GPOS = 0U; BLEN = 8U; BPOS = 0U;
				break;
			default:
				break;
			}
			switch (param.pixel_format) {
			case PixelFormat::ARGB8888:
			case PixelFormat::ABGR8888:
			case PixelFormat::RGBA8888:
			case PixelFormat::BGRA8888:
			case PixelFormat::RGB565:
			case PixelFormat::BGR565:
			case PixelFormat::RGB888:
				ly(PFCR) = _IMM(param.pixel_format);
				ly(FPF0R) = 0U;
				ly(FPF1R) = 0U;
				break;
			case PixelFormat::ARGB1555:
			case PixelFormat::ARGB4444:
			case PixelFormat::L8:
			case PixelFormat::AL44:
			case PixelFormat::AL88:
				ly(PFCR) = 0x7U;
				ly(FPF0R) = (RLEN << 14U) + (RPOS << 9U) + (ALEN << 5U) + APOS;
				ly(FPF1R) = (PSIZE << 18U) + (BLEN << 14U) + (BPOS << 9U) + (GLEN << 5U) + GPOS;
				break;
			default:
				break;
			}
			switch (param.pixel_format) {
			case PixelFormat::ARGB8888:
			case PixelFormat::ABGR8888:
			case PixelFormat::RGBA8888:
			case PixelFormat::BGRA8888:
				stride = 4U;
				break;
			case PixelFormat::RGB888:
				stride = 3U;
				break;
			case PixelFormat::RGB565:
			case PixelFormat::BGR565:
			case PixelFormat::ARGB1555:
			case PixelFormat::ARGB4444:
			case PixelFormat::AL88:
			case PixelFormat::UYVY:
			case PixelFormat::VYUY:
			case PixelFormat::YVYU:
			case PixelFormat::YUYV:
				stride = 2U;
				break;
			case PixelFormat::L8:
			case PixelFormat::AL44:
			default:
				stride = 1U;
				break;
			}
		}
		// Configure the frame buffer line number
		ly(CFBLNR) = param.image_size.y;
		// set the pitch
		ly(CFBLR) = (((0x10000U - param.image_size.x * stride)) << 16U) | (param.window.width * stride + 7U);
		// YUV configurations
		stduint&& mask_yuv_config = 0b111;// YIA | YSPA | YFPA
		if (ly(C1R) & mask_yuv_config) {
			// [YREN OF CBF YF  YCM  YCEN  HPDEN* VPDEN*  ?]
			//    9   8   7  6  5 4   3      2      1     0
			stduint&& mask_pcr = 0b101111111;
			ly(PCR) &= ~mask_pcr;
			switch (param.pixel_format) {
			case PixelFormat::UYVY:
				ly(PCR) = _IMM1S(3) | _IMM1S(7);// YCEN | CBF
				break;
			case PixelFormat::VYUY:
				ly(PCR) = _IMM1S(3); // YCEN;
				break;
			case PixelFormat::YUYV:
				ly(PCR) = _IMM1S(3) | _IMM1S(6) | _IMM1S(7);// YCEN | YF | CBF
				break;
			case PixelFormat::YVYU:
				ly(PCR) = _IMM1S(3) | _IMM1S(6);// YCEN | YF
				break;
			case PixelFormat::NV12:
				ly(PCR) = _IMM1S(3) | (0x1 << 4) | _IMM1S(7);
				break;
			case PixelFormat::NV21:
				ly(PCR) = _IMM1S(3) | (0x1 << 4);
				break;
			case PixelFormat::YUV420:
				ly(PCR) = _IMM1S(3) | (0x2 << 4) | _IMM1S(7);
				break;
			case PixelFormat::YVU420:
				ly(PCR) = _IMM1S(3) | (0x2 << 4);
				break;
			default:
				break;
			}
		}
		// Specifies the blending factors
		ly(BFCR) &= ~_IMM(0x10707);//(BOR | BF2 | BF1);
		_TEMP stduint BlendingOrder = 0;
		tmp = BlendingOrder << 16U;
		ly(BFCR) = (param.factor1_mode ? /*T:PAxCA*/(0b110 << 8) : /*F:CA*/(0b100 << 8)) | (param.factor2_mode ? /*T:PAxCA*/(0b111) : /*F:CA*/(0b101)) | tmp;
		// Configure the layer burst length configuration register
		_TEMP stduint BurstLength = 0;
		ly(BLCR) = BurstLength;
		if (ly(C1R) & mask_yuv_config) {
			// Configure the conversion YCbCr RGB
			// ly(CYR0R) &= ~(CB2B | CR2R);
			ly(CYR0R) = 0x02040199U;
			// ly(CYR1R) &= ~(CR2G | CB2G);
			ly(CYR1R) = 0x006400D0U;
		}
		_TEMP stduint HorMirrorEn = 0;
		_TEMP stduint VertMirrorEn = 0;
		//
		if (!HorMirrorEn && !VertMirrorEn) {
			ly(CFBAR) = _IMM(param.roleaddr);// Configure the color frame buffer start address
			switch (param.pixel_format) {
			case PixelFormat::NV12:
			case PixelFormat::NV21:
				// [AFBADD0] Configure the auxiliary frame buffer address 0
				ly(AFBA0R) = 0;//{} AuxiliaryFB.StartAddressBuffer0;
				// [AFBADD1] Configure the auxiliary frame buffer address 1
				ly(AFBA1R) = 0;//{} AuxiliaryFB.StartAddressBuffer1;
				// Configure the buffer length
				{
					stduint vir_AFBLR = ly(AFBLR);
					Tdsfield AFBP((pureptr_t)&vir_AFBLR, 16, 15);
					Tdsfield AFBLL((pureptr_t)&vir_AFBLR, 0, 14);
					AFBP = param.image_size.x;
					AFBLL = param.window.width + 7U;
					ly(AFBLR) = vir_AFBLR;
				}
				// [AFBLNBR] Configure the frame buffer line number
				ly(AFBLNR) = param.image_size.y >> 1U;
				break;
			case PixelFormat::YUV420:
			case PixelFormat::YVU420:
				// [AFBADD0] Configure the auxiliary frame buffer address 0
				ly(AFBA0R) = 0;//{} AuxiliaryFB.StartAddressBuffer0;
				// [AFBADD1] Configure the auxiliary frame buffer address 1
				ly(AFBA1R) = 0;//{} AuxiliaryFB.StartAddressBuffer1;
				// Configure the buffer length
				{
					stduint vir_AFBLR = ly(AFBLR);
					Tdsfield AFBP((pureptr_t)&vir_AFBLR, 16, 15);
					Tdsfield AFBLL((pureptr_t)&vir_AFBLR, 0, 14);
					AFBP = param.image_size.x >> 1U;
					AFBLL = (param.window.width >> 1U) + 7U;
					ly(AFBLR) = vir_AFBLR;
				}
				// [AFBLNBR] Configure the frame buffer line number
				ly(AFBLNR) = param.image_size.y >> 1U;
				break;
			default:
				break;
			}
			// Configure the color frame buffer pitch in byte
			{
				stduint vir_CFBLR = ly(CFBLR);
				Tdsfield CFBP((pureptr_t)&vir_CFBLR, 16, 15);
				Tdsfield CFBLL((pureptr_t)&vir_CFBLR, 0, 14);
				CFBP = param.image_size.x * stride;
				CFBLL = param.window.width * stride + 7U;
				ly(CFBLR) = vir_CFBLR;
			}
			// Enable LTDC_Layer by setting LEN bit
			ly(CR) = _IMM1S(0);// LEN
		}
		else if (HorMirrorEn && !VertMirrorEn) {
			ly(CFBAR) = _IMM(param.roleaddr) + stride * param.window.width - 1U;
			switch (param.pixel_format)
			{
			case PixelFormat::NV12:
			case PixelFormat::NV21:
				ly(AFBA0R) = 0 + //{} pLayerCfg->AuxiliaryFB.StartAddressBuffer0
					stride * param.window.width - 1U;
				ly(AFBA1R) = 0 + //{} pLayerCfg->AuxiliaryFB.StartAddressBuffer1
					stride * param.window.width - 1U;
				{
					stduint vir_AFBLR = ly(AFBLR);
					Tdsfield AFBP((pureptr_t)&vir_AFBLR, 16, 15);
					Tdsfield AFBLL((pureptr_t)&vir_AFBLR, 0, 14);
					AFBP = param.image_size.x;
					AFBLL = param.window.width + 7U;
					ly(AFBLR) = vir_AFBLR;
				}
				ly(AFBLNR) = param.image_size.y >> 1U;
				break;
			case PixelFormat::YUV420:
			case PixelFormat::YVU420:
				ly(AFBA0R) = 0 + //{} pLayerCfg->AuxiliaryFB.StartAddressBuffer0
					(stride * (param.window.width >> 1U)) - 1U;
				ly(AFBA1R) = 0 + //{} pLayerCfg->AuxiliaryFB.StartAddressBuffer1
					(stride * (param.window.width >> 1U)) - 1U;
				{
					stduint vir_AFBLR = ly(AFBLR);
					Tdsfield AFBP((pureptr_t)&vir_AFBLR, 16, 15);
					Tdsfield AFBLL((pureptr_t)&vir_AFBLR, 0, 14);
					AFBP = param.image_size.x >> 1U;
					AFBLL = (param.window.width >> 1U) + 7U;
					ly(AFBLR) = vir_AFBLR;
				}
				ly(AFBLNR) = param.image_size.y >> 1U;
				break;
			default:
				break;
			}
			{
				stduint vir_CFBLR = ly(CFBLR);
				Tdsfield CFBP((pureptr_t)&vir_CFBLR, 16, 15);
				Tdsfield CFBLL((pureptr_t)&vir_CFBLR, 0, 14);
				CFBP = param.image_size.x * stride;
				CFBLL = param.window.width * stride + 7U;
				ly(CFBLR) = vir_CFBLR;
			}
			// Enable horizontal mirroring bit & LTDC_Layer by setting LEN bit
			ly(CR) = _IMM1S(8) | _IMM1S(0);// HMEN | LEN;
		}
		else if (!HorMirrorEn && VertMirrorEn) {
			ly(CFBAR) = _IMM(param.roleaddr) + stride * param.window.width * (param.window.height - 1);
			switch (param.pixel_format)
			{
			case PixelFormat::NV12:
			case PixelFormat::NV21:
				ly(AFBA0R) = 0 + //{} AuxiliaryFB.StartAddressBuffer0
					stride * param.window.width * ((param.window.height >> 1U) - 1U);
				ly(AFBA1R) = 0 + //{} AuxiliaryFB.StartAddressBuffer1
					stride * param.window.width * ((param.window.height >> 1U) - 1U);
				{
					stduint vir_AFBLR = ly(AFBLR);
					Tdsfield AFBP((pureptr_t)&vir_AFBLR, 16, 15);
					Tdsfield AFBLL((pureptr_t)&vir_AFBLR, 0, 14);
					AFBP = 0x10000U - (param.image_size.x * stride);
					AFBLL = param.window.width * stride + 7U;
					ly(AFBLR) = vir_AFBLR;
				}
				ly(AFBLNR) = param.image_size.y >> 1U;
				break;
			case PixelFormat::YUV420:
			case PixelFormat::YVU420:
				ly(AFBA0R) = 0 +//{} AuxiliaryFB.StartAddressBuffer0
					stride * (param.window.width >> 1U) * ((param.window.height >> 1U) - 1U);
				ly(AFBA1R) = 0 +//{} AuxiliaryFB.StartAddressBuffer1
					stride * (param.window.width >> 1U) * ((param.window.height >> 1U) - 1U);
				{
					stduint vir_AFBLR = ly(AFBLR);
					Tdsfield AFBP((pureptr_t)&vir_AFBLR, 16, 15);
					Tdsfield AFBLL((pureptr_t)&vir_AFBLR, 0, 14);
					AFBP = (0x10000U - (param.image_size.x >> 1U)) * stride;
					AFBLL = (param.window.width >> 1U) * stride + 7U;
					ly(AFBLR) = vir_AFBLR;
				}
				ly(AFBLNR) = param.image_size.y >> 1U;
				break;
			default:
				break;
			}
			{
				stduint vir_CFBLR = ly(CFBLR);
				Tdsfield CFBP((pureptr_t)&vir_CFBLR, 16, 15);
				Tdsfield CFBLL((pureptr_t)&vir_CFBLR, 0, 14);
				CFBP = 0x10000U - param.image_size.x * stride;
				CFBLL = param.window.width * stride + 7U;
				ly(CFBLR) = vir_CFBLR;
			}
			ly(CR) = _IMM1S(0);// LEN
		}
		else if (HorMirrorEn && VertMirrorEn) {
			ly(CFBAR) = _IMM(param.roleaddr) + stride * param.window.width * param.window.height - 1;
			switch (param.pixel_format)
			{
			case PixelFormat::NV12:
			case PixelFormat::NV21:
				ly(AFBA0R) = 0 + //{} AuxiliaryFB.StartAddressBuffer0 +
					(stride * param.window.width * (param.window.height >> 1U)) - 1U;
				ly(AFBA1R) = 0 + //{} AuxiliaryFB.StartAddressBuffer1 +
					(stride * param.window.width * (param.window.height >> 1U)) - 1U;
				{
					stduint vir_AFBLR = ly(AFBLR);
					Tdsfield AFBP((pureptr_t)&vir_AFBLR, 16, 15);
					Tdsfield AFBLL((pureptr_t)&vir_AFBLR, 0, 14);
					AFBP = 0x10000U - param.image_size.x * stride;
					AFBLL = param.window.width * stride + 7U;
					ly(AFBLR) = vir_AFBLR;
				}
				ly(AFBLNR) = param.image_size.y >> 1U;
				break;
			case PixelFormat::YUV420:
			case PixelFormat::YVU420:
				ly(AFBA0R) = 0 + //{} AuxiliaryFB.StartAddressBuffer0 +
					(stride * (param.window.width >> 1U) * (param.window.height >> 1U)) - 1U;
				ly(AFBA1R) = 0 + //{} AuxiliaryFB.StartAddressBuffer1 +
					(stride * (param.window.width >> 1U) * (param.window.height >> 1U)) - 1U;
				{
					stduint vir_AFBLR = ly(AFBLR);
					Tdsfield AFBP((pureptr_t)&vir_AFBLR, 16, 15);
					Tdsfield AFBLL((pureptr_t)&vir_AFBLR, 0, 14);
					AFBP = (0x10000U - (param.image_size.x >> 1U)) * stride;
					AFBLL = ((param.window.width >> 1U) * stride) + 7U;
					ly(AFBLR) = vir_AFBLR;
				}
				ly(AFBLNR) = param.image_size.y >> 1U;
				break;
			default:
				break;
			}
			ly(CR) = _IMM1S(8) | _IMM1S(0);// HMEN | LEN;
		}
		return true;
	}

	bool LTDC_LAYER_t::assert_param(LayerPara param) const {
		asrtret(param.image_size.x <= 0xFFFF);// LTDC_LxCFBLR_CFBLL: color frame buffer line lengthThese bits define the length of one line of pixels in bytes + 7.The line length is computed as follows:active high width * number of bytes per pixel + 7.
		asrtret(param.image_size.y <= 0xFFFF);// LTDC_LxCFBLNR_CFBLNBR: frame buffer line numberThese bits define the number of lines in the frame buffer that corresponds to the active high width. 
		//
		asrtret(param.window.x <= 0xFFFF);// LTDC_LxWHPCR_WHSTPOS
		asrtret(param.window.y <= 0xFFFF);// LTDC_LxWHPCR_WHSTPOS
		asrtret(param.window.x + param.window.width <= 0xFFFF);// LTDC_LxWHPCR_WHSTPOS
		// LTDC_LxWHPCR_WHSTPOS: window horizontal start positionThese bits configure the first visible pixel of a line of the layer window.WHSTPOS[15:0] must be >= AAW[15:0] bits (programmed in LTDC_AWCR register).
		asrtret(param.window.y + param.window.height <= 0xFFFF);// LTDC_LxWHPCR_WHSPPOS
		// LTDC_LxWHPCR_WHSPPOS: window horizontal stop positionThese bits configure the last visible pixel of a line of the layer window.WHSPPOS[15:0] must be <= AHBP[15:0] bits + 1 (programmed in LTDC_BPCR register).
		return true;
	}

	LTDC_t& LTDC_LAYER_t::getParent() const { return LTDC; }

	bool LTDC_LAYER_t::setMode(LayerPara& param) const {
		asrtret(assert_param(param));
		asrtret(setMode_sub(param));
		getParent()[LTDCReg::SRCR] = _IMM1S(0);// IMR
		return true;
	}

	Rectangle LTDC_LAYER_t::getWindow() const {
		return _TEMP Rectangle(Point(0, 0), Size2(800, 480));
	}
	
	VideoControlBlock LTDC_LAYER_t::getControlBlock() const {
		return VideoControlBlock(nullptr, self);
	}

	void LTDC_LAYER_t::SetCursor(const Point& disp) const {
		cursor[getID() - 1] = disp;
	}
	Point LTDC_LAYER_t::GetCursor() const {
		return cursor[getID() - 1];
	}
	void LTDC_LAYER_t::DrawPoint(const Point& disp, Color color) const {
		auto win = getWindow().getSize();
		if (disp.x >= win.x || disp.y >= win.y) return;
		Letvar(p, uint16*, _TEMP 0xC0000000);
		p += disp.x + disp.y * win.x;
		if (_IMM(p) & 0b11) *p = color.ToRGB565();
		else *(uint32_t*)p = (*(uint32_t*)p & 0xFFFF0000) | _TEMP color.ToRGB565();
	}

	//{TODO} Make in MemSet for ARM
	// Draw filled rectangle
	// RGB565 only
	// use -O3 may better
	void LTDC_LAYER_t::DrawRectangle(const Rectangle& rect) const {
		// uint16 0,4,8 0b0000 0b0100 0b1000
		// uint32 0,8,16 0b0000 0b1000 0b10000
		// uint64 0,16,32 0b0000 0b10000 0b100000
		// Better: Draw more Points simultaneously
		// found: 20241203 write uint32 is more quickly
		auto win = getWindow().getSize();
		union {
			uint32* p32; uint16* p16; uint64* p64;
		};
		p16 = (uint16*)(_TEMP 0xC0000000 & ~_IMM(0b11));
		p16 += rect.x + rect.y * win.x;
		stduint hei = 0;
		if (rect.height) {
			uint32 line_color = rect.color.ToRGB565();
			uint32 times = rect.width;
			while (_IMM(p16) & 0b111) {
				*p16++ = line_color;
				times--;
			}
			line_color |= line_color << 16;
			uint64 line_color64 = line_color | ((uint64)line_color << 32);
			if ((times | _IMM(p64)) & 0b111) do { // (times & 0b111) || (_IMM(p64) & 0b111)
				for0(i, times >> 1)
					* p32++ = line_color;
				if (times & 1) *p16++ = line_color;
				p16 += win.x - rect.width;
				hei++;
			} while (hei < rect.height);
			else do {
				stduint _TIME = times >> 2;
				while (_TIME--) {
					*p64++ = line_color64;
				}
				p16 += win.x - rect.width;
				hei++;
			} while (hei < rect.height);
		}
	}
	Color LTDC_LAYER_t::GetColor(Point p) const { return Color(0); }


	///

	void LTDC_t::enClock(bool ena) {
		RCC[ena ? RCCReg::MP_NS_APB4ENSETR : RCCReg::MP_NS_APB4ENCLRR] = 0x00000001;
	}

	LTDC_LAYER_t& LTDC_t::operator[](unsigned layer) const {
		return layers[layer - 1];
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

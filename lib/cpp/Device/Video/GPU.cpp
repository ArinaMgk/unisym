// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Graphic Processing Unit (DMA2D, ...)
// Codifiers: @dosconio: 20241201 ~
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

#include "../../../../inc/cpp/Device/GPU"
#include "../../../../inc/cpp/Device/RCC/RCC"
#include "../../../../inc/cpp/Device/NVIC"
#include "../../../../inc/cpp/Device/SysTick"

#if defined(_MCU_STM32H7x)

namespace uni {

	DMA2D_t DMA2D;

	static DMA2D_LAYER_t layers[2] = { DMA2D_LAYER_t(0), DMA2D_LAYER_t(1) };

	// AKA DMA2D_INPUT_* — unisym PixelFormat → DMA2D FGPFCCR/BGPFCCR.CM[3:0]
	// Returns 0xFF when the format has no DMA2D input equivalent (L4/A8/A4/YCbCr not exposed).
	static byte _DMA2D_InputFormat(PixelFormat pf) {
		switch (pf) {
		case PixelFormat::ARGB8888: return 0;
		case PixelFormat::RGB888:   return 1;
		case PixelFormat::RGB565:   return 2;
		case PixelFormat::ARGB1555: return 3;
		case PixelFormat::ARGB4444: return 4;
		case PixelFormat::L8:       return 5;
		case PixelFormat::AL44:     return 6;
		case PixelFormat::AL88:     return 7;
		default: return 0xFF;
		}
	}

	// AKA DMA2D_OUTPUT_* — unisym PixelFormat → DMA2D OPFCCR.CM[2:0]
	static byte _DMA2D_OutputFormat(PixelFormat pf) {
		switch (pf) {
		case PixelFormat::ARGB8888: return 0;
		case PixelFormat::RGB888:   return 1;
		case PixelFormat::RGB565:   return 2;
		case PixelFormat::ARGB1555: return 3;
		case PixelFormat::ARGB4444: return 4;
		default: return 0xFF;
		}
	}

	DMA2D_LAYER_t& DMA2D_t::operator[](byte layer) const {
		return layers[layer & 1];
	}

	DMA2D_t& DMA2D_LAYER_t::getParent() const { return DMA2D; }

	bool DMA2D_t::enClock(bool ena) {
		RCC[RCCReg::AHB3ENR].setof(4, ena);// DMA2DEN
		return true;
	}

	void DMA2D_t::enAble(bool ena) {
		self[DMA2DReg::CR].setof(_DMA2D_CR_START, ena);
	}

	DMA2D_t::DMA2D_t()
		: state(_DMA2D_STATE_RESET), error_code(_DMA2D_ERROR_NONE),
		mode(DMA2DMode::M2M), output_format(PixelFormat::ARGB8888),
		output_offset(0), alpha_inverted(false), red_blue_swap(false) {}

	bool DMA2D_t::setMode(DMA2DMode _mode, PixelFormat _output_format, stduint _output_offset, bool _alpha_inverted, bool _red_blue_swap) {
		if (_IMM(_mode) > 3) return false;
		byte outc = _DMA2D_OutputFormat(_output_format);
		if (outc > 4) return false;
		if (_output_offset > 0x3FFF) return false;

		enClock();
		mode = _mode;
		output_format = _output_format;
		output_offset = _output_offset;
		alpha_inverted = _alpha_inverted;
		red_blue_swap = _red_blue_swap;

		// AKA HAL_DMA2D_Init
		self[DMA2DReg::CR].maset(_DMA2D_CR_MODE, 2, _IMM(_mode));
		self[DMA2DReg::OPFCCR].maset(_DMA2D_OPFCCR_CM, 3, outc);
		self[DMA2DReg::OOR].maset(0, 14, _output_offset);
		self[DMA2DReg::OPFCCR].setof(_DMA2D_OPFCCR_AI, _alpha_inverted);
		self[DMA2DReg::OPFCCR].setof(_DMA2D_OPFCCR_RBS, _red_blue_swap);

		error_code = _DMA2D_ERROR_NONE;
		state = _DMA2D_STATE_READY;
		return true;
	}

	bool DMA2D_t::canMode() {
		// AKA HAL_DMA2D_DeInit: abort any transfer or CLUT loading first
		if (self[DMA2DReg::CR].bitof(_DMA2D_CR_START)) {
			if (!Abort()) return false;
		} else if (self[DMA2DReg::BGPFCCR].bitof(_DMA2D_PFCCR_START)) {
			if (!AbortCLUT(0)) return false;
		} else if (self[DMA2DReg::FGPFCCR].bitof(_DMA2D_PFCCR_START)) {
			if (!AbortCLUT(1)) return false;
		}
		// Reset control registers
		self[DMA2DReg::CR] = 0;
		self[DMA2DReg::FGOR] = 0;
		self[DMA2DReg::BGOR] = 0;
		self[DMA2DReg::FGPFCCR] = 0;
		self[DMA2DReg::BGPFCCR] = 0;
		self[DMA2DReg::OPFCCR] = 0;
		error_code = _DMA2D_ERROR_NONE;
		state = _DMA2D_STATE_RESET;
		return true;
	}

	void DMA2D_t::setConfig(pureptr_t pdata, pureptr_t dst, stduint width, stduint height) {
		// AKA DMA2D_SetConfig
		self[DMA2DReg::NLR].maset(_DMA2D_NLR_NL, 16, height);
		self[DMA2DReg::NLR].maset(_DMA2D_NLR_PL, 14, width);
		self[DMA2DReg::OMAR] = _IMM(dst);
		if (mode == DMA2DMode::R2M) {
			uint32 tmp1 = _IMM(pdata) & 0xFF000000U;// alpha
			uint32 tmp2 = _IMM(pdata) & 0x00FF0000U;// red
			uint32 tmp3 = _IMM(pdata) & 0x0000FF00U;// green
			uint32 tmp4 = _IMM(pdata) & 0x000000FFU;// blue
			uint32 tmp = 0;
			switch (output_format) {
			case PixelFormat::ARGB8888: tmp = tmp1 | tmp2 | tmp3 | tmp4; break;
			case PixelFormat::RGB888:   tmp = tmp2 | tmp3 | tmp4; break;
			case PixelFormat::RGB565:
				tmp = ((tmp3 >> 10) << 5) | ((tmp2 >> 19) << 11) | (tmp4 >> 3); break;
			case PixelFormat::ARGB1555:
				tmp = ((tmp3 >> 11) << 5) | ((tmp2 >> 19) << 10) | ((tmp1 >> 31) << 15) | (tmp4 >> 3); break;
			case PixelFormat::ARGB4444:
				tmp = ((tmp3 >> 12) << 4) | ((tmp2 >> 20) << 8) | ((tmp1 >> 28) << 12) | (tmp4 >> 4); break;
			default: tmp = 0; break;
			}
			self[DMA2DReg::OCOLR] = tmp;
		} else {
			self[DMA2DReg::FGMAR] = _IMM(pdata);
		}
	}

	bool DMA2D_t::Transfer(pureptr_t pdata, pureptr_t dst, stduint width, stduint height, IOMethod method) {
		if (state != _DMA2D_STATE_READY) return false;
		if (height > 0xFFFF) return false;// AKA IS_DMA2D_LINE
		if (width > 0x3FFF) return false; // AKA IS_DMA2D_PIXEL

		if (method == IOMethod::Rupt) {
			// AKA HAL_DMA2D_Start_IT
			setConfig(pdata, dst, width, height);
			self[DMA2DReg::CR].setof(_DMA2D_CR_TCIE, true);
			self[DMA2DReg::CR].setof(_DMA2D_CR_TEIE, true);
			self[DMA2DReg::CR].setof(_DMA2D_CR_CEIE, true);
			error_code = _DMA2D_ERROR_NONE;
			state = _DMA2D_STATE_BUSY;
			enAble(true);
			return true;
		}
		if (method != IOMethod::Loop) return false;

		// AKA HAL_DMA2D_Start + HAL_DMA2D_PollForTransfer
		setConfig(pdata, dst, width, height);
		error_code = _DMA2D_ERROR_NONE;
		state = _DMA2D_STATE_BUSY;
		enAble(true);

		uint64 tickstart = SysTick::getTick();
		while (!self[DMA2DReg::ISR].bitof(_DMA2D_FLAG_TC)) {
			uint32 isr = uint32(self[DMA2DReg::ISR]);
			if (isr & (_IMM1S(_DMA2D_FLAG_CE) | _IMM1S(_DMA2D_FLAG_TE))) {
				if (isr & _IMM1S(_DMA2D_FLAG_CE)) error_code |= _DMA2D_ERROR_CE;
				if (isr & _IMM1S(_DMA2D_FLAG_TE)) error_code |= _DMA2D_ERROR_TE;
				self[DMA2DReg::IFCR].setof(_DMA2D_FLAG_CE);
				self[DMA2DReg::IFCR].setof(_DMA2D_FLAG_TE);
				state = _DMA2D_STATE_ERROR;
				return false;
			}
			if ((SysTick::getTick() - tickstart) > DMA2D_TIMEOUT_VALUE) {
				error_code |= _DMA2D_ERROR_TIMEOUT;
				state = _DMA2D_STATE_TIMEOUT;
				return false;
			}
		}
		self[DMA2DReg::IFCR].setof(_DMA2D_FLAG_TC);
		state = _DMA2D_STATE_READY;
		return true;
	}

	bool DMA2D_t::Blend(pureptr_t src1, pureptr_t src2, pureptr_t dst, stduint width, stduint height, IOMethod method) {
		if (state != _DMA2D_STATE_READY) return false;
		if (height > 0xFFFF || width > 0x3FFF) return false;

		// AKA HAL_DMA2D_BlendingStart(_IT): background source first
		self[DMA2DReg::BGMAR] = _IMM(src2);

		if (method == IOMethod::Rupt) {
			setConfig(src1, dst, width, height);
			self[DMA2DReg::CR].setof(_DMA2D_CR_TCIE, true);
			self[DMA2DReg::CR].setof(_DMA2D_CR_TEIE, true);
			self[DMA2DReg::CR].setof(_DMA2D_CR_CEIE, true);
			error_code = _DMA2D_ERROR_NONE;
			state = _DMA2D_STATE_BUSY;
			enAble(true);
			return true;
		}
		if (method != IOMethod::Loop) return false;

		setConfig(src1, dst, width, height);
		error_code = _DMA2D_ERROR_NONE;
		state = _DMA2D_STATE_BUSY;
		enAble(true);

		uint64 tickstart = SysTick::getTick();
		while (!self[DMA2DReg::ISR].bitof(_DMA2D_FLAG_TC)) {
			uint32 isr = uint32(self[DMA2DReg::ISR]);
			if (isr & (_IMM1S(_DMA2D_FLAG_CE) | _IMM1S(_DMA2D_FLAG_TE))) {
				if (isr & _IMM1S(_DMA2D_FLAG_CE)) error_code |= _DMA2D_ERROR_CE;
				if (isr & _IMM1S(_DMA2D_FLAG_TE)) error_code |= _DMA2D_ERROR_TE;
				self[DMA2DReg::IFCR].setof(_DMA2D_FLAG_CE);
				self[DMA2DReg::IFCR].setof(_DMA2D_FLAG_TE);
				state = _DMA2D_STATE_ERROR;
				return false;
			}
			if ((SysTick::getTick() - tickstart) > DMA2D_TIMEOUT_VALUE) {
				error_code |= _DMA2D_ERROR_TIMEOUT;
				state = _DMA2D_STATE_TIMEOUT;
				return false;
			}
		}
		self[DMA2DReg::IFCR].setof(_DMA2D_FLAG_TC);
		state = _DMA2D_STATE_READY;
		return true;
	}

	bool DMA2D_t::Abort() {
		// AKA HAL_DMA2D_Abort: MODIFY_REG(CR, ABORT|START, ABORT)
		self[DMA2DReg::CR].setof(_DMA2D_CR_START, false);
		self[DMA2DReg::CR].setof(_DMA2D_CR_ABORT, true);
		uint64 tickstart = SysTick::getTick();
		while (self[DMA2DReg::CR].bitof(_DMA2D_CR_START)) {
			if ((SysTick::getTick() - tickstart) > DMA2D_TIMEOUT_VALUE) {
				error_code |= _DMA2D_ERROR_TIMEOUT;
				state = _DMA2D_STATE_TIMEOUT;
				return false;
			}
		}
		self[DMA2DReg::CR].setof(_DMA2D_CR_TCIE, false);
		self[DMA2DReg::CR].setof(_DMA2D_CR_TEIE, false);
		self[DMA2DReg::CR].setof(_DMA2D_CR_CEIE, false);
		state = _DMA2D_STATE_READY;
		return true;
	}

	bool DMA2D_t::Suspend() {
		// AKA HAL_DMA2D_Suspend: MODIFY_REG(CR, SUSP|START, SUSP)
		self[DMA2DReg::CR].setof(_DMA2D_CR_START, false);
		self[DMA2DReg::CR].setof(_DMA2D_CR_SUSP, true);
		uint64 tickstart = SysTick::getTick();
		while (!self[DMA2DReg::CR].bitof(_DMA2D_CR_SUSP) && self[DMA2DReg::CR].bitof(_DMA2D_CR_START)) {
			if ((SysTick::getTick() - tickstart) > DMA2D_TIMEOUT_VALUE) {
				error_code |= _DMA2D_ERROR_TIMEOUT;
				state = _DMA2D_STATE_TIMEOUT;
				return false;
			}
		}
		if (self[DMA2DReg::CR].bitof(_DMA2D_CR_START)) {
			state = _DMA2D_STATE_SUSPEND;
		} else {
			self[DMA2DReg::CR].setof(_DMA2D_CR_SUSP, false);
		}
		return true;
	}

	bool DMA2D_t::Resume() {
		// AKA HAL_DMA2D_Resume
		if (self[DMA2DReg::CR].bitof(_DMA2D_CR_SUSP) && self[DMA2DReg::CR].bitof(_DMA2D_CR_START)) {
			state = _DMA2D_STATE_BUSY;
		}
		self[DMA2DReg::CR].setof(_DMA2D_CR_SUSP, false);
		self[DMA2DReg::CR].setof(_DMA2D_CR_START, false);
		return true;
	}

	bool DMA2D_t::AbortCLUT(byte layer) {
		// AKA HAL_DMA2D_CLUTLoading_Abort
		DMA2DReg pfccr = (layer == 1) ? DMA2DReg::FGPFCCR : DMA2DReg::BGPFCCR;
		self[DMA2DReg::CR].setof(_DMA2D_CR_ABORT, true);
		uint64 tickstart = SysTick::getTick();
		while (self[pfccr].bitof(_DMA2D_PFCCR_START)) {
			if ((SysTick::getTick() - tickstart) > DMA2D_TIMEOUT_VALUE) {
				error_code |= _DMA2D_ERROR_TIMEOUT;
				state = _DMA2D_STATE_TIMEOUT;
				return false;
			}
		}
		self[DMA2DReg::CR].setof(_DMA2D_CR_CTCIE, false);
		self[DMA2DReg::CR].setof(_DMA2D_CR_TEIE, false);
		self[DMA2DReg::CR].setof(_DMA2D_CR_CEIE, false);
		self[DMA2DReg::CR].setof(_DMA2D_CR_CAEIE, false);
		state = _DMA2D_STATE_READY;
		return true;
	}

	bool DMA2D_t::SuspendCLUT(byte layer) {
		// AKA HAL_DMA2D_CLUTLoading_Suspend
		DMA2DReg pfccr = (layer == 1) ? DMA2DReg::FGPFCCR : DMA2DReg::BGPFCCR;
		self[DMA2DReg::CR].setof(_DMA2D_CR_SUSP, true);
		uint64 tickstart = SysTick::getTick();
		while (!self[DMA2DReg::CR].bitof(_DMA2D_CR_SUSP) && self[pfccr].bitof(_DMA2D_PFCCR_START)) {
			if ((SysTick::getTick() - tickstart) > DMA2D_TIMEOUT_VALUE) {
				error_code |= _DMA2D_ERROR_TIMEOUT;
				state = _DMA2D_STATE_TIMEOUT;
				return false;
			}
		}
		if (self[pfccr].bitof(_DMA2D_PFCCR_START)) {
			state = _DMA2D_STATE_SUSPEND;
		} else {
			self[DMA2DReg::CR].setof(_DMA2D_CR_SUSP, false);
		}
		return true;
	}

	bool DMA2D_t::ResumeCLUT(byte layer) {
		// AKA HAL_DMA2D_CLUTLoading_Resume
		DMA2DReg pfccr = (layer == 1) ? DMA2DReg::FGPFCCR : DMA2DReg::BGPFCCR;
		if (self[DMA2DReg::CR].bitof(_DMA2D_CR_SUSP) && self[pfccr].bitof(_DMA2D_PFCCR_START)) {
			state = _DMA2D_STATE_BUSY;
		}
		self[DMA2DReg::CR].setof(_DMA2D_CR_SUSP, false);
		return true;
	}

	void DMA2D_t::setLineEvent(stduint line) {
		// AKA HAL_DMA2D_ProgramLineEvent (enables transfer watermark interrupt)
		self[DMA2DReg::LWR].maset(0, 16, line);
		self[DMA2DReg::CR].setof(_DMA2D_CR_TWIE, true);
	}

	void DMA2D_t::enDeadTime(bool ena, byte dt) {
		// AKA HAL_DMA2D_EnableDeadTime / DisableDeadTime / ConfigDeadTime
		if (ena) {
			self[DMA2DReg::AMTCR].maset(_DMA2D_AMTCR_DT, 8, dt);
			self[DMA2DReg::AMTCR].setof(_DMA2D_AMTCR_EN, true);
		} else {
			self[DMA2DReg::AMTCR].setof(_DMA2D_AMTCR_EN, false);
		}
	}

	void DMA2D_t::HandleIRQ() {
		// AKA HAL_DMA2D_IRQHandler
		uint32 isrflags = uint32(self[DMA2DReg::ISR]);
		uint32 crflags = uint32(self[DMA2DReg::CR]);

		if (isrflags & _IMM1S(_DMA2D_FLAG_TE)) {
			if (crflags & _IMM1S(_DMA2D_CR_TEIE)) {
				self[DMA2DReg::CR].setof(_DMA2D_CR_TEIE, false);
				error_code |= _DMA2D_ERROR_TE;
				self[DMA2DReg::IFCR].setof(_DMA2D_FLAG_TE);
				state = _DMA2D_STATE_ERROR;
				if (XferErrorCallback) XferErrorCallback();
			}
		}
		if (isrflags & _IMM1S(_DMA2D_FLAG_CE)) {
			if (crflags & _IMM1S(_DMA2D_CR_CEIE)) {
				self[DMA2DReg::CR].setof(_DMA2D_CR_CEIE, false);
				self[DMA2DReg::IFCR].setof(_DMA2D_FLAG_CE);
				error_code |= _DMA2D_ERROR_CE;
				state = _DMA2D_STATE_ERROR;
				if (XferErrorCallback) XferErrorCallback();
			}
		}
		if (isrflags & _IMM1S(_DMA2D_FLAG_CAE)) {
			if (crflags & _IMM1S(_DMA2D_CR_CAEIE)) {
				self[DMA2DReg::CR].setof(_DMA2D_CR_CAEIE, false);
				self[DMA2DReg::IFCR].setof(_DMA2D_FLAG_CAE);
				error_code |= _DMA2D_ERROR_CAE;
				state = _DMA2D_STATE_ERROR;
				if (XferErrorCallback) XferErrorCallback();
			}
		}
		if (isrflags & _IMM1S(_DMA2D_FLAG_TW)) {
			if (crflags & _IMM1S(_DMA2D_CR_TWIE)) {
				self[DMA2DReg::CR].setof(_DMA2D_CR_TWIE, false);
				self[DMA2DReg::IFCR].setof(_DMA2D_FLAG_TW);
				if (LineEventCallback) LineEventCallback();
			}
		}
		if (isrflags & _IMM1S(_DMA2D_FLAG_TC)) {
			if (crflags & _IMM1S(_DMA2D_CR_TCIE)) {
				self[DMA2DReg::CR].setof(_DMA2D_CR_TCIE, false);
				self[DMA2DReg::IFCR].setof(_DMA2D_FLAG_TC);
				state = _DMA2D_STATE_READY;
				if (XferCpltCallback) XferCpltCallback();
			}
		}
		if (isrflags & _IMM1S(_DMA2D_FLAG_CTC)) {
			if (crflags & _IMM1S(_DMA2D_CR_CTCIE)) {
				self[DMA2DReg::CR].setof(_DMA2D_CR_CTCIE, false);
				self[DMA2DReg::IFCR].setof(_DMA2D_FLAG_CTC);
				state = _DMA2D_STATE_READY;
				if (CLUTLoadingCpltCallback) CLUTLoadingCpltCallback();
			}
		}
	}

	// ---- RuptTrait (AKA HAL_DMA2D_MspInit NVIC wiring) ----
	void DMA2D_t::setInterrupt(Handler_t f) const {
		XferCpltCallback = f;
	}
	void DMA2D_t::setInterruptPriority(byte preempt, byte sub_priority) const {
		NVIC.setPriority(IRQ_DMA2D, preempt, sub_priority);
	}
	void DMA2D_t::enInterrupt(bool enable) const {
		NVIC.setAble(IRQ_DMA2D, enable);
	}

	bool DMA2D_LAYER_t::setMode(LayerPara& param) {
		DMA2D_t& dev = getParent();
		byte incm = _DMA2D_InputFormat(param.pixel_format);
		if (incm > 7) return false;// L4/A8/A4/YCbCr not exposed via PixelFormat
		if (param.input_offset > 0x3FFF) return false;

		bool fg = (id == 1);
		DMA2DReg pfccr = fg ? DMA2DReg::FGPFCCR : DMA2DReg::BGPFCCR;
		DMA2DReg oreg = fg ? DMA2DReg::FGOR : DMA2DReg::BGOR;

		// AKA HAL_DMA2D_ConfigLayer
		dev[pfccr].maset(_DMA2D_PFCCR_CM, 4, incm);
		dev[pfccr].maset(_DMA2D_PFCCR_AM, 2, _IMM(param.alpha_mode));
		dev[pfccr].setof(_DMA2D_PFCCR_AI, param.alpha_inverted);
		dev[pfccr].setof(_DMA2D_PFCCR_RBS, param.red_blue_swap);
		dev[pfccr].maset(_DMA2D_PFCCR_ALPHA, 8, param.input_alpha & 0xFF);
		dev[oreg] = param.input_offset;

		layer_param = param;
		return true;
	}

	bool DMA2D_LAYER_t::setCLUT(const uint32* table, stduint size, IOMethod method, bool ena) {
		DMA2D_t& dev = getParent();
		if (size > 0xFF) return false;// AKA IS_DMA2D_CLUT_SIZE
		if (ena && dev.state != _DMA2D_STATE_READY) return false;

		bool fg = (id == 1);
		DMA2DReg cmar = fg ? DMA2DReg::FGCMAR : DMA2DReg::BGCMAR;
		DMA2DReg pfccr = fg ? DMA2DReg::FGPFCCR : DMA2DReg::BGPFCCR;

		// AKA HAL_DMA2D_ConfigCLUT: CLUT memory address + size + color mode
		dev[cmar] = _IMM(table);
		dev[pfccr].maset(_DMA2D_PFCCR_CS, 8, size);
		dev[pfccr].setof(_DMA2D_PFCCR_CCM, false);// CLUT color mode = ARGB8888

		if (!ena) return true;// config only (AKA ConfigCLUT without EnableCLUT)

		// AKA HAL_DMA2D_CLUTLoad / CLUTLoad_IT
		if (method == IOMethod::Rupt) {
			dev[DMA2DReg::CR].setof(_DMA2D_CR_CTCIE, true);
			dev[DMA2DReg::CR].setof(_DMA2D_CR_TEIE, true);
			dev[DMA2DReg::CR].setof(_DMA2D_CR_CEIE, true);
			dev[DMA2DReg::CR].setof(_DMA2D_CR_CAEIE, true);
		}
		dev[pfccr].setof(_DMA2D_PFCCR_START, true);

		if (method == IOMethod::Loop) {
			// AKA PollForTransfer (CLUT branch)
			uint64 tickstart = SysTick::getTick();
			while (!dev[DMA2DReg::ISR].bitof(_DMA2D_FLAG_CTC)) {
				uint32 isr = uint32(dev[DMA2DReg::ISR]);
				if (isr & (_IMM1S(_DMA2D_FLAG_CAE) | _IMM1S(_DMA2D_FLAG_CE) | _IMM1S(_DMA2D_FLAG_TE))) {
					if (isr & _IMM1S(_DMA2D_FLAG_CAE)) dev.error_code |= _DMA2D_ERROR_CAE;
					if (isr & _IMM1S(_DMA2D_FLAG_CE)) dev.error_code |= _DMA2D_ERROR_CE;
					if (isr & _IMM1S(_DMA2D_FLAG_TE)) dev.error_code |= _DMA2D_ERROR_TE;
					dev[DMA2DReg::IFCR].setof(_DMA2D_FLAG_CAE);
					dev[DMA2DReg::IFCR].setof(_DMA2D_FLAG_CE);
					dev[DMA2DReg::IFCR].setof(_DMA2D_FLAG_TE);
					dev.state = _DMA2D_STATE_ERROR;
					return false;
				}
				if ((SysTick::getTick() - tickstart) > DMA2D_TIMEOUT_VALUE) {
					dev.error_code |= _DMA2D_ERROR_TIMEOUT;
					dev.state = _DMA2D_STATE_TIMEOUT;
					return false;
				}
			}
			dev[DMA2DReg::IFCR].setof(_DMA2D_FLAG_CTC);
			dev[DMA2DReg::IFCR].setof(_DMA2D_FLAG_TC);
			dev.state = _DMA2D_STATE_READY;
		} else if (method == IOMethod::Rupt) {
			dev.state = _DMA2D_STATE_BUSY;
		}
		return true;
	}

}

#endif // _MCU_STM32H7x

// UTF-8 CPP-ISO11 TAB4 LF
// Docutitle: [Device] Digital Camera Interface
// Codifiers: @ArinaMgk
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

#include "../../../../inc/cpp/Device/Graphic/DCMI.hpp"

#if defined(_MCU_STM32H7x)
#include "../../../../inc/cpp/Device/RCC/RCC"
#include "../../../../inc/cpp/Device/NVIC"
#include "../../../../inc/cpp/Device/SysTick"
#include "../../../../inc/cpp/Device/Cache"
#include "../../../../inc/cpp/System/Picture.hpp"
#include "../../../../inc/c/prochip/CortexM7.h"
#ifndef SCB_ICSR_RETTOBASE_Pos
#include "../../../../inc/c/prochip/CortexM7/CortexM7-SCB.h"
#endif

namespace uni {
	DCMI_t DCMI;
	DCMI_t* volatile DCMI_t::active_device = nullptr;
	namespace {
		constexpr stduint staging_words = 4096;
		alignas(32) uint32 staging_buffers[2][staging_words];
	}

	void DCMI_t::enClock(bool ena) {
		RCC[RCCReg::AHB2ENR].setof(_DCMI_RCC_POS_ENABLE, ena);
		(void)RCC[RCCReg::AHB2ENR].bitof(_DCMI_RCC_POS_ENABLE);
	}

	void DCMI_t::enAble(bool ena) {
		self[DCMIReg::CR].setof(_DCMI_CR_POS_ENABLE, ena);
	}

	bool DCMI_t::setMode(const DCMIConfig_t& config) {
		if (byte(config.sync_mode) > 1 || byte(config.capture_rate) > 2 ||
			byte(config.data_width) > 3 || byte(config.byte_select) > 3 ||
			byte(config.byte_select_start) > 1 || byte(config.line_select) > 1 ||
			byte(config.line_select_start) > 1) return false;
		if (lock.exchange(true, MemoryOrder_Acquire)) return false;
		if ((active_device && active_device != this) || state == DCMIState::Busy || state == DCMIState::Suspended ||
			(active_dma && !StopTransfer())) {
			lock.store(false, MemoryOrder_Release);
			return false;
		}
		enClock();
		state = DCMIState::Busy;
		Reference cr = self[DCMIReg::CR];
		cr.setof(_DCMI_CR_POS_ESS, config.sync_mode == DCMISyncMode::Embedded);
		cr.setof(_DCMI_CR_POS_PCKPOL, config.pixel_clock_rising);
		cr.setof(_DCMI_CR_POS_HSPOL, config.hsync_high);
		cr.setof(_DCMI_CR_POS_VSPOL, config.vsync_high);
		cr.maset(_DCMI_CR_POS_FCRC, 2, byte(config.capture_rate));
		cr.maset(_DCMI_CR_POS_EDM, 2, byte(config.data_width));
		cr.setof(_DCMI_CR_POS_JPEG, config.jpeg_mode);
		cr.maset(_DCMI_CR_POS_BSM, 2, byte(config.byte_select));
		cr.setof(_DCMI_CR_POS_OEBS, config.byte_select_start == DCMISelectStart::Second);
		cr.setof(_DCMI_CR_POS_LSM, config.line_select == DCMILineSelect::Alternate2);
		cr.setof(_DCMI_CR_POS_OELS, config.line_select_start == DCMISelectStart::Second);
		if (config.sync_mode == DCMISyncMode::Embedded) {
			uint32 codes = 0;
			Reference fields((pureptr_t)&codes);
			fields.maset(_DCMI_ESCR_POS_FSC, 8, config.sync_codes.frame_start);
			fields.maset(_DCMI_ESCR_POS_LSC, 8, config.sync_codes.line_start);
			fields.maset(_DCMI_ESCR_POS_LEC, 8, config.sync_codes.line_end);
			fields.maset(_DCMI_ESCR_POS_FEC, 8, config.sync_codes.frame_end);
			self[DCMIReg::ESCR] = codes;
		}
		self[DCMIReg::IER] = _DCMI_IT_CAPTURE;
		error_code = _DCMI_ERROR_NONE;
		state = DCMIState::Ready;
		lock.store(false, MemoryOrder_Release);
		return true;
	}

	bool DCMI_t::canMode() {
		if (lock.exchange(true, MemoryOrder_Acquire)) return false;
		if ((active_device && active_device != this) || (state != DCMIState::Reset && !StopTransfer())) {
			lock.store(false, MemoryOrder_Release);
			return false;
		}
		if (state != DCMIState::Reset) {
			enInterrupt(false);
			RCC[RCCReg::AHB2RSTR].setof(_DCMI_RCC_POS_RESET, true);
			RCC[RCCReg::AHB2RSTR].setof(_DCMI_RCC_POS_RESET, false);
			enClock(false);
		}
		transfer_count = transfer_size = transfer_number = buffer_address = 0;
		frame_blocks = blocks_left = 0;
		staged_capture = false;
		staged_written = 0;
		capture_capacity = 0;
		variable_capture = finishing_frame = capacity_reached = false;
		received_size.store(0, MemoryOrder_Release);
		frame_ready.store(false, MemoryOrder_Release);
		error_code = _DCMI_ERROR_NONE;
		state = DCMIState::Reset;
		lock.store(false, MemoryOrder_Release);
		return true;
	}

	bool DCMI_t::BindDMA(const DMAStream& stream) {
		if (active_device && active_device != this) return false;
		DMA_t& dma = stream.getParent();
		// DMA callbacks are controller-wide, so another active stream cannot share this binding.
		for (byte i = 0; i < 8; ++i) {
			if (dma.streamStates[i] == _DMA_STATE_BUSY || dma.streamStates[i] == _DMA_STATE_ABORT ||
				dma[DMAReg::CR[i]].bitof(_DMA_SxCR_POS_EN)) return false;
		}
		if (stream.getState() != _DMA_STATE_READY) return false;
		saved_callbacks[0] = dma.XferCpltCallback;
		saved_callbacks[1] = dma.XferHalfCallback;
		saved_callbacks[2] = dma.XferErrorCallback;
		saved_callbacks[3] = dma.XferAbortCallback;
		saved_callbacks[4] = dma.XferM1CpltCallback;
		saved_callbacks[5] = dma.XferM1HalfCpltCallback;
		saved_bind = dma.bind;
		saved_circular = dma[DMAReg::CR[stream.getID()]].bitof(_DMA_SxCR_POS_CIRC);
		active_dma = &stream;
		active_device = this;
		dma.bind = this;
		dma.XferCpltCallback = dma.XferM1CpltCallback = DMATransferComplete;
		dma.XferHalfCallback = dma.XferM1HalfCpltCallback = nullptr;
		dma.XferErrorCallback = DMAError;
		dma.XferAbortCallback = nullptr;
		return true;
	}

	void DCMI_t::ReleaseDMA() {
		if (!active_dma) return;
		DMA_t& dma = active_dma->getParent();
		if (dma.bind == this && dma.XferCpltCallback == DMATransferComplete) {
			dma[DMAReg::CR[active_dma->getID()]].setof(_DMA_SxCR_POS_CIRC, saved_circular);
			dma.XferCpltCallback = saved_callbacks[0];
			dma.XferHalfCallback = saved_callbacks[1];
			dma.XferErrorCallback = saved_callbacks[2];
			dma.XferAbortCallback = saved_callbacks[3];
			dma.XferM1CpltCallback = saved_callbacks[4];
			dma.XferM1HalfCpltCallback = saved_callbacks[5];
			dma.bind = saved_bind;
		}
		active_dma = nullptr;
		deferred_release = false;
		if (active_device == this) active_device = nullptr;
	}

	bool DCMI_t::Start(DCMICaptureMode mode, pureptr_t dest, stduint word_count) {
		return Start(mode, dest, word_count, false);
	}

	bool DCMI_t::Start(DCMICaptureMode mode, ImageBuffer& image) {
		if (!image.pixels || !image.width || !image.height) return false;
		stduint pixel_size;
		switch (image.format) {
		case PixelFormat::L8:
		case PixelFormat::GREY8: pixel_size = 1; break;
		case PixelFormat::RGB565:
		case PixelFormat::BGR565:
		case PixelFormat::ARGB1555:
		case PixelFormat::ARGB4444: pixel_size = 2; break;
		case PixelFormat::UYVY:
		case PixelFormat::VYUY:
		case PixelFormat::YUYV:
		case PixelFormat::YVYU:
			if (image.width & 1U) return false;
			pixel_size = 2;
			break;
		case PixelFormat::RGB888: pixel_size = 3; break;
		case PixelFormat::ARGB8888:
		case PixelFormat::ABGR8888:
		case PixelFormat::RGBA8888:
		case PixelFormat::BGRA8888: pixel_size = 4; break;
		default: return false;
		}
		uint64 row_size = (uint64)image.width * pixel_size;
		if (row_size != image.stride || row_size > 0xFFFFFFFCULL ||
			image.height > 0xFFFFFFFCULL / row_size) return false;
		stduint word_count = (stduint)((row_size * image.height + 3) / 4);
		if ((uint64)word_count * 4 > image.size) return false;
		// AN5020, Data formats and storage: this layout uses packed 8-bit samples.
		if (self[DCMIReg::CR].bitof(_DCMI_CR_POS_JPEG) ||
			self[DCMIReg::CR].masof(_DCMI_CR_POS_EDM, 2) != 0) return false;
		return Start(mode, image.pixels, word_count, false, row_size % 32 ? 0 : (stduint)(row_size / 4));
	}

	bool DCMI_t::StartVariable(pureptr_t dest, stduint capacity_bytes) {
		if (!capacity_bytes || (capacity_bytes & 3U)) return false;
		return Start(DCMICaptureMode::Snapshot, dest, capacity_bytes / 4, true);
	}

	bool DCMI_t::Start(DCMICaptureMode mode, pureptr_t dest, stduint word_count, bool variable, stduint row_words) {
		if (byte(mode) > 1 || !dest || (_IMM(dest) & 3U) || !word_count || word_count > 0x3FFFFFFFU ||
			(uint64)_IMM(dest) + (uint64)word_count * 4 > 0x100000000ULL) return false;
		stduint encoded = _IMM(hdmarx);
		if ((encoded >> 8) < 1 || (encoded >> 8) > 2 || (encoded & 0x8FU)) return false;
		stduint size = word_count, segments = 1;
		while (size > 0xFFFFU) {
			if (size & 1U) return false;
			size /= 2;
			segments *= 2;
		}
		// SDRAM snapshots use SRAM staging; other destinations and continuous capture retain direct DMA.
		bool staged = mode == DCMICaptureMode::Snapshot && _IMM(dest) >= 0xC0000000U &&
			(uint64)_IMM(dest) + (uint64)word_count * 4 <= 0xE0000000ULL;
		stduint staged_size = size, staged_segments = segments;
		stduint block_limit = variable ? 2048 : staging_words;
		while (staged_size > block_limit && !(staged_size & 1U)) {
			staged_size /= 2;
			staged_segments *= 2;
		}
		// Keep RGB copies to one row and JPEG blocks to 2048 words, as in the camera example.
		if (row_words && row_words <= staging_words && !(row_words & 7U) && !(word_count % row_words)) {
			staged_size = row_words;
			staged_segments = word_count / row_words;
		}
		staged = staged && staged_size <= block_limit && !(staged_size & 7U);
		if (staged) { size = staged_size; segments = staged_segments; }
		if (lock.exchange(true, MemoryOrder_Acquire)) return false;
		if (state != DCMIState::Ready || (active_dma && !StopTransfer())) {
			lock.store(false, MemoryOrder_Release);
			return false;
		}
		DMA_t& dma = hdmarx->getParent();
		Reference cr = dma[DMAReg::CR[hdmarx->getID()]];
		if (cr.masof(_DMA_SxCR_POS_DIR, 2) != 0 || cr.masof(_DMA_SxCR_POS_PSIZE, 2) != 2 ||
			cr.masof(_DMA_SxCR_POS_MSIZE, 2) != 2 || cr.bitof(_DMA_SxCR_POS_PINC) ||
			!cr.bitof(_DMA_SxCR_POS_MINC) || cr.bitof(_DMA_SxCR_POS_PFCTRL) || !BindDMA(*hdmarx)) {
			error_code |= _DCMI_ERROR_DMA;
			lock.store(false, MemoryOrder_Release);
			return false;
		}
		state = DCMIState::Busy;
		error_code = _DCMI_ERROR_NONE;
		staged_capture = staged;
		staged_written = 0;
		if (staged_capture) {
			// Linker placement must provide DMA-accessible SRAM, never DTCM or external RAM.
			stduint region = _IMM(staging_buffers) >> 24;
			if (region != 0x24 && region != 0x30 && region != 0x38) {
				error_code = _DCMI_ERROR_DMA;
				state = DCMIState::Ready;
				ReleaseDMA();
				lock.store(false, MemoryOrder_Release);
				return false;
			}
			L1C.CleanInvalidateDCacheRange(staging_buffers, sizeof(staging_buffers));
		}
		variable_capture = variable;
		finishing_frame = capacity_reached = false;
		capture_capacity = word_count * 4;
		received_size.store(0, MemoryOrder_Release);
		frame_ready.store(false, MemoryOrder_Release);
		transfer_size = size;
		buffer_address = _IMM(dest);
		transfer_count = transfer_number = segments > 1 ? segments - 2 : 0;
		frame_blocks = blocks_left = segments;
		self[DCMIReg::ICR] = _DCMI_IT_ALL;
		self[DCMIReg::IER] = variable ? _DCMI_IT_ALL : _DCMI_IT_CAPTURE;
		self[DCMIReg::CR].setof(_DCMI_CR_POS_CM, mode == DCMICaptureMode::Snapshot);
		cr.setof(_DMA_SxCR_POS_CT, false);
		if (variable) cr.setof(_DMA_SxCR_POS_CIRC, false);
		enAble();
		active_dma->setRequest(_DCMI_DMA_REQUEST);
		pureptr_t source = (pureptr_t)(_DCMI_BASE + _IMMx4(DCMIReg::DR));
		bool started;
		if (staged_capture) {
			started = active_dma->MultiBufferTransfer(staging_buffers[0], source, staging_buffers[1], size, IOMethod::Rupt);
		} else if (segments == 1) {
			started = active_dma->Transfer(dest, source, size, IOMethod::Rupt);
		} else {
			started = active_dma->MultiBufferTransfer(dest, source, (pureptr_t)(_IMM(dest) + size * 4), size, IOMethod::Rupt);
		}
		if (!started) {
			self[DCMIReg::IER] = 0;
			enAble(false);
			error_code |= _DCMI_ERROR_DMA;
			state = DCMIState::Ready;
			variable_capture = false;
			ReleaseDMA();
			lock.store(false, MemoryOrder_Release);
			return false;
		}
		// Camera examples use TC without FE interrupts; retain TE/DME reporting without a FIFO IRQ storm.
		dma[DMAReg::FCR[active_dma->getID()]].setof(_DMA_SxFCR_POS_FEIE, false);
		stduint interrupt_mask;
		_ASM volatile("mrs %0, primask\n\tcpsid i" : "=r"(interrupt_mask) : : "memory");
		self[DCMIReg::CR].setof(_DCMI_CR_POS_CAPTURE);
		lock.store(false, MemoryOrder_Release);
		if (!interrupt_mask) _ASM volatile("cpsie i" : : : "memory");
		return true;
	}

	bool DCMI_t::WaitCaptureStopped() {
		uint64 begin = SysTick::getTick();
		stduint count = 0;
		while (self[DCMIReg::CR].bitof(_DCMI_CR_POS_CAPTURE)) {
			if (SysTick::getTick() - begin >= _DCMI_TIMEOUT_VALUE || ++count >= DMA_TIMEOUT_VALUE) {
				error_code |= _DCMI_ERROR_TIMEOUT;
				return false;
			}
		}
		return true;
	}

	bool DCMI_t::StopTransfer() {
		if (state == DCMIState::Busy || state == DCMIState::Suspended)
			frame_ready.store(false, MemoryOrder_Release);
		state = DCMIState::Busy;
		self[DCMIReg::IER] = 0;
		self[DCMIReg::CR].setof(_DCMI_CR_POS_CAPTURE, false);
		bool stopped = WaitCaptureStopped();
		enAble(false);
		if (active_dma) {
			DMA_t& dma = active_dma->getParent();
			byte stream = active_dma->getID();
			bool fifo_error_interrupt = dma[DMAReg::FCR[stream]].bitof(_DMA_SxFCR_POS_FEIE);
			active_dma->setInterruptSub(false);
			byte base = (stream & 2U ? 16 : 0) + (stream & 1U ? 6 : 0);
			uint32 pending_flags = finishing_frame ?
				dma[stream & 4U ? DMAReg::HISR : DMAReg::LISR].masof(base, 6) : 0;
			dma[DMAReg::CR[stream]].setof(_DMA_SxCR_POS_EN, false);
			uint64 begin = SysTick::getTick();
			stduint count = 0;
			while (dma[DMAReg::CR[stream]].bitof(_DMA_SxCR_POS_EN)) {
				if (SysTick::getTick() - begin >= _DCMI_TIMEOUT_VALUE || ++count >= DMA_TIMEOUT_VALUE) {
					error_code |= _DCMI_ERROR_DMA | _DCMI_ERROR_TIMEOUT;
					state = DCMIState::Error;
					return false;
				}
			}
			if (finishing_frame) {
				uint32 flags = dma[stream & 4U ? DMAReg::HISR : DMAReg::LISR].masof(base, 6);
				uint32 errors = _IMM1S(_DMA_SxFLAG_POS_TEIF) | _IMM1S(_DMA_SxFLAG_POS_DMEIF);
				if (variable_capture && fifo_error_interrupt) errors |= _IMM1S(_DMA_SxFLAG_POS_FEIF);
				if (flags & errors) error_code |= _DCMI_ERROR_DMA;
				if (staged_capture && !error_code) {
					bool current_m1 = dma[DMAReg::CR[stream]].bitof(_DMA_SxCR_POS_CT);
					stduint block_bytes = transfer_size * 4;
					bool expected_m1 = (staged_written / block_bytes) & 1U;
					// RM0433, DMA stream interruption: clearing EN also sets TCIF; only pre-stop TCIF indicates backlog.
					if (current_m1 != expected_m1) {
						if (!CopyStagedBuffer(!current_m1, block_bytes)) error_code |= _DCMI_ERROR_OVR;
					} else if (pending_flags & _IMM1S(_DMA_SxFLAG_POS_TCIF)) error_code |= _DCMI_ERROR_OVR;
					stduint remaining = dma[DMAReg::NDTR[stream]];
					if (remaining > transfer_size) error_code |= _DCMI_ERROR_DMA;
					else if (!error_code && remaining < transfer_size &&
						!CopyStagedBuffer(current_m1, (transfer_size - remaining) * 4)) error_code |= _DCMI_ERROR_OVR;
				} else if (variable_capture && !staged_capture &&
					dma[DMAReg::CR[stream]].bitof(_DMA_SxCR_POS_DBM)) {
					bool current_m1 = dma[DMAReg::CR[stream]].bitof(_DMA_SxCR_POS_CT);
					stduint address = dma[current_m1 ? DMAReg::M0AR[stream] : DMAReg::M1AR[stream]];
					stduint current = dma[current_m1 ? DMAReg::M1AR[stream] : DMAReg::M0AR[stream]];
					if (((pending_flags & _IMM1S(_DMA_SxFLAG_POS_TCIF)) && address >= buffer_address &&
						address - buffer_address >= capture_capacity - transfer_size * 4) ||
						(blocks_left < frame_blocks && current == buffer_address)) {
						capacity_reached = true;
						error_code |= _DCMI_ERROR_OVR;
					}
				}
			}
			active_dma->ClearInterruptFlags();
			dma.streamStates[stream] = _DMA_STATE_READY;
			Reference icsr((pureptr_t)&((SysCtrlBlock_Map*)_SCB_BASE)->ICSR);
			if (icsr.masof(SCB_ICSR_VECTACTIVE_Pos, 9) && !icsr.bitof(SCB_ICSR_RETTOBASE_Pos)) {
				// A preempted DMA dispatcher must finish before its callbacks can be restored.
				deferred_release = true;
			} else ReleaseDMA();
		}
		self[DCMIReg::ICR] = _DCMI_IT_ALL;
		variable_capture = false;
		state = DCMIState::Ready;
		return stopped && !deferred_release;
	}

	bool DCMI_t::FinishFrame() {
		stduint interrupt_mask;
		_ASM volatile("mrs %0, primask\n\tcpsid i" : "=r"(interrupt_mask) : : "memory");
		if (!active_dma || finishing_frame || deferred_release) {
			if (!interrupt_mask) _ASM volatile("cpsie i" : : : "memory");
			return false;
		}
		bool snapshot = self[DCMIReg::CR].bitof(_DCMI_CR_POS_CM);
		if (!variable_capture) {
			if (snapshot) self[DCMIReg::IER] = 0;
			self[DCMIReg::IER].setof(_DCMI_IT_POS_FRAME, false);
			self[DCMIReg::ICR] = _IMM1S(_DCMI_IT_POS_FRAME);
			received_size.store(capture_capacity, MemoryOrder_Release);
			if (staged_capture && staged_written != capture_capacity) error_code |= _DCMI_ERROR_OVR;
			bool complete = error_code == _DCMI_ERROR_NONE;
			frame_ready.store(complete, MemoryOrder_Release);
			if (!interrupt_mask) _ASM volatile("cpsie i" : : : "memory");
			if (complete) { asserv(FrameHandler)(); }
			else { asserv(ErrorHandler)(); }
			return complete;
		}
		finishing_frame = true;
		const DMAStream& stream = *active_dma;
		DMA_t& dma = stream.getParent();
		byte stream_id = stream.getID();
		bool variable = variable_capture;
		self[DCMIReg::IER] = 0;
		stream.setInterruptSub(false);
		if (!interrupt_mask) _ASM volatile("cpsie i" : : : "memory");

		self[DCMIReg::CR].setof(_DCMI_CR_POS_CAPTURE, false);
		if (error_code) enAble(false);
		bool stopped = WaitCaptureStopped();
		uint64 begin = SysTick::getTick();
		stduint count = 0;
		// AN5020: end-of-frame padding must leave the DCMI FIFO before DMA is stopped.
		while (stopped && !error_code && self[DCMIReg::SR].bitof(_DCMI_SR_POS_FNE)) {
			if (!dma[DMAReg::CR[stream_id]].bitof(_DMA_SxCR_POS_EN)) {
				error_code |= _DCMI_ERROR_OVR;
				break;
			}
			if (SysTick::getTick() - begin >= _DCMI_TIMEOUT_VALUE || ++count >= DMA_TIMEOUT_VALUE) {
				error_code |= _DCMI_ERROR_TIMEOUT;
				break;
			}
		}
		uint32 flags = self[DCMIReg::RISR];
		if (flags & _IMM1S(_DCMI_IT_POS_OVR)) error_code |= _DCMI_ERROR_OVR;
		if (flags & _IMM1S(_DCMI_IT_POS_ERR)) error_code |= _DCMI_ERROR_SYNC;
		if (stream.getError() != _DMA_ERROR_NONE) error_code |= _DCMI_ERROR_DMA;
		bool cleaned = StopTransfer();
		stopped = (cleaned || (deferred_release && state == DCMIState::Ready)) && stopped;

		stduint length = capture_capacity;
		if (variable && staged_capture) {
			length = staged_written;
			if (length >= capture_capacity) error_code |= _DCMI_ERROR_OVR;
		} else if (variable && !capacity_reached && stopped) {
			stduint remaining = dma[DMAReg::NDTR[stream_id]];
			bool current_m1 = dma[DMAReg::CR[stream_id]].bitof(_DMA_SxCR_POS_CT);
			stduint address = dma[current_m1 ? DMAReg::M1AR[stream_id] : DMAReg::M0AR[stream_id]];
			if (remaining > transfer_size || address < buffer_address ||
				address - buffer_address > capture_capacity) {
				error_code |= _DCMI_ERROR_DMA;
				length = 0;
			} else {
				length = address - buffer_address + (transfer_size - remaining) * 4;
				if (length >= capture_capacity) {
					length = capture_capacity;
					error_code |= _DCMI_ERROR_OVR;
				}
			}
		}
		if (!length) error_code |= _DCMI_ERROR_SYNC;
		bool complete = stopped && error_code == _DCMI_ERROR_NONE;
		received_size.store(stopped ? length : 0, MemoryOrder_Release);
		state = complete ? DCMIState::Ready : DCMIState::Error;
		frame_ready.store(complete, MemoryOrder_Release);
		finishing_frame = false;
		if (complete) { asserv(FrameHandler)(); }
		else { asserv(ErrorHandler)(); }
		return complete;
	}

	bool DCMI_t::Stop() {
		if (lock.exchange(true, MemoryOrder_Acquire)) return false;
		if (active_device && active_device != this) {
			lock.store(false, MemoryOrder_Release);
			return false;
		}
		bool stopped = state == DCMIState::Reset || StopTransfer();
		lock.store(false, MemoryOrder_Release);
		return stopped;
	}

	bool DCMI_t::Suspend() {
		if (lock.exchange(true, MemoryOrder_Acquire)) return false;
		bool suspended = true;
		if (state == DCMIState::Busy) {
			state = DCMIState::Suspended;
			self[DCMIReg::CR].setof(_DCMI_CR_POS_CAPTURE, false);
			suspended = WaitCaptureStopped();
			if (!suspended) state = DCMIState::Ready;
		}
		lock.store(false, MemoryOrder_Release);
		return suspended;
	}

	bool DCMI_t::Resume() {
		if (lock.exchange(true, MemoryOrder_Acquire)) return false;
		if (state == DCMIState::Suspended) {
			state = DCMIState::Busy;
			self[DCMIReg::CR].setof(_DCMI_CR_POS_CAPTURE);
		}
		lock.store(false, MemoryOrder_Release);
		return true;
	}

	bool DCMI_t::ConfigCrop(const Rectangle& rect) {
		if (!rect.width || !rect.height || rect.x > _DCMI_WINDOW_COORDINATE || rect.y > _DCMI_WINDOW_HEIGHT ||
			rect.width - 1 > _DCMI_WINDOW_COORDINATE || rect.height - 1 > _DCMI_WINDOW_COORDINATE) return false;
		if (lock.exchange(true, MemoryOrder_Acquire)) return false;
		if (state != DCMIState::Ready) {
			lock.store(false, MemoryOrder_Release);
			return false;
		}
		state = DCMIState::Busy;
		uint32 size = 0, start = 0;
		Reference size_fields((pureptr_t)&size), start_fields((pureptr_t)&start);
		size_fields.maset(_DCMI_CWSIZE_POS_CAPCNT, 14, rect.width - 1);
		size_fields.maset(_DCMI_CWSIZE_POS_VLINE, 14, rect.height - 1);
		start_fields.maset(_DCMI_CWSTRT_POS_HOFFCNT, 14, rect.x);
		start_fields.maset(_DCMI_CWSTRT_POS_VST, 13, rect.y);
		self[DCMIReg::CWSIZER] = size;
		self[DCMIReg::CWSTRTR] = start;
		state = DCMIState::Ready;
		lock.store(false, MemoryOrder_Release);
		return true;
	}

	bool DCMI_t::enCrop(bool ena) {
		if (lock.exchange(true, MemoryOrder_Acquire)) return false;
		if (state != DCMIState::Ready) {
			lock.store(false, MemoryOrder_Release);
			return false;
		}
		state = DCMIState::Busy;
		self[DCMIReg::CR].setof(_DCMI_CR_POS_CROP, ena);
		state = DCMIState::Ready;
		lock.store(false, MemoryOrder_Release);
		return true;
	}

	void DCMI_t::IRQHandler() {
		uint32 flags = self[DCMIReg::MISR];
		uint32 errors = 0;
		if (flags & _IMM1S(_DCMI_IT_POS_ERR)) errors |= _DCMI_ERROR_SYNC;
		if (flags & _IMM1S(_DCMI_IT_POS_OVR)) errors |= _DCMI_ERROR_OVR;
		if (errors) {
			self[DCMIReg::ICR] = flags & (_IMM1S(_DCMI_IT_POS_ERR) | _IMM1S(_DCMI_IT_POS_OVR));
			error_code |= errors;
			if (staged_capture) received_size.store(staged_written, MemoryOrder_Release);
			frame_ready.store(false, MemoryOrder_Release);
			state = DCMIState::Error;
			if (variable_capture) {
				FinishFrame();
				return;
			}
			if (active_dma && active_dma->getState() == _DMA_STATE_BUSY) {
				DMA_t& dma = active_dma->getParent();
				dma.XferAbortCallback = DMAError;
				dma.streamStates[active_dma->getID()] = _DMA_STATE_ABORT;
				dma[DMAReg::CR[active_dma->getID()]].setof(_DMA_SxCR_POS_EN, false);
			} else if (active_dma && active_dma->getState() != _DMA_STATE_ABORT) {
				DMAError();
			} else if (!active_dma) {
				asserv(ErrorHandler)();
			}
		}
		if (flags & _IMM1S(_DCMI_IT_POS_LINE)) {
			self[DCMIReg::ICR] = _IMM1S(_DCMI_IT_POS_LINE);
			asserv(LineHandler)();
		}
		if (flags & _IMM1S(_DCMI_IT_POS_VSYNC)) {
			self[DCMIReg::ICR] = _IMM1S(_DCMI_IT_POS_VSYNC);
			asserv(VsyncHandler)();
		}
		if (flags & _IMM1S(_DCMI_IT_POS_FRAME)) {
			FinishFrame();
		}
	}

	bool DCMI_t::CopyStagedBuffer(bool memory1, stduint bytes) {
		if (!bytes) return true;
		if (bytes > transfer_size * 4 || staged_written > capture_capacity ||
			bytes > capture_capacity - staged_written) return false;
		uint32* source = staging_buffers[memory1 ? 1 : 0];
		// Cortex-M7 DMA receive: discard speculative cache fills before consuming SRAM data.
		L1C.InvalidateDCacheRange(source, bytes);
		volatile uint32* target = (volatile uint32*)(buffer_address + staged_written);
		for (stduint i = 0; i < bytes / 4; ++i) target[i] = source[i];
		__DSB();
		staged_written += bytes;
		return true;
	}

	void DCMI_t::DMATransferComplete() {
		stduint interrupt_mask;
		_ASM volatile("mrs %0, primask\n\tcpsid i" : "=r"(interrupt_mask) : : "memory");
		DCMI_t* device = active_device;
		if (!device || !device->active_dma) {
			if (!interrupt_mask) _ASM volatile("cpsie i" : : : "memory");
			return;
		}
		const DMAStream& stream = *device->active_dma;
		DMA_t& dma = stream.getParent();
		if (device->finishing_frame || device->deferred_release) {
			if (!interrupt_mask) _ASM volatile("cpsie i" : : : "memory");
			return;
		}
		bool current_m1 = dma[DMAReg::CR[stream.getID()]].bitof(_DMA_SxCR_POS_CT);
		if (device->staged_capture) {
			stduint block_bytes = device->transfer_size * 4;
			bool expected_m1 = !((device->staged_written / block_bytes) & 1U);
			bool copied = current_m1 == expected_m1 && stream.getError() == _DMA_ERROR_NONE &&
				device->CopyStagedBuffer(!current_m1, block_bytes);
			byte id = stream.getID();
			byte base = (id & 2U ? 16 : 0) + (id & 1U ? 6 : 0);
			if (dma[DMAReg::CR[id]].bitof(_DMA_SxCR_POS_CT) != current_m1 ||
				dma[id & 4U ? DMAReg::HISR : DMAReg::LISR].bitof(base + _DMA_SxFLAG_POS_TCIF)) copied = false;
			bool full = device->staged_written == device->capture_capacity;
			if (!copied || (full && device->variable_capture)) {
				device->received_size.store(device->staged_written, MemoryOrder_Release);
				device->error_code |= _DCMI_ERROR_OVR;
				device->state = DCMIState::Error;
				dma[DMAReg::CR[id]].setof(_DMA_SxCR_POS_EN, false);
				device->enAble(false);
				(*device)[DCMIReg::IER] = 0;
			} else if (full) {
				(*device)[DCMIReg::IER].setof(_DCMI_IT_POS_FRAME);
				device->state = DCMIState::Ready;
			}
			if (!interrupt_mask) _ASM volatile("cpsie i" : : : "memory");
			if (device->error_code) { asserv(device->ErrorHandler)(); }
			return;
		}
		if (device->variable_capture) {
			if (stream.getError() != _DMA_ERROR_NONE) {
				if (!interrupt_mask) _ASM volatile("cpsie i" : : : "memory");
				return;
			}
			stduint block_bytes = device->transfer_size * 4;
			if (device->blocks_left) --device->blocks_left;
			bool double_buffer = dma[DMAReg::CR[stream.getID()]].bitof(_DMA_SxCR_POS_DBM);
			stduint address = dma[double_buffer && !current_m1 ?
				DMAReg::M1AR[stream.getID()] : DMAReg::M0AR[stream.getID()]];
			bool full = !double_buffer || address - device->buffer_address >= device->capture_capacity - block_bytes;
			if (full) {
				device->capacity_reached = true;
				device->error_code |= _DCMI_ERROR_OVR;
				(*device)[DCMIReg::IER] = 0;
				dma[DMAReg::CR[stream.getID()]].setof(_DMA_SxCR_POS_EN, false);
				device->enAble(false);
			} else {
				uint64 next = (uint64)address + 2 * block_bytes;
				if (next + block_bytes > (uint64)device->buffer_address + device->capture_capacity)
					next = device->buffer_address;
				stream.ChangeMemory((pureptr_t)(stduint)next, current_m1 ? MemorySel::Memory0 : MemorySel::Memory1);
			}
			if (!interrupt_mask) _ASM volatile("cpsie i" : : : "memory");
			if (full) device->FinishFrame();
			return;
		}
		device->frame_ready.store(false, MemoryOrder_Release);
		if (dma[DMAReg::CR[stream.getID()]].bitof(_DMA_SxCR_POS_DBM)) {
			if (device->transfer_count) {
				if (!(device->transfer_count & 1U) && current_m1) {
					stduint addr = dma[DMAReg::M0AR[stream.getID()]];
					stream.ChangeMemory((pureptr_t)(addr + 8 * device->transfer_size), MemorySel::Memory0);
					--device->transfer_count;
				} else if (!current_m1) {
					stduint addr = dma[DMAReg::M1AR[stream.getID()]];
					stream.ChangeMemory((pureptr_t)(addr + 8 * device->transfer_size), MemorySel::Memory1);
					--device->transfer_count;
				}
			} else if (current_m1) {
				stream.ChangeMemory((pureptr_t)device->buffer_address, MemorySel::Memory0);
			} else {
				stream.ChangeMemory((pureptr_t)(device->buffer_address + 4 * device->transfer_size), MemorySel::Memory1);
				device->transfer_count = device->transfer_number;
			}
		}
		// A two-segment frame requires both completions before becoming ready.
		if (--device->blocks_left == 0) {
			device->blocks_left = device->frame_blocks;
			(*device)[DCMIReg::IER].setof(_DCMI_IT_POS_FRAME);
			if ((*device)[DCMIReg::CR].bitof(_DCMI_CR_POS_CM)) device->state = DCMIState::Ready;
		}
		if (!interrupt_mask) _ASM volatile("cpsie i" : : : "memory");
	}

	void DCMI_t::DMAError() {
		stduint interrupt_mask;
		_ASM volatile("mrs %0, primask\n\tcpsid i" : "=r"(interrupt_mask) : : "memory");
		DCMI_t* device = active_device;
		if (!device || !device->active_dma || device->finishing_frame || device->deferred_release) {
			if (!interrupt_mask) _ASM volatile("cpsie i" : : : "memory");
			return;
		}
		if (device->variable_capture) {
			device->error_code |= _DCMI_ERROR_DMA;
			(*device)[DCMIReg::IER] = 0;
			device->active_dma->setInterruptSub(false);
			if (!interrupt_mask) _ASM volatile("cpsie i" : : : "memory");
			device->FinishFrame();
			return;
		}
		if (device->active_dma->getError() != _DMA_ERROR_FE) {
			device->frame_ready.store(false, MemoryOrder_Release);
			device->state = DCMIState::Ready;
			device->error_code |= _DCMI_ERROR_DMA;
		}
		if (!interrupt_mask) _ASM volatile("cpsie i" : : : "memory");
		asserv(device->ErrorHandler)();
	}

	void DCMI_t::setInterrupt(Handler_t handler) const {
		const_cast<DCMI_t*>(this)->FrameHandler = handler;
	}

	void DCMI_t::setInterruptPriority(byte preempt, byte sub_priority) const {
		NVIC.setPriority(IRQ_DCMI, preempt, sub_priority);
	}

	void DCMI_t::enInterrupt(bool ena) const {
		NVIC.setAble(IRQ_DCMI, ena);
	}
}

extern "C" void DCMI_IRQHandler() {
	uni::DCMI_t* device = uni::DCMI_t::active_device;
	if (!device) device = &uni::DCMI;
	device->IRQHandler();
}

extern "C" void DCMI_PSSI_IRQHandler() {
	DCMI_IRQHandler();
}
#endif

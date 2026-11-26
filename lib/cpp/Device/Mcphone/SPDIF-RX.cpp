// ASCII CPP-ISO11 TAB4 LF
// Docutitle: [Device] Sony/Philips Digital Interface Receiver
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

#include "../../../../inc/cpp/Device/Mcphone/SPDIF-RX.hpp"

#if defined(_MCU_STM32H7x)
#include "../../../../inc/cpp/Device/DMA"
#include "../../../../inc/cpp/Device/Cache"
#include "../../../../inc/cpp/Device/NVIC"
#include "../../../../inc/cpp/Device/RCC/RCC"
#include "../../../../inc/cpp/Device/SysTick"

namespace uni {
	namespace {
		class InterruptGuard {
			stduint mask;
		public:
			InterruptGuard() { _ASM volatile("mrs %0, primask\n\tcpsid i" : "=r"(mask) : : "memory"); }
			~InterruptGuard() {
				if (!mask) _ASM volatile("cpsie i" : : : "memory");
			}
		};

		class OperationGuard {
			Atomic<bool>& operation;
			bool acquired;
		public:
			explicit OperationGuard(Atomic<bool>& operation) : operation(operation), acquired(false) {
				stduint exception;
				_ASM volatile("mrs %0, ipsr" : "=r"(exception));
				if (!exception) acquired = !operation.exchange(true, MemoryOrder_Acquire);
			}
			~OperationGuard() { if (acquired) operation.store(false, MemoryOrder_Release); }
			explicit operator bool() const { return acquired; }
		};

		constexpr uint32 receive_errors = _SPDIFRX_SR_PERR | _SPDIFRX_SR_OVR;
		constexpr stduint stop_poll_limit = 0x10000U;

		bool Expired(uint64 begin, stduint timeout_ms) {
			return timeout_ms != (stduint)-1 && SysTick::getTick() - begin >= timeout_ms;
		}

		SPDIFRXReg ReceiveRegister(SPDIFRXFlow flow) {
			return flow == SPDIFRXFlow::Data ? SPDIFRXReg::DR : SPDIFRXReg::CSR;
		}

		byte DMAEnablePosition(SPDIFRXFlow flow) {
			return flow == SPDIFRXFlow::Data ? _SPDIFRX_CR_POS_RXDMAEN : _SPDIFRX_CR_POS_CBDMAEN;
		}

		uint32 ReceiveError(uint32 status) {
			uint32 error = 0;
			if (status & _SPDIFRX_SR_OVR) error |= (uint32)SPDIFRXError::Overrun;
			if (status & _SPDIFRX_SR_PERR) error |= (uint32)SPDIFRXError::Parity;
			if (status & _SPDIFRX_SR_FERR) error |= (uint32)SPDIFRXError::Frame;
			if (status & _SPDIFRX_SR_SERR) error |= (uint32)SPDIFRXError::Synchronization;
			if (status & _SPDIFRX_SR_TERR) error |= (uint32)SPDIFRXError::TransitionTimeout;
			return error;
		}

		void ApplyDataFormat(Reference reg, const SPDIFRXDataFormat& format) {
			reg.setof(_SPDIFRX_CR_POS_RXSTEO, format.stereo);
			reg.maset(_SPDIFRX_CR_POS_DRFMT, 2, byte(format.packing));
			reg.setof(_SPDIFRX_CR_POS_PMSK, format.mask_parity_error);
			reg.setof(_SPDIFRX_CR_POS_VMSK, format.mask_validity);
			reg.setof(_SPDIFRX_CR_POS_CUMSK, format.mask_channel_status);
			reg.setof(_SPDIFRX_CR_POS_PTMSK, format.mask_preamble);
		}
	}

	SPDIFRX_t SPDIFRX;

	bool SPDIFRX_t::setMode(const SPDIFRXConfig_t& config) {
		if (byte(config.input) > byte(SPDIFRXInput::Input3) || byte(config.retries) > byte(SPDIFRXRetries::SixtyThree) ||
			byte(config.control_channel) > byte(SPDIFRXChannel::B) || byte(config.format.packing) > byte(SPDIFRXPacking::Packed16))
			return false;
		OperationGuard operation_guard(operation);
		if (!operation_guard) return false;
		{
			InterruptGuard guard;
			if (data.active || control.active) return false;
		}
		if (!StopTransfers(10)) return false;

		InterruptGuard guard;
		RCC[RCCReg::APB1LENR].setof(_SPDIFRX_RCC_POS_ENABLE);
		(void)RCC[RCCReg::APB1LENR].bitof(_SPDIFRX_RCC_POS_ENABLE);
		SetIdle();
		Reference reg = self[SPDIFRXReg::CR];
		reg.rstof(_SPDIFRX_CR_POS_RXDMAEN);
		reg.rstof(_SPDIFRX_CR_POS_CBDMAEN);
		reg.maset(_SPDIFRX_CR_POS_INSEL, 3, byte(config.input));
		reg.maset(_SPDIFRX_CR_POS_NBTR, 2, byte(config.retries));
		reg.setof(_SPDIFRX_CR_POS_CHSEL, config.control_channel == SPDIFRXChannel::B);
		reg.setof(_SPDIFRX_CR_POS_WFA, config.wait_for_activity);
		reg.setof(_SPDIFRX_CR_POS_CKSEN, config.symbol_clock);
		reg.setof(_SPDIFRX_CR_POS_CKSBKPEN, config.backup_symbol_clock);
		ApplyDataFormat(reg, config.format);
		configured = true;
		link_failed = false;
		link_error = data.error = control.error = 0;
		return true;
	}

	bool SPDIFRX_t::canMode() {
		OperationGuard operation_guard(operation);
		if (!operation_guard || !StopTransfers(10)) return false;
		InterruptGuard guard;
		if (!configured) return true;
		enInterrupt(false);
		RCC[RCCReg::APB1LRSTR].setof(_SPDIFRX_RCC_POS_RESET);
		RCC[RCCReg::APB1LRSTR].rstof(_SPDIFRX_RCC_POS_RESET);
		RCC[RCCReg::APB1LENR].rstof(_SPDIFRX_RCC_POS_ENABLE);
		configured = false;
		link_failed = false;
		link_error = data.error = control.error = 0;
		return true;
	}

	bool SPDIFRX_t::setDataFormat(const SPDIFRXDataFormat& format) {
		if (byte(format.packing) > byte(SPDIFRXPacking::Packed16)) return false;
		OperationGuard operation_guard(operation);
		if (!operation_guard) return false;
		InterruptGuard guard;
		if (!configured || link_failed) return false;
		Reference reg = self[SPDIFRXReg::CR];
		if (reg.masof(_SPDIFRX_CR_POS_ENABLE, 2) == byte(SPDIFRXHardwareState::Receiving) &&
			(reg.masof(_SPDIFRX_CR_POS_DRFMT, 2) != byte(format.packing) ||
				reg.bitof(_SPDIFRX_CR_POS_RXSTEO) != format.stereo)) return false;
		ApplyDataFormat(reg, format);
		return true;
	}

	bool SPDIFRX_t::StartDMA(SPDIFRXFlow flow, Transfer& transfer) {
		const DMAStream* stream = flow == SPDIFRXFlow::Data ? data_dma : control_dma;
		stduint encoded = _IMM(stream);
		if ((encoded >> 8) < 1 || (encoded >> 8) > 2 || (encoded & 0x8FU) || transfer.remaining > 0xFFFFU)
			return false;

		DMA_t& dma = stream->getParent();
		const DMAStream* other = flow == SPDIFRXFlow::Data ? control.dma : data.dma;
		if ((other && &other->getParent() == &dma) || dma.bind || stream->getState() != _DMA_STATE_READY)
			return false;

		for (byte i = 0; i < 8; ++i) {
			if (dma[DMAReg::CR[i]].bitof(_DMA_SxCR_POS_EN) ||
				dma.streamStates[i] == _DMA_STATE_BUSY || dma.streamStates[i] == _DMA_STATE_ABORT) return false;
		}
		Reference reg = dma[DMAReg::CR[stream->getID()]];
		bool circular = reg.bitof(_DMA_SxCR_POS_CIRC);
		if (reg.masof(_DMA_SxCR_POS_DIR, 2) || reg.masof(_DMA_SxCR_POS_PSIZE, 2) != 2 ||
			reg.masof(_DMA_SxCR_POS_MSIZE, 2) != 2 || reg.bitof(_DMA_SxCR_POS_PINC) ||
			!reg.bitof(_DMA_SxCR_POS_MINC) || reg.bitof(_DMA_SxCR_POS_PFCTRL) ||
			reg.masof(_DMA_SxCR_POS_PBURST, 2) || (circular && flow == SPDIFRXFlow::Control)) return false;

		transfer.dma = stream;
		transfer.circular = circular;
		transfer.saved_half = dma.XferHalfCallback;
		transfer.saved_complete = dma.XferCpltCallback;
		transfer.saved_error = dma.XferErrorCallback;
		dma.bind = &transfer;
		if (flow == SPDIFRXFlow::Data) {
			dma.XferHalfCallback = DataHalfHandler ? DataDMAHalf : nullptr;
			dma.XferCpltCallback = DataDMAComplete;
			dma.XferErrorCallback = DataDMAError;
		} else {
			dma.XferHalfCallback = ControlHalfHandler ? ControlDMAHalf : nullptr;
			dma.XferCpltCallback = ControlDMAComplete;
			dma.XferErrorCallback = ControlDMAError;
		}
		stream->setRequest(flow == SPDIFRXFlow::Data ? _SPDIFRX_DMA_REQUEST_DATA : _SPDIFRX_DMA_REQUEST_CONTROL);
		pureptr_t source = (pureptr_t)(_SPDIFRX_BASE + _IMMx4(ReceiveRegister(flow)));
		stream->setTransfer(transfer.dest, source, transfer.remaining);
		stream->ClearInterruptFlags();
		stream->setInterruptSub(true);
		if (!dma.XferHalfCallback) stream->enInterrupt(false, 2);
		stream->enInterruptNVIC();
		dma.streamStates[stream->getID()] = _DMA_STATE_BUSY;
		dma.streamErrors[stream->getID()] = _DMA_ERROR_NONE;
		// The peripheral request is still disabled; never wait for DMA EN with interrupts masked.
		reg.setof(_DMA_SxCR_POS_EN);
		if (reg.bitof(_DMA_SxCR_POS_EN)) return true;
		stream->setInterruptSub(false);
		stream->ClearInterruptFlags();
		dma.streamStates[stream->getID()] = _DMA_STATE_READY;
		dma.bind = nullptr;
		dma.XferHalfCallback = transfer.saved_half;
		dma.XferCpltCallback = transfer.saved_complete;
		dma.XferErrorCallback = transfer.saved_error;
		transfer.dma = nullptr;
		transfer.circular = false;
		return false;
	}

	bool SPDIFRX_t::ReleaseDMA(Transfer& transfer, stduint timeout_ms) {
		const DMAStream* stream = transfer.dma;
		if (!stream) return true;
		DMA_t& dma = stream->getParent();
		byte index = stream->getID();
		Reference reg = dma[DMAReg::CR[index]];
		uint64 begin = SysTick::getTick();
		stduint polls = 0;
		while (reg.bitof(_DMA_SxCR_POS_EN)) {
			if (++polls >= stop_poll_limit || Expired(begin, timeout_ms)) {
				InterruptGuard guard;
				transfer.error |= (uint32)SPDIFRXError::DMA | (uint32)SPDIFRXError::Timeout;
				return false;
			}
		}

		InterruptGuard guard;
		stream->ClearInterruptFlags();
		dma.streamStates[index] = _DMA_STATE_READY;
		dma.streamErrors[index] = _DMA_ERROR_NONE;
		dma.bind = nullptr;
		dma.XferHalfCallback = transfer.saved_half;
		dma.XferCpltCallback = transfer.saved_complete;
		dma.XferErrorCallback = transfer.saved_error;
		transfer.dma = nullptr;
		transfer.dest = nullptr;
		transfer.circular = false;
		return true;
	}

	void SPDIFRX_t::RequestStop(SPDIFRXFlow flow) {
		Transfer& transfer = SelectTransfer(flow);
		transfer.active = false;
		if (transfer.dma) {
			const DMAStream& stream = *transfer.dma;
			self[SPDIFRXReg::CR].rstof(DMAEnablePosition(flow));
			stream.setInterruptSub(false);
			stream.getParent()[DMAReg::CR[stream.getID()]].rstof(_DMA_SxCR_POS_EN);
		} else {
			transfer.dest = nullptr;
		}
	}

	bool SPDIFRX_t::StopTransfers(stduint timeout_ms) {
		{
			InterruptGuard guard;
			if (!configured) return true;
			self[SPDIFRXReg::IMR] = 0;
			RequestStop(SPDIFRXFlow::Data);
			RequestStop(SPDIFRXFlow::Control);
			SetIdle();
		}
		bool data_stopped = ReleaseDMA(data, timeout_ms);
		bool control_stopped = ReleaseDMA(control, timeout_ms);
		if (data_stopped && control_stopped) {
			InterruptGuard guard;
			link_failed = false;
			return true;
		}
		return false;
	}

	void SPDIFRX_t::SetIdle() {
		// RM0433, Clearing interrupt source: entering Idle acknowledges FERR, SERR and TERR.
		self[SPDIFRXReg::IMR] = 0;
		self[SPDIFRXReg::CR].maset(_SPDIFRX_CR_POS_ENABLE, 2, byte(SPDIFRXHardwareState::Idle));
		self[SPDIFRXReg::IFCR] = _SPDIFRX_IFCR_CLEARABLE;
	}

	void SPDIFRX_t::UpdateInterruptMask() {
		uint32 mask = 0;
		if ((data.active && data.method != IOMethod::Loop) || (control.active && control.method != IOMethod::Loop))
			mask |= _SPDIFRX_IMR_IFEIE | _SPDIFRX_SR_PERR;
		if (data.active && data.method != IOMethod::Loop) mask |= _SPDIFRX_SR_OVR;
		if (data.active && data.method == IOMethod::Rupt) mask |= _SPDIFRX_SR_RXNE;
		if (control.active && control.method == IOMethod::Rupt) mask |= _SPDIFRX_SR_CSRNE;
		self[SPDIFRXReg::IMR] = mask;
	}

	SPDIFRXResult SPDIFRX_t::WaitForSync(stduint timeout_ms) {
		uint64 begin = SysTick::getTick();
		do {
			uint32 status = self[SPDIFRXReg::SR];
			if (status & _SPDIFRX_LINK_ERRORS) {
				InterruptGuard guard;
				link_error |= ReceiveError(status);
				link_failed = true;
				SetIdle();
				return SPDIFRXResult::Error;
			}
			if (status & _SPDIFRX_SR_SYNCD) return SPDIFRXResult::OK;
		} while (!Expired(begin, timeout_ms));
		InterruptGuard guard;
		link_error |= (uint32)SPDIFRXError::Timeout;
		SetIdle();
		return SPDIFRXResult::Timeout;
	}

	SPDIFRXResult SPDIFRX_t::Receive(SPDIFRXFlow flow, uint32* dest, stduint word_count, IOMethod method, stduint timeout_ms) {
		if ((flow != SPDIFRXFlow::Data && flow != SPDIFRXFlow::Control) || !dest || (_IMM(dest) & 3U) || !word_count ||
			(word_count > 0x3FFFFFFFU) || (uint64)_IMM(dest) + (uint64)word_count * 4 > 0x100000000ULL ||
			(method != IOMethod::Loop && method != IOMethod::Rupt && method != IOMethod::DMA) ||
			(method == IOMethod::DMA && word_count > 0xFFFFU)) return SPDIFRXResult::InvalidArgument;

		OperationGuard operation_guard(operation);
		if (!operation_guard) return SPDIFRXResult::Busy;
		Transfer& transfer = SelectTransfer(flow);
		{
			InterruptGuard guard;
			if (!configured) return SPDIFRXResult::Error;
			if (transfer.active || (method == IOMethod::Loop && (data.active || control.active)))
				return SPDIFRXResult::Busy;
		}
		if (!ReleaseDMA(transfer, timeout_ms)) return SPDIFRXResult::Timeout;

		{
			InterruptGuard guard;
			if (data.active || control.active) return StartTransfer(flow, dest, word_count, method, false);
		}
		Transfer& other = flow == SPDIFRXFlow::Data ? control : data;
		if (!ReleaseDMA(other, timeout_ms)) return SPDIFRXResult::Timeout;
		{
			InterruptGuard guard;
			link_failed = false;
			link_error = 0;
			transfer.error = 0;
			SetIdle();
			self[SPDIFRXReg::CR].maset(_SPDIFRX_CR_POS_ENABLE, 2, byte(SPDIFRXHardwareState::Synchronizing));
		}

		SPDIFRXResult result = WaitForSync(timeout_ms);
		if (result != SPDIFRXResult::OK) return result;
		{
			InterruptGuard guard;
			result = StartTransfer(flow, dest, word_count, method, true);
		}
		if (result != SPDIFRXResult::OK || method != IOMethod::Loop) return result;
		return Poll(flow, timeout_ms);
	}

	SPDIFRXResult SPDIFRX_t::StartTransfer(SPDIFRXFlow flow, uint32* dest, stduint word_count,
		IOMethod method, bool start_link) {
		uint32 status = self[SPDIFRXReg::SR];
		if (status & _SPDIFRX_LINK_ERRORS) {
			if (start_link) {
				link_error |= ReceiveError(status);
				link_failed = true;
				SetIdle();
			}
			return SPDIFRXResult::Error;
		}
		Transfer& transfer = SelectTransfer(flow);
		transfer.dest = dest;
		transfer.remaining = word_count;
		transfer.error = 0;
		transfer.method = method;
		if (flow == SPDIFRXFlow::Data) self[SPDIFRXReg::IFCR] = _SPDIFRX_SR_OVR;
		if (method == IOMethod::DMA && !StartDMA(flow, transfer)) {
			transfer.dest = nullptr;
			transfer.remaining = 0;
			transfer.error = (uint32)SPDIFRXError::DMA;
			if (start_link) SetIdle();
			return SPDIFRXResult::Error;
		}
		transfer.active = true;
		if (method == IOMethod::DMA) self[SPDIFRXReg::CR].setof(DMAEnablePosition(flow));
		UpdateInterruptMask();
		if (start_link) self[SPDIFRXReg::CR].maset(_SPDIFRX_CR_POS_ENABLE, 2, byte(SPDIFRXHardwareState::Receiving));
		return SPDIFRXResult::OK;
	}

	SPDIFRXResult SPDIFRX_t::Poll(SPDIFRXFlow flow, stduint timeout_ms) {
		Transfer& transfer = SelectTransfer(flow);
		uint32 ready = flow == SPDIFRXFlow::Data ? _SPDIFRX_SR_RXNE : _SPDIFRX_SR_CSRNE;
		SPDIFRXResult result = SPDIFRXResult::OK;
		uint32 error = 0;
		while (transfer.remaining) {
			uint64 begin = SysTick::getTick();
			uint32 status;
			do {
				status = self[SPDIFRXReg::SR];
				uint32 relevant = flow == SPDIFRXFlow::Data ? status : status & ~_SPDIFRX_SR_OVR;
				error |= ReceiveError(relevant);
				if (status & receive_errors) self[SPDIFRXReg::IFCR] = status & receive_errors;
				if (status & _SPDIFRX_LINK_ERRORS) {
					result = SPDIFRXResult::Error;
					break;
				}
				if (status & ready) break;
				if (Expired(begin, timeout_ms)) {
					result = SPDIFRXResult::Timeout;
					error |= (uint32)SPDIFRXError::Timeout;
					break;
				}
			} while (true);
			if (result != SPDIFRXResult::OK) break;
			*transfer.dest++ = self[ReceiveRegister(flow)];
			--transfer.remaining;
		}

		InterruptGuard guard;
		transfer.error |= error;
		link_failed = result == SPDIFRXResult::Error;
		RequestStop(flow);
		SetIdle();
		return result;
	}

	bool SPDIFRX_t::Stop(stduint timeout_ms) {
		OperationGuard operation_guard(operation);
		return operation_guard && StopTransfers(timeout_ms);
	}

	SPDIFRXState SPDIFRX_t::getState() const {
		InterruptGuard guard;
		if (!configured) return SPDIFRXState::Reset;
		if (link_failed || (!data.active && data.dma && data.error) ||
			(!control.active && control.dma && control.error)) return SPDIFRXState::Error;
		if (data.active && control.active) return SPDIFRXState::Busy;
		if (data.active) return SPDIFRXState::BusyData;
		if (control.active) return SPDIFRXState::BusyControl;
		return operation.load(MemoryOrder_Acquire) ? SPDIFRXState::Busy : SPDIFRXState::Ready;
	}

	uint32 SPDIFRX_t::getError() const {
		InterruptGuard guard;
		return link_error | data.error | control.error;
	}

	uint32 SPDIFRX_t::getError(SPDIFRXFlow flow) const {
		InterruptGuard guard;
		if (flow == SPDIFRXFlow::Data) return link_error | data.error;
		if (flow == SPDIFRXFlow::Control) return link_error | control.error;
		return (uint32)SPDIFRXError::Unknown;
	}

	void SPDIFRX_t::HandleRupt() {
		Handler_t error_handler = nullptr, data_handler = nullptr, control_handler = nullptr;
		{
			InterruptGuard guard;
			if (!configured) return;
			uint32 status = self[SPDIFRXReg::SR], mask = self[SPDIFRXReg::IMR];
			uint32 errors = status & mask & receive_errors;
			if (mask & _SPDIFRX_IMR_IFEIE) errors |= status & _SPDIFRX_LINK_ERRORS;
			if (errors) {
				link_error |= ReceiveError(errors & ~_SPDIFRX_SR_OVR);
				if (errors & _SPDIFRX_SR_OVR) data.error |= (uint32)SPDIFRXError::Overrun;
				self[SPDIFRXReg::IFCR] = errors & receive_errors;
				error_handler = ErrorHandler;
			}
			if (errors & _SPDIFRX_LINK_ERRORS) {
				link_failed = true;
				RequestStop(SPDIFRXFlow::Data);
				RequestStop(SPDIFRXFlow::Control);
				SetIdle();
			} else {
				// RM0433: only a DR or CSR read acknowledges its receive-ready flag.
				if ((status & mask & _SPDIFRX_SR_RXNE) && data.active && data.method == IOMethod::Rupt) {
					*data.dest++ = self[SPDIFRXReg::DR];
					if (--data.remaining == 0) {
						RequestStop(SPDIFRXFlow::Data);
						data_handler = DataHandler;
					}
				}
				if ((status & mask & _SPDIFRX_SR_CSRNE) && control.active && control.method == IOMethod::Rupt) {
					*control.dest++ = self[SPDIFRXReg::CSR];
					if (--control.remaining == 0) {
						RequestStop(SPDIFRXFlow::Control);
						control_handler = ControlHandler;
					}
				}
				if (data.active || control.active) UpdateInterruptMask();
				else SetIdle();
			}
		}
		if (error_handler) error_handler();
		if (data_handler) data_handler();
		if (control_handler) control_handler();
	}

	void SPDIFRX_t::HandleDMAComplete(SPDIFRXFlow flow, bool half) {
		Handler_t handler;
		{
			InterruptGuard guard;
			Transfer& transfer = SelectTransfer(flow);
			if (!transfer.active || !transfer.dma || transfer.dma->getError() != _DMA_ERROR_NONE ||
				(uint32(self[SPDIFRXReg::SR]) & _SPDIFRX_LINK_ERRORS)) return;
			if (!half && !transfer.circular) {
				transfer.remaining = 0;
				RequestStop(flow);
				if (data.active || control.active) UpdateInterruptMask();
				else SetIdle();
			}
			handler = flow == SPDIFRXFlow::Data ? (half ? DataHalfHandler : DataHandler) :
				(half ? ControlHalfHandler : ControlHandler);
		}
		if (handler) handler();
	}

	void SPDIFRX_t::HandleDMAError(SPDIFRXFlow flow) {
		Handler_t handler;
		{
			InterruptGuard guard;
			Transfer& transfer = SelectTransfer(flow);
			if (!transfer.active || !transfer.dma) return;
			transfer.error |= (uint32)SPDIFRXError::DMA;
			RequestStop(flow);
			uint32 status = self[SPDIFRXReg::SR];
			if (status & _SPDIFRX_LINK_ERRORS) {
				link_error |= ReceiveError(status & ~_SPDIFRX_SR_OVR);
				link_failed = true;
				RequestStop(SPDIFRXFlow::Data);
				RequestStop(SPDIFRXFlow::Control);
				SetIdle();
			} else if (data.active || control.active) UpdateInterruptMask();
			else SetIdle();
			handler = ErrorHandler;
		}
		if (handler) handler();
	}

	void SPDIFRX_t::DataDMAHalf() { SPDIFRX.HandleDMAComplete(SPDIFRXFlow::Data, true); }
	void SPDIFRX_t::DataDMAComplete() { SPDIFRX.HandleDMAComplete(SPDIFRXFlow::Data, false); }
	void SPDIFRX_t::DataDMAError() { SPDIFRX.HandleDMAError(SPDIFRXFlow::Data); }
	void SPDIFRX_t::ControlDMAHalf() { SPDIFRX.HandleDMAComplete(SPDIFRXFlow::Control, true); }
	void SPDIFRX_t::ControlDMAComplete() { SPDIFRX.HandleDMAComplete(SPDIFRXFlow::Control, false); }
	void SPDIFRX_t::ControlDMAError() { SPDIFRX.HandleDMAError(SPDIFRXFlow::Control); }

	void SPDIFRX_t::setInterrupt(Handler_t handler) const {
		InterruptGuard guard;
		DataHandler = handler;
	}

	void SPDIFRX_t::setInterruptPriority(byte preempt, byte sub_priority) const {
		NVIC.setPriority(IRQ_SPDIF_RX, preempt, sub_priority);
	}

	void SPDIFRX_t::enInterrupt(bool enable) const {
		NVIC.setAble(IRQ_SPDIF_RX, enable);
	}
}

extern "C" void SPDIF_RX_IRQHandler() { uni::SPDIFRX.HandleRupt(); }

namespace uni {
	SPDIFRXCapture* SPDIFRXCapture::active = nullptr;

	SPDIFRXCapture::~SPDIFRXCapture() {
		Stop();
		if (active == this) {
			receiver.DataHalfHandler = saved_half;
			receiver.DataHandler = saved_complete;
			receiver.ErrorHandler = saved_error;
			active = nullptr;
		}
	}
	void SPDIFRXCapture::HalfComplete() { if (active) active->NotifyHalf(false); }
	void SPDIFRXCapture::Complete() { if (active) active->NotifyHalf(true); }
	void SPDIFRXCapture::Failed() { if (active) active->NotifyError(); }

	AudioResult SPDIFRXCapture::BeginSource() {
		if (active || &receiver != &SPDIFRX || receiver.getState() != SPDIFRXState::Ready)
			return AudioResult::Busy;
		if (!sample_rate || sample_rate != getCaptureFormat().sample_rate) return AudioResult::InvalidFormat;
		if (!receiver.data_dma) return AudioResult::InvalidArgument;
		stduint encoded = _IMM(receiver.data_dma);
		if ((encoded >> 8) < 1 || (encoded >> 8) > 2 || (encoded & 0x8FU)) return AudioResult::InvalidArgument;
		DMA_t& dma = receiver.data_dma->getParent();
		Reference control = dma[DMAReg::CR[receiver.data_dma->getID()]];
		if (control.bitof(_DMA_SxCR_POS_EN) || dma.bind) return AudioResult::Busy;
		Reference format = receiver[SPDIFRXReg::CR];
		saved_format.packing = SPDIFRXPacking(format.masof(_SPDIFRX_CR_POS_DRFMT, 2));
		saved_format.stereo = format.bitof(_SPDIFRX_CR_POS_RXSTEO);
		saved_format.mask_parity_error = format.bitof(_SPDIFRX_CR_POS_PMSK);
		saved_format.mask_validity = format.bitof(_SPDIFRX_CR_POS_VMSK);
		saved_format.mask_channel_status = format.bitof(_SPDIFRX_CR_POS_CUMSK);
		saved_format.mask_preamble = format.bitof(_SPDIFRX_CR_POS_PTMSK);
		SPDIFRXDataFormat pcm;
		pcm.packing = SPDIFRXPacking::Packed16;
		pcm.mask_parity_error = pcm.mask_validity = pcm.mask_channel_status = pcm.mask_preamble = true;
		if (!receiver.setDataFormat(pcm)) return AudioResult::Failed;
		saved_half = receiver.DataHalfHandler;
		saved_complete = receiver.DataHandler;
		saved_error = receiver.ErrorHandler;
		receiver.DataHalfHandler = HalfComplete;
		receiver.DataHandler = Complete;
		receiver.ErrorHandler = Failed;
		saved_circular = control.bitof(_DMA_SxCR_POS_CIRC);
		control.setof(_DMA_SxCR_POS_CIRC);
		bound = true;
		active = this;
		L1C.CleanInvalidateDCacheRange(raw, stduint(half_bytes) * 2);
		receiver.enInterrupt();
		SPDIFRXResult started = receiver.Receive(SPDIFRXFlow::Data, (uint32*)raw,
			half_bytes / 2, IOMethod::DMA);
		return started == SPDIFRXResult::OK ? AudioResult::OK : AudioResult::Failed;
	}
	AudioResult SPDIFRXCapture::EndSource() {
		if (!bound) return AudioResult::OK;
		if (!receiver.Stop()) return AudioResult::Busy;
		receiver.DataHalfHandler = saved_half;
		receiver.DataHandler = saved_complete;
		receiver.ErrorHandler = saved_error;
		receiver.data_dma->getParent()[DMAReg::CR[receiver.data_dma->getID()]].
			setof(_DMA_SxCR_POS_CIRC, saved_circular);
		if (!receiver.setDataFormat(saved_format)) return AudioResult::Failed;
		active = nullptr;
		bound = false;
		return AudioResult::OK;
	}
	bool SPDIFRXCapture::SourceFailed() const {
		return receiver.getError() || receiver.data_dma->getError();
	}
	bool SPDIFRXCapture::IsHalfReadable(bool second) const {
		const DMAStream& stream = *receiver.data_dma;
		DMA_t& dma = stream.getParent();
		byte position = (stream.getID() & 2 ? 16 : 0) + (stream.getID() & 1 ? 6 : 0);
		Reference flags = dma[stream.getID() < 4 ? DMAReg::LISR : DMAReg::HISR];
		if (receiver.getError() || stream.getError() ||
			flags.bitof(position + _DMA_SxFLAG_POS_HTIF) ||
			flags.bitof(position + _DMA_SxFLAG_POS_TCIF)) return false;
		uint32 remaining = dma[DMAReg::NDTR[stream.getID()]];
		return remaining <= half_bytes / 2 && (second ? remaining > half_bytes / 4 : remaining && remaining <= half_bytes / 4);
	}
	void SPDIFRXCapture::ConvertSamples(const byte* source, byte* dest, uint32 bytes) {
		L1C.InvalidateDCacheRange((pureptr_t)source, bytes);
		// RM0433 DRFMT=10: channel A occupies bits 31:16, channel B bits 15:0.
		for (uint32 i = 0; i < bytes; i += 4) {
			dest[i] = source[i + 2];
			dest[i + 1] = source[i + 3];
			dest[i + 2] = source[i];
			dest[i + 3] = source[i + 1];
		}
	}
}

#endif

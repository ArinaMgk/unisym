// ASCII CPP-ISO11 TAB4 LF
// Docutitle: [Device] Serial audio input capture
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/cpp/Device/Mcphone/SAI-RX.hpp"
#if defined(_MCU_STM32H7x)
#include "../../../../inc/cpp/Device/Cache"
#include "../../../../inc/c/ustring.h"

namespace uni {
	SAIRXCapture::~SAIRXCapture() {
		Stop();
		if (bound) {
			// Failed stops pin staging memory, but must not retain a destroyed context.
			receiver.ReceiveBufferHandler = saved_handler;
			receiver.ReceiveContext = saved_context;
		}
	}
	void SAIRXCapture::Completed(void* context, bool second) {
		((SAIRXCapture*)context)->NotifyHalf(second);
	}
	AudioResult SAIRXCapture::BeginSource() {
		if (receiver.getState() != SAIXState::Ready ||
			!receiver.IsPCMReceive(getCaptureFormat().sample_rate)) return AudioResult::InvalidFormat;
		stduint encoded = _IMM(&stream);
		if ((encoded >> 8) < 1 || (encoded >> 8) > 2 || (encoded & 0x8FU)) return AudioResult::InvalidArgument;
		DMA_t& dma = stream.getParent();
		Reference control = dma[DMAReg::CR[stream.getID()]];
		if (dma.bind || stream.getState() != _DMA_STATE_READY ||
			control.bitof(_DMA_SxCR_POS_EN) || control.masof(_DMA_SxCR_POS_DIR, 2) ||
			control.masof(_DMA_SxCR_POS_PSIZE, 2) != 1 ||
			control.masof(_DMA_SxCR_POS_MSIZE, 2) != 1 ||
			control.bitof(_DMA_SxCR_POS_PINC) || !control.bitof(_DMA_SxCR_POS_MINC) ||
			control.bitof(_DMA_SxCR_POS_PFCTRL)) return AudioResult::InvalidArgument;
		saved_tx = receiver.getTxDMA();
		saved_rx = receiver.getRxDMA();
		if (!receiver.setDMA(nullptr, &stream)) return AudioResult::Busy;
		saved_context = receiver.ReceiveContext;
		saved_handler = receiver.ReceiveBufferHandler;
		receiver.ReceiveContext = this;
		receiver.ReceiveBufferHandler = Completed;
		saved_circular = control.bitof(_DMA_SxCR_POS_CIRC);
		control.setof(_DMA_SxCR_POS_CIRC);
		bound = true;
		L1C.CleanInvalidateDCacheRange(raw, stduint(half_bytes) * 2);
		if (receiver.Receive(raw, half_bytes, IOMethod::DMA) != half_bytes)
			return AudioResult::Failed;
		return AudioResult::OK;
	}
	AudioResult SAIRXCapture::EndSource() {
		if (!bound) return AudioResult::OK;
		if (!receiver.StopDMA()) return AudioResult::Busy;
		receiver[SAIReg::IMR] = 0;
		receiver.enAble(false);
		receiver.ReceiveContext = saved_context;
		receiver.ReceiveBufferHandler = saved_handler;
		stream.getParent()[DMAReg::CR[stream.getID()]].setof(_DMA_SxCR_POS_CIRC, saved_circular);
		if (!receiver.setDMA(saved_tx, saved_rx)) return AudioResult::Failed;
		bound = false;
		return AudioResult::OK;
	}
	bool SAIRXCapture::SourceFailed() const {
		return receiver.getError() || stream.getError();
	}
	bool SAIRXCapture::IsHalfReadable(bool second) const {
		DMA_t& dma = stream.getParent();
		byte position = (stream.getID() & 2 ? 16 : 0) + (stream.getID() & 1 ? 6 : 0);
		Reference flags = dma[stream.getID() < 4 ? DMAReg::LISR : DMAReg::HISR];
		if (receiver.getError() || stream.getError() ||
			flags.bitof(position + _DMA_SxFLAG_POS_HTIF) ||
			flags.bitof(position + _DMA_SxFLAG_POS_TCIF)) return false;
		uint32 remaining = dma[DMAReg::NDTR[stream.getID()]];
		return remaining <= half_bytes && (second ? remaining > half_bytes / 2 : remaining && remaining <= half_bytes / 2);
	}
	void SAIRXCapture::ConvertSamples(const byte* source, byte* dest, uint32 bytes) {
		L1C.InvalidateDCacheRange((pureptr_t)source, bytes);
		MemCopyN(dest, source, bytes);
	}
}
#endif

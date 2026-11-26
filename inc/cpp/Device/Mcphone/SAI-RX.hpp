// ASCII CPP-ISO11 TAB4 LF
// Docutitle: [Device] Serial audio input capture
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#ifndef _INC_DEVICE_MCPHONE_SAI_RX
#define _INC_DEVICE_MCPHONE_SAI_RX
#include "../../System/Audiosys/AudioCapture.hpp"
#if defined(_MCU_STM32H7x)
#include "../../../c/driver/SAI.h"
#include "../DMA"

namespace uni {
	class SAIRXCapture : public AudioDMACapture {
		SAI_Block& receiver;
		const DMAStream& stream;
		void* saved_context = nullptr;
		void (*saved_handler)(void*, bool) = nullptr;
		bool bound = false, saved_circular = false;
		const DMAStream* saved_tx = nullptr;
		const DMAStream* saved_rx = nullptr;
		static void Completed(void* context, bool second);
	protected:
		AudioResult BeginSource() override;
		AudioResult EndSource() override;
		bool IsHalfReadable(bool second) const override;
		bool SourceFailed() const override;
		void ConvertSamples(const byte* source, byte* dest, uint32 bytes) override;
	public:
		// The allocator must return DMA1/2-accessible memory. Board code owns SAI clocks and pins.
		SAIRXCapture(SAI_Block& receiver, const DMAStream& stream, trait::Malloc& allocator)
			: AudioDMACapture(allocator), receiver(receiver), stream(stream) {}
		~SAIRXCapture() override;
	};
}
#endif
#endif

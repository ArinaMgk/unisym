// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Audio Common
// Codifiers: @dosconio, @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#ifndef _INCPP_Device_Audio
#define _INCPP_Device_Audio

#include "../../c/stdinc.h"

namespace uni {

	enum class AudioSampleFormat : uint8 {
		U8,
		S16LE,
	};

	struct AudioFormat {
		AudioSampleFormat sample_format = AudioSampleFormat::U8;
		uint16 channels = 1;
		uint32 sample_rate = 0;
	};

	struct AudioBufferView {
		const void* data = nullptr;
		uint32 byte_count = 0;
	};

	struct AudioPlayRequest {
		AudioFormat format = {};
		AudioBufferView buffer = {};
	};

}

// Inn

// Out


#endif

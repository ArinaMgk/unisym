// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device.Audio) Sound Blaster
// Codifiers: @dosconio
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#ifndef _INCPP_Device_Audio_SoundBlaster
#define _INCPP_Device_Audio_SoundBlaster

#include "../../../c/stdinc.h"

namespace uni {

	struct SoundBlasterIo {
		void* context;
		uint8 (*read8)(void* context, uint16 port);
		void (*write8)(void* context, uint16 port, uint8 value);
	};

	enum class SoundBlasterState : uint8 {
		Absent,
		Ready,
		Playing,
		Stopping,
		Failed,
	};

	class SoundBlaster {
		uint16 io_base;
		SoundBlasterIo io;
		SoundBlasterState state;

	public:
		SoundBlaster(uint16 io_base, const SoundBlasterIo& io);

		uint16 GetIoBase() const;
		SoundBlasterState GetState() const;
	};

}

#endif

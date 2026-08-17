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
		void (*delay_us)(void* context, uint32 microseconds);
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
		uint8 dsp_major_version;
		uint8 dsp_minor_version;

		bool WaitWriteReady() const;
		bool WaitReadReady() const;
		bool WriteDsp(uint8 value);
		bool ReadDsp(uint8& value);

	public:
		SoundBlaster(uint16 io_base, const SoundBlasterIo& io);

		bool Reset();
		bool Probe();
		bool ReadVersion(uint8& major, uint8& minor);
		bool Trigger8BitIrq();
		bool SpeakerOn();
		bool SpeakerOff();
		bool SetOutputRate(uint16 sample_rate);
		bool StartSingleCycle8(uint32 byte_count, bool is_signed, bool stereo);
		bool StartAutoInit8(uint32 block_bytes, bool is_signed, bool stereo);
		bool Halt8();
		bool Continue8();
		bool ExitAutoInit8();
		void Acknowledge8BitIrq();
		void Complete8BitPlayback();

		uint16 GetIoBase() const;
		SoundBlasterState GetState() const;
		uint8 GetDspMajorVersion() const;
		uint8 GetDspMinorVersion() const;
	};

}

#endif

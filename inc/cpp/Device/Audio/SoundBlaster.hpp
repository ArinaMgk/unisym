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
		Paused,
		Stopping,
		Failed,
	};

	enum class SoundBlasterMixerChannel : uint8 {
		MasterVolume,
		VoiceVolume,
		MidiVolume,
		CdVolume,
		LineInVolume,
		MicVolume,
	};

	enum class SoundBlasterPortOffset : uint16 {
		MixerAddress   = 0x04,
		MixerData      = 0x05,
		DspReset       = 0x06,
		DspReadData    = 0x0A,
		DspWriteData   = 0x0C,
		DspReadStatus  = 0x0E,
		Dsp16BitIrqAck = 0x0F,
	};

	enum class SoundBlasterDspCommand : uint8 {
		StartSingleCycle8Legacy = 0x14,
		SetTimeConstant         = 0x40,
		SetOutputRate           = 0x41,
		StartSingleCycle16      = 0xB0,
		StartAutoInit16         = 0xB4,
		StartSingleCycle8       = 0xC0,
		StartAutoInit8          = 0xC4,
		Halt8                   = 0xD0,
		SpeakerOn               = 0xD1,
		SpeakerOff              = 0xD3,
		Continue8               = 0xD4,
		Halt16                  = 0xD5,
		Continue16              = 0xD6,
		ExitAutoInit16          = 0xD9,
		ExitAutoInit8           = 0xDA,
		GetVersion              = 0xE1,
		Trigger8BitIrq          = 0xF2,
	};

	enum class SoundBlasterDspMode : uint8 {
		None   = 0x00,
		Signed = 0x10,
		Stereo = 0x20,
	};

	enum class SoundBlasterMixerRegisterSB16 : uint8 {
		Reset       = 0x00,
		MasterLeft  = 0x30,
		MasterRight = 0x31,
		VoiceLeft   = 0x32,
		VoiceRight  = 0x33,
		MidiLeft    = 0x34,
		MidiRight   = 0x35,
		CdLeft      = 0x36,
		CdRight     = 0x37,
		LineLeft    = 0x38,
		LineRight   = 0x39,
		Mic         = 0x3A,
	};

	enum class SoundBlasterMixerRegisterSBPro : uint8 {
		Voice  = 0x04,
		Mic    = 0x0A,
		Master = 0x22,
		Midi   = 0x26,
		Cd     = 0x28,
		Line   = 0x2E,
	};

	class SoundBlaster {
		uint16 io_base;
		SoundBlasterIo io;
		SoundBlasterState state;
		bool is_16bit;
		uint8 dsp_major_version;
		uint8 dsp_minor_version;

		bool WaitWriteReady() const;
		bool WaitReadReady() const;
		bool WriteDsp(uint8 value);
		bool WriteDsp(SoundBlasterDspCommand cmd) {
			return WriteDsp(uint8(cmd));
		}
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
		bool SetTimeConstant(uint16 sample_rate);
		bool StartSingleCycle8(uint32 byte_count, bool is_signed, bool stereo);
		bool StartSingleCycle8Legacy(uint32 byte_count);
		bool StartAutoInit8(uint32 block_bytes, bool is_signed, bool stereo);
		bool StartSingleCycle16(uint32 sample_count, bool is_signed, bool stereo);
		bool StartAutoInit16(uint32 sample_count, bool is_signed, bool stereo);
		bool Halt();
		bool Continue();
		bool ExitAutoInit();
		void Acknowledge8BitIrq();
		void Acknowledge16BitIrq();
		void Complete8BitPlayback();
		void Complete16BitPlayback();

		// Mixer Control
		bool WriteMixer(uint8 reg, uint8 value);
		bool WriteMixer(SoundBlasterMixerRegisterSB16 reg, uint8 value) {
			return WriteMixer(uint8(reg), value);
		}
		bool WriteMixer(SoundBlasterMixerRegisterSBPro reg, uint8 value) {
			return WriteMixer(uint8(reg), value);
		}
		bool ReadMixer(uint8 reg, uint8& value);
		bool ReadMixer(SoundBlasterMixerRegisterSB16 reg, uint8& value) {
			return ReadMixer(uint8(reg), value);
		}
		bool ReadMixer(SoundBlasterMixerRegisterSBPro reg, uint8& value) {
			return ReadMixer(uint8(reg), value);
		}
		bool ResetMixer();
		bool SetVolume(SoundBlasterMixerChannel channel, uint8 left, uint8 right);
		bool GetVolume(SoundBlasterMixerChannel channel, uint8& left, uint8& right);
		bool SetMute(SoundBlasterMixerChannel channel, bool mute);

		uint16 GetIoBase() const;
		SoundBlasterState GetState() const;
		uint8 GetDspMajorVersion() const;
		uint8 GetDspMinorVersion() const;
	};

}

#endif

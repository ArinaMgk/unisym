// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device.Audio) Sound Blaster
// Codifiers: @dosconio
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/cpp/Device/Audio/SoundBlaster.hpp"

namespace {
	constexpr uint8 DspReadyMask = 0x80;
	constexpr uint8 DspResetReply = 0xAA;
	constexpr uint16 DspMinimumOutputRate = 5000;
	constexpr uint16 DspMaximumOutputRate = 45000;
	constexpr uint32 DspPollLimit = 0x10000;
	constexpr uint32 DspResetPulseMicroseconds = 3;
}

uni::SoundBlaster::SoundBlaster(uint16 io_base, const SoundBlasterIo& io) :
	io_base(io_base), io(io), state(SoundBlasterState::Absent),
	is_16bit(false), dsp_major_version(0), dsp_minor_version(0) {}

bool uni::SoundBlaster::WaitWriteReady() const {
	if (!io.read8) return false;
	for (uint32 count = 0; count < DspPollLimit; ++count) {
		if (!(io.read8(io.context, io_base + uint16(SoundBlasterPortOffset::DspWriteData)) & DspReadyMask)) {
			return true;
		}
		if (io.delay_us) io.delay_us(io.context, 1);
	}
	return false;
}

bool uni::SoundBlaster::WaitReadReady() const {
	if (!io.read8) return false;
	for (uint32 count = 0; count < DspPollLimit; ++count) {
		if (io.read8(io.context, io_base + uint16(SoundBlasterPortOffset::DspReadStatus)) & DspReadyMask) {
			return true;
		}
		if (io.delay_us) io.delay_us(io.context, 1);
	}
	return false;
}

bool uni::SoundBlaster::WriteDsp(uint8 value) {
	if (!io.write8 || !WaitWriteReady()) return false;
	io.write8(io.context, io_base + uint16(SoundBlasterPortOffset::DspWriteData), value);
	return true;
}

bool uni::SoundBlaster::ReadDsp(uint8& value) {
	if (!io.read8 || !WaitReadReady()) return false;
	value = io.read8(io.context, io_base + uint16(SoundBlasterPortOffset::DspReadData));
	return true;
}

bool uni::SoundBlaster::Reset() {
	if (!io.read8 || !io.write8 || !io.delay_us) {
		state = SoundBlasterState::Failed;
		return false;
	}

	io.write8(io.context, io_base + uint16(SoundBlasterPortOffset::DspReset), 1);
	io.delay_us(io.context, DspResetPulseMicroseconds);
	io.write8(io.context, io_base + uint16(SoundBlasterPortOffset::DspReset), 0);

	uint8 reply;
	if (!ReadDsp(reply) || reply != DspResetReply) {
		state = SoundBlasterState::Failed;
		return false;
	}
	state = SoundBlasterState::Ready;
	is_16bit = false;
	return true;
}

bool uni::SoundBlaster::Probe() {
	dsp_major_version = 0;
	dsp_minor_version = 0;
	if (!Reset()) {
		state = SoundBlasterState::Absent;
		return false;
	}
	if (!ReadVersion(dsp_major_version, dsp_minor_version)) {
		state = SoundBlasterState::Failed;
		return false;
	}
	state = SoundBlasterState::Ready;
	return true;
}

bool uni::SoundBlaster::ReadVersion(uint8& major, uint8& minor) {
	if (!WriteDsp(SoundBlasterDspCommand::GetVersion) || !ReadDsp(major) || !ReadDsp(minor)) {
		state = SoundBlasterState::Failed;
		return false;
	}
	dsp_major_version = major;
	dsp_minor_version = minor;
	return true;
}

bool uni::SoundBlaster::Trigger8BitIrq() {
	return WriteDsp(SoundBlasterDspCommand::Trigger8BitIrq);
}

bool uni::SoundBlaster::SpeakerOn() {
	if (state != SoundBlasterState::Ready || !WriteDsp(SoundBlasterDspCommand::SpeakerOn)) {
		state = SoundBlasterState::Failed;
		return false;
	}
	return true;
}

bool uni::SoundBlaster::SpeakerOff() {
	if (!WriteDsp(SoundBlasterDspCommand::SpeakerOff)) {
		state = SoundBlasterState::Failed;
		return false;
	}
	return true;
}

bool uni::SoundBlaster::SetOutputRate(uint16 sample_rate) {
	if (state != SoundBlasterState::Ready ||
		sample_rate < DspMinimumOutputRate || sample_rate > DspMaximumOutputRate ||
		!WriteDsp(SoundBlasterDspCommand::SetOutputRate) ||
		!WriteDsp(uint8(sample_rate >> 8)) || !WriteDsp(uint8(sample_rate))) {
		state = SoundBlasterState::Failed;
		return false;
	}
	return true;
}

bool uni::SoundBlaster::SetTimeConstant(uint16 sample_rate) {
	if (state != SoundBlasterState::Ready ||
		sample_rate < DspMinimumOutputRate || sample_rate > DspMaximumOutputRate) {
		state = SoundBlasterState::Failed;
		return false;
	}
	const uint8 time_constant = uint8(256 - (1000000UL / sample_rate));
	if (!WriteDsp(SoundBlasterDspCommand::SetTimeConstant) || !WriteDsp(time_constant)) {
		state = SoundBlasterState::Failed;
		return false;
	}
	return true;
}

bool uni::SoundBlaster::StartSingleCycle8(uint32 byte_count, bool is_signed, bool stereo) {
	if (state != SoundBlasterState::Ready || !byte_count || byte_count > 0x10000) {
		state = SoundBlasterState::Failed;
		return false;
	}
	const uint8 mode = uint8(
		(is_signed ? uint8(SoundBlasterDspMode::Signed) : 0) |
		(stereo ? uint8(SoundBlasterDspMode::Stereo) : 0));
	const uint16 count = uint16(byte_count - 1);
	if (!WriteDsp(SoundBlasterDspCommand::StartSingleCycle8) || !WriteDsp(mode) ||
		!WriteDsp(uint8(count)) || !WriteDsp(uint8(count >> 8))) {
		state = SoundBlasterState::Failed;
		return false;
	}
	is_16bit = false;
	state = SoundBlasterState::Playing;
	return true;
}

bool uni::SoundBlaster::StartSingleCycle8Legacy(uint32 byte_count) {
	if (state != SoundBlasterState::Ready || !byte_count || byte_count > 0x10000) {
		state = SoundBlasterState::Failed;
		return false;
	}
	const uint16 count = uint16(byte_count - 1);
	if (!WriteDsp(SoundBlasterDspCommand::StartSingleCycle8Legacy) ||
		!WriteDsp(uint8(count)) || !WriteDsp(uint8(count >> 8))) {
		state = SoundBlasterState::Failed;
		return false;
	}
	is_16bit = false;
	state = SoundBlasterState::Playing;
	return true;
}

bool uni::SoundBlaster::StartAutoInit8(uint32 block_bytes, bool is_signed, bool stereo) {
	if (state != SoundBlasterState::Ready || !block_bytes || block_bytes > 0x10000) {
		state = SoundBlasterState::Failed;
		return false;
	}
	const uint8 mode = uint8(
		(is_signed ? uint8(SoundBlasterDspMode::Signed) : 0) |
		(stereo ? uint8(SoundBlasterDspMode::Stereo) : 0));
	const uint16 count = uint16(block_bytes - 1);
	if (!WriteDsp(SoundBlasterDspCommand::StartAutoInit8) || !WriteDsp(mode) ||
		!WriteDsp(uint8(count)) || !WriteDsp(uint8(count >> 8))) {
		state = SoundBlasterState::Failed;
		return false;
	}
	is_16bit = false;
	state = SoundBlasterState::Playing;
	return true;
}

bool uni::SoundBlaster::StartSingleCycle16(uint32 sample_count, bool is_signed, bool stereo) {
	if (state != SoundBlasterState::Ready || !sample_count || sample_count > 0x10000) {
		state = SoundBlasterState::Failed;
		return false;
	}
	const uint8 mode = uint8(
		(is_signed ? uint8(SoundBlasterDspMode::Signed) : 0) |
		(stereo ? uint8(SoundBlasterDspMode::Stereo) : 0));
	const uint16 count = uint16(sample_count - 1);
	if (!WriteDsp(SoundBlasterDspCommand::StartSingleCycle16) || !WriteDsp(mode) ||
		!WriteDsp(uint8(count)) || !WriteDsp(uint8(count >> 8))) {
		state = SoundBlasterState::Failed;
		return false;
	}
	is_16bit = true;
	state = SoundBlasterState::Playing;
	return true;
}

bool uni::SoundBlaster::StartAutoInit16(uint32 sample_count, bool is_signed, bool stereo) {
	if (state != SoundBlasterState::Ready || !sample_count || sample_count > 0x10000) {
		state = SoundBlasterState::Failed;
		return false;
	}
	const uint8 mode = uint8(
		(is_signed ? uint8(SoundBlasterDspMode::Signed) : 0) |
		(stereo ? uint8(SoundBlasterDspMode::Stereo) : 0));
	const uint16 count = uint16(sample_count - 1);
	if (!WriteDsp(SoundBlasterDspCommand::StartAutoInit16) || !WriteDsp(mode) ||
		!WriteDsp(uint8(count)) || !WriteDsp(uint8(count >> 8))) {
		state = SoundBlasterState::Failed;
		return false;
	}
	is_16bit = true;
	state = SoundBlasterState::Playing;
	return true;
}

bool uni::SoundBlaster::Halt() {
	if (state != SoundBlasterState::Playing && state != SoundBlasterState::Ready) {
		return false;
	}
	const auto cmd = is_16bit ? SoundBlasterDspCommand::Halt16 : SoundBlasterDspCommand::Halt8;
	if (!WriteDsp(cmd)) {
		state = SoundBlasterState::Failed;
		return false;
	}
	state = SoundBlasterState::Paused;
	return true;
}

bool uni::SoundBlaster::Continue() {
	if (state != SoundBlasterState::Paused && state != SoundBlasterState::Ready) {
		return false;
	}
	const auto cmd = is_16bit ? SoundBlasterDspCommand::Continue16 : SoundBlasterDspCommand::Continue8;
	if (!WriteDsp(cmd)) {
		state = SoundBlasterState::Failed;
		return false;
	}
	state = SoundBlasterState::Playing;
	return true;
}

bool uni::SoundBlaster::ExitAutoInit() {
	if (state != SoundBlasterState::Playing && state != SoundBlasterState::Paused) {
		return false;
	}
	const auto cmd = is_16bit ? SoundBlasterDspCommand::ExitAutoInit16 : SoundBlasterDspCommand::ExitAutoInit8;
	if (!WriteDsp(cmd)) {
		state = SoundBlasterState::Failed;
		return false;
	}
	state = SoundBlasterState::Stopping;
	return true;
}

void uni::SoundBlaster::Acknowledge8BitIrq() {
	if (io.read8) {
		(void)io.read8(io.context, io_base + uint16(SoundBlasterPortOffset::DspReadStatus));
	}
}

void uni::SoundBlaster::Acknowledge16BitIrq() {
	if (io.read8) {
		(void)io.read8(io.context, io_base + uint16(SoundBlasterPortOffset::Dsp16BitIrqAck));
	}
}

void uni::SoundBlaster::Complete8BitPlayback() {
	if (state == SoundBlasterState::Playing ||
		state == SoundBlasterState::Stopping) {
		state = SoundBlasterState::Ready;
	}
}

void uni::SoundBlaster::Complete16BitPlayback() {
	if (state == SoundBlasterState::Playing ||
		state == SoundBlasterState::Stopping) {
		state = SoundBlasterState::Ready;
	}
}

uint16 uni::SoundBlaster::GetIoBase() const {
	return io_base;
}

uni::SoundBlasterState uni::SoundBlaster::GetState() const {
	return state;
}

uint8 uni::SoundBlaster::GetDspMajorVersion() const {
	return dsp_major_version;
}

uint8 uni::SoundBlaster::GetDspMinorVersion() const {
	return dsp_minor_version;
}

bool uni::SoundBlaster::WriteMixer(uint8 reg, uint8 value) {
	if (!io.write8) return false;
	io.write8(io.context, io_base + uint16(SoundBlasterPortOffset::MixerAddress), reg);
	if (io.delay_us) io.delay_us(io.context, 1);
	io.write8(io.context, io_base + uint16(SoundBlasterPortOffset::MixerData), value);
	return true;
}

bool uni::SoundBlaster::ReadMixer(uint8 reg, uint8& value) {
	if (!io.write8 || !io.read8) return false;
	io.write8(io.context, io_base + uint16(SoundBlasterPortOffset::MixerAddress), reg);
	if (io.delay_us) io.delay_us(io.context, 1);
	value = io.read8(io.context, io_base + uint16(SoundBlasterPortOffset::MixerData));
	return true;
}

bool uni::SoundBlaster::ResetMixer() {
	return WriteMixer(SoundBlasterMixerRegisterSB16::Reset, 0x00);
}

bool uni::SoundBlaster::SetVolume(SoundBlasterMixerChannel channel, uint8 left, uint8 right) {
	const bool is_sb16 = (dsp_major_version >= 4);

	// SBPro uses 4 bits (0..15) per channel: bits 7..4 Left, bits 3..0 Right
	const uint8 l4 = (uint8)(((uint32)left * 15) / 255);
	const uint8 r4 = (uint8)(((uint32)right * 15) / 255);
	const uint8 combined_pro = (l4 << 4) | (r4 & 0x0F);

	if (is_sb16) {
		// SB16 uses 5 bits (0..31), high 5 bits of the register (val << 3)
		const uint8 val_l = (uint8)(((uint32)left * 31) / 255) << 3;
		const uint8 val_r = (uint8)(((uint32)right * 31) / 255) << 3;

		switch (channel) {
		case SoundBlasterMixerChannel::MasterVolume:
			WriteMixer(SoundBlasterMixerRegisterSBPro::Master, combined_pro);
			WriteMixer(SoundBlasterMixerRegisterSBPro::Voice, combined_pro);
			WriteMixer(SoundBlasterMixerRegisterSB16::VoiceLeft, val_l);
			WriteMixer(SoundBlasterMixerRegisterSB16::VoiceRight, val_r);
			return WriteMixer(SoundBlasterMixerRegisterSB16::MasterLeft, val_l) &&
			       WriteMixer(SoundBlasterMixerRegisterSB16::MasterRight, val_r);
		case SoundBlasterMixerChannel::VoiceVolume:
			WriteMixer(SoundBlasterMixerRegisterSBPro::Voice, combined_pro);
			return WriteMixer(SoundBlasterMixerRegisterSB16::VoiceLeft, val_l) &&
			       WriteMixer(SoundBlasterMixerRegisterSB16::VoiceRight, val_r);
		case SoundBlasterMixerChannel::MidiVolume:
			WriteMixer(SoundBlasterMixerRegisterSBPro::Midi, combined_pro);
			return WriteMixer(SoundBlasterMixerRegisterSB16::MidiLeft, val_l) &&
			       WriteMixer(SoundBlasterMixerRegisterSB16::MidiRight, val_r);
		case SoundBlasterMixerChannel::CdVolume:
			WriteMixer(SoundBlasterMixerRegisterSBPro::Cd, combined_pro);
			return WriteMixer(SoundBlasterMixerRegisterSB16::CdLeft, val_l) &&
			       WriteMixer(SoundBlasterMixerRegisterSB16::CdRight, val_r);
		case SoundBlasterMixerChannel::LineInVolume:
			WriteMixer(SoundBlasterMixerRegisterSBPro::Line, combined_pro);
			return WriteMixer(SoundBlasterMixerRegisterSB16::LineLeft, val_l) &&
			       WriteMixer(SoundBlasterMixerRegisterSB16::LineRight, val_r);
		case SoundBlasterMixerChannel::MicVolume:
			WriteMixer(SoundBlasterMixerRegisterSBPro::Mic, (uint8)(((uint32)left * 7) / 255));
			return WriteMixer(SoundBlasterMixerRegisterSB16::Mic, val_l);
		default:
			return false;
		}
	} else {
		switch (channel) {
		case SoundBlasterMixerChannel::MasterVolume:
			WriteMixer(SoundBlasterMixerRegisterSBPro::Voice, combined_pro);
			return WriteMixer(SoundBlasterMixerRegisterSBPro::Master, combined_pro);
		case SoundBlasterMixerChannel::VoiceVolume:
			return WriteMixer(SoundBlasterMixerRegisterSBPro::Voice, combined_pro);
		case SoundBlasterMixerChannel::MidiVolume:
			return WriteMixer(SoundBlasterMixerRegisterSBPro::Midi, combined_pro);
		case SoundBlasterMixerChannel::CdVolume:
			return WriteMixer(SoundBlasterMixerRegisterSBPro::Cd, combined_pro);
		case SoundBlasterMixerChannel::LineInVolume:
			return WriteMixer(SoundBlasterMixerRegisterSBPro::Line, combined_pro);
		case SoundBlasterMixerChannel::MicVolume:
			return WriteMixer(SoundBlasterMixerRegisterSBPro::Mic, (uint8)(((uint32)left * 7) / 255));
		default:
			return false;
		}
	}
}

bool uni::SoundBlaster::GetVolume(SoundBlasterMixerChannel channel, uint8& left, uint8& right) {
	const bool is_sb16 = (dsp_major_version >= 4);

	if (is_sb16) {
		uint8 reg_l = 0, reg_r = 0;
		bool ok = false;
		switch (channel) {
		case SoundBlasterMixerChannel::MasterVolume:
			ok = ReadMixer(SoundBlasterMixerRegisterSB16::MasterLeft, reg_l) &&
			     ReadMixer(SoundBlasterMixerRegisterSB16::MasterRight, reg_r);
			break;
		case SoundBlasterMixerChannel::VoiceVolume:
			ok = ReadMixer(SoundBlasterMixerRegisterSB16::VoiceLeft, reg_l) &&
			     ReadMixer(SoundBlasterMixerRegisterSB16::VoiceRight, reg_r);
			break;
		case SoundBlasterMixerChannel::MidiVolume:
			ok = ReadMixer(SoundBlasterMixerRegisterSB16::MidiLeft, reg_l) &&
			     ReadMixer(SoundBlasterMixerRegisterSB16::MidiRight, reg_r);
			break;
		case SoundBlasterMixerChannel::CdVolume:
			ok = ReadMixer(SoundBlasterMixerRegisterSB16::CdLeft, reg_l) &&
			     ReadMixer(SoundBlasterMixerRegisterSB16::CdRight, reg_r);
			break;
		case SoundBlasterMixerChannel::LineInVolume:
			ok = ReadMixer(SoundBlasterMixerRegisterSB16::LineLeft, reg_l) &&
			     ReadMixer(SoundBlasterMixerRegisterSB16::LineRight, reg_r);
			break;
		case SoundBlasterMixerChannel::MicVolume:
			ok = ReadMixer(SoundBlasterMixerRegisterSB16::Mic, reg_l);
			reg_r = reg_l;
			break;
		default:
			return false;
		}
		if (!ok) return false;
		left = (uint8)((((uint32)(reg_l >> 3)) * 255) / 31);
		right = (uint8)((((uint32)(reg_r >> 3)) * 255) / 31);
		return true;
	} else {
		uint8 reg = 0;
		bool ok = false;
		switch (channel) {
		case SoundBlasterMixerChannel::MasterVolume:
			ok = ReadMixer(SoundBlasterMixerRegisterSBPro::Master, reg);
			break;
		case SoundBlasterMixerChannel::VoiceVolume:
			ok = ReadMixer(SoundBlasterMixerRegisterSBPro::Voice, reg);
			break;
		case SoundBlasterMixerChannel::MidiVolume:
			ok = ReadMixer(SoundBlasterMixerRegisterSBPro::Midi, reg);
			break;
		case SoundBlasterMixerChannel::CdVolume:
			ok = ReadMixer(SoundBlasterMixerRegisterSBPro::Cd, reg);
			break;
		case SoundBlasterMixerChannel::LineInVolume:
			ok = ReadMixer(SoundBlasterMixerRegisterSBPro::Line, reg);
			break;
		case SoundBlasterMixerChannel::MicVolume:
			ok = ReadMixer(SoundBlasterMixerRegisterSBPro::Mic, reg);
			left = right = (uint8)((((uint32)(reg & 0x07)) * 255) / 7);
			return ok;
		default:
			return false;
		}
		if (!ok) return false;
		left = (uint8)((((uint32)(reg >> 4)) * 255) / 15);
		right = (uint8)((((uint32)(reg & 0x0F)) * 255) / 15);
		return true;
	}
}

bool uni::SoundBlaster::SetMute(SoundBlasterMixerChannel channel, bool mute) {
	if (mute) {
		return SetVolume(channel, 0, 0);
	}
	return SetVolume(channel, 204, 204);
}


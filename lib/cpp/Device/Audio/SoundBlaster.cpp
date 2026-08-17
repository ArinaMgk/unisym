// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device.Audio) Sound Blaster
// Codifiers: @dosconio
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/cpp/Device/Audio/SoundBlaster.hpp"

namespace {
	constexpr uint16 DspResetOffset = 0x06;
	constexpr uint16 DspReadDataOffset = 0x0A;
	constexpr uint16 DspWriteDataOffset = 0x0C;
	constexpr uint16 DspReadStatusOffset = 0x0E;
	constexpr uint8 DspReadyMask = 0x80;
	constexpr uint8 DspResetReply = 0xAA;
	constexpr uint8 DspGetVersionCommand = 0xE1;
	constexpr uint8 DspTrigger8BitIrqCommand = 0xF2;
	constexpr uint32 DspPollLimit = 0x10000;
	constexpr uint32 DspResetPulseMicroseconds = 3;
}

uni::SoundBlaster::SoundBlaster(uint16 io_base, const SoundBlasterIo& io) :
	io_base(io_base), io(io), state(SoundBlasterState::Absent),
	dsp_major_version(0), dsp_minor_version(0) {}

bool uni::SoundBlaster::WaitWriteReady() const {
	if (!io.read8) return false;
	for (uint32 count = 0; count < DspPollLimit; ++count) {
		if (!(io.read8(io.context, io_base + DspWriteDataOffset) & DspReadyMask)) {
			return true;
		}
	}
	return false;
}

bool uni::SoundBlaster::WaitReadReady() const {
	if (!io.read8) return false;
	for (uint32 count = 0; count < DspPollLimit; ++count) {
		if (io.read8(io.context, io_base + DspReadStatusOffset) & DspReadyMask) {
			return true;
		}
	}
	return false;
}

bool uni::SoundBlaster::WriteDsp(uint8 value) {
	if (!io.write8 || !WaitWriteReady()) return false;
	io.write8(io.context, io_base + DspWriteDataOffset, value);
	return true;
}

bool uni::SoundBlaster::ReadDsp(uint8& value) {
	if (!io.read8 || !WaitReadReady()) return false;
	value = io.read8(io.context, io_base + DspReadDataOffset);
	return true;
}

bool uni::SoundBlaster::Reset() {
	if (!io.read8 || !io.write8 || !io.delay_us) {
		state = SoundBlasterState::Failed;
		return false;
	}

	io.write8(io.context, io_base + DspResetOffset, 1);
	io.delay_us(io.context, DspResetPulseMicroseconds);
	io.write8(io.context, io_base + DspResetOffset, 0);

	uint8 reply;
	if (!ReadDsp(reply) || reply != DspResetReply) {
		state = SoundBlasterState::Failed;
		return false;
	}
	state = SoundBlasterState::Ready;
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
	if (!WriteDsp(DspGetVersionCommand) || !ReadDsp(major) || !ReadDsp(minor)) {
		state = SoundBlasterState::Failed;
		return false;
	}
	dsp_major_version = major;
	dsp_minor_version = minor;
	return true;
}

bool uni::SoundBlaster::Trigger8BitIrq() {
	return WriteDsp(DspTrigger8BitIrqCommand);
}

void uni::SoundBlaster::Acknowledge8BitIrq() {
	if (io.read8) {
		(void)io.read8(io.context, io_base + DspReadStatusOffset);
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

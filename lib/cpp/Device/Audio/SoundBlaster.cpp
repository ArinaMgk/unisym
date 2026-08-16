// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device.Audio) Sound Blaster
// Codifiers: @dosconio
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/cpp/Device/Audio/SoundBlaster.hpp"

uni::SoundBlaster::SoundBlaster(uint16 io_base, const SoundBlasterIo& io) :
	io_base(io_base), io(io), state(SoundBlasterState::Absent) {}

uint16 uni::SoundBlaster::GetIoBase() const {
	return io_base;
}

uni::SoundBlasterState uni::SoundBlaster::GetState() const {
	return state;
}

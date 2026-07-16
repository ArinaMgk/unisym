// ASCII CPP TAB4 CRLF
// Docutitle: (Device) VMware SVGA-II Adapter
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
#include "../../../../inc/cpp/Device/Video/VMware-SVGA.hpp"

#if (_MCCA & 0xFF00) == 0x8600
#include "../../../../inc/c/proctrl/IAx86_64.h"

namespace uni {

VmwareSvgaDevice::VmwareSvgaDevice() {
}

void VmwareSvgaDevice::SetIoBase(uint16_t io_base) {
	io_base_ = io_base;
}

void VmwareSvgaDevice::SetFramebufferBase(uint64_t framebuffer_base) {
	framebuffer_base_ = framebuffer_base;
}

void VmwareSvgaDevice::SetFifoBase(uint64_t fifo_base) {
	fifo_base_ = fifo_base;
}

uint32_t VmwareSvgaDevice::ReadRegister(RegisterIndex index) const {
	if (!io_base_) return 0;
	outpd(io_base_ + IOPORT_INDEX, index);
	return innpd(io_base_ + IOPORT_VALUE);
}

void VmwareSvgaDevice::WriteRegister(RegisterIndex index, uint32_t value) const {
	if (!io_base_) return;
	outpd(io_base_ + IOPORT_INDEX, index);
	outpd(io_base_ + IOPORT_VALUE, value);
}

bool VmwareSvgaDevice::NegotiateVersion() {
	static const uint32_t kIds[] = { SVGA_ID_2, SVGA_ID_1, SVGA_ID_0 };
	for0(i, numsof(kIds)) {
		WriteRegister(REG_ID, kIds[i]);
		if (ReadRegister(REG_ID) == kIds[i]) {
			return true;
		}
	}
	return false;
}

bool VmwareSvgaDevice::Initialize() {
	if (!io_base_) return false;
	if (!NegotiateVersion()) return false;
	WriteRegister(REG_ENABLE, 1);
	WriteRegister(REG_CONFIG_DONE, 1);
	return true;
}

bool VmwareSvgaDevice::ReadMode(uni::Size2& resolution, uint32_t& bpp, uint32_t& pitch) const {
	resolution.x = ReadRegister(REG_WIDTH);
	resolution.y = ReadRegister(REG_HEIGHT);
	bpp = ReadRegister(REG_BITS_PER_PIXEL);
	pitch = ReadRegister(REG_BYTES_PER_LINE);
	return resolution.x != 0 && resolution.y != 0 && bpp != 0 && pitch != 0;
}

bool VmwareSvgaDevice::setMode(uint32_t width, uint32_t height, uint32_t bpp) {
	if (!io_base_) return false;
	WriteRegister(REG_ENABLE, 0);
	WriteRegister(REG_WIDTH, width);
	WriteRegister(REG_HEIGHT, height);
	WriteRegister(REG_BITS_PER_PIXEL, bpp);
	WriteRegister(REG_ENABLE, 1);
	WriteRegister(REG_CONFIG_DONE, 1);
	return ReadRegister(REG_WIDTH) == width &&
		ReadRegister(REG_HEIGHT) == height &&
		ReadRegister(REG_BITS_PER_PIXEL) == bpp;
}

volatile uint32_t* VmwareSvgaDevice::GetFifoPtr() const {
	if (!fifo_base_) return nullptr;
	return reinterpret_cast<volatile uint32_t*>(fifo_base_);
}

void VmwareSvgaDevice::Sync() const {
	WriteRegister(REG_SYNC, 1);
	while (ReadRegister(REG_BUSY));
}

bool VmwareSvgaDevice::InitializeFifo() {
	volatile uint32_t* fifo = GetFifoPtr();
	uint32_t fifo_size = GetFifoSize();
	if (!fifo || fifo_size < 16) return false;

	const uint32_t min = 4 * sizeof(uint32_t);
	fifo[FIFO_MIN] = min;
	fifo[FIFO_MAX] = fifo_size;
	fifo[FIFO_NEXT_CMD] = min;
	fifo[FIFO_STOP] = min;
	return true;
}

void VmwareSvgaDevice::Flush(const Rectangle& rect) {
	volatile uint32_t* fifo = GetFifoPtr();
	if (!fifo) return;

	const uint32_t min = fifo[FIFO_MIN];
	const uint32_t max = fifo[FIFO_MAX];
	uint32_t next = fifo[FIFO_NEXT_CMD];
	uint32_t stop = fifo[FIFO_STOP];
	const uint32_t cmd_bytes = 5 * sizeof(uint32_t);

	if (next + cmd_bytes > max) {
		if (next != stop) {
			Sync();
			stop = fifo[FIFO_STOP];
			next = fifo[FIFO_NEXT_CMD];
		}
		if (next + cmd_bytes > max) {
			next = min;
			if (stop == fifo[FIFO_NEXT_CMD]) {
				fifo[FIFO_STOP] = min;
				stop = min;
			}
		}
	}

	if (next == stop && next != min) {
		next = min;
		fifo[FIFO_NEXT_CMD] = min;
		fifo[FIFO_STOP] = min;
	}

	volatile uint32_t* cmd = reinterpret_cast<volatile uint32_t*>(
		reinterpret_cast<volatile byte*>(fifo) + next);
	cmd[0] = CMD_UPDATE;
	cmd[1] = rect.x;
	cmd[2] = rect.y;
	cmd[3] = rect.width;
	cmd[4] = rect.height;
	fifo[FIFO_NEXT_CMD] = next + cmd_bytes;
	Sync();
}

uint32_t VmwareSvgaDevice::GetFramebufferOffset() const {
	return ReadRegister(REG_FB_OFFSET);
}

uint32_t VmwareSvgaDevice::GetFramebufferSize() const {
	return ReadRegister(REG_FB_SIZE);
}

uint32_t VmwareSvgaDevice::GetVramSize() const {
	return ReadRegister(REG_VRAM_SIZE);
}

uint32_t VmwareSvgaDevice::GetFifoSize() const {
	return ReadRegister(REG_MEM_SIZE);
}

uint32_t VmwareSvgaDevice::GetCapabilities() const {
	return ReadRegister(REG_CAPABILITIES);
}

} // namespace uni

#endif // (_MCCA & 0xFF00) == 0x8600

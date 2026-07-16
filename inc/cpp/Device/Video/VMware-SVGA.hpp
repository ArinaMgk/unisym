// ASCII CPP TAB4 CRLF
// Docutitle: (Device) VMware SVGA-II Adapter
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
#ifndef _INC_DEVICE_Video_VMware_SVGA
#define _INC_DEVICE_Video_VMware_SVGA

#include "../../unisym"
#include "../../../c/graphic.h"

namespace uni {

class VmwareSvgaDevice {
public:
	static const uint32_t SVGA_ID_0 = 0x90000000u;
	static const uint32_t SVGA_ID_1 = 0x90000001u;
	static const uint32_t SVGA_ID_2 = 0x90000002u;

	enum IoPortOffset : uint16_t {
		IOPORT_INDEX = 0,
		IOPORT_VALUE = 1,
		IOPORT_BIOS  = 2,
		IOPORT_IRQSTATUS = 8,
	};

	enum RegisterIndex : uint32_t {
		REG_ID = 0,
		REG_ENABLE = 1,
		REG_WIDTH = 2,
		REG_HEIGHT = 3,
		REG_MAX_WIDTH = 4,
		REG_MAX_HEIGHT = 5,
		REG_DEPTH = 6,
		REG_BITS_PER_PIXEL = 7,
		REG_PSEUDOCOLOR = 8,
		REG_RED_MASK = 9,
		REG_GREEN_MASK = 10,
		REG_BLUE_MASK = 11,
		REG_BYTES_PER_LINE = 12,
		REG_FB_START = 13,
		REG_FB_OFFSET = 14,
		REG_VRAM_SIZE = 15,
		REG_FB_SIZE = 16,
		REG_CAPABILITIES = 17,
		REG_MEM_START = 18,
		REG_MEM_SIZE = 19,
		REG_CONFIG_DONE = 20,
		REG_SYNC = 21,
		REG_BUSY = 22,
		REG_GUEST_ID = 23,
	};

	enum FifoIndex : uint32_t {
		FIFO_MIN = 0,
		FIFO_MAX = 1,
		FIFO_NEXT_CMD = 2,
		FIFO_STOP = 3,
		FIFO_CAPABILITIES = 4,
		FIFO_FLAGS = 5,
	};

	enum FifoCommand : uint32_t {
		CMD_UPDATE = 1,
	};

	VmwareSvgaDevice();

	void SetIoBase(uint16_t io_base);
	void SetFramebufferBase(uint64_t framebuffer_base);
	void SetFifoBase(uint64_t fifo_base);

	uint32_t ReadRegister(RegisterIndex index) const;
	void WriteRegister(RegisterIndex index, uint32_t value) const;

	bool NegotiateVersion();
	bool Initialize();
	bool ReadMode(uni::Size2& resolution, uint32_t& bpp, uint32_t& pitch) const;
	bool setMode(uint32_t width, uint32_t height, uint32_t bpp = 32);
	bool InitializeFifo();
	void Flush(const Rectangle& rect);

	uint32_t GetFramebufferOffset() const;
	uint32_t GetFramebufferSize() const;
	uint32_t GetVramSize() const;
	uint32_t GetFifoSize() const;
	uint32_t GetCapabilities() const;

	uint16_t GetIoBase() const { return io_base_; }
	uint64_t GetFramebufferBase() const { return framebuffer_base_; }
	uint64_t GetFifoBase() const { return fifo_base_; }

private:
	volatile uint32_t* GetFifoPtr() const;
	void Sync() const;

	uint16_t io_base_ = 0;
	uint64_t framebuffer_base_ = 0;
	uint64_t fifo_base_ = 0;
};

} // namespace uni

#endif // _INC_DEVICE_Video_VMware_SVGA

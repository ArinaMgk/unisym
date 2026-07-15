// ASCII CPP TAB4 CRLF
// Docutitle: (Device) Bochs VBE Graphics Adapter
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
#ifndef _INC_DEVICE_Video_Bochs_GrafAda
#define _INC_DEVICE_Video_Bochs_GrafAda

#include "../../unisym"

namespace uni {

class BochsGrafAda {
public:
	static const uint16_t IOPORT_INDEX = 0x01CE;
	static const uint16_t IOPORT_DATA  = 0x01CF;

	enum RegisterIndex : uint16_t {
		INDEX_ID          = 0,
		INDEX_XRES        = 1,
		INDEX_YRES        = 2,
		INDEX_BPP         = 3,
		INDEX_ENABLE      = 4,
		INDEX_BANK        = 5,
		INDEX_VIRT_WIDTH  = 6,
		INDEX_VIRT_HEIGHT = 7,
		INDEX_X_OFFSET    = 8,
		INDEX_Y_OFFSET    = 9
	};

	enum EnableFlags : uint16_t {
		DISABLED        = 0x00,
		ENABLED         = 0x01,
		GETCAPS         = 0x02,
		_8BIT_DAC       = 0x20,
		LFB_ENABLED     = 0x40,
		NOCLEARMEM      = 0x80
	};

	BochsGrafAda();

	uint16_t ReadRegister(RegisterIndex index) const;
	void WriteRegister(RegisterIndex index, uint16_t value);

	bool SetResolution(uint16_t width, uint16_t height, uint16_t bpp, bool use_lfb = true, bool clear_mem = true);
};

} // namespace uni

#endif // _INC_DEVICE_Video_Bochs_GrafAda

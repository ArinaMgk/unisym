// ASCII CPP TAB4 CRLF
// Docutitle: (Device) Bochs VBE Graphics Adapter
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
#include "../../../../inc/cpp/Device/Video/Bochs-GrafAda.hpp"

#if (_MCCA & 0xFF00) == 0x8600
#include "../../../../inc/c/proctrl/IAx86_64.h"


namespace uni {

BochsGrafAda::BochsGrafAda() {
}

uint16_t BochsGrafAda::ReadRegister(RegisterIndex index) const {
	outpw(IOPORT_INDEX, index);
	return innpw(IOPORT_DATA);
}

void BochsGrafAda::WriteRegister(RegisterIndex index, uint16_t value) {
	outpw(IOPORT_INDEX, index);
	outpw(IOPORT_DATA, value);
}

bool BochsGrafAda::SetResolution(uint16_t width, uint16_t height, uint16_t bpp, bool use_lfb, bool clear_mem) {
	// Disable display first
	WriteRegister(INDEX_ENABLE, DISABLED);
	
	WriteRegister(INDEX_XRES, width);
	WriteRegister(INDEX_YRES, height);
	WriteRegister(INDEX_BPP, bpp);

	uint16_t enable_flags = ENABLED;
	if (use_lfb) enable_flags |= LFB_ENABLED;
	if (!clear_mem) enable_flags |= NOCLEARMEM;
	
	WriteRegister(INDEX_ENABLE, enable_flags);

	// Verify if the resolution was accepted
	if (ReadRegister(INDEX_XRES) != width || ReadRegister(INDEX_YRES) != height) {
		return false; // Display controller rejected the mode
	}
	return true;
}

} // namespace uni

#endif // (_MCCA & 0xFF00) == 0x8600

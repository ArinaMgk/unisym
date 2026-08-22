
#include "../../../../inc/cpp/Device/USB/PCD.hpp"
#include "../../../../inc/cpp/Device/USB/HCD.hpp"

#if defined(_MCU_STM32H7x)
_ESYM_C{
	// AKA HAL: OTG_HS_IRQHandler (OTG1_HS, IRQ_OTG_HS = 77) / OTG_FS_IRQHandler (OTG2_FS, IRQ_OTG_FS = 101)
	// Shared by PCD (device mode) and HCD (host mode): the OTG core is either
	// a device or a host at any time, so dispatch by the current core mode.
	Handler_t FUNC_OTG_HS[1] = { 0 };
	Handler_t FUNC_OTG_FS[1] = { 0 };

	void OTG_HS_IRQHandler(void) {
		// CMOD bit: 0 = device mode, 1 = host mode (AKA USB_GetMode)
		if (uni::OTG::getMode(_OTG1_HS_ADDR) == 0)
			uni::PCD1.HandleIRQ();
		else
			uni::HCD1.HandleIRQ();
		if (FUNC_OTG_HS[0]) FUNC_OTG_HS[0]();// user callback registered by setInterrupt()
	}

	void OTG_FS_IRQHandler(void) {
		if (uni::OTG::getMode(_OTG2_FS_ADDR) == 0)
			uni::PCD2.HandleIRQ();
		else
			uni::HCD2.HandleIRQ();
		if (FUNC_OTG_FS[0]) FUNC_OTG_FS[0]();
	}
}
#endif

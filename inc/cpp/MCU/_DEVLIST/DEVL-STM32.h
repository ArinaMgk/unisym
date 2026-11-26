
#ifndef _DEVL_STM32_H
#define _DEVL_STM32_H
#if defined(_MCU_STM32H7x)

namespace uni {
	enum class QrsDeviceList {
		RTC,
		LTDC,
		FmcSDRAM,
		Sdmmc1,
		SAI1,
		SAI2,
		//
		DeviceCount // must be last
	};

}


#endif

#endif


#include "../../../c/stdinc.h"// cheat parser
#define _LNK_WEAK __attribute__((weak))// means TODO in UNISYM

// relationship: lib\cpp\Device\Interrupt\interset_stm32mp13
// [from]
// - mp135fxx_ca7.h
typedef enum IRQn
{
	Request_None = 0,
	// ----  Cortex-A Processor Specific Interrupt Numbers ----
	//: Software Generated Interrupts
	IRQ_SGI0 = 0,
	IRQ_SGI1 = 1,
	IRQ_SGI2 = 2,
	IRQ_SGI3 = 3,
	IRQ_SGI4 = 4,
	IRQ_SGI5 = 5,
	IRQ_SGI6 = 6,
	IRQ_SGI7 = 7,
	IRQ_SGI8 = 8,
	IRQ_SGI9 = 9,
	IRQ_SGI10 = 10,
	IRQ_SGI11 = 11,
	IRQ_SGI12 = 12,
	IRQ_SGI13 = 13,
	IRQ_SGI14 = 14,
	IRQ_SGI15 = 15,
	// ---- Private Peripheral Interrupts ----
	IRQ_VirtualMaintenance = 25,     // Virtual Maintenance
	IRQ_HypervisorTimer = 26,     // Hypervisor Timer
	IRQ_VirtualTimer = 27,     // Virtual Timer
	IRQ_Legacy_nFIQ = 28,     // Legacy nFIQ
	IRQ_SecurePhyTimer = 29,     // Secure Physical Timer
	IRQ_NonSecurePhyTimer = 30,     // Non-Secure Physical Timer
	IRQ_Legacy_nIRQ = 31,     // Legacy nIRQ
	// ----  STM32 specific  Numbers ---- RESERVED(32,155)
	IRQ_PVD_AVD = 33,     // PVD & AVD detector through EXTI
	IRQ_TAMP = 34,     // Tamper s through the EXTI line
	IRQ_RTC_WKUP_ALARM = 35,     // RTC Wakeup and Alarm (A & B)  through the EXTI line
	IRQ_TZC_IT = 36,     // TrustZone DDR address space controller
	IRQ_RCC = 37,     // RCC global
	IRQ_EXTI0 = 38,     // EXTI Line0
	IRQ_EXTI1 = 39,     // EXTI Line1
	IRQ_EXTI2 = 40,     // EXTI Line2
	IRQ_EXTI3 = 41,     // EXTI Line3
	IRQ_EXTI4 = 42,     // EXTI Line4
	IRQ_DMA1_Stream0 = 43,     // DMA1 Stream 0 global
	IRQ_DMA1_Stream1 = 44,     // DMA1 Stream 1 global
	IRQ_DMA1_Stream2 = 45,     // DMA1 Stream 2 global
	IRQ_DMA1_Stream3 = 46,     // DMA1 Stream 3 global
	IRQ_DMA1_Stream4 = 47,     // DMA1 Stream 4 global
	IRQ_DMA1_Stream5 = 48,     // DMA1 Stream 5 global
	IRQ_DMA1_Stream6 = 49,     // DMA1 Stream 6 global
	IRQ_ADC1 = 50,     // ADC1 global s
	IRQ_ADC2 = 51,     // ADC1 global s
	IRQ_FDCAN1_IT0 = 52,     // FDCAN1  line 0
	IRQ_FDCAN2_IT0 = 53,     // FDCAN2  line 0
	IRQ_FDCAN1_IT1 = 54,     // FDCAN1  line 1
	IRQ_FDCAN2_IT1 = 55,     // FDCAN2  line 1
	IRQ_EXTI5 = 56,     // External Line[9:5] s
	IRQ_TIM1_BRK = 57,     // TIM1 Break
	IRQ_TIM1_UP = 58,     // TIM1 Update
	IRQ_TIM1_TRG_COM = 59,     // TIM1 Trigger and Commutation
	IRQ_TIM1_CC = 60,     // TIM1 Capture Compare
	IRQ_TIM2 = 61,     // TIM2 global
	IRQ_TIM3 = 62,     // TIM3 global
	IRQ_TIM4 = 63,     // TIM4 global
	IRQ_I2C1_EV = 64,     // I2C1 Event
	IRQ_I2C1_ER = 65,     // I2C1 Error
	IRQ_I2C2_EV = 66,     // I2C2 Event
	IRQ_I2C2_ER = 67,     // I2C2 Error
	IRQ_SPI1 = 68,     // SPI1 global
	IRQ_SPI2 = 69,     // SPI2 global
	IRQ_USART1 = 70,     // USART1 global
	IRQ_USART2 = 71,     // USART2 global
	IRQ_USART3 = 72,     // USART3 global
	IRQ_EXTI10 = 73,     // EXTI Line 10 s
	IRQ_RTC_TIMESTAMP = 74,     // RTC TimeStamp through EXTI Line
	IRQ_EXTI11 = 75,     // EXTI Line 11 s
	IRQ_TIM8_BRK = 76,     // TIM8 Break
	IRQ_TIM8_UP = 77,     // TIM8 Update
	IRQ_TIM8_TRG_COM = 78,     // TIM8 Trigger and Commutation
	IRQ_TIM8_CC = 79,     // TIM8 Capture Compare
	IRQ_DMA1_Stream7 = 80,     // DMA1 Stream7
	IRQ_FMC = 81,     // FMC global
	IRQ_SDMMC1 = 82,     // SDMMC1 global
	IRQ_TIM5 = 83,     // TIM5 global
	IRQ_SPI3 = 84,     // SPI3 global
	IRQ_UART4 = 85,     // UART4 global
	IRQ_UART5 = 86,     // UART5 global
	IRQ_TIM6 = 87,     // TIM6 global
	IRQ_TIM7 = 88,     // TIM7 global
	IRQ_DMA2_Stream0 = 89,     // DMA2 Stream 0 global
	IRQ_DMA2_Stream1 = 90,     // DMA2 Stream 1 global
	IRQ_DMA2_Stream2 = 91,     // DMA2 Stream 2 global
	IRQ_DMA2_Stream3 = 92,     // GPDMA2 Stream 3 global
	IRQ_DMA2_Stream4 = 93,     // GPDMA2 Stream 4 global
	IRQ_ETH1 = 94,     // Ethernet global
	IRQ_ETH1_WKUP = 95,     // Ethernet Wakeup through EXTI line
	IRQ_FDCAN_CAL = 96,     // CAN calibration unit
	IRQ_EXTI6 = 97,     // EXTI Line 6 s
	IRQ_EXTI7 = 98,     // EXTI Line 7 s
	IRQ_EXTI8 = 99,     // EXTI Line 8 s
	IRQ_EXTI9 = 100,    // EXTI Line 9 s
	IRQ_DMA2_Stream5 = 101,    // DMA2 Stream 5 global
	IRQ_DMA2_Stream6 = 102,    // DMA2 Stream 6 global
	IRQ_DMA2_Stream7 = 103,    // DMA2 Stream 7 global
	IRQ_USART6 = 104,    // USART6 global
	IRQ_I2C3_EV = 105,    // I2C3 event
	IRQ_I2C3_ER = 106,    // I2C3 error
	IRQ_USBH_PORT1 = 107,    // USB Host port 1
	IRQ_USBH_PORT2 = 108,    // USB Host port 2
	IRQ_EXTI12 = 109,    // EXTI Line 76 s
	IRQ_EXTI13 = 110,    // EXTI Line 77 s
	IRQ_DCMIPP = 111,    // DCMIPP global
	IRQ_CRYP1 = 112,    /// 112: 135F{CRYP crypto global} None{135D}
	IRQ_HASH1 = 113,    // Hash global
	IRQ_SAES = 114,    // 114: 135F{Secure AES global} None{135D}
	IRQ_UART7 = 115,    // UART7 global
	IRQ_UART8 = 116,    // UART8 global
	IRQ_SPI4 = 117,    // SPI4 global
	IRQ_SPI5 = 118,    // SPI5 global
	IRQ_SAI1 = 119,    // SAI1 global
	IRQ_LTDC = 120,    // LTDC global
	IRQ_LTDC_ER = 121,    // LTDC Error global
	IRQ_SAI2 = 122,    // SAI2 global
	IRQ_QUADSPI = 123,    // Quad SPI global
	IRQ_LPTIM1 = 124,    // LP TIM1
	IRQ_I2C4_EV = 125,    // I2C4 Event
	IRQ_I2C4_ER = 126,    // I2C4 Error
	IRQ_SPDIF_RX = 127,    // SPDIF-RX global
	IRQ_OTG = 128,    // USB On The Go global
	IRQ_ETH2 = 129,    // Ethernet2 global
	IRQ_ETH2_WKUP = 130,    // Ethernet2 wkup  (PMT)
	IRQ_DMAMUX1_OVR = 131,    // DMAMUX1 Overrun
	IRQ_DMAMUX2_OVR = 132,    // DMAMUX2 Overrun
	IRQ_TIM15 = 133,    // TIM15 global
	IRQ_TIM16 = 134,    // TIM16 global
	IRQ_TIM17 = 135,    // TIM17 global
	IRQ_TIM12 = 136,    // TIM12 global
	IRQ_PKA = 137,    // 137 135F{PKA global} None{135D}
	IRQ_EXTI14 = 138,    // EXTI Line 14 s
	IRQ_MDMA = 139,    // MDMA global
	IRQ_SDMMC2 = 140,    // SDMMC2 global
	IRQ_EXTI15 = 141,    // EXTI Line 15 s
	IRQ_MDMA_SEC_IT = 142,    // MDMA global Secure
	IRQ_TIM13 = 143,    // TIM13 global
	IRQ_TIM14 = 144,    // TIM14 global
	IRQ_RNG1 = 145,    // RNG1
	IRQ_I2C5_EV = 146,    // I2C5 Event
	IRQ_I2C5_ER = 147,    // I2C5 Error
	IRQ_LPTIM2 = 148,    // LP TIM2 global
	IRQ_LPTIM3 = 149,    // LP TIM3 global
	IRQ_LPTIM4 = 150,    // LP TIM4 global
	IRQ_LPTIM5 = 151,    // LP TIM5 global
	IRQ_ETH1_LPI = 152,    // ETH1_LPI
	IRQ_ETH2_LPI = 153,    // ETH2_LPI
	IRQ_RCC_WAKEUP = 154,    // RCC Wake up
	IRQ_DTS = 156,    // Digital Temperature Sensor
	IRQ_MPU_WAKEUP_PIN = 157,    //  for all 6 wake-up enabled by MPU
	IRQ_IWDG1 = 158,    // IWDG1 Early
	IRQ_IWDG2 = 159,    // IWDG2 Early
	IRQ_TAMP_S = 160,    // TAMP Tamper Secure
	IRQ_RTC_WKUP_ALARM_S = 161,    // RTC Wakeup Timer and Alarms (A and B) Secure
	IRQ_RTC_TS_S = 162,    // RTC TimeStamp Secure
	IRQ_LTDC_SEC = 163,    // LTDC secure global
	IRQ_LTDC_SEC_ER = 164,    // LTDC secure global Error
	IRQ_PMUIRQ0 = 165,    // Cortex-A7 Core#0 Performance Monitor
	IRQ_COMMRX0 = 166,    // Cortex-A7 Core#0 Debug Communication Channel Receive
	IRQ_COMMTX0 = 167,    // Cortex-A7 Core#0 Debug Communication Channel Transmit
	IRQ_AXIERRIRQ = 168,    // Asynchronous AXI Abort
	IRQ_DDRPERFM = 169,    // DDR Performance Monitor
	IRQ_nCTIIRQ0 = 170,    // Cortex-A7 Core#0 CTI
	IRQ_MCE = 171,    // 171 135F{MCE} None{135D}
	IRQ_DFSDM1 = 172,    // DFSDM Filter1
	IRQ_DFSDM2 = 173,    // DFSDM Filter2
	IRQ_DMA3_Stream0 = 174,    // DMA3 Stream 0 global
	IRQ_DMA3_Stream1 = 175,    // DMA3 Stream 1 global
	IRQ_DMA3_Stream2 = 176,    // DMA3 Stream 2 global
	IRQ_DMA3_Stream3 = 177,    // DMA3 Stream 3 global
	IRQ_DMA3_Stream4 = 178,    // DMA3 Stream 4 global
	IRQ_DMA3_Stream5 = 179,    // DMA3 Stream 5 global
	IRQ_DMA3_Stream6 = 180,    // DMA3 Stream 6 global
	IRQ_DMA3_Stream7 = 181,    // DMA3 Stream 7 global
	MAX_IRQ_n,
	IRQ_Force_enum_size = 1048 // Dummy entry to ensure IRQn_Type is more than 8 bits. Otherwise GIC init loop would fail
} IRQn_Type, Request_t;


// SoC External Interrupt Handler
// THIS started on startup_stm32mp135dxx_ca7.c
extern "C" {
	symbol_t RESERVED_IRQHandler;
	symbol_t SGI0_IRQHandler;
	symbol_t SGI1_IRQHandler;
	symbol_t SGI2_IRQHandler;
	symbol_t SGI3_IRQHandler;
	symbol_t SGI4_IRQHandler;
	symbol_t SGI5_IRQHandler;
	symbol_t SGI6_IRQHandler;
	symbol_t SGI7_IRQHandler;
	symbol_t SGI8_IRQHandler;
	symbol_t SGI9_IRQHandler;
	symbol_t SGI10_IRQHandler;
	symbol_t SGI11_IRQHandler;
	symbol_t SGI12_IRQHandler;
	symbol_t SGI13_IRQHandler;
	symbol_t SGI14_IRQHandler;
	symbol_t SGI15_IRQHandler;
	symbol_t VirtualMaintenanceInterrupt_IRQHandler;
	symbol_t HypervisorTimer_IRQHandler;
	symbol_t VirtualTimer_IRQHandler;
	symbol_t Legacy_nFIQ_IRQHandler;
	symbol_t SecurePhysicalTimer_IRQHandler;
	symbol_t NonSecurePhysicalTimer_IRQHandler;
	symbol_t Legacy_nIRQ_IRQHandler;
	symbol_t PVD_AVD_IRQHandler;
	symbol_t TAMP_IRQHandler;
	symbol_t RTC_WKUP_ALARM_IRQHandler;
	symbol_t TZC_IT_IRQHandler;
	symbol_t RCC_IRQHandler;
	symbol_t EXTI0_IRQHandler;
	symbol_t EXTI1_IRQHandler;
	symbol_t EXTI2_IRQHandler;
	symbol_t EXTI3_IRQHandler;
	symbol_t EXTI4_IRQHandler;
	symbol_t DMA1_Stream0_IRQHandler;
	symbol_t DMA1_Stream1_IRQHandler;
	symbol_t DMA1_Stream2_IRQHandler;
	symbol_t DMA1_Stream3_IRQHandler;
	symbol_t DMA1_Stream4_IRQHandler;
	symbol_t DMA1_Stream5_IRQHandler;
	symbol_t DMA1_Stream6_IRQHandler;
	symbol_t ADC1_IRQHandler;
	symbol_t ADC2_IRQHandler;
	symbol_t FDCAN1_IT0_IRQHandler;
	symbol_t FDCAN2_IT0_IRQHandler;
	symbol_t FDCAN1_IT1_IRQHandler;
	symbol_t FDCAN2_IT1_IRQHandler;
	symbol_t EXTI5_IRQHandler;
	symbol_t TIM1_BRK_IRQHandler;
	symbol_t TIM1_UP_IRQHandler;
	symbol_t TIM1_TRG_COM_IRQHandler;
	symbol_t TIM1_CC_IRQHandler;
	symbol_t TIM2_IRQHandler;
	symbol_t TIM3_IRQHandler;
	symbol_t TIM4_IRQHandler;
	symbol_t I2C1_EV_IRQHandler;
	symbol_t I2C1_ER_IRQHandler;
	symbol_t I2C2_EV_IRQHandler;
	symbol_t I2C2_ER_IRQHandler;
	symbol_t SPI1_IRQHandler;
	symbol_t SPI2_IRQHandler;
	symbol_t USART1_IRQHandler;
	symbol_t USART2_IRQHandler;
	symbol_t USART3_IRQHandler;
	symbol_t EXTI10_IRQHandler;
	symbol_t RTC_TIMESTAMP_IRQHandler;
	symbol_t EXTI11_IRQHandler;
	symbol_t TIM8_BRK_IRQHandler;
	symbol_t TIM8_UP_IRQHandler;
	symbol_t TIM8_TRG_COM_IRQHandler;
	symbol_t TIM8_CC_IRQHandler;
	symbol_t DMA1_Stream7_IRQHandler;
	symbol_t FMC_IRQHandler;
	symbol_t SDMMC1_IRQHandler;
	symbol_t TIM5_IRQHandler;
	symbol_t SPI3_IRQHandler;
	symbol_t UART4_IRQHandler;
	symbol_t UART5_IRQHandler;
	symbol_t TIM6_IRQHandler;
	symbol_t TIM7_IRQHandler;
	symbol_t DMA2_Stream0_IRQHandler;
	symbol_t DMA2_Stream1_IRQHandler;
	symbol_t DMA2_Stream2_IRQHandler;
	symbol_t DMA2_Stream3_IRQHandler;
	symbol_t DMA2_Stream4_IRQHandler;
	symbol_t ETH1_IRQHandler;
	symbol_t ETH1_WKUP_IRQHandler;
	symbol_t FDCAN_CAL_IRQHandler;
	symbol_t EXTI6_IRQHandler;
	symbol_t EXTI7_IRQHandler;
	symbol_t EXTI8_IRQHandler;
	symbol_t EXTI9_IRQHandler;
	symbol_t DMA2_Stream5_IRQHandler;
	symbol_t DMA2_Stream6_IRQHandler;
	symbol_t DMA2_Stream7_IRQHandler;
	symbol_t USART6_IRQHandler;
	symbol_t I2C3_EV_IRQHandler;
	symbol_t I2C3_ER_IRQHandler;
	symbol_t USBH_PORT1_IRQHandler;
	symbol_t USBH_PORT2_IRQHandler;
	symbol_t EXTI12_IRQHandler;
	symbol_t EXTI13_IRQHandler;
	symbol_t DCMIPP_IRQHandler;

	// 112
	symbol_t CRYP1_IRQHandler;// 135F

	symbol_t HASH1_IRQHandler;

	// 114
	symbol_t SAES_IRQHandler;// 135F

	symbol_t UART7_IRQHandler;
	symbol_t UART8_IRQHandler;
	symbol_t SPI4_IRQHandler;
	symbol_t SPI5_IRQHandler;
	symbol_t SAI1_IRQHandler;
	symbol_t LTDC_IRQHandler;
	symbol_t LTDC_ER_IRQHandler;
	symbol_t SAI2_IRQHandler;
	symbol_t QUADSPI_IRQHandler;
	symbol_t LPTIM1_IRQHandler;
	symbol_t I2C4_EV_IRQHandler;
	symbol_t I2C4_ER_IRQHandler;
	symbol_t SPDIF_RX_IRQHandler;
	symbol_t OTG_IRQHandler;
	symbol_t ETH2_IRQHandler;
	symbol_t ETH2_WKUP_IRQHandler;
	symbol_t DMAMUX1_OVR_IRQHandler;
	symbol_t DMAMUX2_OVR_IRQHandler;
	symbol_t TIM15_IRQHandler;
	symbol_t TIM16_IRQHandler;
	symbol_t TIM17_IRQHandler;
	symbol_t TIM12_IRQHandler;

	// 137
	symbol_t PKA_IRQHandler;// 135F

	symbol_t EXTI14_IRQHandler;
	symbol_t MDMA_IRQHandler;
	symbol_t SDMMC2_IRQHandler;
	symbol_t EXTI15_IRQHandler;
	symbol_t MDMA_SEC_IT_IRQHandler;
	symbol_t TIM13_IRQHandler;
	symbol_t TIM14_IRQHandler;
	symbol_t RNG1_IRQHandler;
	symbol_t I2C5_EV_IRQHandler;
	symbol_t I2C5_ER_IRQHandler;
	symbol_t LPTIM2_IRQHandler;
	symbol_t LPTIM3_IRQHandler;
	symbol_t LPTIM4_IRQHandler;
	symbol_t LPTIM5_IRQHandler;
	symbol_t ETH1_LPI_IRQHandler;
	symbol_t ETH2_LPI_IRQHandler;
	symbol_t RCC_WAKEUP__IRQHandler;
	symbol_t DTS_IRQHandler;
	symbol_t MPU_WAKEUP_PIN_IRQHandler;
	symbol_t IWDG1_IRQHandler;
	symbol_t IWDG2_IRQHandler;
	symbol_t TAMP_SIRQHandler;
	symbol_t RTC_WKUP_ALARM_S_IRQHandler;
	symbol_t RTC_TS_S_IRQHandler;
	symbol_t LTDC_SEC_IRQHandler;
	symbol_t LTDC_SEC_ER_IRQHandler;
	symbol_t PMUIRQ0_IRQHandler;
	symbol_t COMMRX0_IRQHandler;
	symbol_t COMMTX0_IRQHandler;
	symbol_t AXIERRIRQ_IRQHandler;
	symbol_t DDRPERFM_IRQHandler;
	symbol_t nCTIIRQ0_IRQHandler;

	// 171
	symbol_t MCE_IRQHandler;

	symbol_t DFSDM1_IRQHandler;
	symbol_t DFSDM2_IRQHandler;
	symbol_t DMA3_Stream0_IRQHandler;
	symbol_t DMA3_Stream1_IRQHandler;
	symbol_t DMA3_Stream2_IRQHandler;
	symbol_t DMA3_Stream3_IRQHandler;
	symbol_t DMA3_Stream4_IRQHandler;
	symbol_t DMA3_Stream5_IRQHandler;
	symbol_t DMA3_Stream6_IRQHandler;
	symbol_t DMA3_Stream7_IRQHandler;
};

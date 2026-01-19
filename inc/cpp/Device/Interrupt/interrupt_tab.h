// ASCII CPP TAB4 CRLF
// Docutitle: Interrupt List Abstract
// Codifiers: @dosconio: 20240528
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/
#ifndef _INC_Interrupt_Table
#define _INC_Interrupt_Table

#include "../../../c/stdinc.h"

#if defined(_MCCA) && (_MCCA==0x8616||_MCCA==0x8632)

#include "../../../c/board/IBM.h"

#elif defined(_MCCA) && ((_MCCA & 0xFF00)==0x1000)// qemuvirt-riscv

namespace uni {
	// see https://github.com/qemu/qemu/blob/master/include/hw/riscv/virt.h
	enum Request_t {
		IRQ_VIRTIO_1 = 1,
		IRQ_VIRTIO_2 = 2,
		IRQ_VIRTIO_3 = 3,
		IRQ_VIRTIO_4 = 4,
		IRQ_VIRTIO_5 = 5,
		IRQ_VIRTIO_6 = 6,
		IRQ_VIRTIO_7 = 7,
		IRQ_VIRTIO_8 = 8,

		IRQ_UART0 = 10,
		IRQ_RTC = 11,

		IRQ_PCIE_1 = 32,
		IRQ_PCIE_2 = 33,
		IRQ_PCIE_3 = 34,
		IRQ_PCIE_4 = 35,
		IRQ_IOMMU_SYS_1 = 36,
		IRQ_IOMMU_SYS_2 = 37,
		IRQ_IOMMU_SYS_3 = 38,
		IRQ_IOMMU_SYS_4 = 39,

		IRQ_VIRT_PLATFORM_BUS_1 = 64,// 64~95
	};
}

#elif defined(_MCU_STM32F1x)

// startup_stm32f103xe.keil.s
/* Borrow Idens from Official __Vectors
{TODO}	DCD     __initial_sp               ; Top of Stack
{TODO}	DCD     Reset_Handler              ; Reset Handler
{TODO}	DCD     NMI_Handler                ; NMI Handler
{TODO}	DCD     HardFault_Handler          ; Hard Fault Handler
{TODO}	DCD     MemManage_Handler          ; MPU Fault Handler
{TODO}	DCD     BusFault_Handler           ; Bus Fault Handler
{TODO}	DCD     UsageFault_Handler         ; Usage Fault Handler
{TODO}	DCD     0                          ; Reserved
{TODO}	DCD     0                          ; Reserved
{TODO}	DCD     0                          ; Reserved
{TODO}	DCD     0                          ; Reserved
{TODO}	DCD     SVC_Handler                ; SVCall Handler
{TODO}	DCD     DebugMon_Handler           ; Debug Monitor Handler
{TODO}	DCD     0                          ; Reserved
{TODO}	DCD     PendSV_Handler             ; PendSV Handler
{TODO}	DCD     SysTick_Handler            ; SysTick Handler
; External Interrupts
{TODO}	DCD     WWDG_IRQHandler            ; Window Watchdog
{TODO}	DCD     PVD_IRQHandler             ; PVD through EXTI Line detect
{TODO}	DCD     TAMPER_IRQHandler          ; Tamper
{TODO}	DCD     RTC_IRQHandler             ; RTC
{TODO}	DCD     FLASH_IRQHandler           ; Flash
{TODO}	DCD     RCC_IRQHandler             ; RCC
		DCD     EXTI0_IRQHandler           ; EXTI Line 0
		DCD     EXTI1_IRQHandler           ; EXTI Line 1
		DCD     EXTI2_IRQHandler           ; EXTI Line 2
		DCD     EXTI3_IRQHandler           ; EXTI Line 3
		DCD     EXTI4_IRQHandler           ; EXTI Line 4
		DCD     DMA1_Channel1_IRQHandler   ; DMA1 Channel 1
		DCD     DMA1_Channel2_IRQHandler   ; DMA1 Channel 2
		DCD     DMA1_Channel3_IRQHandler   ; DMA1 Channel 3
		DCD     DMA1_Channel4_IRQHandler   ; DMA1 Channel 4
		DCD     DMA1_Channel5_IRQHandler   ; DMA1 Channel 5
		DCD     DMA1_Channel6_IRQHandler   ; DMA1 Channel 6
		DCD     DMA1_Channel7_IRQHandler   ; DMA1 Channel 7
		DCD     ADC1_2_IRQHandler          ; ADC1 & ADC2
{TODO}	DCD     USB_HP_CAN1_TX_IRQHandler  ; USB High Priority or CAN1 TX
{TODO}	DCD     USB_LP_CAN1_RX0_IRQHandler ; USB Low  Priority or CAN1 RX0
{TODO}	DCD     CAN1_RX1_IRQHandler        ; CAN1 RX1
{TODO}	DCD     CAN1_SCE_IRQHandler        ; CAN1 SCE
		DCD     EXTI9_5_IRQHandler         ; EXTI Line 9..5
{TODO}	DCD     TIM1_BRK_IRQHandler        ; TIM1 Break
{TODO}	DCD     TIM1_UP_IRQHandler         ; TIM1 Update
{TODO}	DCD     TIM1_TRG_COM_IRQHandler    ; TIM1 Trigger and Commutation
{TODO}	DCD     TIM1_CC_IRQHandler         ; TIM1 Capture Compare
		DCD     TIM2_IRQHandler            ; TIM2
		DCD     TIM3_IRQHandler            ; TIM3
		DCD     TIM4_IRQHandler            ; TIM4
{TODO}	DCD     I2C1_EV_IRQHandler         ; I2C1 Event
{TODO}	DCD     I2C1_ER_IRQHandler         ; I2C1 Error
{TODO}	DCD     I2C2_EV_IRQHandler         ; I2C2 Event
{TODO}	DCD     I2C2_ER_IRQHandler         ; I2C2 Error
{TODO}	DCD     SPI1_IRQHandler            ; SPI1
{TODO}	DCD     SPI2_IRQHandler            ; SPI2
		DCD     USART1_IRQHandler          ; USART1
		DCD     USART2_IRQHandler          ; USART2
		DCD     USART3_IRQHandler          ; USART3
		DCD     EXTI15_10_IRQHandler       ; EXTI Line 15..10
{TODO}	DCD     RTC_Alarm_IRQHandler       ; RTC Alarm through EXTI Line
{TODO}	DCD     USBWakeUp_IRQHandler       ; USB Wakeup from suspend
{TODO}	DCD     TIM8_BRK_IRQHandler        ; TIM8 Break
{TODO}	DCD     TIM8_UP_IRQHandler         ; TIM8 Update
{TODO}	DCD     TIM8_TRG_COM_IRQHandler    ; TIM8 Trigger and Commutation
{TODO}	DCD     TIM8_CC_IRQHandler         ; TIM8 Capture Compare
		DCD     ADC3_IRQHandler            ; ADC3
{TODO}	DCD     FSMC_IRQHandler            ; FSMC
{TODO}	DCD     SDIO_IRQHandler            ; SDIO
{TODO}	DCD     TIM5_IRQHandler            ; TIM5
{TODO}	DCD     SPI3_IRQHandler            ; SPI3
		DCD     UART4_IRQHandler           ; UART4
		DCD     UART5_IRQHandler           ; UART5
		DCD     TIM6_IRQHandler            ; TIM6
		DCD     TIM7_IRQHandler            ; TIM7
		DCD     DMA2_Channel1_IRQHandler   ; DMA2 Channel1
		DCD     DMA2_Channel2_IRQHandler   ; DMA2 Channel2
		DCD     DMA2_Channel3_IRQHandler   ; DMA2 Channel3
		DCD     DMA2_Channel4_5_IRQHandler ; DMA2 Channel4 & Channel5
__Vectors_End */

#elif defined(_MCU_STM32F4x)

// startup_stm32f407xx.keil.s
/*
		DCD     __initial_sp               ; Top of Stack
{TODO}	DCD     Reset_Handler              ; Reset Handler
{TODO}	DCD     NMI_Handler                ; NMI Handler
{TODO}	DCD     HardFault_Handler          ; Hard Fault Handler
{TODO}	DCD     MemManage_Handler          ; MPU Fault Handler
{TODO}	DCD     BusFault_Handler           ; Bus Fault Handler
{TODO}	DCD     UsageFault_Handler         ; Usage Fault Handler
{TODO}	DCD     0                          ; Reserved
{TODO}	DCD     0                          ; Reserved
{TODO}	DCD     0                          ; Reserved
{TODO}	DCD     0                          ; Reserved
{TODO}	DCD     SVC_Handler                ; SVCall Handler
{TODO}	DCD     DebugMon_Handler           ; Debug Monitor Handler
{TODO}	DCD     0                          ; Reserved
{TODO}	DCD     PendSV_Handler             ; PendSV Handler
{TODO}	DCD     SysTick_Handler            ; SysTick Handler
; External Interrupts
{TODO}	DCD     WWDG_IRQHandler                   ; Window WatchDog
{TODO}	DCD     PVD_IRQHandler                    ; PVD through EXTI Line detection
{TODO}	DCD     TAMP_STAMP_IRQHandler             ; Tamper and TimeStamps through the EXTI line
{TODO}	DCD     RTC_WKUP_IRQHandler               ; RTC Wakeup through the EXTI line
{TODO}	DCD     FLASH_IRQHandler                  ; FLASH
{TODO}	DCD     RCC_IRQHandler                    ; RCC
		DCD     EXTI0_IRQHandler                  ; EXTI Line0
		DCD     EXTI1_IRQHandler                  ; EXTI Line1
		DCD     EXTI2_IRQHandler                  ; EXTI Line2
		DCD     EXTI3_IRQHandler                  ; EXTI Line3
		DCD     EXTI4_IRQHandler                  ; EXTI Line4
{TODO}	DCD     DMA1_Stream0_IRQHandler           ; DMA1 Stream 0
{TODO}	DCD     DMA1_Stream1_IRQHandler           ; DMA1 Stream 1
{TODO}	DCD     DMA1_Stream2_IRQHandler           ; DMA1 Stream 2
{TODO}	DCD     DMA1_Stream3_IRQHandler           ; DMA1 Stream 3
{TODO}	DCD     DMA1_Stream4_IRQHandler           ; DMA1 Stream 4
{TODO}	DCD     DMA1_Stream5_IRQHandler           ; DMA1 Stream 5
{TODO}	DCD     DMA1_Stream6_IRQHandler           ; DMA1 Stream 6
		DCD     ADC_IRQHandler                    ; ADC1, ADC2 and ADC3s
{TODO}	DCD     CAN1_TX_IRQHandler                ; CAN1 TX
{TODO}	DCD     CAN1_RX0_IRQHandler               ; CAN1 RX0
{TODO}	DCD     CAN1_RX1_IRQHandler               ; CAN1 RX1
{TODO}	DCD     CAN1_SCE_IRQHandler               ; CAN1 SCE
		DCD     EXTI9_5_IRQHandler                ; External Line[9:5]s
{TODO}	DCD     TIM1_BRK_TIM9_IRQHandler          ; TIM1 Break and TIM9
{TODO}	DCD     TIM1_UP_TIM10_IRQHandler          ; TIM1 Update and TIM10
{TODO}	DCD     TIM1_TRG_COM_TIM11_IRQHandler     ; TIM1 Trigger and Commutation and TIM11
{TODO}	DCD     TIM1_CC_IRQHandler                ; TIM1 Capture Compare
		DCD     TIM2_IRQHandler                   ; TIM2
		DCD     TIM3_IRQHandler                   ; TIM3
		DCD     TIM4_IRQHandler                   ; TIM4
{TODO}	DCD     I2C1_EV_IRQHandler                ; I2C1 Event
{TODO}	DCD     I2C1_ER_IRQHandler                ; I2C1 Error
{TODO}	DCD     I2C2_EV_IRQHandler                ; I2C2 Event
{TODO}	DCD     I2C2_ER_IRQHandler                ; I2C2 Error
{TODO}	DCD     SPI1_IRQHandler                   ; SPI1
{TODO}	DCD     SPI2_IRQHandler                   ; SPI2
		DCD     USART1_IRQHandler                 ; USART1
		DCD     USART2_IRQHandler                 ; USART2
		DCD     USART3_IRQHandler                 ; USART3
		DCD     EXTI15_10_IRQHandler              ; External Line[15:10]s
{TODO}	DCD     RTC_Alarm_IRQHandler              ; RTC Alarm (A and B) through EXTI Line
{TODO}	DCD     OTG_FS_WKUP_IRQHandler            ; USB OTG FS Wakeup through EXTI line
{TODO}	DCD     TIM8_BRK_TIM12_IRQHandler         ; TIM8 Break and TIM12
{TODO}	DCD     TIM8_UP_TIM13_IRQHandler          ; TIM8 Update and TIM13
{TODO}	DCD     TIM8_TRG_COM_TIM14_IRQHandler     ; TIM8 Trigger and Commutation and TIM14
{TODO}	DCD     TIM8_CC_IRQHandler                ; TIM8 Capture Compare
{TODO}	DCD     DMA1_Stream7_IRQHandler           ; DMA1 Stream7
{TODO}	DCD     FMC_IRQHandler                    ; FMC
{TODO}	DCD     SDIO_IRQHandler                   ; SDIO
{TODO}	DCD     TIM5_IRQHandler                   ; TIM5
{TODO}	DCD     SPI3_IRQHandler                   ; SPI3
		DCD     UART4_IRQHandler                  ; UART4
		DCD     UART5_IRQHandler                  ; UART5
{TODO}	DCD     TIM6_DAC_IRQHandler               ; TIM6 and DAC1&2 underrun errors
{TODO}	DCD     TIM7_IRQHandler                   ; TIM7
{TODO}	DCD     DMA2_Stream0_IRQHandler           ; DMA2 Stream 0
{TODO}	DCD     DMA2_Stream1_IRQHandler           ; DMA2 Stream 1
{TODO}	DCD     DMA2_Stream2_IRQHandler           ; DMA2 Stream 2
{TODO}	DCD     DMA2_Stream3_IRQHandler           ; DMA2 Stream 3
{TODO}	DCD     DMA2_Stream4_IRQHandler           ; DMA2 Stream 4
{TODO}	DCD     ETH_IRQHandler                    ; Ethernet
{TODO}	DCD     ETH_WKUP_IRQHandler               ; Ethernet Wakeup through EXTI line
{TODO}	DCD     CAN2_TX_IRQHandler                ; CAN2 TX
{TODO}	DCD     CAN2_RX0_IRQHandler               ; CAN2 RX0
{TODO}	DCD     CAN2_RX1_IRQHandler               ; CAN2 RX1
{TODO}	DCD     CAN2_SCE_IRQHandler               ; CAN2 SCE
{TODO}	DCD     OTG_FS_IRQHandler                 ; USB OTG FS
{TODO}	DCD     DMA2_Stream5_IRQHandler           ; DMA2 Stream 5
{TODO}	DCD     DMA2_Stream6_IRQHandler           ; DMA2 Stream 6
{TODO}	DCD     DMA2_Stream7_IRQHandler           ; DMA2 Stream 7
		DCD     USART6_IRQHandler                 ; USART6
{TODO}	DCD     I2C3_EV_IRQHandler                ; I2C3 event
{TODO}	DCD     I2C3_ER_IRQHandler                ; I2C3 error
{TODO}	DCD     OTG_HS_EP1_OUT_IRQHandler         ; USB OTG HS End Point 1 Out
{TODO}	DCD     OTG_HS_EP1_IN_IRQHandler          ; USB OTG HS End Point 1 In
{TODO}	DCD     OTG_HS_WKUP_IRQHandler            ; USB OTG HS Wakeup through EXTI
{TODO}	DCD     OTG_HS_IRQHandler                 ; USB OTG HS
{TODO}	DCD     DCMI_IRQHandler                   ; DCMI
{TODO}	DCD     0                                 ; Reserved
{TODO}	DCD     HASH_RNG_IRQHandler               ; Hash and Rng
{TODO}	DCD     FPU_IRQHandler                    ; FPU
*/

#endif

#ifdef _MCU_STM32
extern "C" {
	extern Handler_t FUNC_EXTI[];
	extern Handler_t FUNC_XART[];// 5 xart
	extern Handler_t FUNC_TIMx[];
	extern Handler_t FUNC_ADCx[];
}
#endif

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)

namespace uni {
	typedef enum {
	// Cortex-M4 Processor Exceptions Numbers
		IRQ_NonMaskableInt = -14,      // 2 Non Maskable Interrupt
		IRQ_HardFault = -13,           // 3 Cortex-M3   Hard Fault Interrupt
		IRQ_MemoryManagement = -12,    // 4 Cortex-M3/4 Memory Management Interrupt
		IRQ_BusFault = -11,            // 5 Cortex-M3/4 Bus Fault Interrupt
		IRQ_UsageFault = -10,          // 6 Cortex-M3/4 Usage Fault Interrupt
		IRQ_SVCall = -5,               //11 Cortex-M3/4 SV Call Interrupt
		IRQ_DebugMonitor = -4,         //12 Cortex-M3/4 Debug Monitor Interrupt
		IRQ_PendSV = -2,               //14 Cortex-M3/4 Pend SV Interrupt
		IRQ_SysTick = -1,              //15 Cortex-M3/4 System Tick Interrupt

		Request_None = 0,  // Special for WTDog Setting
		// STM32 specific Interrupt Numbers
		IRQ_WWDG = 0,                //Window WatchDog Interrupt
		IRQ_PVD = 1,                 //PVD through EXTI Line detection Interrupt
		IRQ_xTAMP = 2,               //(F1&4)Tamper, (F4)TimeStamp interrupts through the EXTI line
		IRQ_RTC = 3,                 //RTC normal(F1) anf Wakeup(F1&4)
		IRQ_FLASH = 4,               //FLASH global Interrupt
		IRQ_RCC = 5,                 //RCC global Interrupt
		IRQ_EXTI0 = 6,               //EXTI Line0 Interrupt
		IRQ_EXTI1 = 7,               //EXTI Line1 Interrupt
		IRQ_EXTI2 = 8,               //EXTI Line2 Interrupt
		IRQ_EXTI3 = 9,               //EXTI Line3 Interrupt
		IRQ_EXTI4 = 10,              //EXTI Line4 Interrupt

		IRQ_DMA1_Stream0 = 11,       //DMA1 Stream 0(F4) global Interrupt
		IRQ_DMA1_Stream1 = 12,       //DMA1 Stream 1(F4) global Interrupt
		IRQ_DMA1_Stream2 = 13,       //DMA1 Stream 2(F4) global Interrupt
		IRQ_DMA1_Stream3 = 14,       //DMA1 Stream 3(F4) global Interrupt
		IRQ_DMA1_Stream4 = 15,       //DMA1 Stream 4(F4) global Interrupt
		IRQ_DMA1_Stream5 = 16,       //DMA1 Stream 5(F4) global Interrupt
		IRQ_DMA1_Stream6 = 17,       //DMA1 Stream 6(F4) global Interrupt
		IRQ_DMA1_Channel1 = 11,      //DMA1 Channel1(F1) global Interrupt
		IRQ_DMA1_Channel2 = 12,      //DMA1 Channel2(F1) global Interrupt
		IRQ_DMA1_Channel3 = 13,      //DMA1 Channel3(F1) global Interrupt
		IRQ_DMA1_Channel4 = 14,      //DMA1 Channel4(F1) global Interrupt
		IRQ_DMA1_Channel5 = 15,      //DMA1 Channel5(F1) global Interrupt
		IRQ_DMA1_Channel6 = 16,      //DMA1 Channel6(F1) global Interrupt
		IRQ_DMA1_Channel7 = 17,      //DMA1 Channel7(F1) global Interrupt

		IRQ_ADC = 18,                //ADC1, ADC2 and ADC3(F4) global Interrupts
		IRQ_CAN1_TX = 19,            //CAN1 TX Interrupt
		IRQ_USB_HP_CAN1_TX = 19,     //USB  Device High Priority or CAN1 TX Interrupts
		IRQ_CAN1_RX0 = 20,           //CAN1 RX0 Interrupt
		IRQ_USB_LP_CAN1_RX0 = 20,    //USB  Device Low Priority or CAN1 RX0 Interrupts
		IRQ_CAN1_RX1 = 21,           //CAN1 RX1 Interrupt
		IRQ_CAN1_SCE = 22,           //CAN1 SCE Interrupt
		IRQ_EXTI9_5 = 23,            //External Line[9:5] Interrupts
		IRQ_TIM1BRK = 24,            //TIM1 Break Interrupt
		IRQ_TIM1BRK_TIM9 = 24,       //TIM1 Break interrupt and TIM9 global interrupt
		IRQ_TIM1UP = 25,             //TIM1 Update Interrupt
		IRQ_TIM1UP_TIM10 = 25,       //TIM1 Update Interrupt and TIM10 global interrupt
		IRQ_TIM1TRG_COM = 26,        //TIM1 Trigger and Commutation Interrupt
		IRQ_TIM1TRG_COM_TIM11 = 26,  //TIM1 Trigger and Commutation Interrupt and TIM11 global interrupt */
		IRQ_TIM1CC = 27,             //TIM1 Capture Compare Interrupt
		IRQ_TIM2 = 28,               //TIM2 global Interrupt
		IRQ_TIM3 = 29,               //TIM3 global Interrupt
		IRQ_TIM4 = 30,               //TIM4 global Interrupt
		IRQ_I2C1_EV = 31,            //I2C1 Event Interrupt
		IRQ_I2C1_ER = 32,            //I2C1 Error Interrupt
		IRQ_I2C2_EV = 33,            //I2C2 Event Interrupt
		IRQ_I2C2_ER = 34,            //I2C2 Error Interrupt
		IRQ_SPI1 = 35,               //SPI1 global Interrupt
		IRQ_SPI2 = 36,               //SPI2 global Interrupt
		IRQ_USART1 = 37,             //USART1 global Interrupt
		IRQ_USART2 = 38,             //USART2 global Interrupt
		IRQ_USART3 = 39,             //USART3 global Interrupt
		IRQ_EXTI15_10 = 40,          //External Line[15:10] Interrupts
		IRQ_RTC_Alarm = 41,          //RTC Alarm (A and B) through EXTI Line Interrupt
		IRQ_USBWakeUp = 42,          //USB Device WakeUp from suspend through EXTI Line Interrupt
		IRQ_OTG_FS_WKUP = 42,        //USB OTG FS Wakeup through EXTI line interrupt
		IRQ_TIM8_BRK = 43,           //TIM8 Break Interrupt
		IRQ_TIM8_BRK_TIM12 = 43,     //TIM8 Break Interrupt and TIM12 global interrupt
		IRQ_TIM8_UP_TIM13 = 44,      //TIM8 Update Interrupt and TIM13 global interrupt
		IRQ_TIM8_UP = 44,            //TIM8 Update Interrupt
		IRQ_TIM8_TRG_COM = 45,       //TIM8 Trigger and Commutation Interrupt
		IRQ_TIM8_TRG_COM_TIM14 = 45, //TIM8 Trigger and Commutation Interrupt and TIM14 global interrupt */
		IRQ_TIM8_CC = 46,            //TIM8 Capture Compare global interrupt
		IRQ_DMA1_Stream7 = 47,       //DMA1 (F4) Stream7 Interrupt
		IRQ_ADC3 = 47,               //ADC3 (F1) global Interrupt
		IRQ_FSMC = 48,               //FSMC global Interrupt
		IRQ_SDIO = 49,               //SDIO global Interrupt
		IRQ_TIM5 = 50,               //TIM5 global Interrupt
		IRQ_SPI3 = 51,               //SPI3 global Interrupt
		IRQ_UART4 = 52,              //UART4 global Interrupt
		IRQ_UART5 = 53,              //UART5 global Interrupt
		IRQ_TIM6 = 54,               //TIM6 global and DAC1&2 underrun error  interrupts (DAC)
		IRQ_TIM7 = 55,               //TIM7 global interrupt
		IRQ_DMA2_Stream0 = 56,       //DMA2 Stream 0 global Interrupt
		IRQ_DMA2_Stream1 = 57,       //DMA2 Stream 1 global Interrupt
		IRQ_DMA2_Stream2 = 58,       //DMA2 Stream 2 global Interrupt
		IRQ_DMA2_Stream3 = 59,       //DMA2 Stream 3 global Interrupt
		IRQ_DMA2_Channel1 = 56,      //DMA2 Channel1 global Interrupt
		IRQ_DMA2_Channel2 = 57,      //DMA2 Channel2 global Interrupt
		IRQ_DMA2_Channel3 = 58,      //DMA2 Channel3 global Interrupt
		IRQ_DMA2_Channel4_5 = 59,    //DMA2 Channel4 and Channel 5 global 
		//: F4 Area
		IRQ_DMA2_Stream4 = 60,       //DMA2 Stream 4 global Interrupt
		IRQ_ETH = 61,                //Ethernet global Interrupt
		IRQ_ETH_WKUP = 62,           //Ethernet Wakeup through EXTI line Interrupt
		IRQ_CAN2_TX = 63,            //CAN2 TX Interrupt
		IRQ_CAN2_RX0 = 64,           //CAN2 RX0 Interrupt
		IRQ_CAN2_RX1 = 65,           //CAN2 RX1 Interrupt
		IRQ_CAN2_SCE = 66,           //CAN2 SCE Interrupt
		IRQ_OTG_FS = 67,             //USB OTG FS global Interrupt
		IRQ_DMA2_Stream5 = 68,       //DMA2 Stream 5 global interrupt
		IRQ_DMA2_Stream6 = 69,       //DMA2 Stream 6 global interrupt
		IRQ_DMA2_Stream7 = 70,       //DMA2 Stream 7 global interrupt
		IRQ_USART6 = 71,             //USART6 global interrupt
		IRQ_I2C3_EV = 72,            //I2C3 event interrupt
		IRQ_I2C3_ER = 73,            //I2C3 error interrupt
		IRQ_OTG_HS_EP1_OUT = 74,     //USB OTG HS End Point 1 Out global interrupt
		IRQ_OTG_HS_EP1_IN = 75,      //USB OTG HS End Point 1 In global interrupt
		IRQ_OTG_HS_WKUP = 76,        //USB OTG HS Wakeup through EXTI interrupt
		IRQ_OTG_HS = 77,             //USB OTG HS global interrupt
		IRQ_DCMI = 78,               //DCMI global interrupt
		IRQ_RNG = 80,                //RNG global Interrupt
		IRQ_FPU = 81                 //FPU global interrupt
	} Request_t;
}
#elif defined(_MCU_STM32H7x)

namespace uni {
	typedef enum
	{
	//*****  Cortex-M Processor Exceptions Numbers
		IRQ_NonMaskableInt         = -14,    // [ ] 2  Non Maskable Interrupt                                          
		IRQ_HardFault              = -13,    // [v] 4  Cortex-M Memory Management Interrupt                            
		IRQ_MemoryManagement       = -12,    // [ ] 4  Cortex-M Memory Management Interrupt                            
		IRQ_BusFault               = -11,    // [ ] 5  Cortex-M Bus Fault Interrupt                                    
		IRQ_UsageFault             = -10,    // [ ] 6  Cortex-M Usage Fault Interrupt                                  
		IRQ_SVCall                 = -5,     // [ ] 11 Cortex-M SV Call Interrupt                                     
		IRQ_DebugMonitor           = -4,     // [ ] 12 Cortex-M Debug Monitor Interrupt                               
		IRQ_PendSV                 = -2,     // [ ] 14 Cortex-M Pend SV Interrupt                                     
		IRQ_SysTick                = -1,     // [x] 15 Cortex-M System Tick Interrupt                                 
	//*****  STM32 specific Interrupt Numbers
		Request_None               = nil,
		IRQ_WWDG = 0,      // [ ] Window WatchDog Interrupt                                         
		IRQ_PVD_AVD                = 1,      // [ ] PVD/AVD through EXTI Line detection Interrupt                     
		IRQ_TAMP_STAMP             = 2,      // [ ] Tamper and TimeStamp interrupts through the EXTI line             
		IRQ_RTC_WKUP               = 3,      // [ ] RTC Wakeup interrupt through the EXTI line                        
		IRQ_FLASH                  = 4,      // [ ] FLASH global Interrupt                                            
		IRQ_RCC                    = 5,      // [ ] RCC global Interrupt                                              
		IRQ_EXTI0                  = 6,      // [ ] EXTI Line0 Interrupt                                              
		IRQ_EXTI1                  = 7,      // [ ] EXTI Line1 Interrupt                                              
		IRQ_EXTI2                  = 8,      // [ ] EXTI Line2 Interrupt                                              
		IRQ_EXTI3                  = 9,      // [ ] EXTI Line3 Interrupt                                              
		IRQ_EXTI4                  = 10,     // [ ] EXTI Line4 Interrupt                                              
		IRQ_DMA1_Stream0           = 11,     // [ ] DMA1 Stream 0 global Interrupt                                    
		IRQ_DMA1_Stream1           = 12,     // [ ] DMA1 Stream 1 global Interrupt                                    
		IRQ_DMA1_Stream2           = 13,     // [ ] DMA1 Stream 2 global Interrupt                                    
		IRQ_DMA1_Stream3           = 14,     // [ ] DMA1 Stream 3 global Interrupt                                    
		IRQ_DMA1_Stream4           = 15,     // [ ] DMA1 Stream 4 global Interrupt                                    
		IRQ_DMA1_Stream5           = 16,     // [ ] DMA1 Stream 5 global Interrupt                                    
		IRQ_DMA1_Stream6           = 17,     // [ ] DMA1 Stream 6 global Interrupt                                    
		IRQ_ADC                    = 18,     // [ ] ADC1 and  ADC2 global Interrupts                                  
		IRQ_FDCAN1_IT0             = 19,     // [ ] FDCAN1 Interrupt line 0                                           
		IRQ_FDCAN2_IT0             = 20,     // [ ] FDCAN2 Interrupt line 0                                           
		IRQ_FDCAN1_IT1             = 21,     // [ ] FDCAN1 Interrupt line 1                                           
		IRQ_FDCAN2_IT1             = 22,     // [ ] FDCAN2 Interrupt line 1                                           
		IRQ_EXTI9_5                = 23,     // [ ] External Line[9:5] Interrupts                                     
		IRQ_TIM1_BRK               = 24,     // [ ] TIM1 Break Interrupt                                              
		IRQ_TIM1_UP                = 25,     // [ ] TIM1 Update Interrupt                                             
		IRQ_TIM1_TRG_COM           = 26,     // [ ] TIM1 Trigger and Commutation Interrupt                            
		IRQ_TIM1_CC                = 27,     // [ ] TIM1 Capture Compare Interrupt                                    
		IRQ_TIM2                   = 28,     // [ ] TIM2 global Interrupt                                             
		IRQ_TIM3                   = 29,     // [ ] TIM3 global Interrupt                                             
		IRQ_TIM4                   = 30,     // [ ] TIM4 global Interrupt                                             
		IRQ_I2C1_EV                = 31,     // [ ] I2C1 Event Interrupt                                              
		IRQ_I2C1_ER                = 32,     // [ ] I2C1 Error Interrupt                                              
		IRQ_I2C2_EV                = 33,     // [ ] I2C2 Event Interrupt                                              
		IRQ_I2C2_ER                = 34,     // [ ] I2C2 Error Interrupt                                              
		IRQ_SPI1                   = 35,     // [ ] SPI1 global Interrupt                                             
		IRQ_SPI2                   = 36,     // [ ] SPI2 global Interrupt                                             
		IRQ_USART1                 = 37,     // [x] USART1 global Interrupt                                           
		IRQ_USART2                 = 38,     // [x] USART2 global Interrupt                                           
		IRQ_USART3                 = 39,     // [x] USART3 global Interrupt                                           
		IRQ_EXTI15_10              = 40,     // [ ] External Line[15:10] Interrupts                                   
		IRQ_RTC_Alarm              = 41,     // [ ] RTC Alarm (A and B) through EXTI Line Interrupt                    
		IRQ_TIM8_BRK_TIM12         = 43,     // [ ] TIM8 Break Interrupt and TIM12 global interrupt                   
		IRQ_TIM8_UP_TIM13          = 44,     // [ ] TIM8 Update Interrupt and TIM13 global interrupt                  
		IRQ_TIM8_TRG_COM_TIM14     = 45,     // [ ] TIM8 Trigger and Commutation Interrupt and TIM14 global interrupt 
		IRQ_TIM8_CC                = 46,     // [ ] TIM8 Capture Compare Interrupt                                    
		IRQ_DMA1_Stream7           = 47,     // [ ] DMA1 Stream7 Interrupt                                            
		IRQ_FMC                    = 48,     // [ ] FMC global Interrupt                                              
		IRQ_SDMMC1                 = 49,     // [ ] SDMMC1 global Interrupt                                           
		IRQ_TIM5                   = 50,     // [ ] TIM5 global Interrupt                                             
		IRQ_SPI3                   = 51,     // [ ] SPI3 global Interrupt                                             
		IRQ_UART4                  = 52,     // [x] UART4 global Interrupt                                            
		IRQ_UART5                  = 53,     // [x] UART5 global Interrupt                                            
		IRQ_TIM6_DAC               = 54,     // [ ] TIM6 global and DAC1&2 underrun error  interrupts                 
		IRQ_TIM7                   = 55,     // [ ] TIM7 global interrupt                                             
		IRQ_DMA2_Stream0           = 56,     // [ ] DMA2 Stream 0 global Interrupt                                  
		IRQ_DMA2_Stream1           = 57,     // [ ] DMA2 Stream 1 global Interrupt                                  
		IRQ_DMA2_Stream2           = 58,     // [ ] DMA2 Stream 2 global Interrupt                                  
		IRQ_DMA2_Stream3           = 59,     // [ ] DMA2 Stream 3 global Interrupt                                  
		IRQ_DMA2_Stream4           = 60,     // [ ] DMA2 Stream 4 global Interrupt                                  
		IRQ_ETH                    = 61,     // [ ] Ethernet global Interrupt                                         
		IRQ_ETH_WKUP               = 62,     // [ ] Ethernet Wakeup through EXTI line Interrupt                       
		IRQ_FDCAN_CAL              = 63,     // [ ] FDCAN Calibration unit Interrupt                                  
		IRQ_DMA2_Stream5           = 68,     // [ ] DMA2 Stream 5 global interrupt                                    
		IRQ_DMA2_Stream6           = 69,     // [ ] DMA2 Stream 6 global interrupt                                    
		IRQ_DMA2_Stream7           = 70,     // [ ] DMA2 Stream 7 global interrupt                                    
		IRQ_USART6                 = 71,     // [x] USART6 global interrupt                                           
		IRQ_I2C3_EV                = 72,     // [ ] I2C3 event interrupt                                              
		IRQ_I2C3_ER                = 73,     // [ ] I2C3 error interrupt                                              
		IRQ_OTG_HS_EP1_OUT         = 74,     // [ ] USB OTG HS End Point 1 Out global interrupt                       
		IRQ_OTG_HS_EP1_IN          = 75,     // [ ] USB OTG HS End Point 1 In global interrupt                        
		IRQ_OTG_HS_WKUP            = 76,     // [ ] USB OTG HS Wakeup through EXTI interrupt                          
		IRQ_OTG_HS                 = 77,     // [ ] USB OTG HS global interrupt                                       
		IRQ_DCMI                   = 78,     // [ ] DCMI global interrupt                                             
		IRQ_RNG                    = 80,     // [ ] RNG global interrupt                                              
		IRQ_FPU                    = 81,     // [ ] FPU global interrupt                                              
		IRQ_UART7                  = 82,     // [x] UART7 global interrupt                                            
		IRQ_UART8                  = 83,     // [x] UART8 global interrupt                                            
		IRQ_SPI4                   = 84,     // [ ] SPI4 global Interrupt                                             
		IRQ_SPI5                   = 85,     // [ ] SPI5 global Interrupt                                             
		IRQ_SPI6                   = 86,     // [ ] SPI6 global Interrupt                                             
		IRQ_SAI1                   = 87,     // [ ] SAI1 global Interrupt                                             
		IRQ_LTDC                   = 88,     // [ ] LTDC global Interrupt                                             
		IRQ_LTDC_ER                = 89,     // [ ] LTDC Error global Interrupt                                       
		IRQ_DMA2D                  = 90,     // [ ] DMA2D global Interrupt                                            
		IRQ_SAI2                   = 91,     // [ ] SAI2 global Interrupt                                             
		IRQ_QUADSPI                = 92,     // [ ] Quad SPI global interrupt                                         
		IRQ_LPTIM1                 = 93,     // [ ] LP TIM1 interrupt                                                 
		IRQ_CEC                    = 94,     // [ ] HDMI-CEC global Interrupt                                         
		IRQ_I2C4_EV                = 95,     // [ ] I2C4 Event Interrupt                                              
		IRQ_I2C4_ER                = 96,     // [ ] I2C4 Error Interrupt                                              
		IRQ_SPDIF_RX               = 97,     // [ ] SPDIF-RX global Interrupt                                         
		IRQ_OTG_FS_EP1_OUT         = 98,     // [ ] USB OTG HS2 global interrupt                                      
		IRQ_OTG_FS_EP1_IN          = 99,     // [ ] USB OTG HS2 End Point 1 Out global interrupt                      
		IRQ_OTG_FS_WKUP            = 100,    // [ ] USB OTG HS2 End Point 1 In global interrupt                       
		IRQ_OTG_FS                 = 101,    // [ ] USB OTG HS2 Wakeup through EXTI interrupt                         
		IRQ_DMAMUX1_OVR	           = 102,    // [ ] DMAMUX1 Overrun interrupt                                          
		IRQ_HRTIM1_Master          = 103,    // [ ] HRTIM Master Timer global Interrupts                              
		IRQ_HRTIM1_TIMA            = 104,    // [ ] HRTIM Timer A global Interrupt                                    
		IRQ_HRTIM1_TIMB            = 105,    // [ ] HRTIM Timer B global Interrupt                                    
		IRQ_HRTIM1_TIMC            = 106,    // [ ] HRTIM Timer C global Interrupt                                    
		IRQ_HRTIM1_TIMD            = 107,    // [ ] HRTIM Timer D global Interrupt                                    
		IRQ_HRTIM1_TIME            = 108,    // [ ] HRTIM Timer E global Interrupt                                    
		IRQ_HRTIM1_FLT             = 109,    // [ ] HRTIM Fault global Interrupt                                      
		IRQ_DFSDM1_FLT0            = 110,    // [ ] DFSDM Filter1 Interrupt                                            
		IRQ_DFSDM1_FLT1            = 111,    // [ ] DFSDM Filter2 Interrupt                                            
		IRQ_DFSDM1_FLT2            = 112,    // [ ] DFSDM Filter3 Interrupt                                            
		IRQ_DFSDM1_FLT3            = 113,    // [ ] DFSDM Filter4 Interrupt                                            
		IRQ_SAI3                   = 114,    // [ ] SAI3 global Interrupt                                             
		IRQ_SWPMI1                 = 115,    // [ ] Serial Wire Interface 1 global interrupt                          
		IRQ_TIM15                  = 116,    // [ ] TIM15 global Interrupt                                            
		IRQ_TIM16                  = 117,    // [ ] TIM16 global Interrupt                                            
		IRQ_TIM17                  = 118,    // [ ] TIM17 global Interrupt                                            
		IRQ_MDIOS_WKUP             = 119,    // [ ] MDIOS Wakeup  Interrupt                                           
		IRQ_MDIOS                  = 120,    // [ ] MDIOS global Interrupt                                            
		IRQ_JPEG                   = 121,    // [ ] JPEG global Interrupt                                             
		IRQ_MDMA                   = 122,    // [ ] MDMA global Interrupt                                             
		IRQ_SDMMC2                 = 124,    // [ ] SDMMC2 global Interrupt                                           
		IRQ_HSEM1                  = 125,    // [ ] HSEM1 global Interrupt                                            
		IRQ_ADC3                   = 127,    // [ ] ADC3 global Interrupt                                             
		IRQ_DMAMUX2_OVR            = 128,    // [ ] DMAMUX2 Overrun interrupt                                          
		IRQ_BDMA_Channel0          = 129,    // [ ] BDMA Channel 0 global Interrupt                                   
		IRQ_BDMA_Channel1          = 130,    // [ ] BDMA Channel 1 global Interrupt                                   
		IRQ_BDMA_Channel2          = 131,    // [ ] BDMA Channel 2 global Interrupt                                   
		IRQ_BDMA_Channel3          = 132,    // [ ] BDMA Channel 3 global Interrupt                                   
		IRQ_BDMA_Channel4          = 133,    // [ ] BDMA Channel 4 global Interrupt                                   
		IRQ_BDMA_Channel5          = 134,    // [ ] BDMA Channel 5 global Interrupt                                   
		IRQ_BDMA_Channel6          = 135,    // [ ] BDMA Channel 6 global Interrupt                                   
		IRQ_BDMA_Channel7          = 136,    // [ ] BDMA Channel 7 global Interrupt                                   
		IRQ_COMP                   = 137,    // [ ] COMP global Interrupt                                             
		IRQ_LPTIM2                 = 138,    // [ ] LP TIM2 global interrupt                                          
		IRQ_LPTIM3                 = 139,    // [ ] LP TIM3 global interrupt                                          
		IRQ_LPTIM4                 = 140,    // [ ] LP TIM4 global interrupt                                          
		IRQ_LPTIM5                 = 141,    // [ ] LP TIM5 global interrupt                                          
		IRQ_LPUART1                = 142,    // [ ] LP UART1 interrupt                                                
		IRQ_CRS                    = 144,    // [ ] Clock Recovery Global Interrupt                                   
		IRQ_SAI4                   = 146,    // [ ] SAI4 global interrupt                                             
		IRQ_WAKEUP_PIN             = 149,    // [ ] Interrupt for all 6 wake-up pins                                  
	} Request_t;
}

#elif defined(_MPU_STM32MP13)
#include "interrupt_stm32mp13.h"
#include "../../../c/driver/interrupt/GIC.h"
#endif

#endif

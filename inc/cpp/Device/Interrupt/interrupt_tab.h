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
#ifndef _INC_Interrupt_Table_X
#define _INC_Interrupt_Table_X

extern "C" {

#if 0
//
#elif defined(_MCU_STM32F10x)
/* Borrow Idens from Official
__Vectors
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
{TODO}	DCD     DMA1_Channel1_IRQHandler   ; DMA1 Channel 1
{TODO}	DCD     DMA1_Channel2_IRQHandler   ; DMA1 Channel 2
{TODO}	DCD     DMA1_Channel3_IRQHandler   ; DMA1 Channel 3
{TODO}	DCD     DMA1_Channel4_IRQHandler   ; DMA1 Channel 4
{TODO}	DCD     DMA1_Channel5_IRQHandler   ; DMA1 Channel 5
{TODO}	DCD     DMA1_Channel6_IRQHandler   ; DMA1 Channel 6
{TODO}	DCD     DMA1_Channel7_IRQHandler   ; DMA1 Channel 7
{TODO}	DCD     ADC1_2_IRQHandler          ; ADC1 & ADC2
{TODO}	DCD     USB_HP_CAN1_TX_IRQHandler  ; USB High Priority or CAN1 TX
{TODO}	DCD     USB_LP_CAN1_RX0_IRQHandler ; USB Low  Priority or CAN1 RX0
{TODO}	DCD     CAN1_RX1_IRQHandler        ; CAN1 RX1
{TODO}	DCD     CAN1_SCE_IRQHandler        ; CAN1 SCE
		DCD     EXTI9_5_IRQHandler         ; EXTI Line 9..5
{TODO}	DCD     TIM1_BRK_IRQHandler        ; TIM1 Break
{TODO}	DCD     TIM1_UP_IRQHandler         ; TIM1 Update
{TODO}	DCD     TIM1_TRG_COM_IRQHandler    ; TIM1 Trigger and Commutation
{TODO}	DCD     TIM1_CC_IRQHandler         ; TIM1 Capture Compare
{TODO}	DCD     TIM2_IRQHandler            ; TIM2
{TODO}	DCD     TIM3_IRQHandler            ; TIM3
{TODO}	DCD     TIM4_IRQHandler            ; TIM4
{TODO}	DCD     I2C1_EV_IRQHandler         ; I2C1 Event
{TODO}	DCD     I2C1_ER_IRQHandler         ; I2C1 Error
{TODO}	DCD     I2C2_EV_IRQHandler         ; I2C2 Event
{TODO}	DCD     I2C2_ER_IRQHandler         ; I2C2 Error
{TODO}	DCD     SPI1_IRQHandler            ; SPI1
{TODO}	DCD     SPI2_IRQHandler            ; SPI2
{TODO}	DCD     USART1_IRQHandler          ; USART1
{TODO}	DCD     USART2_IRQHandler          ; USART2
{TODO}	DCD     USART3_IRQHandler          ; USART3
		DCD     EXTI15_10_IRQHandler       ; EXTI Line 15..10
{TODO}	DCD     RTC_Alarm_IRQHandler       ; RTC Alarm through EXTI Line
{TODO}	DCD     USBWakeUp_IRQHandler       ; USB Wakeup from suspend
{TODO}	DCD     TIM8_BRK_IRQHandler        ; TIM8 Break
{TODO}	DCD     TIM8_UP_IRQHandler         ; TIM8 Update
{TODO}	DCD     TIM8_TRG_COM_IRQHandler    ; TIM8 Trigger and Commutation
{TODO}	DCD     TIM8_CC_IRQHandler         ; TIM8 Capture Compare
{TODO}	DCD     ADC3_IRQHandler            ; ADC3
{TODO}	DCD     FSMC_IRQHandler            ; FSMC
{TODO}	DCD     SDIO_IRQHandler            ; SDIO
{TODO}	DCD     TIM5_IRQHandler            ; TIM5
{TODO}	DCD     SPI3_IRQHandler            ; SPI3
{TODO}	DCD     UART4_IRQHandler           ; UART4
{TODO}	DCD     UART5_IRQHandler           ; UART5
{TODO}	DCD     TIM6_IRQHandler            ; TIM6
{TODO}	DCD     TIM7_IRQHandler            ; TIM7
{TODO}	DCD     DMA2_Channel1_IRQHandler   ; DMA2 Channel1
{TODO}	DCD     DMA2_Channel2_IRQHandler   ; DMA2 Channel2
{TODO}	DCD     DMA2_Channel3_IRQHandler   ; DMA2 Channel3
{TODO}	DCD     DMA2_Channel4_5_IRQHandler ; DMA2 Channel4 & Channel5
__Vectors_End
*/
	typedef void (*Handler_t)(void);
	extern Handler_t FUNC_EXTI[];

	void EXTI0_IRQHandler(void);




	
	#else

#endif
}

#endif

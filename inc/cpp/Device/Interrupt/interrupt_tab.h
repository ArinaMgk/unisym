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

#if defined(_MCCA) //{TEMP} Use CPL

#include "../../../c/stdinc.h"
typedef enum
{
	IRQ_PIT = 0, // 8253/8254 PIT (Programmable Interval Timer)
	IRQ_Keyboard = 1, // 8042? Keyboard Controller
	IRQ_Cascade = 2, // 8259A Cascade
	IRQ_Serial = 3, // default ethernet interrupt vector, or RS232 interrupt vector for port 2 {??? 16450 Serial Port}
	IRQ_RS232_Port1 = 4, //  RS232 interrupt vector for port 1
	IRQ_XT_WINI = 5, // ???
	IRQ_Floppy = 6, // 8250 Floppy Disk Controller
	_TODO
} Request_Master_t;

#elif defined(_MCU_STM32F10x)

typedef enum
{
	//  Cortex-M3 Processor Exceptions Numbers
	IRQ_NonMaskableInt = -14,   // 2 Non Maskable Interrupt                   
	IRQ_HardFault = -13,   // 3 Cortex-M3 Hard Fault Interrupt           
	IRQ_MemoryManagement = -12,   // 4 Cortex-M3 Memory Management Interrupt    
	IRQ_BusFault = -11,   // 5 Cortex-M3 Bus Fault Interrupt            
	IRQ_UsageFault = -10,   // 6 Cortex-M3 Usage Fault Interrupt          
	IRQ_SVCall = -5,    // 11 Cortex-M3 SV Call Interrupt             
	IRQ_DebugMonitor = -4,    // 12 Cortex-M3 Debug Monitor Interrupt       
	IRQ_PendSV = -2,    // 14 Cortex-M3 Pend SV Interrupt             
	IRQ_SysTick = -1,    // 15 Cortex-M3 System Tick Interrupt         
	//  STM32 specific Interrupt Numbers
	IRQ_WWDG = 0,      // Window WatchDog Interrupt                  
	IRQ_PVD = 1,       // PVD through EXTI Line detection Interrupt  
	IRQ_TAMPER = 2,    // Tamper Interrupt                           
	IRQ_RTC = 3,       // RTC global Interrupt                       
	IRQ_FLASH = 4,     // FLASH global Interrupt                     
	IRQ_RCC = 5,       // RCC global Interrupt                       
	IRQ_EXTI0 = 6,     // EXTI Line0 Interrupt                       
	IRQ_EXTI1 = 7,     // EXTI Line1 Interrupt                       
	IRQ_EXTI2 = 8,     // EXTI Line2 Interrupt                       
	IRQ_EXTI3 = 9,     // EXTI Line3 Interrupt                       
	IRQ_EXTI4 = 10,    // EXTI Line4 Interrupt                       
	IRQ_DMA1_Channel1 = 11,    // DMA1 Channel 1 global Interrupt            
	IRQ_DMA1_Channel2 = 12,    // DMA1 Channel 2 global Interrupt            
	IRQ_DMA1_Channel3 = 13,    // DMA1 Channel 3 global Interrupt            
	IRQ_DMA1_Channel4 = 14,    // DMA1 Channel 4 global Interrupt            
	IRQ_DMA1_Channel5 = 15,    // DMA1 Channel 5 global Interrupt            
	IRQ_DMA1_Channel6 = 16,    // DMA1 Channel 6 global Interrupt            
	IRQ_DMA1_Channel7 = 17,    // DMA1 Channel 7 global Interrupt            
	IRQ_ADC1_2 = 18,    // ADC1 and ADC2 global Interrupt             
	IRQ_USB_HP_CAN1_TX = 19,    // USB Device High Priority or CAN1 TX Interrupts  
	IRQ_USB_LP_CAN1_RX0 = 20,    // USB Device Low Priority or CAN1 RX0 Interrupts  
	IRQ_CAN1_RX1 = 21,   // CAN1 RX1 Interrupt                         
	IRQ_CAN1_SCE = 22,   // CAN1 SCE Interrupt                         
	IRQ_EXTI9_5 = 23,    // External Line[9:5] Interrupts              
	IRQ_TIM1_BRK = 24,   // TIM1 Break Interrupt                       
	IRQ_TIM1_UP = 25,    // TIM1 Update Interrupt                      
	IRQ_TIM1_TRG_COM = 26,    // TIM1 Trigger and Commutation Interrupt     
	IRQ_TIM1_CC = 27,    // TIM1 Capture Compare Interrupt             
	IRQ_TIM2 = 28,       // TIM2 global Interrupt                      
	IRQ_TIM3 = 29,       // TIM3 global Interrupt                      
	IRQ_TIM4 = 30,       // TIM4 global Interrupt                      
	IRQ_I2C1_EV = 31,    // I2C1 Event Interrupt                       
	IRQ_I2C1_ER = 32,    // I2C1 Error Interrupt                       
	IRQ_I2C2_EV = 33,    // I2C2 Event Interrupt                       
	IRQ_I2C2_ER = 34,    // I2C2 Error Interrupt                       
	IRQ_SPI1 = 35,       // SPI1 global Interrupt                      
	IRQ_SPI2 = 36,       // SPI2 global Interrupt                      
	IRQ_USART1 = 37,    // USART1 global Interrupt                    
	IRQ_USART2 = 38,    // USART2 global Interrupt                    
	IRQ_USART3 = 39,    // USART3 global Interrupt                    
	IRQ_EXTI15_10 = 40,    // External Line[15:10] Interrupts            
	IRQ_RTC_Alarm = 41,    // RTC Alarm through EXTI Line Interrupt      
	IRQ_USBWakeUp = 42,    // USB Device WakeUp from suspend through EXTI Line Interrupt */
	IRQ_TIM8_BRK = 43,    // TIM8 Break Interrupt                       
	IRQ_TIM8_UP = 44,    // TIM8 Update Interrupt                      
	IRQ_TIM8_TRG_COM = 45,    // TIM8 Trigger and Commutation Interrupt     
	IRQ_TIM8_CC = 46,    // TIM8 Capture Compare Interrupt             
	IRQ_ADC3 = 47,    // ADC3 global Interrupt                      
	IRQ_FSMC = 48,    // FSMC global Interrupt                      
	IRQ_SDIO = 49,    // SDIO global Interrupt                      
	IRQ_TIM5 = 50,    // TIM5 global Interrupt                      
	IRQ_SPI3 = 51,    // SPI3 global Interrupt                      
	IRQ_UART4 = 52,    // UART4 global Interrupt                     
	IRQ_UART5 = 53,    // UART5 global Interrupt                     
	IRQ_TIM6 = 54,    // TIM6 global Interrupt                      
	IRQ_TIM7 = 55,    // TIM7 global Interrupt                      
	IRQ_DMA2_Channel1 = 56,    // DMA2 Channel 1 global Interrupt            
	IRQ_DMA2_Channel2 = 57,    // DMA2 Channel 2 global Interrupt            
	IRQ_DMA2_Channel3 = 58,    // DMA2 Channel 3 global Interrupt            
	IRQ_DMA2_Channel4_5 = 59,    // DMA2 Channel 4 and Channel 5 global Interrupt   
} Request_t;

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
		DCD     TIM6_IRQHandler            ; TIM6
{TODO}	DCD     TIM7_IRQHandler            ; TIM7
{TODO}	DCD     DMA2_Channel1_IRQHandler   ; DMA2 Channel1
{TODO}	DCD     DMA2_Channel2_IRQHandler   ; DMA2 Channel2
{TODO}	DCD     DMA2_Channel3_IRQHandler   ; DMA2 Channel3
{TODO}	DCD     DMA2_Channel4_5_IRQHandler ; DMA2 Channel4 & Channel5
__Vectors_End
*/
typedef void (*Handler_t)(void);

extern "C" {
extern Handler_t FUNC_EXTI[];
extern Handler_t FUNC_TIMx[];
extern Handler_t FUNC_ADCx[];

void EXTI0_IRQHandler(void);
// ...
}



	
#else

#endif


#endif

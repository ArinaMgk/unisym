
#if 0

#elif defined(_MCU_STM32H7x)


_Comment("Peripheral_memory_map")
#define D1_ITCMRAM_BASE      (0x00000000) // 64KB RAM reserved for CPU execution/instruction accessible over ITCM  */
#define D1_ITCMICP_BASE      (0x00100000) // (up to 128KB) embedded Test FLASH memory accessible over ITCM         */
#define D1_DTCMRAM_BASE      (0x20000000) // 128KB system data RAM accessible over DTCM                            */
#define D1_AXIFLASH_BASE     (0x08000000) // (up to 2 MB) embedded FLASH memory accessible over AXI                */
#define D1_AXIICP_BASE       (0x1FF00000) // (up to 128KB) embedded Test FLASH memory accessible over AXI          */
#define D1_AXISRAM_BASE      (0x24000000) // (up to 512KB) system data RAM accessible over over AXI                */
#define D2_AXISRAM_BASE      (0x10000000) // (up to 288KB) system data RAM accessible over over AXI                */
#define D2_AHBSRAM_BASE      (0x30000000) // (up to 288KB) system data RAM accessible over over AXI->AHB Bridge    */
#define D3_BKPSRAM_BASE      (0x38800000) // Backup SRAM(4 KB) over AXI->AHB Bridge                                */
#define D3_SRAM_BASE         (0x38000000) // Backup SRAM(64 KB) over AXI->AHB Bridge                               */ 
#define PERIPH_BASE          (0x40000000) // AHB/ABP Peripherals                                                   */
#define QSPI_BASE            (0x90000000) // QSPI memories  accessible over AXI                                    */
#define FLASH_BANK1_BASE     (0x08000000) // (up to 1 MB) Flash Bank1 accessible over AXI                          */ 
#define FLASH_BANK2_BASE     (0x08100000) // (up to 1 MB) Flash Bank2 accessible over AXI                          */ 
#define FLASH_END            (0x081FFFFF) // FLASH end address                                                     */
#define FLASH_OTP_BANK1_BASE (0x1FF00000) // (up to 128KB) embedded FLASH Bank1 OTP Area                           */ 
#define FLASH_OTP_BANK1_END  (0x1FF1FFFF) // End address of : (up to 128KB) embedded FLASH Bank1 OTP Area          */ 
#define FLASH_OTP_BANK2_BASE (0x1FF40000) // (up to 128KB) embedded FLASH Bank2 OTP Area                           */ 
#define FLASH_OTP_BANK2_END  (0x1FF5FFFF) // End address of : (up to 128KB) embedded FLASH Bank2 OTP Area          */ 

#define FLASH_BASE                FLASH_BANK1_BASE

#define D2_APB1PERIPH_BASE        PERIPH_BASE
#define D2_APB2PERIPH_BASE       (PERIPH_BASE + 0x00010000)
#define D2_AHB1PERIPH_BASE       (PERIPH_BASE + 0x00020000)
#define D2_AHB2PERIPH_BASE       (PERIPH_BASE + 0x08020000)

#define D1_APB1PERIPH_BASE       (PERIPH_BASE + 0x10000000)
#define D1_AHB1PERIPH_BASE       (PERIPH_BASE + 0x12000000)

#define D3_APB1PERIPH_BASE       (PERIPH_BASE + 0x18000000)
#define D3_AHB1PERIPH_BASE       (PERIPH_BASE + 0x18020000)

#define APB1PERIPH_BASE        PERIPH_BASE
#define APB2PERIPH_BASE       (PERIPH_BASE + 0x00010000)
#define AHB1PERIPH_BASE       (PERIPH_BASE + 0x00020000)
#define AHB2PERIPH_BASE       (PERIPH_BASE + 0x08000000)

#elif defined(_MPU_STM32MP13)

#define AHB_SRAM                ((uint32_t)0x30000000UL) // (up to 32KB) system data RAM accessible over over AHB

#define SYSRAM_BASE             ((uint32_t)0x2FFE0000UL) // (up to 128KB) System RAM accessible over over AXI
#define SRAM_BASE               AHB_SRAM
#define PERIPH_BASE             ((uint32_t)0x40000000UL) // AHB/ABP Peripherals
#define AXI_BUS_MEMORY_BASE     ((uint32_t)0x60000000UL) // AXI Bus

#define FMC_NOR_MEM_BASE        (AXI_BUS_MEMORY_BASE)              // FMC NOR memories  accessible over AXI
#define QSPI_MEM_BASE           (AXI_BUS_MEMORY_BASE + 0x10000000UL) // QSPI memories  accessible over AXI
#define FMC_NAND_MEM_BASE       (AXI_BUS_MEMORY_BASE + 0x20000000UL) // FMC NAND memories  accessible over AXI
#define DRAM_MEM_BASE           (AXI_BUS_MEMORY_BASE + 0x60000000UL) // DRAM (DDR) over AXI

// ---- Device electronic signature memory map ----
#define UID_BASE                (0x5C005234UL)// Unique Device ID register base address
#define PACKAGE_BASE            (0x5C005240UL)// Package Data register base address
#define RPN_BASE                (0x5C005204UL)// Device Part Number register base address
#define DV_BASE                 (0x50081000UL)// Device Version register base address

// ---- Peripheral Memory Map ----

#define APB1_PERIPH_BASE        (PERIPH_BASE + 0x00000000UL)
#define APB2_PERIPH_BASE        (PERIPH_BASE + 0x04000000UL)
#define AHB2_PERIPH_BASE        (PERIPH_BASE + 0x08000000UL)
#define APB6_PERIPH_BASE        (PERIPH_BASE + 0x0C000000UL)
#define AHB4_PERIPH_BASE        (PERIPH_BASE + 0x10000000UL)
#define APB3_PERIPH_BASE        (PERIPH_BASE + 0x10020000UL)
#define APB_DEBUG_PERIPH_BASE   (PERIPH_BASE + 0x10080000UL)
#define AHB5_PERIPH_BASE        (PERIPH_BASE + 0x14000000UL)
#define GPV_PERIPH_BASE         (PERIPH_BASE + 0x17000000UL)
#define AHB6_PERIPH_BASE        (PERIPH_BASE + 0x18000000UL)
#define APB4_PERIPH_BASE        (PERIPH_BASE + 0x1A000000UL)
#define APB5_PERIPH_BASE        (PERIPH_BASE + 0x1C000000UL)



#endif

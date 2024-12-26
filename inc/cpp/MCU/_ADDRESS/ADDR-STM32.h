
#if defined(_MPU_STM32MP13)

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

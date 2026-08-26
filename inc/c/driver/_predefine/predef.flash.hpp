
#ifndef _INC_DEV_PREDEF_FLASH
#define _INC_DEV_PREDEF_FLASH

#if defined(_MCU_STM32H7x)
#include "../../../cpp/MCU/_ADDRESS/ADDR-STM32.h"
#include "../../../cpp/Device/SysTick"
#endif

#if defined(_MCU_STM32H7x)
namespace uni {

	// FLASH_CR bits (bank1 CR1 / bank2 CR2 share layout, STM32H743)
	#define _FLASH_CR_POS_LOCK  0
	#define _FLASH_CR_POS_PG    1
	#define _FLASH_CR_POS_SER   2
	#define _FLASH_CR_POS_BER   3
	#define _FLASH_CR_POS_PSIZE 4 // 2b program/erase parallelism (64-bit = both bits set)
	#define _FLASH_CR_POS_FW    6
	#define _FLASH_CR_POS_START 7
	#define _FLASH_CR_POS_SNB   8 // 3b sector number 0..7
	#define _FLASH_CR_PSIZE_DOUBLEWORD (0x3U << _FLASH_CR_POS_PSIZE)

	// FLASH_SR bits (bank1 SR1 / bank2 SR2)
	#define _FLASH_SR_POS_BSY     0
	#define _FLASH_SR_POS_WBNE    1
	#define _FLASH_SR_POS_QW      2
	#define _FLASH_SR_POS_EOP     16
	#define _FLASH_SR_POS_WRPERR  17
	#define _FLASH_SR_POS_PGSERR  18
	#define _FLASH_SR_POS_STRBERR 19
	#define _FLASH_SR_POS_INCERR  21
	#define _FLASH_SR_POS_OPERR   22
	// combined programming/erase error flags (for polling and CCR clear)
	#define _FLASH_SR_ERRORS ((1U<<_FLASH_SR_POS_WRPERR) | (1U<<_FLASH_SR_POS_PGSERR) | \
		(1U<<_FLASH_SR_POS_STRBERR) | (1U<<_FLASH_SR_POS_INCERR) | (1U<<_FLASH_SR_POS_OPERR))

	// unlock keys (AKA FLASH_KEY1 / FLASH_KEY2)
	#define _FLASH_KEY1 0x45670123U
	#define _FLASH_KEY2 0xCDEF89ABU

	// flash geometry (H743: 2 banks x 1MB, 16 sectors of 128KB)
	#define _FLASH_BANK1_BASE  0x08000000U
	#define _FLASH_BANK2_BASE  0x08100000U
	#define _FLASH_SECTOR_SIZE 0x00020000U // 128KB
	#define _FLASH_SECTOR_TOTAL 16
	#define _FLASH_ROW_SIZE    32U         // 256-bit program row

	// clear mask written to CCR to drop EOP + all programming/erase error flags
	#define _FLASH_CCR_CLEAR ((1U << _FLASH_SR_POS_EOP) | _FLASH_SR_ERRORS)

	// ---- interrupt mode (CR1/CR2 enable bits, AKA FLASH_CR_*IE) ----
	#define _FLASH_CR_POS_EOPIE     16
	#define _FLASH_CR_POS_WRPERRIE  17
	#define _FLASH_CR_POS_PGSERRIE  18
	#define _FLASH_CR_POS_STRBERRIE 19
	#define _FLASH_CR_POS_INCERRIE  21
	#define _FLASH_CR_POS_OPERRIE   22
	// all operation interrupts armed for IOMethod::Rupt
	#define _FLASH_CR_OPER_IT ((1U<<_FLASH_CR_POS_EOPIE) | (1U<<_FLASH_CR_POS_WRPERRIE) | \
		(1U<<_FLASH_CR_POS_PGSERRIE) | (1U<<_FLASH_CR_POS_STRBERRIE) | \
		(1U<<_FLASH_CR_POS_INCERRIE) | (1U<<_FLASH_CR_POS_OPERRIE))

	// interrupt-mode process state (AKA FLASH_ProcessTypeDef.ProcedureOnGoing)
	#define _FLASH_PROC_NONE                0
	#define _FLASH_PROC_PROGRAM_BANK1       1
	#define _FLASH_PROC_PROGRAM_BANK2       2
	#define _FLASH_PROC_SECTERASE_BANK1     3
	#define _FLASH_PROC_SECTERASE_BANK2     4
	#define _FLASH_PROC_MASSERASE_BANK1     5
	#define _FLASH_PROC_MASSERASE_BANK2     6

	// ---- Option Bytes (AKA FLASH_OPTCR / OPTSR / PRAR / SCAR / WPSN / BOOT) ----
	#define _FLASH_OPTCR_POS_OPTLOCK   0
	#define _FLASH_OPTCR_POS_OPTSTART  1
	#define _FLASH_OPTCR_POS_MER       3
	#define _FLASH_OPTCR_POS_SWAP_BANK 31

	#define _FLASH_OPTSR_POS_OPT_BUSY       0
	#define _FLASH_OPTSR_POS_BOR_LEV        2  // 2b
	#define _FLASH_OPTSR_POS_IWDG1_SW       4
	#define _FLASH_OPTSR_POS_NRST_STOP_D1   6
	#define _FLASH_OPTSR_POS_NRST_STBY_D1   7
	#define _FLASH_OPTSR_POS_RDP            8  // 8b
	#define _FLASH_OPTSR_POS_FZ_IWDG_STOP   17
	#define _FLASH_OPTSR_POS_FZ_IWDG_SDBY   18
	#define _FLASH_OPTSR_POS_ST_RAM_SIZE    19 // 2b
	#define _FLASH_OPTSR_POS_SECURITY       21
	#define _FLASH_OPTSR_POS_IO_HSLV        29
	#define _FLASH_OPTSR_POS_OPTCHANGEERR   30
	#define _FLASH_OPTSR_POS_SWAP_BANK_OPT  31

	#define _FLASH_PRAR_POS_PROT_AREA_START 0  // 12b
	#define _FLASH_PRAR_POS_PROT_AREA_END   16 // 12b
	#define _FLASH_PRAR_POS_DMEP            31

	#define _FLASH_SCAR_POS_SEC_AREA_START  0  // 12b
	#define _FLASH_SCAR_POS_SEC_AREA_END    16 // 12b
	#define _FLASH_SCAR_POS_DMES            31

	#define _FLASH_WPSN_POS_WRPSN   0 // 8b
	#define _FLASH_BOOT_POS_ADD0    0 // 16b
	#define _FLASH_BOOT_POS_ADD1    16 // 16b

	// option bytes unlock keys (AKA FLASH_OPT_KEY1 / KEY2)
	#define _FLASH_OPT_KEY1 0x08192A3BU
	#define _FLASH_OPT_KEY2 0x4C5D6E7FU
	// read protection level bytes (AKA OB_RDP_LEVEL_0/1/2, stored in OPTSR.RDP)
	#define _FLASH_OB_RDP_LEVEL0 0xAAU
	#define _FLASH_OB_RDP_LEVEL1 0x55U
	#define _FLASH_OB_RDP_LEVEL2 0xCCU
	// bank selectors (AKA FLASH_BANK_1 / FLASH_BANK_2)
	#define _FLASH_BANK1 0x01U
	#define _FLASH_BANK2 0x02U

}
#endif // _MCU_STM32H7x

#endif

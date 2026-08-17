
// Modern SPI/I2S IP register bit definition for I2S mode (STM32H7 / STM32MP13)
// AKA stm32h7xx / stm32mp13xx CMSIS SPI_I2SCFGR bitfield macros (I2S part)
// Note: SPI_CR1 / CFG1 / CFG2 / IER / SR / IFCR bits and I2SMOD are shared
//       with the SPI driver, see SPI-STM32H7.hpp.
// Note: WSINV / FIXCH / DATFMT bit positions DIFFER between H7 and MP13:
//       H7  : WSINV=12, FIXCH=13, DATFMT=12 (2-bit field 13:12)
//       MP13: FIXCH=12, WSINV=13, DATFMT=14 (1-bit)

#ifndef _INC_DEV_INNER_I2S_H7
#define _INC_DEV_INNER_I2S_H7

#include "SPI-STM32H7.hpp"

/*******************  Bit definition for SPI_I2SCFGR register (I2S part)  *******************/
#if defined(_MCU_STM32H7x)
#define SPI_I2SCFGR_I2SCFG_Pos            (1U)
#define SPI_I2SCFGR_I2SCFG_Msk            (0x7U << SPI_I2SCFGR_I2SCFG_Pos)     /*!< 0x0000000E */
#define SPI_I2SCFGR_I2SCFG_0              (0x1U << SPI_I2SCFGR_I2SCFG_Pos)     /*!< 0x00000002 */
#define SPI_I2SCFGR_I2SCFG_1              (0x2U << SPI_I2SCFGR_I2SCFG_Pos)     /*!< 0x00000004 */
#define SPI_I2SCFGR_I2SCFG_2              (0x4U << SPI_I2SCFGR_I2SCFG_Pos)     /*!< 0x00000008 */
#define SPI_I2SCFGR_I2SSTD_Pos            (4U)
#define SPI_I2SCFGR_I2SSTD_Msk            (0x3U << SPI_I2SCFGR_I2SSTD_Pos)     /*!< 0x00000030 */
#define SPI_I2SCFGR_I2SSTD_0              (0x1U << SPI_I2SCFGR_I2SSTD_Pos)     /*!< 0x00000010 */
#define SPI_I2SCFGR_I2SSTD_1              (0x2U << SPI_I2SCFGR_I2SSTD_Pos)     /*!< 0x00000020 */
#define SPI_I2SCFGR_PCMSYNC_Pos           (7U)
#define SPI_I2SCFGR_PCMSYNC_Msk           (0x1U << SPI_I2SCFGR_PCMSYNC_Pos)    /*!< 0x00000080 */
#define SPI_I2SCFGR_DATLEN_Pos            (8U)
#define SPI_I2SCFGR_DATLEN_Msk            (0x3U << SPI_I2SCFGR_DATLEN_Pos)     /*!< 0x00000300 */
#define SPI_I2SCFGR_DATLEN_0              (0x1U << SPI_I2SCFGR_DATLEN_Pos)     /*!< 0x00000100 */
#define SPI_I2SCFGR_DATLEN_1              (0x2U << SPI_I2SCFGR_DATLEN_Pos)     /*!< 0x00000200 */
#define SPI_I2SCFGR_CHLEN_Pos             (10U)
#define SPI_I2SCFGR_CHLEN_Msk             (0x1U << SPI_I2SCFGR_CHLEN_Pos)      /*!< 0x00000400 */
#define SPI_I2SCFGR_CKPOL_Pos             (11U)
#define SPI_I2SCFGR_CKPOL_Msk             (0x1U << SPI_I2SCFGR_CKPOL_Pos)      /*!< 0x00000800 */
#define SPI_I2SCFGR_WSINV_Pos             (12U)
#define SPI_I2SCFGR_WSINV_Msk             (0x1U << SPI_I2SCFGR_WSINV_Pos)      /*!< 0x00001000 */
#define SPI_I2SCFGR_FIXCH_Pos             (13U)
#define SPI_I2SCFGR_FIXCH_Msk             (0x1U << SPI_I2SCFGR_FIXCH_Pos)      /*!< 0x00002000 */
#define SPI_I2SCFGR_DATFMT_Pos            (12U)
#define SPI_I2SCFGR_DATFMT_Msk            (0x3U << SPI_I2SCFGR_DATFMT_Pos)     /*!< 0x00003000 */
#define SPI_I2SCFGR_I2SDIV_Pos            (16U)
#define SPI_I2SCFGR_I2SDIV_Msk            (0xFFU << SPI_I2SCFGR_I2SDIV_Pos)    /*!< 0x00FF0000 */
#define SPI_I2SCFGR_ODD_Pos               (24U)
#define SPI_I2SCFGR_ODD_Msk               (0x1U << SPI_I2SCFGR_ODD_Pos)        /*!< 0x01000000 */
#define SPI_I2SCFGR_MCKOE_Pos             (25U)
#define SPI_I2SCFGR_MCKOE_Msk             (0x1U << SPI_I2SCFGR_MCKOE_Pos)      /*!< 0x02000000 */
#elif defined(_MPU_STM32MP13)
#define SPI_I2SCFGR_I2SCFG_Pos            (1U)
#define SPI_I2SCFGR_I2SCFG_Msk            (0x7UL << SPI_I2SCFGR_I2SCFG_Pos)    /*!< 0x0000000E */
#define SPI_I2SCFGR_I2SCFG_0              (0x1UL << SPI_I2SCFGR_I2SCFG_Pos)    /*!< 0x00000002 */
#define SPI_I2SCFGR_I2SCFG_1              (0x2UL << SPI_I2SCFGR_I2SCFG_Pos)    /*!< 0x00000004 */
#define SPI_I2SCFGR_I2SCFG_2              (0x4UL << SPI_I2SCFGR_I2SCFG_Pos)    /*!< 0x00000008 */
#define SPI_I2SCFGR_I2SSTD_Pos            (4U)
#define SPI_I2SCFGR_I2SSTD_Msk            (0x3UL << SPI_I2SCFGR_I2SSTD_Pos)    /*!< 0x00000030 */
#define SPI_I2SCFGR_I2SSTD_0              (0x1UL << SPI_I2SCFGR_I2SSTD_Pos)    /*!< 0x00000010 */
#define SPI_I2SCFGR_I2SSTD_1              (0x2UL << SPI_I2SCFGR_I2SSTD_Pos)    /*!< 0x00000020 */
#define SPI_I2SCFGR_PCMSYNC_Pos           (7U)
#define SPI_I2SCFGR_PCMSYNC_Msk           (0x1UL << SPI_I2SCFGR_PCMSYNC_Pos)   /*!< 0x00000080 */
#define SPI_I2SCFGR_DATLEN_Pos            (8U)
#define SPI_I2SCFGR_DATLEN_Msk            (0x3UL << SPI_I2SCFGR_DATLEN_Pos)    /*!< 0x00000300 */
#define SPI_I2SCFGR_DATLEN_0              (0x1UL << SPI_I2SCFGR_DATLEN_Pos)    /*!< 0x00000100 */
#define SPI_I2SCFGR_DATLEN_1              (0x2UL << SPI_I2SCFGR_DATLEN_Pos)    /*!< 0x00000200 */
#define SPI_I2SCFGR_CHLEN_Pos             (10U)
#define SPI_I2SCFGR_CHLEN_Msk             (0x1UL << SPI_I2SCFGR_CHLEN_Pos)     /*!< 0x00000400 */
#define SPI_I2SCFGR_CKPOL_Pos             (11U)
#define SPI_I2SCFGR_CKPOL_Msk             (0x1UL << SPI_I2SCFGR_CKPOL_Pos)     /*!< 0x00000800 */
#define SPI_I2SCFGR_FIXCH_Pos             (12U)
#define SPI_I2SCFGR_FIXCH_Msk             (0x1UL << SPI_I2SCFGR_FIXCH_Pos)     /*!< 0x00001000 */
#define SPI_I2SCFGR_WSINV_Pos             (13U)
#define SPI_I2SCFGR_WSINV_Msk             (0x1UL << SPI_I2SCFGR_WSINV_Pos)     /*!< 0x00002000 */
#define SPI_I2SCFGR_DATFMT_Pos            (14U)
#define SPI_I2SCFGR_DATFMT_Msk            (0x1UL << SPI_I2SCFGR_DATFMT_Pos)    /*!< 0x00004000 */
#define SPI_I2SCFGR_I2SDIV_Pos            (16U)
#define SPI_I2SCFGR_I2SDIV_Msk            (0xFFUL << SPI_I2SCFGR_I2SDIV_Pos)   /*!< 0x00FF0000 */
#define SPI_I2SCFGR_ODD_Pos               (24U)
#define SPI_I2SCFGR_ODD_Msk               (0x1UL << SPI_I2SCFGR_ODD_Pos)       /*!< 0x01000000 */
#define SPI_I2SCFGR_MCKOE_Pos             (25U)
#define SPI_I2SCFGR_MCKOE_Msk             (0x1UL << SPI_I2SCFGR_MCKOE_Pos)     /*!< 0x02000000 */
#endif

#endif

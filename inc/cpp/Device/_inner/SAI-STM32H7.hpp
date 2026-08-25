// SAI (Serial Audio Interface) register bit definition (STM32H7 / STM32MP13)
// AKA stm32h7xx / stm32mp13xx CMSIS SAI bitfield macros (identical IP)
// Note: SAI register layout is identical across H7 and MP13.

#ifndef _INC_DEV_INNER_SAI_H7
#define _INC_DEV_INNER_SAI_H7

/*******************  Bit definition for SAI_xCR1 register  *******************/
#define SAI_xCR1_MODE_Pos          (0U)
#define SAI_xCR1_MODE_Msk          (0x3U << SAI_xCR1_MODE_Pos)   /*!< 0x00000003 */
#define SAI_xCR1_MODE_0            (0x1U << SAI_xCR1_MODE_Pos)   /*!< 0x00000001 */
#define SAI_xCR1_MODE_1            (0x2U << SAI_xCR1_MODE_Pos)   /*!< 0x00000002 */
#define SAI_xCR1_PRTCFG_Pos        (2U)
#define SAI_xCR1_PRTCFG_Msk        (0x3U << SAI_xCR1_PRTCFG_Pos) /*!< 0x0000000C */
#define SAI_xCR1_PRTCFG_0          (0x1U << SAI_xCR1_PRTCFG_Pos) /*!< 0x00000004 */
#define SAI_xCR1_PRTCFG_1          (0x2U << SAI_xCR1_PRTCFG_Pos) /*!< 0x00000008 */
#define SAI_xCR1_DS_Pos            (5U)
#define SAI_xCR1_DS_Msk            (0x7U << SAI_xCR1_DS_Pos)     /*!< 0x000000E0 */
#define SAI_xCR1_DS_0              (0x1U << SAI_xCR1_DS_Pos)     /*!< 0x00000020 */
#define SAI_xCR1_DS_1              (0x2U << SAI_xCR1_DS_Pos)     /*!< 0x00000040 */
#define SAI_xCR1_DS_2              (0x4U << SAI_xCR1_DS_Pos)     /*!< 0x00000080 */
#define SAI_xCR1_LSBFIRST_Pos      (8U)
#define SAI_xCR1_LSBFIRST_Msk      (0x1U << SAI_xCR1_LSBFIRST_Pos) /*!< 0x00000100 */
#define SAI_xCR1_CKSTR_Pos         (9U)
#define SAI_xCR1_CKSTR_Msk         (0x1U << SAI_xCR1_CKSTR_Pos)  /*!< 0x00000200 */
#define SAI_xCR1_SYNCEN_Pos        (10U)
#define SAI_xCR1_SYNCEN_Msk        (0x3U << SAI_xCR1_SYNCEN_Pos) /*!< 0x00000C00 */
#define SAI_xCR1_SYNCEN_0          (0x1U << SAI_xCR1_SYNCEN_Pos) /*!< 0x00000400 */
#define SAI_xCR1_SYNCEN_1          (0x2U << SAI_xCR1_SYNCEN_Pos) /*!< 0x00000800 */
#define SAI_xCR1_MONO_Pos          (12U)
#define SAI_xCR1_MONO_Msk          (0x1U << SAI_xCR1_MONO_Pos)   /*!< 0x00001000 */
#define SAI_xCR1_OUTDRIV_Pos       (13U)
#define SAI_xCR1_OUTDRIV_Msk       (0x1U << SAI_xCR1_OUTDRIV_Pos) /*!< 0x00002000 */
#define SAI_xCR1_SAIEN_Pos         (16U)
#define SAI_xCR1_SAIEN_Msk         (0x1U << SAI_xCR1_SAIEN_Pos)  /*!< 0x00010000 */
#define SAI_xCR1_DMAEN_Pos         (17U)
#define SAI_xCR1_DMAEN_Msk         (0x1U << SAI_xCR1_DMAEN_Pos)  /*!< 0x00020000 */
#define SAI_xCR1_NOMCK_Pos         (19U)
#define SAI_xCR1_NOMCK_Msk         (0x1U << SAI_xCR1_NOMCK_Pos)  /*!< 0x00080000 */
#define SAI_xCR1_MCKDIV_Pos        (20U)
#define SAI_xCR1_MCKDIV_Msk        (0x3FU << SAI_xCR1_MCKDIV_Pos) /*!< 0x03F00000 */
#define SAI_xCR1_OSR_Pos           (26U)
#define SAI_xCR1_OSR_Msk           (0x1U << SAI_xCR1_OSR_Pos)    /*!< 0x04000000 */
#define SAI_xCR1_MCKEN_Pos         (27U)
#define SAI_xCR1_MCKEN_Msk         (0x1U << SAI_xCR1_MCKEN_Pos)  /*!< 0x08000000 */

/*******************  Bit definition for SAI_xCR2 register  *******************/
#define SAI_xCR2_FTH_Pos           (0U)
#define SAI_xCR2_FTH_Msk           (0x7U << SAI_xCR2_FTH_Pos)    /*!< 0x00000007 */
#define SAI_xCR2_FFLUSH_Pos        (3U)
#define SAI_xCR2_FFLUSH_Msk        (0x1U << SAI_xCR2_FFLUSH_Pos) /*!< 0x00000008 */
#define SAI_xCR2_TRIS_Pos          (4U)
#define SAI_xCR2_TRIS_Msk          (0x1U << SAI_xCR2_TRIS_Pos)   /*!< 0x00000010 */
#define SAI_xCR2_MUTE_Pos          (5U)
#define SAI_xCR2_MUTE_Msk          (0x1U << SAI_xCR2_MUTE_Pos)   /*!< 0x00000020 */
#define SAI_xCR2_MUTEVAL_Pos       (6U)
#define SAI_xCR2_MUTEVAL_Msk       (0x1U << SAI_xCR2_MUTEVAL_Pos) /*!< 0x00000040 */
#define SAI_xCR2_MUTECNT_Pos       (7U)
#define SAI_xCR2_MUTECNT_Msk       (0x3FU << SAI_xCR2_MUTECNT_Pos) /*!< 0x00001F80 */
#define SAI_xCR2_CPL_Pos           (13U)
#define SAI_xCR2_CPL_Msk           (0x1U << SAI_xCR2_CPL_Pos)    /*!< 0x00002000 */
#define SAI_xCR2_COMP_Pos          (14U)
#define SAI_xCR2_COMP_Msk          (0x3U << SAI_xCR2_COMP_Pos)   /*!< 0x0000C000 */

/*******************  Bit definition for SAI_xFRCR register  *******************/
#define SAI_xFRCR_FRL_Pos          (0U)
#define SAI_xFRCR_FRL_Msk          (0xFFU << SAI_xFRCR_FRL_Pos)  /*!< 0x000000FF */
#define SAI_xFRCR_FSALL_Pos        (8U)
#define SAI_xFRCR_FSALL_Msk        (0x7FU << SAI_xFRCR_FSALL_Pos) /*!< 0x00007F00 */
#define SAI_xFRCR_FSDEF_Pos        (16U)
#define SAI_xFRCR_FSDEF_Msk        (0x1U << SAI_xFRCR_FSDEF_Pos) /*!< 0x00010000 */
#define SAI_xFRCR_FSPOL_Pos        (17U)
#define SAI_xFRCR_FSPOL_Msk        (0x1U << SAI_xFRCR_FSPOL_Pos) /*!< 0x00020000 */
#define SAI_xFRCR_FSOFF_Pos        (18U)
#define SAI_xFRCR_FSOFF_Msk        (0x1U << SAI_xFRCR_FSOFF_Pos) /*!< 0x00040000 */

/*******************  Bit definition for SAI_xSLOTR register  *******************/
#define SAI_xSLOTR_FBOFF_Pos       (0U)
#define SAI_xSLOTR_FBOFF_Msk       (0x1FU << SAI_xSLOTR_FBOFF_Pos) /*!< 0x0000001F */
#define SAI_xSLOTR_SLOTSZ_Pos      (6U)
#define SAI_xSLOTR_SLOTSZ_Msk      (0x3U << SAI_xSLOTR_SLOTSZ_Pos) /*!< 0x000000C0 */
#define SAI_xSLOTR_NBSLOT_Pos      (8U)
#define SAI_xSLOTR_NBSLOT_Msk      (0xFU << SAI_xSLOTR_NBSLOT_Pos) /*!< 0x00000F00 */
#define SAI_xSLOTR_SLOTEN_Pos      (16U)
#define SAI_xSLOTR_SLOTEN_Msk      (0xFFFFU << SAI_xSLOTR_SLOTEN_Pos) /*!< 0xFFFF0000 */

/*******************  Bit definition for SAI_xIMR / SAI_xSR / SAI_xCLRFR  *******************/
#define SAI_xSR_OVRUDR_Pos         (0U)
#define SAI_xSR_OVRUDR_Msk         (0x1U << SAI_xSR_OVRUDR_Pos)  /*!< 0x00000001 */
#define SAI_xSR_MUTEDET_Pos        (1U)
#define SAI_xSR_MUTEDET_Msk        (0x1U << SAI_xSR_MUTEDET_Pos) /*!< 0x00000002 */
#define SAI_xSR_WCKCFG_Pos         (2U)
#define SAI_xSR_WCKCFG_Msk         (0x1U << SAI_xSR_WCKCFG_Pos)  /*!< 0x00000004 */
#define SAI_xSR_FREQ_Pos           (3U)
#define SAI_xSR_FREQ_Msk           (0x1U << SAI_xSR_FREQ_Pos)    /*!< 0x00000008 */
#define SAI_xSR_CNRDY_Pos          (4U)
#define SAI_xSR_CNRDY_Msk          (0x1U << SAI_xSR_CNRDY_Pos)   /*!< 0x00000010 */
#define SAI_xSR_AFSDET_Pos         (5U)
#define SAI_xSR_AFSDET_Msk         (0x1U << SAI_xSR_AFSDET_Pos)  /*!< 0x00000020 */
#define SAI_xSR_LFSDET_Pos         (6U)
#define SAI_xSR_LFSDET_Msk         (0x1U << SAI_xSR_LFSDET_Pos)  /*!< 0x00000040 */
#define SAI_xSR_FLVL_Pos           (16U)
#define SAI_xSR_FLVL_Msk           (0x7U << SAI_xSR_FLVL_Pos)    /*!< 0x00070000 */

/*******************  Bit definition for SAI_GCR register  *******************/
#define SAI_GCR_SYNCIN_Pos         (0U)
#define SAI_GCR_SYNCIN_Msk         (0x3U << SAI_GCR_SYNCIN_Pos)  /*!< 0x00000003 */
#define SAI_GCR_SYNCIN_0           (0x1U << SAI_GCR_SYNCIN_Pos)  /*!< 0x00000001 */
#define SAI_GCR_SYNCIN_1           (0x2U << SAI_GCR_SYNCIN_Pos)  /*!< 0x00000002 */
#define SAI_GCR_SYNCOUT_Pos        (4U)
#define SAI_GCR_SYNCOUT_Msk        (0x3U << SAI_GCR_SYNCOUT_Pos) /*!< 0x00000030 */
#define SAI_GCR_SYNCOUT_0          (0x1U << SAI_GCR_SYNCOUT_Pos) /*!< 0x00000010 */
#define SAI_GCR_SYNCOUT_1          (0x2U << SAI_GCR_SYNCOUT_Pos) /*!< 0x00000020 */

/*******************  Bit definition for SAI_PDMCR register  *******************/
#define SAI_PDMCR_PDMEN_Pos        (0U)
#define SAI_PDMCR_PDMEN_Msk        (0x1U << SAI_PDMCR_PDMEN_Pos) /*!< 0x00000001 */
#define SAI_PDMCR_MICNBR_Pos       (4U)
#define SAI_PDMCR_MICNBR_Msk       (0x3U << SAI_PDMCR_MICNBR_Pos) /*!< 0x00000030 */
#define SAI_PDMCR_CKEN1_Pos        (8U)
#define SAI_PDMCR_CKEN1_Msk        (0x1U << SAI_PDMCR_CKEN1_Pos) /*!< 0x00000100 */

#endif

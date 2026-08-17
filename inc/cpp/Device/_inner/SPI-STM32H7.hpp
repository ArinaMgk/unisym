
// Modern SPI IP register bit definition (STM32H7 / STM32MP13)
// AKA stm32h7xx CMSIS SPI bitfield macros

#ifndef _INC_DEV_INNER_SPI_H7
#define _INC_DEV_INNER_SPI_H7

/*******************  Bit definition for SPI_CR1 register  *******************/
#define SPI_CR1_SPE_Pos               (0U)
#define SPI_CR1_SPE_Msk               (0x1UL << SPI_CR1_SPE_Pos)              /*!< 0x00000001 */
#define SPI_CR1_SPE                   SPI_CR1_SPE_Msk                         /*!< Peripheral enable */
#define SPI_CR1_MASRX_Pos             (8U)
#define SPI_CR1_MASRX_Msk             (0x1UL << SPI_CR1_MASRX_Pos)            /*!< 0x00000100 */
#define SPI_CR1_MASRX                 SPI_CR1_MASRX_Msk                       /*!< Master automatic suspend */
#define SPI_CR1_CSTART_Pos            (9U)
#define SPI_CR1_CSTART_Msk            (0x1UL << SPI_CR1_CSTART_Pos)           /*!< 0x00000200 */
#define SPI_CR1_CSTART                SPI_CR1_CSTART_Msk                      /*!< Master transfer start */
#define SPI_CR1_CSUSP_Pos             (10U)
#define SPI_CR1_CSUSP_Msk             (0x1UL << SPI_CR1_CSUSP_Pos)            /*!< 0x00000400 */
#define SPI_CR1_CSUSP                 SPI_CR1_CSUSP_Msk                       /*!< Master suspend request */
#define SPI_CR1_HDDIR_Pos             (11U)
#define SPI_CR1_HDDIR_Msk             (0x1UL << SPI_CR1_HDDIR_Pos)            /*!< 0x00000800 */
#define SPI_CR1_HDDIR                 SPI_CR1_HDDIR_Msk                       /*!< Half-duplex direction */
#define SPI_CR1_SSI_Pos               (12U)
#define SPI_CR1_SSI_Msk               (0x1UL << SPI_CR1_SSI_Pos)              /*!< 0x00001000 */
#define SPI_CR1_SSI                   SPI_CR1_SSI_Msk                         /*!< Internal slave select */
#define SPI_CR1_CRC33_17_Pos          (13U)
#define SPI_CR1_CRC33_17_Msk          (0x1UL << SPI_CR1_CRC33_17_Pos)         /*!< 0x00002000 */
#define SPI_CR1_CRC33_17              SPI_CR1_CRC33_17_Msk                    /*!< 32-bit CRC polynomial */
#define SPI_CR1_RCRCINI_Pos           (14U)
#define SPI_CR1_RCRCINI_Msk           (0x1UL << SPI_CR1_RCRCINI_Pos)          /*!< 0x00004000 */
#define SPI_CR1_RCRCINI               SPI_CR1_RCRCINI_Msk                     /*!< RX CRC init pattern */
#define SPI_CR1_TCRCINI_Pos           (15U)
#define SPI_CR1_TCRCINI_Msk           (0x1UL << SPI_CR1_TCRCINI_Pos)          /*!< 0x00008000 */
#define SPI_CR1_TCRCINI               SPI_CR1_TCRCINI_Msk                     /*!< TX CRC init pattern */
#define SPI_CR1_IOLOCK_Pos            (16U)
#define SPI_CR1_IOLOCK_Msk            (0x1UL << SPI_CR1_IOLOCK_Pos)           /*!< 0x00010000 */
#define SPI_CR1_IOLOCK                SPI_CR1_IOLOCK_Msk                      /*!< IO lock */

/*******************  Bit definition for SPI_CR2 register  *******************/
#define SPI_CR2_TSIZE_Pos             (0U)
#define SPI_CR2_TSIZE_Msk             (0xFFFFUL << SPI_CR2_TSIZE_Pos)         /*!< 0x0000FFFF */
#define SPI_CR2_TSIZE                 SPI_CR2_TSIZE_Msk                       /*!< Number of data frames */
#define SPI_CR2_TSER_Pos              (16U)
#define SPI_CR2_TSER_Msk              (0xFFFFUL << SPI_CR2_TSER_Pos)           /*!< 0xFFFF0000 */
#define SPI_CR2_TSER                  SPI_CR2_TSER_Msk                        /*!< Number of data transfer extension */

/*******************  Bit definition for SPI_CFG1 register  *******************/
#define SPI_CFG1_DSIZE_Pos            (0U)
#define SPI_CFG1_DSIZE_Msk            (0x1FUL << SPI_CFG1_DSIZE_Pos)          /*!< 0x0000001F */
#define SPI_CFG1_DSIZE                SPI_CFG1_DSIZE_Msk                      /*!< Data size */
#define SPI_CFG1_FTHLV_Pos            (5U)
#define SPI_CFG1_FTHLV_Msk            (0xFUL << SPI_CFG1_FTHLV_Pos)           /*!< 0x000001E0 */
#define SPI_CFG1_FTHLV                SPI_CFG1_FTHLV_Msk                      /*!< FIFO threshold */
#define SPI_CFG1_UDRCFG_Pos           (9U)
#define SPI_CFG1_UDRCFG_Msk           (0x3UL << SPI_CFG1_UDRCFG_Pos)          /*!< 0x00000600 */
#define SPI_CFG1_UDRCFG               SPI_CFG1_UDRCFG_Msk                     /*!< Underrun behaviour */
#define SPI_CFG1_UDRCFG_0             (0x1UL << SPI_CFG1_UDRCFG_Pos)
#define SPI_CFG1_UDRCFG_1             (0x2UL << SPI_CFG1_UDRCFG_Pos)
#define SPI_CFG1_UDRDET_Pos           (11U)
#define SPI_CFG1_UDRDET_Msk           (0x3UL << SPI_CFG1_UDRDET_Pos)          /*!< 0x00001800 */
#define SPI_CFG1_UDRDET               SPI_CFG1_UDRDET_Msk                     /*!< Underrun detection */
#define SPI_CFG1_UDRDET_0             (0x1UL << SPI_CFG1_UDRDET_Pos)
#define SPI_CFG1_UDRDET_1             (0x2UL << SPI_CFG1_UDRDET_Pos)
#define SPI_CFG1_RXDMAEN_Pos          (14U)
#define SPI_CFG1_RXDMAEN_Msk          (0x1UL << SPI_CFG1_RXDMAEN_Pos)         /*!< 0x00004000 */
#define SPI_CFG1_RXDMAEN              SPI_CFG1_RXDMAEN_Msk                    /*!< RX DMA enable */
#define SPI_CFG1_TXDMAEN_Pos          (15U)
#define SPI_CFG1_TXDMAEN_Msk          (0x1UL << SPI_CFG1_TXDMAEN_Pos)         /*!< 0x00008000 */
#define SPI_CFG1_TXDMAEN              SPI_CFG1_TXDMAEN_Msk                    /*!< TX DMA enable */
#define SPI_CFG1_CRCSIZE_Pos          (16U)
#define SPI_CFG1_CRCSIZE_Msk          (0x1FUL << SPI_CFG1_CRCSIZE_Pos)        /*!< 0x001F0000 */
#define SPI_CFG1_CRCSIZE              SPI_CFG1_CRCSIZE_Msk                    /*!< CRC length */
#define SPI_CFG1_CRCEN_Pos            (22U)
#define SPI_CFG1_CRCEN_Msk            (0x1UL << SPI_CFG1_CRCEN_Pos)           /*!< 0x00400000 */
#define SPI_CFG1_CRCEN                SPI_CFG1_CRCEN_Msk                      /*!< CRC enable */
#define SPI_CFG1_MBR_Pos              (28U)
#define SPI_CFG1_MBR_Msk              (0x7UL << SPI_CFG1_MBR_Pos)             /*!< 0x70000000 */
#define SPI_CFG1_MBR                  SPI_CFG1_MBR_Msk                        /*!< Master baud rate */

/*******************  Bit definition for SPI_CFG2 register  *******************/
#define SPI_CFG2_MSSI_Pos             (0U)
#define SPI_CFG2_MSSI_Msk             (0xFUL << SPI_CFG2_MSSI_Pos)            /*!< 0x0000000F */
#define SPI_CFG2_MSSI                 SPI_CFG2_MSSI_Msk                       /*!< Master SS idleness */
#define SPI_CFG2_MIDI_Pos             (4U)
#define SPI_CFG2_MIDI_Msk             (0xFUL << SPI_CFG2_MIDI_Pos)            /*!< 0x000000F0 */
#define SPI_CFG2_MIDI                 SPI_CFG2_MIDI_Msk                       /*!< Master inter-data idleness */
#define SPI_CFG2_IOSWP_Pos            (15U)
#define SPI_CFG2_IOSWP_Msk            (0x1UL << SPI_CFG2_IOSWP_Pos)           /*!< 0x00008000 */
#define SPI_CFG2_IOSWP                SPI_CFG2_IOSWP_Msk                      /*!< IO swap */
#define SPI_CFG2_COMM_Pos             (17U)
#define SPI_CFG2_COMM_Msk             (0x3UL << SPI_CFG2_COMM_Pos)            /*!< 0x00060000 */
#define SPI_CFG2_COMM                 SPI_CFG2_COMM_Msk                       /*!< Communication mode */
#define SPI_CFG2_COMM_0               (0x1UL << SPI_CFG2_COMM_Pos)
#define SPI_CFG2_COMM_1               (0x2UL << SPI_CFG2_COMM_Pos)
#define SPI_CFG2_SP_Pos               (19U)
#define SPI_CFG2_SP_Msk               (0x7UL << SPI_CFG2_SP_Pos)              /*!< 0x00380000 */
#define SPI_CFG2_SP                   SPI_CFG2_SP_Msk                         /*!< Serial protocol */
#define SPI_CFG2_SP_0                 (0x1UL << SPI_CFG2_SP_Pos)
#define SPI_CFG2_MASTER_Pos           (22U)
#define SPI_CFG2_MASTER_Msk           (0x1UL << SPI_CFG2_MASTER_Pos)          /*!< 0x00400000 */
#define SPI_CFG2_MASTER               SPI_CFG2_MASTER_Msk                     /*!< Master mode */
#define SPI_CFG2_LSBFRST_Pos          (23U)
#define SPI_CFG2_LSBFRST_Msk          (0x1UL << SPI_CFG2_LSBFRST_Pos)         /*!< 0x00800000 */
#define SPI_CFG2_LSBFRST              SPI_CFG2_LSBFRST_Msk                    /*!< LSB first */
#define SPI_CFG2_CPHA_Pos             (24U)
#define SPI_CFG2_CPHA_Msk             (0x1UL << SPI_CFG2_CPHA_Pos)            /*!< 0x01000000 */
#define SPI_CFG2_CPHA                 SPI_CFG2_CPHA_Msk                       /*!< Clock phase */
#define SPI_CFG2_CPOL_Pos             (25U)
#define SPI_CFG2_CPOL_Msk             (0x1UL << SPI_CFG2_CPOL_Pos)            /*!< 0x02000000 */
#define SPI_CFG2_CPOL                 SPI_CFG2_CPOL_Msk                       /*!< Clock polarity */
#define SPI_CFG2_SSM_Pos              (26U)
#define SPI_CFG2_SSM_Msk              (0x1UL << SPI_CFG2_SSM_Pos)             /*!< 0x04000000 */
#define SPI_CFG2_SSM                  SPI_CFG2_SSM_Msk                        /*!< Software slave select */
#define SPI_CFG2_SSIOP_Pos            (28U)
#define SPI_CFG2_SSIOP_Msk            (0x1UL << SPI_CFG2_SSIOP_Pos)           /*!< 0x10000000 */
#define SPI_CFG2_SSIOP                SPI_CFG2_SSIOP_Msk                      /*!< SS input/output polarity */
#define SPI_CFG2_SSOE_Pos             (29U)
#define SPI_CFG2_SSOE_Msk             (0x1UL << SPI_CFG2_SSOE_Pos)            /*!< 0x20000000 */
#define SPI_CFG2_SSOE                 SPI_CFG2_SSOE_Msk                       /*!< SS output enable */
#define SPI_CFG2_SSOM_Pos             (30U)
#define SPI_CFG2_SSOM_Msk             (0x1UL << SPI_CFG2_SSOM_Pos)            /*!< 0x40000000 */
#define SPI_CFG2_SSOM                 SPI_CFG2_SSOM_Msk                       /*!< SS output management */
#define SPI_CFG2_AFCNTR_Pos           (31U)
#define SPI_CFG2_AFCNTR_Msk           (0x1UL << SPI_CFG2_AFCNTR_Pos)          /*!< 0x80000000 */
#define SPI_CFG2_AFCNTR               SPI_CFG2_AFCNTR_Msk                     /*!< Alternate function control */

/*******************  Bit definition for SPI_IER register  *******************/
#define SPI_IER_RXPIE_Pos             (0U)
#define SPI_IER_RXPIE_Msk             (0x1UL << SPI_IER_RXPIE_Pos)            /*!< 0x00000001 */
#define SPI_IER_RXPIE                 SPI_IER_RXPIE_Msk                       /*!< RX FIFO threshold interrupt */
#define SPI_IER_TXPIE_Pos             (1U)
#define SPI_IER_TXPIE_Msk             (0x1UL << SPI_IER_TXPIE_Pos)            /*!< 0x00000002 */
#define SPI_IER_TXPIE                 SPI_IER_TXPIE_Msk                       /*!< TX FIFO threshold interrupt */
#define SPI_IER_DXPIE_Pos             (2U)
#define SPI_IER_DXPIE_Msk             (0x1UL << SPI_IER_DXPIE_Pos)            /*!< 0x00000004 */
#define SPI_IER_DXPIE                 SPI_IER_DXPIE_Msk                       /*!< TX/RX data packed interrupt */
#define SPI_IER_EOTIE_Pos             (3U)
#define SPI_IER_EOTIE_Msk             (0x1UL << SPI_IER_EOTIE_Pos)            /*!< 0x00000008 */
#define SPI_IER_EOTIE                 SPI_IER_EOTIE_Msk                       /*!< End of transfer interrupt */
#define SPI_IER_TXTFIE_Pos            (4U)
#define SPI_IER_TXTFIE_Msk            (0x1UL << SPI_IER_TXTFIE_Pos)           /*!< 0x00000010 */
#define SPI_IER_TXTFIE                SPI_IER_TXTFIE_Msk                      /*!< TX transfer filled interrupt */
#define SPI_IER_UDRIE_Pos             (5U)
#define SPI_IER_UDRIE_Msk             (0x1UL << SPI_IER_UDRIE_Pos)            /*!< 0x00000020 */
#define SPI_IER_UDRIE                 SPI_IER_UDRIE_Msk                       /*!< Underrun interrupt */
#define SPI_IER_OVRIE_Pos             (6U)
#define SPI_IER_OVRIE_Msk             (0x1UL << SPI_IER_OVRIE_Pos)            /*!< 0x00000040 */
#define SPI_IER_OVRIE                 SPI_IER_OVRIE_Msk                       /*!< Overrun interrupt */
#define SPI_IER_CRCIE_Pos             (7U)
#define SPI_IER_CRCIE_Msk             (0x1UL << SPI_IER_CRCIE_Pos)            /*!< 0x00000080 */
#define SPI_IER_CRCIE                 SPI_IER_CRCIE_Msk                       /*!< CRC error interrupt */
#define SPI_IER_TIFREIE_Pos           (8U)
#define SPI_IER_TIFREIE_Msk           (0x1UL << SPI_IER_TIFREIE_Pos)          /*!< 0x00000100 */
#define SPI_IER_TIFREIE               SPI_IER_TIFREIE_Msk                     /*!< TI frame error interrupt */
#define SPI_IER_MODFIE_Pos            (9U)
#define SPI_IER_MODFIE_Msk            (0x1UL << SPI_IER_MODFIE_Pos)           /*!< 0x00000200 */
#define SPI_IER_MODFIE                SPI_IER_MODFIE_Msk                      /*!< Mode fault interrupt */
#define SPI_IER_TSERIE_Pos            (10U)
#define SPI_IER_TSERIE_Msk            (0x1UL << SPI_IER_TSERIE_Pos)           /*!< 0x00000400 */
#define SPI_IER_TSERIE                SPI_IER_TSERIE_Msk                      /*!< TSIZE reload interrupt */

/*******************  Bit definition for SPI_SR register  *******************/
#define SPI_SR_RXP_Pos                (0U)
#define SPI_SR_RXP_Msk                (0x1UL << SPI_SR_RXP_Pos)               /*!< 0x00000001 */
#define SPI_SR_RXP                    SPI_SR_RXP_Msk                          /*!< RX FIFO threshold flag */
#define SPI_SR_TXP_Pos                (1U)
#define SPI_SR_TXP_Msk                (0x1UL << SPI_SR_TXP_Pos)               /*!< 0x00000002 */
#define SPI_SR_TXP                    SPI_SR_TXP_Msk                          /*!< TX FIFO threshold flag */
#define SPI_SR_DXP_Pos                (2U)
#define SPI_SR_DXP_Msk                (0x1UL << SPI_SR_DXP_Pos)               /*!< 0x00000004 */
#define SPI_SR_DXP                    SPI_SR_DXP_Msk                          /*!< TX/RX data packed flag */
#define SPI_SR_EOT_Pos                (3U)
#define SPI_SR_EOT_Msk                (0x1UL << SPI_SR_EOT_Pos)               /*!< 0x00000008 */
#define SPI_SR_EOT                    SPI_SR_EOT_Msk                          /*!< End of transfer flag */
#define SPI_SR_TXTF_Pos               (4U)
#define SPI_SR_TXTF_Msk               (0x1UL << SPI_SR_TXTF_Pos)              /*!< 0x00000010 */
#define SPI_SR_TXTF                   SPI_SR_TXTF_Msk                         /*!< TX transfer filled flag */
#define SPI_SR_UDR_Pos                (5U)
#define SPI_SR_UDR_Msk                (0x1UL << SPI_SR_UDR_Pos)               /*!< 0x00000020 */
#define SPI_SR_UDR                    SPI_SR_UDR_Msk                          /*!< Underrun flag */
#define SPI_SR_OVR_Pos                (6U)
#define SPI_SR_OVR_Msk                (0x1UL << SPI_SR_OVR_Pos)               /*!< 0x00000040 */
#define SPI_SR_OVR                    SPI_SR_OVR_Msk                          /*!< Overrun flag */
#define SPI_SR_CRCE_Pos               (7U)
#define SPI_SR_CRCE_Msk               (0x1UL << SPI_SR_CRCE_Pos)              /*!< 0x00000080 */
#define SPI_SR_CRCE                   SPI_SR_CRCE_Msk                         /*!< CRC error flag */
#define SPI_SR_TIFRE_Pos              (8U)
#define SPI_SR_TIFRE_Msk              (0x1UL << SPI_SR_TIFRE_Pos)             /*!< 0x00000100 */
#define SPI_SR_TIFRE                  SPI_SR_TIFRE_Msk                        /*!< TI frame error flag */
#define SPI_SR_MODF_Pos               (9U)
#define SPI_SR_MODF_Msk               (0x1UL << SPI_SR_MODF_Pos)              /*!< 0x00000200 */
#define SPI_SR_MODF                   SPI_SR_MODF_Msk                         /*!< Mode fault flag */
#define SPI_SR_TSERF_Pos              (10U)
#define SPI_SR_TSERF_Msk              (0x1UL << SPI_SR_TSERF_Pos)             /*!< 0x00000400 */
#define SPI_SR_TSERF                  SPI_SR_TSERF_Msk                        /*!< TSIZE reload flag */
#define SPI_SR_SUSP_Pos               (11U)
#define SPI_SR_SUSP_Msk               (0x1UL << SPI_SR_SUSP_Pos)              /*!< 0x00000800 */
#define SPI_SR_SUSP                   SPI_SR_SUSP_Msk                         /*!< Suspend flag */
#define SPI_SR_TXC_Pos                (12U)
#define SPI_SR_TXC_Msk                (0x1UL << SPI_SR_TXC_Pos)               /*!< 0x00001000 */
#define SPI_SR_TXC                    SPI_SR_TXC_Msk                          /*!< TX complete flag */
#define SPI_SR_RXPLVL_Pos             (13U)
#define SPI_SR_RXPLVL_Msk             (0x3UL << SPI_SR_RXPLVL_Pos)            /*!< 0x00006000 */
#define SPI_SR_RXPLVL                 SPI_SR_RXPLVL_Msk                       /*!< RX FIFO level */
#define SPI_SR_RXWNE_Pos              (15U)
#define SPI_SR_RXWNE_Msk              (0x1UL << SPI_SR_RXWNE_Pos)             /*!< 0x00008000 */
#define SPI_SR_RXWNE                  SPI_SR_RXWNE_Msk                        /*!< RX FIFO word not empty */
#define SPI_SR_CTSIZE_Pos             (16U)
#define SPI_SR_CTSIZE_Msk             (0xFFFFUL << SPI_SR_CTSIZE_Pos)         /*!< 0xFFFF0000 */
#define SPI_SR_CTSIZE                 SPI_SR_CTSIZE_Msk                       /*!< Current transfer size */

/*******************  Bit definition for SPI_IFCR register  *******************/
#define SPI_IFCR_EOTC_Pos             (3U)
#define SPI_IFCR_EOTC_Msk             (0x1UL << SPI_IFCR_EOTC_Pos)            /*!< 0x00000008 */
#define SPI_IFCR_EOTC                 SPI_IFCR_EOTC_Msk                       /*!< Clear EOT flag */
#define SPI_IFCR_TXTFC_Pos            (4U)
#define SPI_IFCR_TXTFC_Msk            (0x1UL << SPI_IFCR_TXTFC_Pos)           /*!< 0x00000010 */
#define SPI_IFCR_TXTFC                SPI_IFCR_TXTFC_Msk                      /*!< Clear TXTF flag */
#define SPI_IFCR_UDRC_Pos             (5U)
#define SPI_IFCR_UDRC_Msk             (0x1UL << SPI_IFCR_UDRC_Pos)            /*!< 0x00000020 */
#define SPI_IFCR_UDRC                 SPI_IFCR_UDRC_Msk                       /*!< Clear UDR flag */
#define SPI_IFCR_OVRC_Pos             (6U)
#define SPI_IFCR_OVRC_Msk             (0x1UL << SPI_IFCR_OVRC_Pos)            /*!< 0x00000040 */
#define SPI_IFCR_OVRC                 SPI_IFCR_OVRC_Msk                       /*!< Clear OVR flag */
#define SPI_IFCR_CRCEC_Pos            (7U)
#define SPI_IFCR_CRCEC_Msk            (0x1UL << SPI_IFCR_CRCEC_Pos)           /*!< 0x00000080 */
#define SPI_IFCR_CRCEC                SPI_IFCR_CRCEC_Msk                      /*!< Clear CRCE flag */
#define SPI_IFCR_TIFREC_Pos           (8U)
#define SPI_IFCR_TIFREC_Msk           (0x1UL << SPI_IFCR_TIFREC_Pos)          /*!< 0x00000100 */
#define SPI_IFCR_TIFREC               SPI_IFCR_TIFREC_Msk                     /*!< Clear TIFRE flag */
#define SPI_IFCR_MODFC_Pos            (9U)
#define SPI_IFCR_MODFC_Msk            (0x1UL << SPI_IFCR_MODFC_Pos)           /*!< 0x00000200 */
#define SPI_IFCR_MODFC                SPI_IFCR_MODFC_Msk                      /*!< Clear MODF flag */
#define SPI_IFCR_TSERFC_Pos           (10U)
#define SPI_IFCR_TSERFC_Msk           (0x1UL << SPI_IFCR_TSERFC_Pos)          /*!< 0x00000400 */
#define SPI_IFCR_TSERFC               SPI_IFCR_TSERFC_Msk                     /*!< Clear TSERF flag */
#define SPI_IFCR_SUSPC_Pos            (11U)
#define SPI_IFCR_SUSPC_Msk            (0x1UL << SPI_IFCR_SUSPC_Pos)           /*!< 0x00000800 */
#define SPI_IFCR_SUSPC                SPI_IFCR_SUSPC_Msk                      /*!< Clear SUSP flag */

/*******************  Bit definition for SPI_I2SCFGR register  *******************/
#define SPI_I2SCFGR_I2SMOD_Pos        (0U)
#define SPI_I2SCFGR_I2SMOD_Msk        (0x1UL << SPI_I2SCFGR_I2SMOD_Pos)       /*!< 0x00000001 */
#define SPI_I2SCFGR_I2SMOD            SPI_I2SCFGR_I2SMOD_Msk                  /*!< I2S mode selection */

/*******************  SPI Error codes (AKA HAL_SPI_ERROR_*)  *******************/
#define _SPI_ERR_NONE     0x00
#define _SPI_ERR_MODF     0x01
#define _SPI_ERR_CRC      0x02
#define _SPI_ERR_OVR      0x04
#define _SPI_ERR_FRE      0x08
#define _SPI_ERR_DMA      0x10
#define _SPI_ERR_ABORT    0x40
#define _SPI_ERR_UDR      0x80
#define _SPI_ERR_TIMEOUT  0x100
#define _SPI_ERR_NOT_SUPPORTED 0x400
#define _SPI_TIMEOUT_VALUE 0x1FFFFFFUL

#endif

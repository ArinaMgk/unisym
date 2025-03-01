// wo H743

// Analog Voltage Detector level selection
#define PWR_CR1_ALS Stdfield(PWR[PWRReg::CR1], 17, 2)
// Analog Voltage Detector Enable
#define PWR_CR1_AVDEN Stdfield(PWR[PWRReg::CR1], 16, 1)
// System STOP mode Voltage Scaling selection
#define PWR_CR1_SVOS Stdfield(PWR[PWRReg::CR1], 14, 2)




#define PWR_CR1_RLPSN_Pos              (10U)
#define PWR_CR1_RLPSN_Msk              (0x1U << PWR_CR1_RLPSN_Pos)             /*!< 0x00000400 */
#define PWR_CR1_RLPSN                  PWR_CR1_RLPSN_Msk                       /*!< RAM low power mode disable in STOP. */

#define PWR_CR1_FLPS_Pos               (9U)
#define PWR_CR1_FLPS_Msk               (0x1U << PWR_CR1_FLPS_Pos)              /*!< 0x00000200 */
#define PWR_CR1_FLPS                   PWR_CR1_FLPS_Msk                        /*!< Flash low power mode in DSTOP */

#define PWR_CR1_DBP_Pos                (8U)
#define PWR_CR1_DBP_Msk                (0x1U << PWR_CR1_DBP_Pos)               /*!< 0x00000100 */
#define PWR_CR1_DBP                    PWR_CR1_DBP_Msk                         /*!< Disable Back-up domain Protection */

#define PWR_CR1_PLS_Pos                (5U)
#define PWR_CR1_PLS_Msk                (0x7U << PWR_CR1_PLS_Pos)               /*!< 0x000000E0 */
#define PWR_CR1_PLS                    PWR_CR1_PLS_Msk                         /*!< Programmable Voltage Detector level selection */
#define PWR_CR1_PLS_0                  (0x1U << PWR_CR1_PLS_Pos)               /*!< 0x00000020 */
#define PWR_CR1_PLS_1                  (0x2U << PWR_CR1_PLS_Pos)               /*!< 0x00000040 */
#define PWR_CR1_PLS_2                  (0x4U << PWR_CR1_PLS_Pos)               /*!< 0x00000080 */

#define PWR_CR1_PVDEN_Pos              (4U)
#define PWR_CR1_PVDEN_Msk              (0x1U << PWR_CR1_PVDEN_Pos)             /*!< 0x00000010 */
#define PWR_CR1_PVDEN                  PWR_CR1_PVDEN_Msk                       /*!< Programmable Voltage detector enable. */

#define PWR_CR1_LPDS_Pos               (0U)
#define PWR_CR1_LPDS_Msk               (0x1U << PWR_CR1_LPDS_Pos)              /*!< 0x00000001 */
#define PWR_CR1_LPDS                   PWR_CR1_LPDS_Msk                        /*!< Low Power Deepsleep with SVOS3 */

/*!< PVD level configuration */
#define PWR_CR1_PLS_LEV0               ((uint32_t)0x00000000U)                 /*!< PVD level 0 */
#define PWR_CR1_PLS_LEV1_Pos           (5U)
#define PWR_CR1_PLS_LEV1_Msk           (0x1U << PWR_CR1_PLS_LEV1_Pos)          /*!< 0x00000020 */
#define PWR_CR1_PLS_LEV1               PWR_CR1_PLS_LEV1_Msk                    /*!< PVD level 1 */

#define PWR_CR1_PLS_LEV2_Pos           (6U)
#define PWR_CR1_PLS_LEV2_Msk           (0x1U << PWR_CR1_PLS_LEV2_Pos)          /*!< 0x00000040 */
#define PWR_CR1_PLS_LEV2               PWR_CR1_PLS_LEV2_Msk                    /*!< PVD level 2 */

#define PWR_CR1_PLS_LEV3_Pos           (5U)
#define PWR_CR1_PLS_LEV3_Msk           (0x3U << PWR_CR1_PLS_LEV3_Pos)          /*!< 0x00000060 */
#define PWR_CR1_PLS_LEV3               PWR_CR1_PLS_LEV3_Msk                    /*!< PVD level 3 */

#define PWR_CR1_PLS_LEV4_Pos           (7U)
#define PWR_CR1_PLS_LEV4_Msk           (0x1U << PWR_CR1_PLS_LEV4_Pos)          /*!< 0x00000080 */
#define PWR_CR1_PLS_LEV4               PWR_CR1_PLS_LEV4_Msk                    /*!< PVD level 4 */

#define PWR_CR1_PLS_LEV5_Pos           (5U)
#define PWR_CR1_PLS_LEV5_Msk           (0x5U << PWR_CR1_PLS_LEV5_Pos)          /*!< 0x000000A0 */
#define PWR_CR1_PLS_LEV5               PWR_CR1_PLS_LEV5_Msk                    /*!< PVD level 5 */

#define PWR_CR1_PLS_LEV6_Pos           (6U)
#define PWR_CR1_PLS_LEV6_Msk           (0x3U << PWR_CR1_PLS_LEV6_Pos)          /*!< 0x000000C0 */
#define PWR_CR1_PLS_LEV6               PWR_CR1_PLS_LEV6_Msk                    /*!< PVD level 6 */

#define PWR_CR1_PLS_LEV7_Pos           (5U)
#define PWR_CR1_PLS_LEV7_Msk           (0x7U << PWR_CR1_PLS_LEV7_Pos)          /*!< 0x000000E0 */
#define PWR_CR1_PLS_LEV7               PWR_CR1_PLS_LEV7_Msk                    /*!< PVD level 7 */

/*!< AVD level configuration */
#define PWR_CR1_ALS_LEV0               ((uint32_t)0x00000000U)                 /*!< AVD level 0 */
#define PWR_CR1_ALS_LEV1_Pos           (17U)
#define PWR_CR1_ALS_LEV1_Msk           (0x1U << PWR_CR1_ALS_LEV1_Pos)          /*!< 0x00020000 */
#define PWR_CR1_ALS_LEV1               PWR_CR1_ALS_LEV1_Msk                    /*!< AVD level 1 */
#define PWR_CR1_ALS_LEV2_Pos           (18U)
#define PWR_CR1_ALS_LEV2_Msk           (0x1U << PWR_CR1_ALS_LEV2_Pos)          /*!< 0x00040000 */
#define PWR_CR1_ALS_LEV2               PWR_CR1_ALS_LEV2_Msk                    /*!< AVD level 2 */
#define PWR_CR1_ALS_LEV3_Pos           (17U)
#define PWR_CR1_ALS_LEV3_Msk           (0x3U << PWR_CR1_ALS_LEV3_Pos)          /*!< 0x00060000 */
#define PWR_CR1_ALS_LEV3               PWR_CR1_ALS_LEV3_Msk                    /*!< AVD level 3 */

/********************  Bit definition for PWR_CSR1 register  ********************/
#define PWR_CSR1_AVDO_Pos              (16U)
#define PWR_CSR1_AVDO_Msk              (0x1U << PWR_CSR1_AVDO_Pos)             /*!< 0x00010000 */
#define PWR_CSR1_AVDO                  PWR_CSR1_AVDO_Msk                       /*!< Analog Voltage Detect Output */
#define PWR_CSR1_ACTVOS_Pos            (14U)
#define PWR_CSR1_ACTVOS_Msk            (0x3U << PWR_CSR1_ACTVOS_Pos)           /*!< 0x0000C000 */
#define PWR_CSR1_ACTVOS                PWR_CSR1_ACTVOS_Msk                     /*!< Current actual used VOS for VDD11 Voltage Scaling */
#define PWR_CSR1_ACTVOS_0              (0x1U << PWR_CSR1_ACTVOS_Pos)           /*!< 0x00004000 */
#define PWR_CSR1_ACTVOS_1              (0x2U << PWR_CSR1_ACTVOS_Pos)           /*!< 0x00008000 */
#define PWR_CSR1_ACTVOSRDY_Pos         (13U)
#define PWR_CSR1_ACTVOSRDY_Msk         (0x1U << PWR_CSR1_ACTVOSRDY_Pos)        /*!< 0x00002000 */
#define PWR_CSR1_ACTVOSRDY             PWR_CSR1_ACTVOSRDY_Msk                  /*!< Ready bit for current actual used VOS for VDD11 Voltage Scaling  */
#define PWR_CSR1_PVDO_Pos              (4U)
#define PWR_CSR1_PVDO_Msk              (0x1U << PWR_CSR1_PVDO_Pos)             /*!< 0x00000010 */
#define PWR_CSR1_PVDO                  PWR_CSR1_PVDO_Msk                       /*!< Programmable Voltage Detect Output */

/********************  Bit definition for PWR_CR2 register  ********************/
#define PWR_CR2_TEMPH_Pos              (23U)
#define PWR_CR2_TEMPH_Msk              (0x1U << PWR_CR2_TEMPH_Pos)             /*!< 0x00800000 */
#define PWR_CR2_TEMPH                  PWR_CR2_TEMPH_Msk                       /*!< Monitored temperature level above high threshold */
#define PWR_CR2_TEMPL_Pos              (22U)
#define PWR_CR2_TEMPL_Msk              (0x1U << PWR_CR2_TEMPL_Pos)             /*!< 0x00400000 */
#define PWR_CR2_TEMPL                  PWR_CR2_TEMPL_Msk                       /*!< Monitored temperature level above low threshold */
#define PWR_CR2_VBATH_Pos              (21U)
#define PWR_CR2_VBATH_Msk              (0x1U << PWR_CR2_VBATH_Pos)             /*!< 0x00200000 */
#define PWR_CR2_VBATH                  PWR_CR2_VBATH_Msk                       /*!< Monitored VBAT level above high threshold */
#define PWR_CR2_VBATL_Pos              (20U)
#define PWR_CR2_VBATL_Msk              (0x1U << PWR_CR2_VBATL_Pos)             /*!< 0x00100000 */
#define PWR_CR2_VBATL                  PWR_CR2_VBATL_Msk                       /*!< Monitored VBAT level above low threshold */
#define PWR_CR2_BRRDY_Pos              (16U)
#define PWR_CR2_BRRDY_Msk              (0x1U << PWR_CR2_BRRDY_Pos)             /*!< 0x00010000 */
#define PWR_CR2_BRRDY                  PWR_CR2_BRRDY_Msk                       /*!< Backup regulator ready */
#define PWR_CR2_MONEN_Pos              (4U)
#define PWR_CR2_MONEN_Msk              (0x1U << PWR_CR2_MONEN_Pos)             /*!< 0x00000010 */
#define PWR_CR2_MONEN                  PWR_CR2_MONEN_Msk                       /*!< VBAT and temperature monitoring enable */
#define PWR_CR2_BREN_Pos               (0U)
#define PWR_CR2_BREN_Msk               (0x1U << PWR_CR2_BREN_Pos)              /*!< 0x00000001 */
#define PWR_CR2_BREN                   PWR_CR2_BREN_Msk                        /*!< Backup regulator enable */

/********************  Bit definition for PWR_CR3 register  ********************/
#define PWR_CR3_USB33RDY_Pos           (26U)
#define PWR_CR3_USB33RDY_Msk           (0x1U << PWR_CR3_USB33RDY_Pos)          /*!< 0x04000000 */
#define PWR_CR3_USB33RDY               PWR_CR3_USB33RDY_Msk                    /*!< USB supply ready */
#define PWR_CR3_USBREGEN_Pos           (25U)
#define PWR_CR3_USBREGEN_Msk           (0x1U << PWR_CR3_USBREGEN_Pos)          /*!< 0x02000000 */
#define PWR_CR3_USBREGEN               PWR_CR3_USBREGEN_Msk                    /*!< USB regulator enable */
#define PWR_CR3_USB33DEN_Pos           (24U)
#define PWR_CR3_USB33DEN_Msk           (0x1U << PWR_CR3_USB33DEN_Pos)          /*!< 0x01000000 */
#define PWR_CR3_USB33DEN               PWR_CR3_USB33DEN_Msk                    /*!< VDD33_USB voltage level detector enable */
#define PWR_CR3_VBRS_Pos               (9U)
#define PWR_CR3_VBRS_Msk               (0x1U << PWR_CR3_VBRS_Pos)              /*!< 0x00000200 */
#define PWR_CR3_VBRS                   PWR_CR3_VBRS_Msk                        /*!< VBAT charging resistor selection */
#define PWR_CR3_VBE_Pos                (8U)
#define PWR_CR3_VBE_Msk                (0x1U << PWR_CR3_VBE_Pos)               /*!< 0x00000100 */
#define PWR_CR3_VBE                    PWR_CR3_VBE_Msk                         /*!< VBAT charging enable */

// Supply configuration update enable
#define PWR_CR3_SCUEN Stdfield(PWR[PWRReg::CR3], 2, 1)

#define PWR_CR3_LDOEN_Pos              (1U)
#define PWR_CR3_LDOEN_Msk              (0x1U << PWR_CR3_LDOEN_Pos)             /*!< 0x00000002 */
#define PWR_CR3_LDOEN                  PWR_CR3_LDOEN_Msk                       /*!< Low Drop Output regulator enable */
#define PWR_CR3_BYPASS_Pos             (0U)
#define PWR_CR3_BYPASS_Msk             (0x1U << PWR_CR3_BYPASS_Pos)            /*!< 0x00000001 */
#define PWR_CR3_BYPASS                 PWR_CR3_BYPASS_Msk                      /*!< Power Management Unit bypass */

/********************  Bit definition for PWR_CPUCR register  ********************/
#define PWR_CPUCR_RUN_D3_Pos           (11U)
#define PWR_CPUCR_RUN_D3_Msk           (0x1U << PWR_CPUCR_RUN_D3_Pos)          /*!< 0x00000800 */
#define PWR_CPUCR_RUN_D3               PWR_CPUCR_RUN_D3_Msk                    /*!< Keep system D3 domain in RUN mode regardless of the CPU sub-systems modes */
#define PWR_CPUCR_CSSF_Pos             (9U)
#define PWR_CPUCR_CSSF_Msk             (0x1U << PWR_CPUCR_CSSF_Pos)            /*!< 0x00000200 */
#define PWR_CPUCR_CSSF                 PWR_CPUCR_CSSF_Msk                      /*!< Clear D1 domain CPU STANDBY, STOP and HOLD flags */
#define PWR_CPUCR_SBF_D2_Pos           (8U)
#define PWR_CPUCR_SBF_D2_Msk           (0x1U << PWR_CPUCR_SBF_D2_Pos)          /*!< 0x00000100 */
#define PWR_CPUCR_SBF_D2               PWR_CPUCR_SBF_D2_Msk                    /*!< D2 domain DSTANDBY Flag */
#define PWR_CPUCR_SBF_D1_Pos           (7U)
#define PWR_CPUCR_SBF_D1_Msk           (0x1U << PWR_CPUCR_SBF_D1_Pos)          /*!< 0x00000080 */
#define PWR_CPUCR_SBF_D1               PWR_CPUCR_SBF_D1_Msk                    /*!< D1 domain DSTANDBY Flag */
#define PWR_CPUCR_SBF_Pos              (6U)
#define PWR_CPUCR_SBF_Msk              (0x1U << PWR_CPUCR_SBF_Pos)             /*!< 0x00000040 */
#define PWR_CPUCR_SBF                  PWR_CPUCR_SBF_Msk                       /*!< System STANDBY Flag */
#define PWR_CPUCR_STOPF_Pos            (5U)
#define PWR_CPUCR_STOPF_Msk            (0x1U << PWR_CPUCR_STOPF_Pos)           /*!< 0x00000020 */
#define PWR_CPUCR_STOPF                PWR_CPUCR_STOPF_Msk                     /*!< STOP Flag */
#define PWR_CPUCR_PDDS_D3_Pos          (2U)
#define PWR_CPUCR_PDDS_D3_Msk          (0x1U << PWR_CPUCR_PDDS_D3_Pos)         /*!< 0x00000004 */
#define PWR_CPUCR_PDDS_D3              PWR_CPUCR_PDDS_D3_Msk                   /*!< System D3 domain Power Down Deepsleep */
#define PWR_CPUCR_PDDS_D2_Pos          (1U)
#define PWR_CPUCR_PDDS_D2_Msk          (0x1U << PWR_CPUCR_PDDS_D2_Pos)         /*!< 0x00000002 */
#define PWR_CPUCR_PDDS_D2              PWR_CPUCR_PDDS_D2_Msk                   /*!< D2 domain Power Down Deepsleep */
#define PWR_CPUCR_PDDS_D1_Pos          (0U)
#define PWR_CPUCR_PDDS_D1_Msk          (0x1U << PWR_CPUCR_PDDS_D1_Pos)         /*!< 0x00000001 */
#define PWR_CPUCR_PDDS_D1              PWR_CPUCR_PDDS_D1_Msk                   /*!< D1 domain Power Down Deepsleep selection */


/********************  Bit definition for PWR_D3CR register  ********************/

// Voltage Scaling selection according performance
#define PWR_D3CR_VOS Stdfield(PWR[PWRReg::D3CR], 14, 2)
// VOS Ready bit for VDD11 Voltage Scaling output selection
#define PWR_D3CR_VOSRDY Stdfield(PWR[PWRReg::D3CR], 13, 1)

/********************  Bit definition for PWR_WKUPCR register  ********************/
#define PWR_WKUPCR_WKUPC6_Pos          (5U)
#define PWR_WKUPCR_WKUPC6_Msk          (0x1U << PWR_WKUPCR_WKUPC6_Pos)         /*!< 0x00000020 */
#define PWR_WKUPCR_WKUPC6              PWR_WKUPCR_WKUPC6_Msk                   /*!< Clear Wakeup Pin Flag 6 */
#define PWR_WKUPCR_WKUPC5_Pos          (4U)
#define PWR_WKUPCR_WKUPC5_Msk          (0x1U << PWR_WKUPCR_WKUPC5_Pos)         /*!< 0x00000010 */
#define PWR_WKUPCR_WKUPC5              PWR_WKUPCR_WKUPC5_Msk                   /*!< Clear Wakeup Pin Flag 5 */
#define PWR_WKUPCR_WKUPC4_Pos          (3U)
#define PWR_WKUPCR_WKUPC4_Msk          (0x1U << PWR_WKUPCR_WKUPC4_Pos)         /*!< 0x00000008 */
#define PWR_WKUPCR_WKUPC4              PWR_WKUPCR_WKUPC4_Msk                   /*!< Clear Wakeup Pin Flag 4 */
#define PWR_WKUPCR_WKUPC3_Pos          (2U)
#define PWR_WKUPCR_WKUPC3_Msk          (0x1U << PWR_WKUPCR_WKUPC3_Pos)         /*!< 0x00000004 */
#define PWR_WKUPCR_WKUPC3              PWR_WKUPCR_WKUPC3_Msk                   /*!< Clear Wakeup Pin Flag 3 */
#define PWR_WKUPCR_WKUPC2_Pos          (1U)
#define PWR_WKUPCR_WKUPC2_Msk          (0x1U << PWR_WKUPCR_WKUPC2_Pos)         /*!< 0x00000002 */
#define PWR_WKUPCR_WKUPC2              PWR_WKUPCR_WKUPC2_Msk                   /*!< Clear Wakeup Pin Flag 2 */
#define PWR_WKUPCR_WKUPC1_Pos          (0U)
#define PWR_WKUPCR_WKUPC1_Msk          (0x1U << PWR_WKUPCR_WKUPC1_Pos)         /*!< 0x00000001 */
#define PWR_WKUPCR_WKUPC1              PWR_WKUPCR_WKUPC1_Msk                   /*!< Clear Wakeup Pin Flag 1 */

/********************  Bit definition for PWR_WKUPFR register  ********************/
#define PWR_WKUPFR_WKUPF6_Pos          (5U)
#define PWR_WKUPFR_WKUPF6_Msk          (0x1U << PWR_WKUPFR_WKUPF6_Pos)         /*!< 0x00000020 */
#define PWR_WKUPFR_WKUPF6              PWR_WKUPFR_WKUPF6_Msk                   /*!< Wakeup Pin Flag 6 */
#define PWR_WKUPFR_WKUPF5_Pos          (4U)
#define PWR_WKUPFR_WKUPF5_Msk          (0x1U << PWR_WKUPFR_WKUPF5_Pos)         /*!< 0x00000010 */
#define PWR_WKUPFR_WKUPF5              PWR_WKUPFR_WKUPF5_Msk                   /*!< Wakeup Pin Flag 5 */
#define PWR_WKUPFR_WKUPF4_Pos          (3U)
#define PWR_WKUPFR_WKUPF4_Msk          (0x1U << PWR_WKUPFR_WKUPF4_Pos)         /*!< 0x00000008 */
#define PWR_WKUPFR_WKUPF4              PWR_WKUPFR_WKUPF4_Msk                   /*!< Wakeup Pin Flag 4 */
#define PWR_WKUPFR_WKUPF3_Pos          (2U)
#define PWR_WKUPFR_WKUPF3_Msk          (0x1U << PWR_WKUPFR_WKUPF3_Pos)         /*!< 0x00000004 */
#define PWR_WKUPFR_WKUPF3              PWR_WKUPFR_WKUPF3_Msk                   /*!< Wakeup Pin Flag 3 */
#define PWR_WKUPFR_WKUPF2_Pos          (1U)
#define PWR_WKUPFR_WKUPF2_Msk          (0x1U << PWR_WKUPFR_WKUPF2_Pos)         /*!< 0x00000002 */
#define PWR_WKUPFR_WKUPF2              PWR_WKUPFR_WKUPF2_Msk                   /*!< Wakeup Pin Flag 2 */
#define PWR_WKUPFR_WKUPF1_Pos          (0U)
#define PWR_WKUPFR_WKUPF1_Msk          (0x1U << PWR_WKUPFR_WKUPF1_Pos)         /*!< 0x00000001 */
#define PWR_WKUPFR_WKUPF1              PWR_WKUPFR_WKUPF1_Msk                   /*!< Wakeup Pin Flag 1 */

/********************  Bit definition for PWR_WKUPEPR register  ********************/
#define PWR_WKUPEPR_WKUPPUPD6_Pos      (26U)
#define PWR_WKUPEPR_WKUPPUPD6_Msk      (0x3U << PWR_WKUPEPR_WKUPPUPD6_Pos)     /*!< 0x0C000000 */
#define PWR_WKUPEPR_WKUPPUPD6          PWR_WKUPEPR_WKUPPUPD6_Msk               /*!< Wakeup Pin pull configuration for WKUP6 */
#define PWR_WKUPEPR_WKUPPUPD6_0        (0x1U << PWR_WKUPEPR_WKUPPUPD6_Pos)     /*!< 0x04000000 */
#define PWR_WKUPEPR_WKUPPUPD6_1        (0x2U << PWR_WKUPEPR_WKUPPUPD6_Pos)     /*!< 0x08000000 */
#define PWR_WKUPEPR_WKUPPUPD5_Pos      (24U)
#define PWR_WKUPEPR_WKUPPUPD5_Msk      (0x3U << PWR_WKUPEPR_WKUPPUPD5_Pos)     /*!< 0x03000000 */
#define PWR_WKUPEPR_WKUPPUPD5          PWR_WKUPEPR_WKUPPUPD5_Msk               /*!< Wakeup Pin pull configuration for WKUP5 */
#define PWR_WKUPEPR_WKUPPUPD5_0        (0x1U << PWR_WKUPEPR_WKUPPUPD5_Pos)     /*!< 0x01000000 */
#define PWR_WKUPEPR_WKUPPUPD5_1        (0x2U << PWR_WKUPEPR_WKUPPUPD5_Pos)     /*!< 0x02000000 */
#define PWR_WKUPEPR_WKUPPUPD4_Pos      (22U)
#define PWR_WKUPEPR_WKUPPUPD4_Msk      (0x3U << PWR_WKUPEPR_WKUPPUPD4_Pos)     /*!< 0x00C00000 */
#define PWR_WKUPEPR_WKUPPUPD4          PWR_WKUPEPR_WKUPPUPD4_Msk               /*!< Wakeup Pin pull configuration for WKUP4 */
#define PWR_WKUPEPR_WKUPPUPD4_0        (0x1U << PWR_WKUPEPR_WKUPPUPD4_Pos)     /*!< 0x00400000 */
#define PWR_WKUPEPR_WKUPPUPD4_1        (0x2U << PWR_WKUPEPR_WKUPPUPD4_Pos)     /*!< 0x00800000 */
#define PWR_WKUPEPR_WKUPPUPD3_Pos      (20U)
#define PWR_WKUPEPR_WKUPPUPD3_Msk      (0x3U << PWR_WKUPEPR_WKUPPUPD3_Pos)     /*!< 0x00300000 */
#define PWR_WKUPEPR_WKUPPUPD3          PWR_WKUPEPR_WKUPPUPD3_Msk               /*!< Wakeup Pin pull configuration for WKUP3 */
#define PWR_WKUPEPR_WKUPPUPD3_0        (0x1U << PWR_WKUPEPR_WKUPPUPD3_Pos)     /*!< 0x00100000 */
#define PWR_WKUPEPR_WKUPPUPD3_1        (0x2U << PWR_WKUPEPR_WKUPPUPD3_Pos)     /*!< 0x00200000 */
#define PWR_WKUPEPR_WKUPPUPD2_Pos      (18U)
#define PWR_WKUPEPR_WKUPPUPD2_Msk      (0x3U << PWR_WKUPEPR_WKUPPUPD2_Pos)     /*!< 0x000C0000 */
#define PWR_WKUPEPR_WKUPPUPD2          PWR_WKUPEPR_WKUPPUPD2_Msk               /*!< Wakeup Pin pull configuration for WKUP2 */
#define PWR_WKUPEPR_WKUPPUPD2_0        (0x1U << PWR_WKUPEPR_WKUPPUPD2_Pos)     /*!< 0x00040000 */
#define PWR_WKUPEPR_WKUPPUPD2_1        (0x2U << PWR_WKUPEPR_WKUPPUPD2_Pos)     /*!< 0x00080000 */
#define PWR_WKUPEPR_WKUPPUPD1_Pos      (16U)
#define PWR_WKUPEPR_WKUPPUPD1_Msk      (0x3U << PWR_WKUPEPR_WKUPPUPD1_Pos)     /*!< 0x00030000 */
#define PWR_WKUPEPR_WKUPPUPD1          PWR_WKUPEPR_WKUPPUPD1_Msk               /*!< Wakeup Pin pull configuration for WKUP1 */
#define PWR_WKUPEPR_WKUPPUPD1_0        (0x1U << PWR_WKUPEPR_WKUPPUPD1_Pos)     /*!< 0x00010000 */
#define PWR_WKUPEPR_WKUPPUPD1_1        (0x2U << PWR_WKUPEPR_WKUPPUPD1_Pos)     /*!< 0x00020000 */
#define PWR_WKUPEPR_WKUPP_6            ((uint32_t)0x00002000U)                 /*!< Wakeup Pin Polarity for WKUP6 */
#define PWR_WKUPEPR_WKUPP_5            ((uint32_t)0x00001000U)                 /*!< Wakeup Pin Polarity for WKUP5 */
#define PWR_WKUPEPR_WKUPP_4            ((uint32_t)0x00000800U)                 /*!< Wakeup Pin Polarity for WKUP4 */
#define PWR_WKUPEPR_WKUPP_3            ((uint32_t)0x00000400U)                 /*!< Wakeup Pin Polarity for WKUP3 */
#define PWR_WKUPEPR_WKUPP_2            ((uint32_t)0x00000200U)                 /*!< Wakeup Pin Polarity for WKUP2 */
#define PWR_WKUPEPR_WKUPP_1            ((uint32_t)0x00000100U)                 /*!< Wakeup Pin Polarity for WKUP1 */
#define PWR_WKUPEPR_WKUPEN_6           ((uint32_t)0x00000020U)                 /*!< Enable Wakeup Pin WKUP6 */
#define PWR_WKUPEPR_WKUPEN_5           ((uint32_t)0x00000010U)                 /*!< Enable Wakeup Pin WKUP5 */
#define PWR_WKUPEPR_WKUPEN_4           ((uint32_t)0x00000008U)                 /*!< Enable Wakeup Pin WKUP4 */
#define PWR_WKUPEPR_WKUPEN_3           ((uint32_t)0x00000004U)                 /*!< Enable Wakeup Pin WKUP3 */
#define PWR_WKUPEPR_WKUPEN_2           ((uint32_t)0x00000002U)                 /*!< Enable Wakeup Pin WKUP2 */
#define PWR_WKUPEPR_WKUPEN_1           ((uint32_t)0x00000001U)                 /*!< Enable Wakeup Pin WKUP1 */

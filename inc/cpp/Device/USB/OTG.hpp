// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Device.USB) OTG registers
// Codifiers: @ArinaMgk
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

#ifndef _INC_DEVICE_USB_OTG
#define _INC_DEVICE_USB_OTG

#include "../../../c/stdinc.h"
#include "../../reference"
#include "../../interrupt"

#if defined(_MCU_STM32H7x)

// OTG1_HS at 0x40040000, OTG2_FS at 0x40080000
#define _OTG1_HS_ADDR 0x40040000
#define _OTG2_FS_ADDR 0x40080000

// Register offsets (byte), ported from the STM32H7 CMSIS USB_OTG_*_TypeDef.
// All offsets are absolute from the OTG base (Global/Device/Host/EP blocks).

// AKA USB_OTG_GlobalTypeDef
enum class OTGGlobalReg : unsigned {
	GOTGCTL = 0x000,
	GOTGINT = 0x004,
	GAHBCFG = 0x008,
	GUSBCFG = 0x00C,
	GRSTCTL = 0x010,
	GINTSTS = 0x014,
	GINTMSK = 0x018,
	GRXSTSR = 0x01C,
	GRXSTSP = 0x020,
	GRXFSIZ = 0x024,
	DIEPTXF0_HNPTXFSIZ = 0x028,
	HNPTXSTS = 0x02C,
	GCCFG = 0x038,
	CID = 0x03C,
	GSNPSID = 0x040,
	GHWCFG1 = 0x044,
	GHWCFG2 = 0x048,
	GHWCFG3 = 0x04C,
	GLPMCFG = 0x054,
	GPWRDN = 0x058,
	GDFIFOCFG = 0x05C,
	GADPCTL = 0x60C,
	HPTXFSIZ = 0x100,
	DIEPTXF0 = 0x104, // DIEPTXF[0..14], 4 bytes each
};

// AKA USB_OTG_DeviceTypeDef
enum class OTGDeviceReg : unsigned {
	DCFG = 0x800,
	DCTL = 0x804,
	DSTS = 0x808,
	DIEPMSK = 0x810,
	DOEPMSK = 0x814,
	DAINT = 0x818,
	DAINTMSK = 0x81C,
	DVBUSDIS = 0x828,
	DVBUSPULSE = 0x82C,
	DTHRCTL = 0x830,
	DIEPEMPMSK = 0x834,
	DEACHINT = 0x838,
	DEACHMSK = 0x83C,
	DINEP1MSK = 0x844,
	DOUTEP1MSK = 0x884,
};

// AKA USB_OTG_HostTypeDef
enum class OTGHostReg : unsigned {
	HCFG = 0x400,
	HFIR = 0x404,
	HFNUM = 0x408,
	HPTXSTS = 0x410,
	HAINT = 0x414,
	HAINTMSK = 0x418,
	HPRT = 0x440,
};

// AKA USB_OTG_INEndpointTypeDef (each IN endpoint, 0x20 bytes stride)
enum class OTGInEPReg : unsigned {
	DIEPCTL = 0x000,
	DIEPINT = 0x004,
	DIEPTSIZ = 0x008,
	DIEPDMA = 0x00C,
	DTXFSTS = 0x010,
};

// AKA USB_OTG_OUTEndpointTypeDef (each OUT endpoint, 0x20 bytes stride)
enum class OTGOutEPReg : unsigned {
	DOEPCTL = 0x000,
	DOEPINT = 0x004,
	DOEPTSIZ = 0x008,
	DOEPDMA = 0x00C,
};

// ---- USB_OTG bit definitions (full CMSIS set) ----
#define USB_OTG_GLOBAL_BASE                  ((uint32)0x000)
#define USB_OTG_DEVICE_BASE                  ((uint32)0x800)
#define USB_OTG_IN_ENDPOINT_BASE             ((uint32)0x900)
#define USB_OTG_OUT_ENDPOINT_BASE            ((uint32)0xB00)
#define USB_OTG_EP_REG_SIZE                  ((uint32)0x20)
#define USB_OTG_HOST_BASE                    ((uint32)0x400)
#define USB_OTG_HOST_PORT_BASE               ((uint32)0x440)
#define USB_OTG_HOST_CHANNEL_BASE            ((uint32)0x500)
#define USB_OTG_HOST_CHANNEL_SIZE            ((uint32)0x20)
#define USB_OTG_PCGCCTL_BASE                 ((uint32)0xE00)
#define USB_OTG_FIFO_BASE                    ((uint32)0x1000)
#define USB_OTG_FIFO_SIZE                    ((uint32)0x1000)
#define USB_OTG_HS                   USB1_OTG_HS         
#define USB_OTG_FS                   USB2_OTG_FS         
#define USB_OTG_HS_PERIPH_BASE       USB1_OTG_HS_PERIPH_BASE
#define USB_OTG_FS_PERIPH_BASE       USB2_OTG_FS_PERIPH_BASE
#define USB_OTG_GOTGCTL_SRQSCS_Pos               (0U)                          
#define USB_OTG_GOTGCTL_SRQSCS_Msk               (0x1U << USB_OTG_GOTGCTL_SRQSCS_Pos) /*!< 0x00000001 */
#define USB_OTG_GOTGCTL_SRQSCS                   USB_OTG_GOTGCTL_SRQSCS_Msk    /*!< Session request success */
#define USB_OTG_GOTGCTL_SRQ_Pos                  (1U)                          
#define USB_OTG_GOTGCTL_SRQ_Msk                  (0x1U << USB_OTG_GOTGCTL_SRQ_Pos) /*!< 0x00000002 */
#define USB_OTG_GOTGCTL_SRQ                      USB_OTG_GOTGCTL_SRQ_Msk       /*!< Session request */
#define USB_OTG_GOTGCTL_VBVALOEN_Pos             (2U)                          
#define USB_OTG_GOTGCTL_VBVALOEN_Msk             (0x1U << USB_OTG_GOTGCTL_VBVALOEN_Pos) /*!< 0x00000004 */
#define USB_OTG_GOTGCTL_VBVALOEN                 USB_OTG_GOTGCTL_VBVALOEN_Msk  /*!< VBUS valid override enable */
#define USB_OTG_GOTGCTL_VBVALOVAL_Pos            (3U)                          
#define USB_OTG_GOTGCTL_VBVALOVAL_Msk            (0x1U << USB_OTG_GOTGCTL_VBVALOVAL_Pos) /*!< 0x00000008 */
#define USB_OTG_GOTGCTL_VBVALOVAL                USB_OTG_GOTGCTL_VBVALOVAL_Msk /*!< VBUS valid override value */
#define USB_OTG_GOTGCTL_AVALOEN_Pos              (4U)                          
#define USB_OTG_GOTGCTL_AVALOEN_Msk              (0x1U << USB_OTG_GOTGCTL_AVALOEN_Pos) /*!< 0x00000010 */
#define USB_OTG_GOTGCTL_AVALOEN                  USB_OTG_GOTGCTL_AVALOEN_Msk   /*!< A-peripheral session valid override enable */
#define USB_OTG_GOTGCTL_AVALOVAL_Pos             (5U)                          
#define USB_OTG_GOTGCTL_AVALOVAL_Msk             (0x1U << USB_OTG_GOTGCTL_AVALOVAL_Pos) /*!< 0x00000020 */
#define USB_OTG_GOTGCTL_AVALOVAL                 USB_OTG_GOTGCTL_AVALOVAL_Msk  /*!< A-peripheral session valid override value */
#define USB_OTG_GOTGCTL_BVALOEN_Pos              (6U)                          
#define USB_OTG_GOTGCTL_BVALOEN_Msk              (0x1U << USB_OTG_GOTGCTL_BVALOEN_Pos) /*!< 0x00000040 */
#define USB_OTG_GOTGCTL_BVALOEN                  USB_OTG_GOTGCTL_BVALOEN_Msk   /*!< B-peripheral session valid override enable */
#define USB_OTG_GOTGCTL_BVALOVAL_Pos             (7U)                          
#define USB_OTG_GOTGCTL_BVALOVAL_Msk             (0x1U << USB_OTG_GOTGCTL_BVALOVAL_Pos) /*!< 0x00000080 */
#define USB_OTG_GOTGCTL_BVALOVAL                 USB_OTG_GOTGCTL_BVALOVAL_Msk  /*!< B-peripheral session valid override value  */
#define USB_OTG_GOTGCTL_HNGSCS_Pos               (8U)                          
#define USB_OTG_GOTGCTL_HNGSCS_Msk               (0x1U << USB_OTG_GOTGCTL_HNGSCS_Pos) /*!< 0x00000100 */
#define USB_OTG_GOTGCTL_HNGSCS                   USB_OTG_GOTGCTL_HNGSCS_Msk    /*!< Host set HNP enable */
#define USB_OTG_GOTGCTL_HNPRQ_Pos                (9U)                          
#define USB_OTG_GOTGCTL_HNPRQ_Msk                (0x1U << USB_OTG_GOTGCTL_HNPRQ_Pos) /*!< 0x00000200 */
#define USB_OTG_GOTGCTL_HNPRQ                    USB_OTG_GOTGCTL_HNPRQ_Msk     /*!< HNP request */
#define USB_OTG_GOTGCTL_HSHNPEN_Pos              (10U)                         
#define USB_OTG_GOTGCTL_HSHNPEN_Msk              (0x1U << USB_OTG_GOTGCTL_HSHNPEN_Pos) /*!< 0x00000400 */
#define USB_OTG_GOTGCTL_HSHNPEN                  USB_OTG_GOTGCTL_HSHNPEN_Msk   /*!< Host set HNP enable */
#define USB_OTG_GOTGCTL_DHNPEN_Pos               (11U)                         
#define USB_OTG_GOTGCTL_DHNPEN_Msk               (0x1U << USB_OTG_GOTGCTL_DHNPEN_Pos) /*!< 0x00000800 */
#define USB_OTG_GOTGCTL_DHNPEN                   USB_OTG_GOTGCTL_DHNPEN_Msk    /*!< Device HNP enabled */
#define USB_OTG_GOTGCTL_EHEN_Pos                 (12U)                         
#define USB_OTG_GOTGCTL_EHEN_Msk                 (0x1U << USB_OTG_GOTGCTL_EHEN_Pos) /*!< 0x00001000 */
#define USB_OTG_GOTGCTL_EHEN                     USB_OTG_GOTGCTL_EHEN_Msk      /*!< Embedded host enable */
#define USB_OTG_GOTGCTL_CIDSTS_Pos               (16U)                         
#define USB_OTG_GOTGCTL_CIDSTS_Msk               (0x1U << USB_OTG_GOTGCTL_CIDSTS_Pos) /*!< 0x00010000 */
#define USB_OTG_GOTGCTL_CIDSTS                   USB_OTG_GOTGCTL_CIDSTS_Msk    /*!< Connector ID status */
#define USB_OTG_GOTGCTL_DBCT_Pos                 (17U)                         
#define USB_OTG_GOTGCTL_DBCT_Msk                 (0x1U << USB_OTG_GOTGCTL_DBCT_Pos) /*!< 0x00020000 */
#define USB_OTG_GOTGCTL_DBCT                     USB_OTG_GOTGCTL_DBCT_Msk      /*!< Long/short debounce time */
#define USB_OTG_GOTGCTL_ASVLD_Pos                (18U)                         
#define USB_OTG_GOTGCTL_ASVLD_Msk                (0x1U << USB_OTG_GOTGCTL_ASVLD_Pos) /*!< 0x00040000 */
#define USB_OTG_GOTGCTL_ASVLD                    USB_OTG_GOTGCTL_ASVLD_Msk     /*!< A-session valid  */
#define USB_OTG_GOTGCTL_BSESVLD_Pos              (19U)                         
#define USB_OTG_GOTGCTL_BSESVLD_Msk              (0x1U << USB_OTG_GOTGCTL_BSESVLD_Pos) /*!< 0x00080000 */
#define USB_OTG_GOTGCTL_BSESVLD                  USB_OTG_GOTGCTL_BSESVLD_Msk   /*!< B-session valid */
#define USB_OTG_GOTGCTL_OTGVER_Pos               (20U)                         
#define USB_OTG_GOTGCTL_OTGVER_Msk               (0x1U << USB_OTG_GOTGCTL_OTGVER_Pos) /*!< 0x00100000 */
#define USB_OTG_GOTGCTL_OTGVER                   USB_OTG_GOTGCTL_OTGVER_Msk    /*!< OTG version  */
#define USB_OTG_HCFG_FSLSPCS_Pos                 (0U)                          
#define USB_OTG_HCFG_FSLSPCS_Msk                 (0x3U << USB_OTG_HCFG_FSLSPCS_Pos) /*!< 0x00000003 */
#define USB_OTG_HCFG_FSLSPCS                     USB_OTG_HCFG_FSLSPCS_Msk      /*!< FS/LS PHY clock select  */
#define USB_OTG_HCFG_FSLSPCS_0                   (0x1U << USB_OTG_HCFG_FSLSPCS_Pos) /*!< 0x00000001 */
#define USB_OTG_HCFG_FSLSPCS_1                   (0x2U << USB_OTG_HCFG_FSLSPCS_Pos) /*!< 0x00000002 */
#define USB_OTG_HCFG_FSLSS_Pos                   (2U)                          
#define USB_OTG_HCFG_FSLSS_Msk                   (0x1U << USB_OTG_HCFG_FSLSS_Pos) /*!< 0x00000004 */
#define USB_OTG_HCFG_FSLSS                       USB_OTG_HCFG_FSLSS_Msk        /*!< FS- and LS-only support */
#define USB_OTG_DCFG_DSPD_Pos                    (0U)                          
#define USB_OTG_DCFG_DSPD_Msk                    (0x3U << USB_OTG_DCFG_DSPD_Pos) /*!< 0x00000003 */
#define USB_OTG_DCFG_DSPD                        USB_OTG_DCFG_DSPD_Msk         /*!< Device speed */
#define USB_OTG_DCFG_DSPD_0                      (0x1U << USB_OTG_DCFG_DSPD_Pos) /*!< 0x00000001 */
#define USB_OTG_DCFG_DSPD_1                      (0x2U << USB_OTG_DCFG_DSPD_Pos) /*!< 0x00000002 */
#define USB_OTG_DCFG_NZLSOHSK_Pos                (2U)                          
#define USB_OTG_DCFG_NZLSOHSK_Msk                (0x1U << USB_OTG_DCFG_NZLSOHSK_Pos) /*!< 0x00000004 */
#define USB_OTG_DCFG_NZLSOHSK                    USB_OTG_DCFG_NZLSOHSK_Msk     /*!< Nonzero-length status OUT handshake */
#define USB_OTG_DCFG_DAD_Pos                     (4U)                          
#define USB_OTG_DCFG_DAD_Msk                     (0x7FU << USB_OTG_DCFG_DAD_Pos) /*!< 0x000007F0 */
#define USB_OTG_DCFG_DAD                         USB_OTG_DCFG_DAD_Msk          /*!< Device address */
#define USB_OTG_DCFG_DAD_0                       (0x01U << USB_OTG_DCFG_DAD_Pos) /*!< 0x00000010 */
#define USB_OTG_DCFG_DAD_1                       (0x02U << USB_OTG_DCFG_DAD_Pos) /*!< 0x00000020 */
#define USB_OTG_DCFG_DAD_2                       (0x04U << USB_OTG_DCFG_DAD_Pos) /*!< 0x00000040 */
#define USB_OTG_DCFG_DAD_3                       (0x08U << USB_OTG_DCFG_DAD_Pos) /*!< 0x00000080 */
#define USB_OTG_DCFG_DAD_4                       (0x10U << USB_OTG_DCFG_DAD_Pos) /*!< 0x00000100 */
#define USB_OTG_DCFG_DAD_5                       (0x20U << USB_OTG_DCFG_DAD_Pos) /*!< 0x00000200 */
#define USB_OTG_DCFG_DAD_6                       (0x40U << USB_OTG_DCFG_DAD_Pos) /*!< 0x00000400 */
#define USB_OTG_DCFG_PFIVL_Pos                   (11U)                         
#define USB_OTG_DCFG_PFIVL_Msk                   (0x3U << USB_OTG_DCFG_PFIVL_Pos) /*!< 0x00001800 */
#define USB_OTG_DCFG_PFIVL                       USB_OTG_DCFG_PFIVL_Msk        /*!< Periodic (micro)frame interval */
#define USB_OTG_DCFG_PFIVL_0                     (0x1U << USB_OTG_DCFG_PFIVL_Pos) /*!< 0x00000800 */
#define USB_OTG_DCFG_PFIVL_1                     (0x2U << USB_OTG_DCFG_PFIVL_Pos) /*!< 0x00001000 */
#define USB_OTG_DCFG_PERSCHIVL_Pos               (24U)                         
#define USB_OTG_DCFG_PERSCHIVL_Msk               (0x3U << USB_OTG_DCFG_PERSCHIVL_Pos) /*!< 0x03000000 */
#define USB_OTG_DCFG_PERSCHIVL                   USB_OTG_DCFG_PERSCHIVL_Msk    /*!< Periodic scheduling interval */
#define USB_OTG_DCFG_PERSCHIVL_0                 (0x1U << USB_OTG_DCFG_PERSCHIVL_Pos) /*!< 0x01000000 */
#define USB_OTG_DCFG_PERSCHIVL_1                 (0x2U << USB_OTG_DCFG_PERSCHIVL_Pos) /*!< 0x02000000 */
#define USB_OTG_PCGCR_STPPCLK_Pos                (0U)                          
#define USB_OTG_PCGCR_STPPCLK_Msk                (0x1U << USB_OTG_PCGCR_STPPCLK_Pos) /*!< 0x00000001 */
#define USB_OTG_PCGCR_STPPCLK                    USB_OTG_PCGCR_STPPCLK_Msk     /*!< Stop PHY clock */
#define USB_OTG_PCGCR_GATEHCLK_Pos               (1U)                          
#define USB_OTG_PCGCR_GATEHCLK_Msk               (0x1U << USB_OTG_PCGCR_GATEHCLK_Pos) /*!< 0x00000002 */
#define USB_OTG_PCGCR_GATEHCLK                   USB_OTG_PCGCR_GATEHCLK_Msk    /*!< Gate HCLK */
#define USB_OTG_PCGCR_PHYSUSP_Pos                (4U)                          
#define USB_OTG_PCGCR_PHYSUSP_Msk                (0x1U << USB_OTG_PCGCR_PHYSUSP_Pos) /*!< 0x00000010 */
#define USB_OTG_PCGCR_PHYSUSP                    USB_OTG_PCGCR_PHYSUSP_Msk     /*!< PHY suspended */
#define USB_OTG_GOTGINT_SEDET_Pos                (2U)                          
#define USB_OTG_GOTGINT_SEDET_Msk                (0x1U << USB_OTG_GOTGINT_SEDET_Pos) /*!< 0x00000004 */
#define USB_OTG_GOTGINT_SEDET                    USB_OTG_GOTGINT_SEDET_Msk     /*!< Session end detected                   */
#define USB_OTG_GOTGINT_SRSSCHG_Pos              (8U)                          
#define USB_OTG_GOTGINT_SRSSCHG_Msk              (0x1U << USB_OTG_GOTGINT_SRSSCHG_Pos) /*!< 0x00000100 */
#define USB_OTG_GOTGINT_SRSSCHG                  USB_OTG_GOTGINT_SRSSCHG_Msk   /*!< Session request success status change  */
#define USB_OTG_GOTGINT_HNSSCHG_Pos              (9U)                          
#define USB_OTG_GOTGINT_HNSSCHG_Msk              (0x1U << USB_OTG_GOTGINT_HNSSCHG_Pos) /*!< 0x00000200 */
#define USB_OTG_GOTGINT_HNSSCHG                  USB_OTG_GOTGINT_HNSSCHG_Msk   /*!< Host negotiation success status change */
#define USB_OTG_GOTGINT_HNGDET_Pos               (17U)                         
#define USB_OTG_GOTGINT_HNGDET_Msk               (0x1U << USB_OTG_GOTGINT_HNGDET_Pos) /*!< 0x00020000 */
#define USB_OTG_GOTGINT_HNGDET                   USB_OTG_GOTGINT_HNGDET_Msk    /*!< Host negotiation detected              */
#define USB_OTG_GOTGINT_ADTOCHG_Pos              (18U)                         
#define USB_OTG_GOTGINT_ADTOCHG_Msk              (0x1U << USB_OTG_GOTGINT_ADTOCHG_Pos) /*!< 0x00040000 */
#define USB_OTG_GOTGINT_ADTOCHG                  USB_OTG_GOTGINT_ADTOCHG_Msk   /*!< A-device timeout change                */
#define USB_OTG_GOTGINT_DBCDNE_Pos               (19U)                         
#define USB_OTG_GOTGINT_DBCDNE_Msk               (0x1U << USB_OTG_GOTGINT_DBCDNE_Pos) /*!< 0x00080000 */
#define USB_OTG_GOTGINT_DBCDNE                   USB_OTG_GOTGINT_DBCDNE_Msk    /*!< Debounce done                          */
#define USB_OTG_DCTL_RWUSIG_Pos                  (0U)                          
#define USB_OTG_DCTL_RWUSIG_Msk                  (0x1U << USB_OTG_DCTL_RWUSIG_Pos) /*!< 0x00000001 */
#define USB_OTG_DCTL_RWUSIG                      USB_OTG_DCTL_RWUSIG_Msk       /*!< Remote wakeup signaling */
#define USB_OTG_DCTL_SDIS_Pos                    (1U)                          
#define USB_OTG_DCTL_SDIS_Msk                    (0x1U << USB_OTG_DCTL_SDIS_Pos) /*!< 0x00000002 */
#define USB_OTG_DCTL_SDIS                        USB_OTG_DCTL_SDIS_Msk         /*!< Soft disconnect         */
#define USB_OTG_DCTL_GINSTS_Pos                  (2U)                          
#define USB_OTG_DCTL_GINSTS_Msk                  (0x1U << USB_OTG_DCTL_GINSTS_Pos) /*!< 0x00000004 */
#define USB_OTG_DCTL_GINSTS                      USB_OTG_DCTL_GINSTS_Msk       /*!< Global IN NAK status    */
#define USB_OTG_DCTL_GONSTS_Pos                  (3U)                          
#define USB_OTG_DCTL_GONSTS_Msk                  (0x1U << USB_OTG_DCTL_GONSTS_Pos) /*!< 0x00000008 */
#define USB_OTG_DCTL_GONSTS                      USB_OTG_DCTL_GONSTS_Msk       /*!< Global OUT NAK status   */
#define USB_OTG_DCTL_TCTL_Pos                    (4U)                          
#define USB_OTG_DCTL_TCTL_Msk                    (0x7U << USB_OTG_DCTL_TCTL_Pos) /*!< 0x00000070 */
#define USB_OTG_DCTL_TCTL                        USB_OTG_DCTL_TCTL_Msk         /*!< Test control */
#define USB_OTG_DCTL_TCTL_0                      (0x1U << USB_OTG_DCTL_TCTL_Pos) /*!< 0x00000010 */
#define USB_OTG_DCTL_TCTL_1                      (0x2U << USB_OTG_DCTL_TCTL_Pos) /*!< 0x00000020 */
#define USB_OTG_DCTL_TCTL_2                      (0x4U << USB_OTG_DCTL_TCTL_Pos) /*!< 0x00000040 */
#define USB_OTG_DCTL_SGINAK_Pos                  (7U)                          
#define USB_OTG_DCTL_SGINAK_Msk                  (0x1U << USB_OTG_DCTL_SGINAK_Pos) /*!< 0x00000080 */
#define USB_OTG_DCTL_SGINAK                      USB_OTG_DCTL_SGINAK_Msk       /*!< Set global IN NAK         */
#define USB_OTG_DCTL_CGINAK_Pos                  (8U)                          
#define USB_OTG_DCTL_CGINAK_Msk                  (0x1U << USB_OTG_DCTL_CGINAK_Pos) /*!< 0x00000100 */
#define USB_OTG_DCTL_CGINAK                      USB_OTG_DCTL_CGINAK_Msk       /*!< Clear global IN NAK       */
#define USB_OTG_DCTL_SGONAK_Pos                  (9U)                          
#define USB_OTG_DCTL_SGONAK_Msk                  (0x1U << USB_OTG_DCTL_SGONAK_Pos) /*!< 0x00000200 */
#define USB_OTG_DCTL_SGONAK                      USB_OTG_DCTL_SGONAK_Msk       /*!< Set global OUT NAK        */
#define USB_OTG_DCTL_CGONAK_Pos                  (10U)                         
#define USB_OTG_DCTL_CGONAK_Msk                  (0x1U << USB_OTG_DCTL_CGONAK_Pos) /*!< 0x00000400 */
#define USB_OTG_DCTL_CGONAK                      USB_OTG_DCTL_CGONAK_Msk       /*!< Clear global OUT NAK      */
#define USB_OTG_DCTL_POPRGDNE_Pos                (11U)                         
#define USB_OTG_DCTL_POPRGDNE_Msk                (0x1U << USB_OTG_DCTL_POPRGDNE_Pos) /*!< 0x00000800 */
#define USB_OTG_DCTL_POPRGDNE                    USB_OTG_DCTL_POPRGDNE_Msk     /*!< Power-on programming done */
#define USB_OTG_HFIR_FRIVL_Pos                   (0U)                          
#define USB_OTG_HFIR_FRIVL_Msk                   (0xFFFFU << USB_OTG_HFIR_FRIVL_Pos) /*!< 0x0000FFFF */
#define USB_OTG_HFIR_FRIVL                       USB_OTG_HFIR_FRIVL_Msk        /*!< Frame interval */
#define USB_OTG_HFNUM_FRNUM_Pos                  (0U)                          
#define USB_OTG_HFNUM_FRNUM_Msk                  (0xFFFFU << USB_OTG_HFNUM_FRNUM_Pos) /*!< 0x0000FFFF */
#define USB_OTG_HFNUM_FRNUM                      USB_OTG_HFNUM_FRNUM_Msk       /*!< Frame number         */
#define USB_OTG_HFNUM_FTREM_Pos                  (16U)                         
#define USB_OTG_HFNUM_FTREM_Msk                  (0xFFFFU << USB_OTG_HFNUM_FTREM_Pos) /*!< 0xFFFF0000 */
#define USB_OTG_HFNUM_FTREM                      USB_OTG_HFNUM_FTREM_Msk       /*!< Frame time remaining */
#define USB_OTG_DSTS_SUSPSTS_Pos                 (0U)                          
#define USB_OTG_DSTS_SUSPSTS_Msk                 (0x1U << USB_OTG_DSTS_SUSPSTS_Pos) /*!< 0x00000001 */
#define USB_OTG_DSTS_SUSPSTS                     USB_OTG_DSTS_SUSPSTS_Msk      /*!< Suspend status   */
#define USB_OTG_DSTS_ENUMSPD_Pos                 (1U)                          
#define USB_OTG_DSTS_ENUMSPD_Msk                 (0x3U << USB_OTG_DSTS_ENUMSPD_Pos) /*!< 0x00000006 */
#define USB_OTG_DSTS_ENUMSPD                     USB_OTG_DSTS_ENUMSPD_Msk      /*!< Enumerated speed */
#define USB_OTG_DSTS_ENUMSPD_0                   (0x1U << USB_OTG_DSTS_ENUMSPD_Pos) /*!< 0x00000002 */
#define USB_OTG_DSTS_ENUMSPD_1                   (0x2U << USB_OTG_DSTS_ENUMSPD_Pos) /*!< 0x00000004 */
#define USB_OTG_DSTS_EERR_Pos                    (3U)                          
#define USB_OTG_DSTS_EERR_Msk                    (0x1U << USB_OTG_DSTS_EERR_Pos) /*!< 0x00000008 */
#define USB_OTG_DSTS_EERR                        USB_OTG_DSTS_EERR_Msk         /*!< Erratic error     */
#define USB_OTG_DSTS_FNSOF_Pos                   (8U)                          
#define USB_OTG_DSTS_FNSOF_Msk                   (0x3FFFU << USB_OTG_DSTS_FNSOF_Pos) /*!< 0x003FFF00 */
#define USB_OTG_DSTS_FNSOF                       USB_OTG_DSTS_FNSOF_Msk        /*!< Frame number of the received SOF */
#define USB_OTG_GAHBCFG_GINT_Pos                 (0U)                          
#define USB_OTG_GAHBCFG_GINT_Msk                 (0x1U << USB_OTG_GAHBCFG_GINT_Pos) /*!< 0x00000001 */
#define USB_OTG_GAHBCFG_GINT                     USB_OTG_GAHBCFG_GINT_Msk      /*!< Global interrupt mask */
#define USB_OTG_GAHBCFG_HBSTLEN_Pos              (1U)                          
#define USB_OTG_GAHBCFG_HBSTLEN_Msk              (0xFU << USB_OTG_GAHBCFG_HBSTLEN_Pos) /*!< 0x0000001E */
#define USB_OTG_GAHBCFG_HBSTLEN                  USB_OTG_GAHBCFG_HBSTLEN_Msk   /*!< Burst length/type */
#define USB_OTG_GAHBCFG_HBSTLEN_0                (0x1U << USB_OTG_GAHBCFG_HBSTLEN_Pos) /*!< 0x00000002 */
#define USB_OTG_GAHBCFG_HBSTLEN_1                (0x2U << USB_OTG_GAHBCFG_HBSTLEN_Pos) /*!< 0x00000004 */
#define USB_OTG_GAHBCFG_HBSTLEN_2                (0x4U << USB_OTG_GAHBCFG_HBSTLEN_Pos) /*!< 0x00000008 */
#define USB_OTG_GAHBCFG_HBSTLEN_3                (0x8U << USB_OTG_GAHBCFG_HBSTLEN_Pos) /*!< 0x00000010 */
#define USB_OTG_GAHBCFG_DMAEN_Pos                (5U)                          
#define USB_OTG_GAHBCFG_DMAEN_Msk                (0x1U << USB_OTG_GAHBCFG_DMAEN_Pos) /*!< 0x00000020 */
#define USB_OTG_GAHBCFG_DMAEN                    USB_OTG_GAHBCFG_DMAEN_Msk     /*!< DMA enable */
#define USB_OTG_GAHBCFG_TXFELVL_Pos              (7U)                          
#define USB_OTG_GAHBCFG_TXFELVL_Msk              (0x1U << USB_OTG_GAHBCFG_TXFELVL_Pos) /*!< 0x00000080 */
#define USB_OTG_GAHBCFG_TXFELVL                  USB_OTG_GAHBCFG_TXFELVL_Msk   /*!< TxFIFO empty level */
#define USB_OTG_GAHBCFG_PTXFELVL_Pos             (8U)                          
#define USB_OTG_GAHBCFG_PTXFELVL_Msk             (0x1U << USB_OTG_GAHBCFG_PTXFELVL_Pos) /*!< 0x00000100 */
#define USB_OTG_GAHBCFG_PTXFELVL                 USB_OTG_GAHBCFG_PTXFELVL_Msk  /*!< Periodic TxFIFO empty level */
#define USB_OTG_GUSBCFG_TOCAL_Pos                (0U)                          
#define USB_OTG_GUSBCFG_TOCAL_Msk                (0x7U << USB_OTG_GUSBCFG_TOCAL_Pos) /*!< 0x00000007 */
#define USB_OTG_GUSBCFG_TOCAL                    USB_OTG_GUSBCFG_TOCAL_Msk     /*!< FS timeout calibration */
#define USB_OTG_GUSBCFG_TOCAL_0                  (0x1U << USB_OTG_GUSBCFG_TOCAL_Pos) /*!< 0x00000001 */
#define USB_OTG_GUSBCFG_TOCAL_1                  (0x2U << USB_OTG_GUSBCFG_TOCAL_Pos) /*!< 0x00000002 */
#define USB_OTG_GUSBCFG_TOCAL_2                  (0x4U << USB_OTG_GUSBCFG_TOCAL_Pos) /*!< 0x00000004 */
#define USB_OTG_GUSBCFG_PHYSEL_Pos               (6U)                          
#define USB_OTG_GUSBCFG_PHYSEL_Msk               (0x1U << USB_OTG_GUSBCFG_PHYSEL_Pos) /*!< 0x00000040 */
#define USB_OTG_GUSBCFG_PHYSEL                   USB_OTG_GUSBCFG_PHYSEL_Msk    /*!< USB 2.0 high-speed ULPI PHY or USB 1.1 full-speed serial transceiver select */
#define USB_OTG_GUSBCFG_SRPCAP_Pos               (8U)                          
#define USB_OTG_GUSBCFG_SRPCAP_Msk               (0x1U << USB_OTG_GUSBCFG_SRPCAP_Pos) /*!< 0x00000100 */
#define USB_OTG_GUSBCFG_SRPCAP                   USB_OTG_GUSBCFG_SRPCAP_Msk    /*!< SRP-capable */
#define USB_OTG_GUSBCFG_HNPCAP_Pos               (9U)                          
#define USB_OTG_GUSBCFG_HNPCAP_Msk               (0x1U << USB_OTG_GUSBCFG_HNPCAP_Pos) /*!< 0x00000200 */
#define USB_OTG_GUSBCFG_HNPCAP                   USB_OTG_GUSBCFG_HNPCAP_Msk    /*!< HNP-capable */
#define USB_OTG_GUSBCFG_TRDT_Pos                 (10U)                         
#define USB_OTG_GUSBCFG_TRDT_Msk                 (0xFU << USB_OTG_GUSBCFG_TRDT_Pos) /*!< 0x00003C00 */
#define USB_OTG_GUSBCFG_TRDT                     USB_OTG_GUSBCFG_TRDT_Msk      /*!< USB turnaround time */
#define USB_OTG_GUSBCFG_TRDT_0                   (0x1U << USB_OTG_GUSBCFG_TRDT_Pos) /*!< 0x00000400 */
#define USB_OTG_GUSBCFG_TRDT_1                   (0x2U << USB_OTG_GUSBCFG_TRDT_Pos) /*!< 0x00000800 */
#define USB_OTG_GUSBCFG_TRDT_2                   (0x4U << USB_OTG_GUSBCFG_TRDT_Pos) /*!< 0x00001000 */
#define USB_OTG_GUSBCFG_TRDT_3                   (0x8U << USB_OTG_GUSBCFG_TRDT_Pos) /*!< 0x00002000 */
#define USB_OTG_GUSBCFG_PHYLPCS_Pos              (15U)                         
#define USB_OTG_GUSBCFG_PHYLPCS_Msk              (0x1U << USB_OTG_GUSBCFG_PHYLPCS_Pos) /*!< 0x00008000 */
#define USB_OTG_GUSBCFG_PHYLPCS                  USB_OTG_GUSBCFG_PHYLPCS_Msk   /*!< PHY Low-power clock select */
#define USB_OTG_GUSBCFG_ULPIFSLS_Pos             (17U)                         
#define USB_OTG_GUSBCFG_ULPIFSLS_Msk             (0x1U << USB_OTG_GUSBCFG_ULPIFSLS_Pos) /*!< 0x00020000 */
#define USB_OTG_GUSBCFG_ULPIFSLS                 USB_OTG_GUSBCFG_ULPIFSLS_Msk  /*!< ULPI FS/LS select               */
#define USB_OTG_GUSBCFG_ULPIAR_Pos               (18U)                         
#define USB_OTG_GUSBCFG_ULPIAR_Msk               (0x1U << USB_OTG_GUSBCFG_ULPIAR_Pos) /*!< 0x00040000 */
#define USB_OTG_GUSBCFG_ULPIAR                   USB_OTG_GUSBCFG_ULPIAR_Msk    /*!< ULPI Auto-resume                */
#define USB_OTG_GUSBCFG_ULPICSM_Pos              (19U)                         
#define USB_OTG_GUSBCFG_ULPICSM_Msk              (0x1U << USB_OTG_GUSBCFG_ULPICSM_Pos) /*!< 0x00080000 */
#define USB_OTG_GUSBCFG_ULPICSM                  USB_OTG_GUSBCFG_ULPICSM_Msk   /*!< ULPI Clock SuspendM             */
#define USB_OTG_GUSBCFG_ULPIEVBUSD_Pos           (20U)                         
#define USB_OTG_GUSBCFG_ULPIEVBUSD_Msk           (0x1U << USB_OTG_GUSBCFG_ULPIEVBUSD_Pos) /*!< 0x00100000 */
#define USB_OTG_GUSBCFG_ULPIEVBUSD               USB_OTG_GUSBCFG_ULPIEVBUSD_Msk /*!< ULPI External VBUS Drive        */
#define USB_OTG_GUSBCFG_ULPIEVBUSI_Pos           (21U)                         
#define USB_OTG_GUSBCFG_ULPIEVBUSI_Msk           (0x1U << USB_OTG_GUSBCFG_ULPIEVBUSI_Pos) /*!< 0x00200000 */
#define USB_OTG_GUSBCFG_ULPIEVBUSI               USB_OTG_GUSBCFG_ULPIEVBUSI_Msk /*!< ULPI external VBUS indicator    */
#define USB_OTG_GUSBCFG_TSDPS_Pos                (22U)                         
#define USB_OTG_GUSBCFG_TSDPS_Msk                (0x1U << USB_OTG_GUSBCFG_TSDPS_Pos) /*!< 0x00400000 */
#define USB_OTG_GUSBCFG_TSDPS                    USB_OTG_GUSBCFG_TSDPS_Msk     /*!< TermSel DLine pulsing selection */
#define USB_OTG_GUSBCFG_PCCI_Pos                 (23U)                         
#define USB_OTG_GUSBCFG_PCCI_Msk                 (0x1U << USB_OTG_GUSBCFG_PCCI_Pos) /*!< 0x00800000 */
#define USB_OTG_GUSBCFG_PCCI                     USB_OTG_GUSBCFG_PCCI_Msk      /*!< Indicator complement            */
#define USB_OTG_GUSBCFG_PTCI_Pos                 (24U)                         
#define USB_OTG_GUSBCFG_PTCI_Msk                 (0x1U << USB_OTG_GUSBCFG_PTCI_Pos) /*!< 0x01000000 */
#define USB_OTG_GUSBCFG_PTCI                     USB_OTG_GUSBCFG_PTCI_Msk      /*!< Indicator pass through          */
#define USB_OTG_GUSBCFG_ULPIIPD_Pos              (25U)                         
#define USB_OTG_GUSBCFG_ULPIIPD_Msk              (0x1U << USB_OTG_GUSBCFG_ULPIIPD_Pos) /*!< 0x02000000 */
#define USB_OTG_GUSBCFG_ULPIIPD                  USB_OTG_GUSBCFG_ULPIIPD_Msk   /*!< ULPI interface protect disable  */
#define USB_OTG_GUSBCFG_FHMOD_Pos                (29U)                         
#define USB_OTG_GUSBCFG_FHMOD_Msk                (0x1U << USB_OTG_GUSBCFG_FHMOD_Pos) /*!< 0x20000000 */
#define USB_OTG_GUSBCFG_FHMOD                    USB_OTG_GUSBCFG_FHMOD_Msk     /*!< Forced host mode                */
#define USB_OTG_GUSBCFG_FDMOD_Pos                (30U)                         
#define USB_OTG_GUSBCFG_FDMOD_Msk                (0x1U << USB_OTG_GUSBCFG_FDMOD_Pos) /*!< 0x40000000 */
#define USB_OTG_GUSBCFG_FDMOD                    USB_OTG_GUSBCFG_FDMOD_Msk     /*!< Forced peripheral mode          */
#define USB_OTG_GUSBCFG_CTXPKT_Pos               (31U)                         
#define USB_OTG_GUSBCFG_CTXPKT_Msk               (0x1U << USB_OTG_GUSBCFG_CTXPKT_Pos) /*!< 0x80000000 */
#define USB_OTG_GUSBCFG_CTXPKT                   USB_OTG_GUSBCFG_CTXPKT_Msk    /*!< Corrupt Tx packet               */
#define USB_OTG_GRSTCTL_CSRST_Pos                (0U)                          
#define USB_OTG_GRSTCTL_CSRST_Msk                (0x1U << USB_OTG_GRSTCTL_CSRST_Pos) /*!< 0x00000001 */
#define USB_OTG_GRSTCTL_CSRST                    USB_OTG_GRSTCTL_CSRST_Msk     /*!< Core soft reset          */
#define USB_OTG_GRSTCTL_HSRST_Pos                (1U)                          
#define USB_OTG_GRSTCTL_HSRST_Msk                (0x1U << USB_OTG_GRSTCTL_HSRST_Pos) /*!< 0x00000002 */
#define USB_OTG_GRSTCTL_HSRST                    USB_OTG_GRSTCTL_HSRST_Msk     /*!< HCLK soft reset          */
#define USB_OTG_GRSTCTL_FCRST_Pos                (2U)                          
#define USB_OTG_GRSTCTL_FCRST_Msk                (0x1U << USB_OTG_GRSTCTL_FCRST_Pos) /*!< 0x00000004 */
#define USB_OTG_GRSTCTL_FCRST                    USB_OTG_GRSTCTL_FCRST_Msk     /*!< Host frame counter reset */
#define USB_OTG_GRSTCTL_RXFFLSH_Pos              (4U)                          
#define USB_OTG_GRSTCTL_RXFFLSH_Msk              (0x1U << USB_OTG_GRSTCTL_RXFFLSH_Pos) /*!< 0x00000010 */
#define USB_OTG_GRSTCTL_RXFFLSH                  USB_OTG_GRSTCTL_RXFFLSH_Msk   /*!< RxFIFO flush             */
#define USB_OTG_GRSTCTL_TXFFLSH_Pos              (5U)                          
#define USB_OTG_GRSTCTL_TXFFLSH_Msk              (0x1U << USB_OTG_GRSTCTL_TXFFLSH_Pos) /*!< 0x00000020 */
#define USB_OTG_GRSTCTL_TXFFLSH                  USB_OTG_GRSTCTL_TXFFLSH_Msk   /*!< TxFIFO flush             */
#define USB_OTG_GRSTCTL_TXFNUM_Pos               (6U)                          
#define USB_OTG_GRSTCTL_TXFNUM_Msk               (0x1FU << USB_OTG_GRSTCTL_TXFNUM_Pos) /*!< 0x000007C0 */
#define USB_OTG_GRSTCTL_TXFNUM                   USB_OTG_GRSTCTL_TXFNUM_Msk    /*!< TxFIFO number */
#define USB_OTG_GRSTCTL_TXFNUM_0                 (0x01U << USB_OTG_GRSTCTL_TXFNUM_Pos) /*!< 0x00000040 */
#define USB_OTG_GRSTCTL_TXFNUM_1                 (0x02U << USB_OTG_GRSTCTL_TXFNUM_Pos) /*!< 0x00000080 */
#define USB_OTG_GRSTCTL_TXFNUM_2                 (0x04U << USB_OTG_GRSTCTL_TXFNUM_Pos) /*!< 0x00000100 */
#define USB_OTG_GRSTCTL_TXFNUM_3                 (0x08U << USB_OTG_GRSTCTL_TXFNUM_Pos) /*!< 0x00000200 */
#define USB_OTG_GRSTCTL_TXFNUM_4                 (0x10U << USB_OTG_GRSTCTL_TXFNUM_Pos) /*!< 0x00000400 */
#define USB_OTG_GRSTCTL_DMAREQ_Pos               (30U)                         
#define USB_OTG_GRSTCTL_DMAREQ_Msk               (0x1U << USB_OTG_GRSTCTL_DMAREQ_Pos) /*!< 0x40000000 */
#define USB_OTG_GRSTCTL_DMAREQ                   USB_OTG_GRSTCTL_DMAREQ_Msk    /*!< DMA request signal */
#define USB_OTG_GRSTCTL_AHBIDL_Pos               (31U)                         
#define USB_OTG_GRSTCTL_AHBIDL_Msk               (0x1U << USB_OTG_GRSTCTL_AHBIDL_Pos) /*!< 0x80000000 */
#define USB_OTG_GRSTCTL_AHBIDL                   USB_OTG_GRSTCTL_AHBIDL_Msk    /*!< AHB master idle */
#define USB_OTG_DIEPMSK_XFRCM_Pos                (0U)                          
#define USB_OTG_DIEPMSK_XFRCM_Msk                (0x1U << USB_OTG_DIEPMSK_XFRCM_Pos) /*!< 0x00000001 */
#define USB_OTG_DIEPMSK_XFRCM                    USB_OTG_DIEPMSK_XFRCM_Msk     /*!< Transfer completed interrupt mask                 */
#define USB_OTG_DIEPMSK_EPDM_Pos                 (1U)                          
#define USB_OTG_DIEPMSK_EPDM_Msk                 (0x1U << USB_OTG_DIEPMSK_EPDM_Pos) /*!< 0x00000002 */
#define USB_OTG_DIEPMSK_EPDM                     USB_OTG_DIEPMSK_EPDM_Msk      /*!< Endpoint disabled interrupt mask                  */
#define USB_OTG_DIEPMSK_TOM_Pos                  (3U)                          
#define USB_OTG_DIEPMSK_TOM_Msk                  (0x1U << USB_OTG_DIEPMSK_TOM_Pos) /*!< 0x00000008 */
#define USB_OTG_DIEPMSK_TOM                      USB_OTG_DIEPMSK_TOM_Msk       /*!< Timeout condition mask (nonisochronous endpoints) */
#define USB_OTG_DIEPMSK_ITTXFEMSK_Pos            (4U)                          
#define USB_OTG_DIEPMSK_ITTXFEMSK_Msk            (0x1U << USB_OTG_DIEPMSK_ITTXFEMSK_Pos) /*!< 0x00000010 */
#define USB_OTG_DIEPMSK_ITTXFEMSK                USB_OTG_DIEPMSK_ITTXFEMSK_Msk /*!< IN token received when TxFIFO empty mask          */
#define USB_OTG_DIEPMSK_INEPNMM_Pos              (5U)                          
#define USB_OTG_DIEPMSK_INEPNMM_Msk              (0x1U << USB_OTG_DIEPMSK_INEPNMM_Pos) /*!< 0x00000020 */
#define USB_OTG_DIEPMSK_INEPNMM                  USB_OTG_DIEPMSK_INEPNMM_Msk   /*!< IN token received with EP mismatch mask           */
#define USB_OTG_DIEPMSK_INEPNEM_Pos              (6U)                          
#define USB_OTG_DIEPMSK_INEPNEM_Msk              (0x1U << USB_OTG_DIEPMSK_INEPNEM_Pos) /*!< 0x00000040 */
#define USB_OTG_DIEPMSK_INEPNEM                  USB_OTG_DIEPMSK_INEPNEM_Msk   /*!< IN endpoint NAK effective mask                    */
#define USB_OTG_DIEPMSK_TXFURM_Pos               (8U)                          
#define USB_OTG_DIEPMSK_TXFURM_Msk               (0x1U << USB_OTG_DIEPMSK_TXFURM_Pos) /*!< 0x00000100 */
#define USB_OTG_DIEPMSK_TXFURM                   USB_OTG_DIEPMSK_TXFURM_Msk    /*!< FIFO underrun mask                                */
#define USB_OTG_DIEPMSK_BIM_Pos                  (9U)                          
#define USB_OTG_DIEPMSK_BIM_Msk                  (0x1U << USB_OTG_DIEPMSK_BIM_Pos) /*!< 0x00000200 */
#define USB_OTG_DIEPMSK_BIM                      USB_OTG_DIEPMSK_BIM_Msk       /*!< BNA interrupt mask                                */
#define USB_OTG_HPTXSTS_PTXFSAVL_Pos             (0U)                          
#define USB_OTG_HPTXSTS_PTXFSAVL_Msk             (0xFFFFU << USB_OTG_HPTXSTS_PTXFSAVL_Pos) /*!< 0x0000FFFF */
#define USB_OTG_HPTXSTS_PTXFSAVL                 USB_OTG_HPTXSTS_PTXFSAVL_Msk  /*!< Periodic transmit data FIFO space available     */
#define USB_OTG_HPTXSTS_PTXQSAV_Pos              (16U)                         
#define USB_OTG_HPTXSTS_PTXQSAV_Msk              (0xFFU << USB_OTG_HPTXSTS_PTXQSAV_Pos) /*!< 0x00FF0000 */
#define USB_OTG_HPTXSTS_PTXQSAV                  USB_OTG_HPTXSTS_PTXQSAV_Msk   /*!< Periodic transmit request queue space available */
#define USB_OTG_HPTXSTS_PTXQSAV_0                (0x01U << USB_OTG_HPTXSTS_PTXQSAV_Pos) /*!< 0x00010000 */
#define USB_OTG_HPTXSTS_PTXQSAV_1                (0x02U << USB_OTG_HPTXSTS_PTXQSAV_Pos) /*!< 0x00020000 */
#define USB_OTG_HPTXSTS_PTXQSAV_2                (0x04U << USB_OTG_HPTXSTS_PTXQSAV_Pos) /*!< 0x00040000 */
#define USB_OTG_HPTXSTS_PTXQSAV_3                (0x08U << USB_OTG_HPTXSTS_PTXQSAV_Pos) /*!< 0x00080000 */
#define USB_OTG_HPTXSTS_PTXQSAV_4                (0x10U << USB_OTG_HPTXSTS_PTXQSAV_Pos) /*!< 0x00100000 */
#define USB_OTG_HPTXSTS_PTXQSAV_5                (0x20U << USB_OTG_HPTXSTS_PTXQSAV_Pos) /*!< 0x00200000 */
#define USB_OTG_HPTXSTS_PTXQSAV_6                (0x40U << USB_OTG_HPTXSTS_PTXQSAV_Pos) /*!< 0x00400000 */
#define USB_OTG_HPTXSTS_PTXQSAV_7                (0x80U << USB_OTG_HPTXSTS_PTXQSAV_Pos) /*!< 0x00800000 */
#define USB_OTG_HPTXSTS_PTXQTOP_Pos              (24U)                         
#define USB_OTG_HPTXSTS_PTXQTOP_Msk              (0xFFU << USB_OTG_HPTXSTS_PTXQTOP_Pos) /*!< 0xFF000000 */
#define USB_OTG_HPTXSTS_PTXQTOP                  USB_OTG_HPTXSTS_PTXQTOP_Msk   /*!< Top of the periodic transmit request queue */
#define USB_OTG_HPTXSTS_PTXQTOP_0                (0x01U << USB_OTG_HPTXSTS_PTXQTOP_Pos) /*!< 0x01000000 */
#define USB_OTG_HPTXSTS_PTXQTOP_1                (0x02U << USB_OTG_HPTXSTS_PTXQTOP_Pos) /*!< 0x02000000 */
#define USB_OTG_HPTXSTS_PTXQTOP_2                (0x04U << USB_OTG_HPTXSTS_PTXQTOP_Pos) /*!< 0x04000000 */
#define USB_OTG_HPTXSTS_PTXQTOP_3                (0x08U << USB_OTG_HPTXSTS_PTXQTOP_Pos) /*!< 0x08000000 */
#define USB_OTG_HPTXSTS_PTXQTOP_4                (0x10U << USB_OTG_HPTXSTS_PTXQTOP_Pos) /*!< 0x10000000 */
#define USB_OTG_HPTXSTS_PTXQTOP_5                (0x20U << USB_OTG_HPTXSTS_PTXQTOP_Pos) /*!< 0x20000000 */
#define USB_OTG_HPTXSTS_PTXQTOP_6                (0x40U << USB_OTG_HPTXSTS_PTXQTOP_Pos) /*!< 0x40000000 */
#define USB_OTG_HPTXSTS_PTXQTOP_7                (0x80U << USB_OTG_HPTXSTS_PTXQTOP_Pos) /*!< 0x80000000 */
#define USB_OTG_HAINT_HAINT_Pos                  (0U)                          
#define USB_OTG_HAINT_HAINT_Msk                  (0xFFFFU << USB_OTG_HAINT_HAINT_Pos) /*!< 0x0000FFFF */
#define USB_OTG_HAINT_HAINT                      USB_OTG_HAINT_HAINT_Msk       /*!< Channel interrupts */
#define USB_OTG_DOEPMSK_XFRCM_Pos                (0U)                          
#define USB_OTG_DOEPMSK_XFRCM_Msk                (0x1U << USB_OTG_DOEPMSK_XFRCM_Pos) /*!< 0x00000001 */
#define USB_OTG_DOEPMSK_XFRCM                    USB_OTG_DOEPMSK_XFRCM_Msk     /*!< Transfer completed interrupt mask */
#define USB_OTG_DOEPMSK_EPDM_Pos                 (1U)                          
#define USB_OTG_DOEPMSK_EPDM_Msk                 (0x1U << USB_OTG_DOEPMSK_EPDM_Pos) /*!< 0x00000002 */
#define USB_OTG_DOEPMSK_EPDM                     USB_OTG_DOEPMSK_EPDM_Msk      /*!< Endpoint disabled interrupt mask               */
#define USB_OTG_DOEPMSK_STUPM_Pos                (3U)                          
#define USB_OTG_DOEPMSK_STUPM_Msk                (0x1U << USB_OTG_DOEPMSK_STUPM_Pos) /*!< 0x00000008 */
#define USB_OTG_DOEPMSK_STUPM                    USB_OTG_DOEPMSK_STUPM_Msk     /*!< SETUP phase done mask                          */
#define USB_OTG_DOEPMSK_OTEPDM_Pos               (4U)                          
#define USB_OTG_DOEPMSK_OTEPDM_Msk               (0x1U << USB_OTG_DOEPMSK_OTEPDM_Pos) /*!< 0x00000010 */
#define USB_OTG_DOEPMSK_OTEPDM                   USB_OTG_DOEPMSK_OTEPDM_Msk    /*!< OUT token received when endpoint disabled mask */
#define USB_OTG_DOEPMSK_B2BSTUP_Pos              (6U)                          
#define USB_OTG_DOEPMSK_B2BSTUP_Msk              (0x1U << USB_OTG_DOEPMSK_B2BSTUP_Pos) /*!< 0x00000040 */
#define USB_OTG_DOEPMSK_B2BSTUP                  USB_OTG_DOEPMSK_B2BSTUP_Msk   /*!< Back-to-back SETUP packets received mask       */
#define USB_OTG_DOEPMSK_OPEM_Pos                 (8U)                          
#define USB_OTG_DOEPMSK_OPEM_Msk                 (0x1U << USB_OTG_DOEPMSK_OPEM_Pos) /*!< 0x00000100 */
#define USB_OTG_DOEPMSK_OPEM                     USB_OTG_DOEPMSK_OPEM_Msk      /*!< OUT packet error mask                          */
#define USB_OTG_DOEPMSK_BOIM_Pos                 (9U)                          
#define USB_OTG_DOEPMSK_BOIM_Msk                 (0x1U << USB_OTG_DOEPMSK_BOIM_Pos) /*!< 0x00000200 */
#define USB_OTG_DOEPMSK_BOIM                     USB_OTG_DOEPMSK_BOIM_Msk      /*!< BNA interrupt mask                             */
#define USB_OTG_GINTSTS_CMOD_Pos                 (0U)                          
#define USB_OTG_GINTSTS_CMOD_Msk                 (0x1U << USB_OTG_GINTSTS_CMOD_Pos) /*!< 0x00000001 */
#define USB_OTG_GINTSTS_CMOD                     USB_OTG_GINTSTS_CMOD_Msk      /*!< Current mode of operation                      */
#define USB_OTG_GINTSTS_MMIS_Pos                 (1U)                          
#define USB_OTG_GINTSTS_MMIS_Msk                 (0x1U << USB_OTG_GINTSTS_MMIS_Pos) /*!< 0x00000002 */
#define USB_OTG_GINTSTS_MMIS                     USB_OTG_GINTSTS_MMIS_Msk      /*!< Mode mismatch interrupt                        */
#define USB_OTG_GINTSTS_OTGINT_Pos               (2U)                          
#define USB_OTG_GINTSTS_OTGINT_Msk               (0x1U << USB_OTG_GINTSTS_OTGINT_Pos) /*!< 0x00000004 */
#define USB_OTG_GINTSTS_OTGINT                   USB_OTG_GINTSTS_OTGINT_Msk    /*!< OTG interrupt                                  */
#define USB_OTG_GINTSTS_SOF_Pos                  (3U)                          
#define USB_OTG_GINTSTS_SOF_Msk                  (0x1U << USB_OTG_GINTSTS_SOF_Pos) /*!< 0x00000008 */
#define USB_OTG_GINTSTS_SOF                      USB_OTG_GINTSTS_SOF_Msk       /*!< Start of frame                                 */
#define USB_OTG_GINTSTS_RXFLVL_Pos               (4U)                          
#define USB_OTG_GINTSTS_RXFLVL_Msk               (0x1U << USB_OTG_GINTSTS_RXFLVL_Pos) /*!< 0x00000010 */
#define USB_OTG_GINTSTS_RXFLVL                   USB_OTG_GINTSTS_RXFLVL_Msk    /*!< RxFIFO nonempty                                */
#define USB_OTG_GINTSTS_NPTXFE_Pos               (5U)                          
#define USB_OTG_GINTSTS_NPTXFE_Msk               (0x1U << USB_OTG_GINTSTS_NPTXFE_Pos) /*!< 0x00000020 */
#define USB_OTG_GINTSTS_NPTXFE                   USB_OTG_GINTSTS_NPTXFE_Msk    /*!< Nonperiodic TxFIFO empty                       */
#define USB_OTG_GINTSTS_GINAKEFF_Pos             (6U)                          
#define USB_OTG_GINTSTS_GINAKEFF_Msk             (0x1U << USB_OTG_GINTSTS_GINAKEFF_Pos) /*!< 0x00000040 */
#define USB_OTG_GINTSTS_GINAKEFF                 USB_OTG_GINTSTS_GINAKEFF_Msk  /*!< Global IN nonperiodic NAK effective            */
#define USB_OTG_GINTSTS_BOUTNAKEFF_Pos           (7U)                          
#define USB_OTG_GINTSTS_BOUTNAKEFF_Msk           (0x1U << USB_OTG_GINTSTS_BOUTNAKEFF_Pos) /*!< 0x00000080 */
#define USB_OTG_GINTSTS_BOUTNAKEFF               USB_OTG_GINTSTS_BOUTNAKEFF_Msk /*!< Global OUT NAK effective                       */
#define USB_OTG_GINTSTS_ESUSP_Pos                (10U)                         
#define USB_OTG_GINTSTS_ESUSP_Msk                (0x1U << USB_OTG_GINTSTS_ESUSP_Pos) /*!< 0x00000400 */
#define USB_OTG_GINTSTS_ESUSP                    USB_OTG_GINTSTS_ESUSP_Msk     /*!< Early suspend                                  */
#define USB_OTG_GINTSTS_USBSUSP_Pos              (11U)                         
#define USB_OTG_GINTSTS_USBSUSP_Msk              (0x1U << USB_OTG_GINTSTS_USBSUSP_Pos) /*!< 0x00000800 */
#define USB_OTG_GINTSTS_USBSUSP                  USB_OTG_GINTSTS_USBSUSP_Msk   /*!< USB suspend                                    */
#define USB_OTG_GINTSTS_USBRST_Pos               (12U)                         
#define USB_OTG_GINTSTS_USBRST_Msk               (0x1U << USB_OTG_GINTSTS_USBRST_Pos) /*!< 0x00001000 */
#define USB_OTG_GINTSTS_USBRST                   USB_OTG_GINTSTS_USBRST_Msk    /*!< USB reset                                      */
#define USB_OTG_GINTSTS_ENUMDNE_Pos              (13U)                         
#define USB_OTG_GINTSTS_ENUMDNE_Msk              (0x1U << USB_OTG_GINTSTS_ENUMDNE_Pos) /*!< 0x00002000 */
#define USB_OTG_GINTSTS_ENUMDNE                  USB_OTG_GINTSTS_ENUMDNE_Msk   /*!< Enumeration done                               */
#define USB_OTG_GINTSTS_ISOODRP_Pos              (14U)                         
#define USB_OTG_GINTSTS_ISOODRP_Msk              (0x1U << USB_OTG_GINTSTS_ISOODRP_Pos) /*!< 0x00004000 */
#define USB_OTG_GINTSTS_ISOODRP                  USB_OTG_GINTSTS_ISOODRP_Msk   /*!< Isochronous OUT packet dropped interrupt       */
#define USB_OTG_GINTSTS_EOPF_Pos                 (15U)                         
#define USB_OTG_GINTSTS_EOPF_Msk                 (0x1U << USB_OTG_GINTSTS_EOPF_Pos) /*!< 0x00008000 */
#define USB_OTG_GINTSTS_EOPF                     USB_OTG_GINTSTS_EOPF_Msk      /*!< End of periodic frame interrupt                */
#define USB_OTG_GINTSTS_IEPINT_Pos               (18U)                         
#define USB_OTG_GINTSTS_IEPINT_Msk               (0x1U << USB_OTG_GINTSTS_IEPINT_Pos) /*!< 0x00040000 */
#define USB_OTG_GINTSTS_IEPINT                   USB_OTG_GINTSTS_IEPINT_Msk    /*!< IN endpoint interrupt                          */
#define USB_OTG_GINTSTS_OEPINT_Pos               (19U)                         
#define USB_OTG_GINTSTS_OEPINT_Msk               (0x1U << USB_OTG_GINTSTS_OEPINT_Pos) /*!< 0x00080000 */
#define USB_OTG_GINTSTS_OEPINT                   USB_OTG_GINTSTS_OEPINT_Msk    /*!< OUT endpoint interrupt                         */
#define USB_OTG_GINTSTS_IISOIXFR_Pos             (20U)                         
#define USB_OTG_GINTSTS_IISOIXFR_Msk             (0x1U << USB_OTG_GINTSTS_IISOIXFR_Pos) /*!< 0x00100000 */
#define USB_OTG_GINTSTS_IISOIXFR                 USB_OTG_GINTSTS_IISOIXFR_Msk  /*!< Incomplete isochronous IN transfer             */
#define USB_OTG_GINTSTS_PXFR_INCOMPISOOUT_Pos    (21U)                         
#define USB_OTG_GINTSTS_PXFR_INCOMPISOOUT_Msk    (0x1U << USB_OTG_GINTSTS_PXFR_INCOMPISOOUT_Pos) /*!< 0x00200000 */
#define USB_OTG_GINTSTS_PXFR_INCOMPISOOUT        USB_OTG_GINTSTS_PXFR_INCOMPISOOUT_Msk /*!< Incomplete periodic transfer                   */
#define USB_OTG_GINTSTS_DATAFSUSP_Pos            (22U)                         
#define USB_OTG_GINTSTS_DATAFSUSP_Msk            (0x1U << USB_OTG_GINTSTS_DATAFSUSP_Pos) /*!< 0x00400000 */
#define USB_OTG_GINTSTS_DATAFSUSP                USB_OTG_GINTSTS_DATAFSUSP_Msk /*!< Data fetch suspended                           */
#define USB_OTG_GINTSTS_RSTDET_Pos               (23U)                         
#define USB_OTG_GINTSTS_RSTDET_Msk               (0x1U << USB_OTG_GINTSTS_RSTDET_Pos) /*!< 0x00800000 */
#define USB_OTG_GINTSTS_RSTDET                   USB_OTG_GINTSTS_RSTDET_Msk    /*!< Reset detected interrupt                       */
#define USB_OTG_GINTSTS_HPRTINT_Pos              (24U)                         
#define USB_OTG_GINTSTS_HPRTINT_Msk              (0x1U << USB_OTG_GINTSTS_HPRTINT_Pos) /*!< 0x01000000 */
#define USB_OTG_GINTSTS_HPRTINT                  USB_OTG_GINTSTS_HPRTINT_Msk   /*!< Host port interrupt                            */
#define USB_OTG_GINTSTS_HCINT_Pos                (25U)                         
#define USB_OTG_GINTSTS_HCINT_Msk                (0x1U << USB_OTG_GINTSTS_HCINT_Pos) /*!< 0x02000000 */
#define USB_OTG_GINTSTS_HCINT                    USB_OTG_GINTSTS_HCINT_Msk     /*!< Host channels interrupt                        */
#define USB_OTG_GINTSTS_PTXFE_Pos                (26U)                         
#define USB_OTG_GINTSTS_PTXFE_Msk                (0x1U << USB_OTG_GINTSTS_PTXFE_Pos) /*!< 0x04000000 */
#define USB_OTG_GINTSTS_PTXFE                    USB_OTG_GINTSTS_PTXFE_Msk     /*!< Periodic TxFIFO empty                          */
#define USB_OTG_GINTSTS_LPMINT_Pos               (27U)                         
#define USB_OTG_GINTSTS_LPMINT_Msk               (0x1U << USB_OTG_GINTSTS_LPMINT_Pos) /*!< 0x08000000 */
#define USB_OTG_GINTSTS_LPMINT                   USB_OTG_GINTSTS_LPMINT_Msk    /*!< LPM interrupt                                  */
#define USB_OTG_GINTSTS_CIDSCHG_Pos              (28U)                         
#define USB_OTG_GINTSTS_CIDSCHG_Msk              (0x1U << USB_OTG_GINTSTS_CIDSCHG_Pos) /*!< 0x10000000 */
#define USB_OTG_GINTSTS_CIDSCHG                  USB_OTG_GINTSTS_CIDSCHG_Msk   /*!< Connector ID status change                     */
#define USB_OTG_GINTSTS_DISCINT_Pos              (29U)                         
#define USB_OTG_GINTSTS_DISCINT_Msk              (0x1U << USB_OTG_GINTSTS_DISCINT_Pos) /*!< 0x20000000 */
#define USB_OTG_GINTSTS_DISCINT                  USB_OTG_GINTSTS_DISCINT_Msk   /*!< Disconnect detected interrupt                  */
#define USB_OTG_GINTSTS_SRQINT_Pos               (30U)                         
#define USB_OTG_GINTSTS_SRQINT_Msk               (0x1U << USB_OTG_GINTSTS_SRQINT_Pos) /*!< 0x40000000 */
#define USB_OTG_GINTSTS_SRQINT                   USB_OTG_GINTSTS_SRQINT_Msk    /*!< Session request/new session detected interrupt */
#define USB_OTG_GINTSTS_WKUINT_Pos               (31U)                         
#define USB_OTG_GINTSTS_WKUINT_Msk               (0x1U << USB_OTG_GINTSTS_WKUINT_Pos) /*!< 0x80000000 */
#define USB_OTG_GINTSTS_WKUINT                   USB_OTG_GINTSTS_WKUINT_Msk    /*!< Resume/remote wakeup detected interrupt        */
#define USB_OTG_GINTMSK_MMISM_Pos                (1U)                          
#define USB_OTG_GINTMSK_MMISM_Msk                (0x1U << USB_OTG_GINTMSK_MMISM_Pos) /*!< 0x00000002 */
#define USB_OTG_GINTMSK_MMISM                    USB_OTG_GINTMSK_MMISM_Msk     /*!< Mode mismatch interrupt mask                        */
#define USB_OTG_GINTMSK_OTGINT_Pos               (2U)                          
#define USB_OTG_GINTMSK_OTGINT_Msk               (0x1U << USB_OTG_GINTMSK_OTGINT_Pos) /*!< 0x00000004 */
#define USB_OTG_GINTMSK_OTGINT                   USB_OTG_GINTMSK_OTGINT_Msk    /*!< OTG interrupt mask                                  */
#define USB_OTG_GINTMSK_SOFM_Pos                 (3U)                          
#define USB_OTG_GINTMSK_SOFM_Msk                 (0x1U << USB_OTG_GINTMSK_SOFM_Pos) /*!< 0x00000008 */
#define USB_OTG_GINTMSK_SOFM                     USB_OTG_GINTMSK_SOFM_Msk      /*!< Start of frame mask                                 */
#define USB_OTG_GINTMSK_RXFLVLM_Pos              (4U)                          
#define USB_OTG_GINTMSK_RXFLVLM_Msk              (0x1U << USB_OTG_GINTMSK_RXFLVLM_Pos) /*!< 0x00000010 */
#define USB_OTG_GINTMSK_RXFLVLM                  USB_OTG_GINTMSK_RXFLVLM_Msk   /*!< Receive FIFO nonempty mask                          */
#define USB_OTG_GINTMSK_NPTXFEM_Pos              (5U)                          
#define USB_OTG_GINTMSK_NPTXFEM_Msk              (0x1U << USB_OTG_GINTMSK_NPTXFEM_Pos) /*!< 0x00000020 */
#define USB_OTG_GINTMSK_NPTXFEM                  USB_OTG_GINTMSK_NPTXFEM_Msk   /*!< Nonperiodic TxFIFO empty mask                       */
#define USB_OTG_GINTMSK_GINAKEFFM_Pos            (6U)                          
#define USB_OTG_GINTMSK_GINAKEFFM_Msk            (0x1U << USB_OTG_GINTMSK_GINAKEFFM_Pos) /*!< 0x00000040 */
#define USB_OTG_GINTMSK_GINAKEFFM                USB_OTG_GINTMSK_GINAKEFFM_Msk /*!< Global nonperiodic IN NAK effective mask            */
#define USB_OTG_GINTMSK_GONAKEFFM_Pos            (7U)                          
#define USB_OTG_GINTMSK_GONAKEFFM_Msk            (0x1U << USB_OTG_GINTMSK_GONAKEFFM_Pos) /*!< 0x00000080 */
#define USB_OTG_GINTMSK_GONAKEFFM                USB_OTG_GINTMSK_GONAKEFFM_Msk /*!< Global OUT NAK effective mask                       */
#define USB_OTG_GINTMSK_ESUSPM_Pos               (10U)                         
#define USB_OTG_GINTMSK_ESUSPM_Msk               (0x1U << USB_OTG_GINTMSK_ESUSPM_Pos) /*!< 0x00000400 */
#define USB_OTG_GINTMSK_ESUSPM                   USB_OTG_GINTMSK_ESUSPM_Msk    /*!< Early suspend mask                                  */
#define USB_OTG_GINTMSK_USBSUSPM_Pos             (11U)                         
#define USB_OTG_GINTMSK_USBSUSPM_Msk             (0x1U << USB_OTG_GINTMSK_USBSUSPM_Pos) /*!< 0x00000800 */
#define USB_OTG_GINTMSK_USBSUSPM                 USB_OTG_GINTMSK_USBSUSPM_Msk  /*!< USB suspend mask                                    */
#define USB_OTG_GINTMSK_USBRST_Pos               (12U)                         
#define USB_OTG_GINTMSK_USBRST_Msk               (0x1U << USB_OTG_GINTMSK_USBRST_Pos) /*!< 0x00001000 */
#define USB_OTG_GINTMSK_USBRST                   USB_OTG_GINTMSK_USBRST_Msk    /*!< USB reset mask                                      */
#define USB_OTG_GINTMSK_ENUMDNEM_Pos             (13U)                         
#define USB_OTG_GINTMSK_ENUMDNEM_Msk             (0x1U << USB_OTG_GINTMSK_ENUMDNEM_Pos) /*!< 0x00002000 */
#define USB_OTG_GINTMSK_ENUMDNEM                 USB_OTG_GINTMSK_ENUMDNEM_Msk  /*!< Enumeration done mask                               */
#define USB_OTG_GINTMSK_ISOODRPM_Pos             (14U)                         
#define USB_OTG_GINTMSK_ISOODRPM_Msk             (0x1U << USB_OTG_GINTMSK_ISOODRPM_Pos) /*!< 0x00004000 */
#define USB_OTG_GINTMSK_ISOODRPM                 USB_OTG_GINTMSK_ISOODRPM_Msk  /*!< Isochronous OUT packet dropped interrupt mask       */
#define USB_OTG_GINTMSK_EOPFM_Pos                (15U)                         
#define USB_OTG_GINTMSK_EOPFM_Msk                (0x1U << USB_OTG_GINTMSK_EOPFM_Pos) /*!< 0x00008000 */
#define USB_OTG_GINTMSK_EOPFM                    USB_OTG_GINTMSK_EOPFM_Msk     /*!< End of periodic frame interrupt mask                */
#define USB_OTG_GINTMSK_EPMISM_Pos               (17U)                         
#define USB_OTG_GINTMSK_EPMISM_Msk               (0x1U << USB_OTG_GINTMSK_EPMISM_Pos) /*!< 0x00020000 */
#define USB_OTG_GINTMSK_EPMISM                   USB_OTG_GINTMSK_EPMISM_Msk    /*!< Endpoint mismatch interrupt mask                    */
#define USB_OTG_GINTMSK_IEPINT_Pos               (18U)                         
#define USB_OTG_GINTMSK_IEPINT_Msk               (0x1U << USB_OTG_GINTMSK_IEPINT_Pos) /*!< 0x00040000 */
#define USB_OTG_GINTMSK_IEPINT                   USB_OTG_GINTMSK_IEPINT_Msk    /*!< IN endpoints interrupt mask                         */
#define USB_OTG_GINTMSK_OEPINT_Pos               (19U)                         
#define USB_OTG_GINTMSK_OEPINT_Msk               (0x1U << USB_OTG_GINTMSK_OEPINT_Pos) /*!< 0x00080000 */
#define USB_OTG_GINTMSK_OEPINT                   USB_OTG_GINTMSK_OEPINT_Msk    /*!< OUT endpoints interrupt mask                        */
#define USB_OTG_GINTMSK_IISOIXFRM_Pos            (20U)                         
#define USB_OTG_GINTMSK_IISOIXFRM_Msk            (0x1U << USB_OTG_GINTMSK_IISOIXFRM_Pos) /*!< 0x00100000 */
#define USB_OTG_GINTMSK_IISOIXFRM                USB_OTG_GINTMSK_IISOIXFRM_Msk /*!< Incomplete isochronous IN transfer mask             */
#define USB_OTG_GINTMSK_PXFRM_IISOOXFRM_Pos      (21U)                         
#define USB_OTG_GINTMSK_PXFRM_IISOOXFRM_Msk      (0x1U << USB_OTG_GINTMSK_PXFRM_IISOOXFRM_Pos) /*!< 0x00200000 */
#define USB_OTG_GINTMSK_PXFRM_IISOOXFRM          USB_OTG_GINTMSK_PXFRM_IISOOXFRM_Msk /*!< Incomplete periodic transfer mask                   */
#define USB_OTG_GINTMSK_FSUSPM_Pos               (22U)                         
#define USB_OTG_GINTMSK_FSUSPM_Msk               (0x1U << USB_OTG_GINTMSK_FSUSPM_Pos) /*!< 0x00400000 */
#define USB_OTG_GINTMSK_FSUSPM                   USB_OTG_GINTMSK_FSUSPM_Msk    /*!< Data fetch suspended mask                           */
#define USB_OTG_GINTMSK_RSTDEM_Pos               (23U)                         
#define USB_OTG_GINTMSK_RSTDEM_Msk               (0x1U << USB_OTG_GINTMSK_RSTDEM_Pos) /*!< 0x00800000 */
#define USB_OTG_GINTMSK_RSTDEM                   USB_OTG_GINTMSK_RSTDEM_Msk    /*!< Reset detected interrupt mask                      */
#define USB_OTG_GINTMSK_PRTIM_Pos                (24U)                         
#define USB_OTG_GINTMSK_PRTIM_Msk                (0x1U << USB_OTG_GINTMSK_PRTIM_Pos) /*!< 0x01000000 */
#define USB_OTG_GINTMSK_PRTIM                    USB_OTG_GINTMSK_PRTIM_Msk     /*!< Host port interrupt mask                            */
#define USB_OTG_GINTMSK_HCIM_Pos                 (25U)                         
#define USB_OTG_GINTMSK_HCIM_Msk                 (0x1U << USB_OTG_GINTMSK_HCIM_Pos) /*!< 0x02000000 */
#define USB_OTG_GINTMSK_HCIM                     USB_OTG_GINTMSK_HCIM_Msk      /*!< Host channels interrupt mask                        */
#define USB_OTG_GINTMSK_PTXFEM_Pos               (26U)                         
#define USB_OTG_GINTMSK_PTXFEM_Msk               (0x1U << USB_OTG_GINTMSK_PTXFEM_Pos) /*!< 0x04000000 */
#define USB_OTG_GINTMSK_PTXFEM                   USB_OTG_GINTMSK_PTXFEM_Msk    /*!< Periodic TxFIFO empty mask                          */
#define USB_OTG_GINTMSK_LPMINTM_Pos              (27U)                         
#define USB_OTG_GINTMSK_LPMINTM_Msk              (0x1U << USB_OTG_GINTMSK_LPMINTM_Pos) /*!< 0x08000000 */
#define USB_OTG_GINTMSK_LPMINTM                  USB_OTG_GINTMSK_LPMINTM_Msk   /*!< LPM interrupt Mask                                  */
#define USB_OTG_GINTMSK_CIDSCHGM_Pos             (28U)                         
#define USB_OTG_GINTMSK_CIDSCHGM_Msk             (0x1U << USB_OTG_GINTMSK_CIDSCHGM_Pos) /*!< 0x10000000 */
#define USB_OTG_GINTMSK_CIDSCHGM                 USB_OTG_GINTMSK_CIDSCHGM_Msk  /*!< Connector ID status change mask                     */
#define USB_OTG_GINTMSK_DISCINT_Pos              (29U)                         
#define USB_OTG_GINTMSK_DISCINT_Msk              (0x1U << USB_OTG_GINTMSK_DISCINT_Pos) /*!< 0x20000000 */
#define USB_OTG_GINTMSK_DISCINT                  USB_OTG_GINTMSK_DISCINT_Msk   /*!< Disconnect detected interrupt mask                  */
#define USB_OTG_GINTMSK_SRQIM_Pos                (30U)                         
#define USB_OTG_GINTMSK_SRQIM_Msk                (0x1U << USB_OTG_GINTMSK_SRQIM_Pos) /*!< 0x40000000 */
#define USB_OTG_GINTMSK_SRQIM                    USB_OTG_GINTMSK_SRQIM_Msk     /*!< Session request/new session detected interrupt mask */
#define USB_OTG_GINTMSK_WUIM_Pos                 (31U)                         
#define USB_OTG_GINTMSK_WUIM_Msk                 (0x1U << USB_OTG_GINTMSK_WUIM_Pos) /*!< 0x80000000 */
#define USB_OTG_GINTMSK_WUIM                     USB_OTG_GINTMSK_WUIM_Msk      /*!< Resume/remote wakeup detected interrupt mask        */
#define USB_OTG_DAINT_IEPINT_Pos                 (0U)                          
#define USB_OTG_DAINT_IEPINT_Msk                 (0xFFFFU << USB_OTG_DAINT_IEPINT_Pos) /*!< 0x0000FFFF */
#define USB_OTG_DAINT_IEPINT                     USB_OTG_DAINT_IEPINT_Msk      /*!< IN endpoint interrupt bits  */
#define USB_OTG_DAINT_OEPINT_Pos                 (16U)                         
#define USB_OTG_DAINT_OEPINT_Msk                 (0xFFFFU << USB_OTG_DAINT_OEPINT_Pos) /*!< 0xFFFF0000 */
#define USB_OTG_DAINT_OEPINT                     USB_OTG_DAINT_OEPINT_Msk      /*!< OUT endpoint interrupt bits */
#define USB_OTG_HAINTMSK_HAINTM_Pos              (0U)                          
#define USB_OTG_HAINTMSK_HAINTM_Msk              (0xFFFFU << USB_OTG_HAINTMSK_HAINTM_Pos) /*!< 0x0000FFFF */
#define USB_OTG_HAINTMSK_HAINTM                  USB_OTG_HAINTMSK_HAINTM_Msk   /*!< Channel interrupt mask */
#define USB_OTG_GRXSTSP_EPNUM_Pos                (0U)                          
#define USB_OTG_GRXSTSP_EPNUM_Msk                (0xFU << USB_OTG_GRXSTSP_EPNUM_Pos) /*!< 0x0000000F */
#define USB_OTG_GRXSTSP_EPNUM                    USB_OTG_GRXSTSP_EPNUM_Msk     /*!< IN EP interrupt mask bits  */
#define USB_OTG_GRXSTSP_BCNT_Pos                 (4U)                          
#define USB_OTG_GRXSTSP_BCNT_Msk                 (0x7FFU << USB_OTG_GRXSTSP_BCNT_Pos) /*!< 0x00007FF0 */
#define USB_OTG_GRXSTSP_BCNT                     USB_OTG_GRXSTSP_BCNT_Msk      /*!< OUT EP interrupt mask bits */
#define USB_OTG_GRXSTSP_DPID_Pos                 (15U)                         
#define USB_OTG_GRXSTSP_DPID_Msk                 (0x3U << USB_OTG_GRXSTSP_DPID_Pos) /*!< 0x00018000 */
#define USB_OTG_GRXSTSP_DPID                     USB_OTG_GRXSTSP_DPID_Msk      /*!< OUT EP interrupt mask bits */
#define USB_OTG_GRXSTSP_PKTSTS_Pos               (17U)                         
#define USB_OTG_GRXSTSP_PKTSTS_Msk               (0xFU << USB_OTG_GRXSTSP_PKTSTS_Pos) /*!< 0x001E0000 */
#define USB_OTG_GRXSTSP_PKTSTS                   USB_OTG_GRXSTSP_PKTSTS_Msk    /*!< OUT EP interrupt mask bits */
#define USB_OTG_DAINTMSK_IEPM_Pos                (0U)                          
#define USB_OTG_DAINTMSK_IEPM_Msk                (0xFFFFU << USB_OTG_DAINTMSK_IEPM_Pos) /*!< 0x0000FFFF */
#define USB_OTG_DAINTMSK_IEPM                    USB_OTG_DAINTMSK_IEPM_Msk     /*!< IN EP interrupt mask bits */
#define USB_OTG_DAINTMSK_OEPM_Pos                (16U)                         
#define USB_OTG_DAINTMSK_OEPM_Msk                (0xFFFFU << USB_OTG_DAINTMSK_OEPM_Pos) /*!< 0xFFFF0000 */
#define USB_OTG_DAINTMSK_OEPM                    USB_OTG_DAINTMSK_OEPM_Msk     /*!< OUT EP interrupt mask bits */
#define USB_OTG_CHNUM_Pos                        (0U)                          
#define USB_OTG_CHNUM_Msk                        (0xFU << USB_OTG_CHNUM_Pos)   /*!< 0x0000000F */
#define USB_OTG_CHNUM                            USB_OTG_CHNUM_Msk             /*!< Channel number */
#define USB_OTG_CHNUM_0                          (0x1U << USB_OTG_CHNUM_Pos)   /*!< 0x00000001 */
#define USB_OTG_CHNUM_1                          (0x2U << USB_OTG_CHNUM_Pos)   /*!< 0x00000002 */
#define USB_OTG_CHNUM_2                          (0x4U << USB_OTG_CHNUM_Pos)   /*!< 0x00000004 */
#define USB_OTG_CHNUM_3                          (0x8U << USB_OTG_CHNUM_Pos)   /*!< 0x00000008 */
#define USB_OTG_BCNT_Pos                         (4U)                          
#define USB_OTG_BCNT_Msk                         (0x7FFU << USB_OTG_BCNT_Pos)  /*!< 0x00007FF0 */
#define USB_OTG_BCNT                             USB_OTG_BCNT_Msk              /*!< Byte count */
#define USB_OTG_DPID_Pos                         (15U)                         
#define USB_OTG_DPID_Msk                         (0x3U << USB_OTG_DPID_Pos)    /*!< 0x00018000 */
#define USB_OTG_DPID                             USB_OTG_DPID_Msk              /*!< Data PID */
#define USB_OTG_DPID_0                           (0x1U << USB_OTG_DPID_Pos)    /*!< 0x00008000 */
#define USB_OTG_DPID_1                           (0x2U << USB_OTG_DPID_Pos)    /*!< 0x00010000 */
#define USB_OTG_PKTSTS_Pos                       (17U)                         
#define USB_OTG_PKTSTS_Msk                       (0xFU << USB_OTG_PKTSTS_Pos)  /*!< 0x001E0000 */
#define USB_OTG_PKTSTS                           USB_OTG_PKTSTS_Msk            /*!< Packet status */
#define USB_OTG_PKTSTS_0                         (0x1U << USB_OTG_PKTSTS_Pos)  /*!< 0x00020000 */
#define USB_OTG_PKTSTS_1                         (0x2U << USB_OTG_PKTSTS_Pos)  /*!< 0x00040000 */
#define USB_OTG_PKTSTS_2                         (0x4U << USB_OTG_PKTSTS_Pos)  /*!< 0x00080000 */
#define USB_OTG_PKTSTS_3                         (0x8U << USB_OTG_PKTSTS_Pos)  /*!< 0x00100000 */
#define USB_OTG_EPNUM_Pos                        (0U)                          
#define USB_OTG_EPNUM_Msk                        (0xFU << USB_OTG_EPNUM_Pos)   /*!< 0x0000000F */
#define USB_OTG_EPNUM                            USB_OTG_EPNUM_Msk             /*!< Endpoint number */
#define USB_OTG_EPNUM_0                          (0x1U << USB_OTG_EPNUM_Pos)   /*!< 0x00000001 */
#define USB_OTG_EPNUM_1                          (0x2U << USB_OTG_EPNUM_Pos)   /*!< 0x00000002 */
#define USB_OTG_EPNUM_2                          (0x4U << USB_OTG_EPNUM_Pos)   /*!< 0x00000004 */
#define USB_OTG_EPNUM_3                          (0x8U << USB_OTG_EPNUM_Pos)   /*!< 0x00000008 */
#define USB_OTG_FRMNUM_Pos                       (21U)                         
#define USB_OTG_FRMNUM_Msk                       (0xFU << USB_OTG_FRMNUM_Pos)  /*!< 0x01E00000 */
#define USB_OTG_FRMNUM                           USB_OTG_FRMNUM_Msk            /*!< Frame number */
#define USB_OTG_FRMNUM_0                         (0x1U << USB_OTG_FRMNUM_Pos)  /*!< 0x00200000 */
#define USB_OTG_FRMNUM_1                         (0x2U << USB_OTG_FRMNUM_Pos)  /*!< 0x00400000 */
#define USB_OTG_FRMNUM_2                         (0x4U << USB_OTG_FRMNUM_Pos)  /*!< 0x00800000 */
#define USB_OTG_FRMNUM_3                         (0x8U << USB_OTG_FRMNUM_Pos)  /*!< 0x01000000 */
#define USB_OTG_GRXFSIZ_RXFD_Pos                 (0U)                          
#define USB_OTG_GRXFSIZ_RXFD_Msk                 (0xFFFFU << USB_OTG_GRXFSIZ_RXFD_Pos) /*!< 0x0000FFFF */
#define USB_OTG_GRXFSIZ_RXFD                     USB_OTG_GRXFSIZ_RXFD_Msk      /*!< RxFIFO depth */
#define USB_OTG_DVBUSDIS_VBUSDT_Pos              (0U)                          
#define USB_OTG_DVBUSDIS_VBUSDT_Msk              (0xFFFFU << USB_OTG_DVBUSDIS_VBUSDT_Pos) /*!< 0x0000FFFF */
#define USB_OTG_DVBUSDIS_VBUSDT                  USB_OTG_DVBUSDIS_VBUSDT_Msk   /*!< Device VBUS discharge time */
#define USB_OTG_NPTXFSA_Pos                      (0U)                          
#define USB_OTG_NPTXFSA_Msk                      (0xFFFFU << USB_OTG_NPTXFSA_Pos) /*!< 0x0000FFFF */
#define USB_OTG_NPTXFSA                          USB_OTG_NPTXFSA_Msk           /*!< Nonperiodic transmit RAM start address */
#define USB_OTG_NPTXFD_Pos                       (16U)                         
#define USB_OTG_NPTXFD_Msk                       (0xFFFFU << USB_OTG_NPTXFD_Pos) /*!< 0xFFFF0000 */
#define USB_OTG_NPTXFD                           USB_OTG_NPTXFD_Msk            /*!< Nonperiodic TxFIFO depth               */
#define USB_OTG_TX0FSA_Pos                       (0U)                          
#define USB_OTG_TX0FSA_Msk                       (0xFFFFU << USB_OTG_TX0FSA_Pos) /*!< 0x0000FFFF */
#define USB_OTG_TX0FSA                           USB_OTG_TX0FSA_Msk            /*!< Endpoint 0 transmit RAM start address  */
#define USB_OTG_TX0FD_Pos                        (16U)                         
#define USB_OTG_TX0FD_Msk                        (0xFFFFU << USB_OTG_TX0FD_Pos) /*!< 0xFFFF0000 */
#define USB_OTG_TX0FD                            USB_OTG_TX0FD_Msk             /*!< Endpoint 0 TxFIFO depth                */
#define USB_OTG_DVBUSPULSE_DVBUSP_Pos            (0U)                          
#define USB_OTG_DVBUSPULSE_DVBUSP_Msk            (0xFFFU << USB_OTG_DVBUSPULSE_DVBUSP_Pos) /*!< 0x00000FFF */
#define USB_OTG_DVBUSPULSE_DVBUSP                USB_OTG_DVBUSPULSE_DVBUSP_Msk /*!< Device VBUS pulsing time */
#define USB_OTG_GNPTXSTS_NPTXFSAV_Pos            (0U)                          
#define USB_OTG_GNPTXSTS_NPTXFSAV_Msk            (0xFFFFU << USB_OTG_GNPTXSTS_NPTXFSAV_Pos) /*!< 0x0000FFFF */
#define USB_OTG_GNPTXSTS_NPTXFSAV                USB_OTG_GNPTXSTS_NPTXFSAV_Msk /*!< Nonperiodic TxFIFO space available */
#define USB_OTG_GNPTXSTS_NPTQXSAV_Pos            (16U)                         
#define USB_OTG_GNPTXSTS_NPTQXSAV_Msk            (0xFFU << USB_OTG_GNPTXSTS_NPTQXSAV_Pos) /*!< 0x00FF0000 */
#define USB_OTG_GNPTXSTS_NPTQXSAV                USB_OTG_GNPTXSTS_NPTQXSAV_Msk /*!< Nonperiodic transmit request queue space available */
#define USB_OTG_GNPTXSTS_NPTQXSAV_0              (0x01U << USB_OTG_GNPTXSTS_NPTQXSAV_Pos) /*!< 0x00010000 */
#define USB_OTG_GNPTXSTS_NPTQXSAV_1              (0x02U << USB_OTG_GNPTXSTS_NPTQXSAV_Pos) /*!< 0x00020000 */
#define USB_OTG_GNPTXSTS_NPTQXSAV_2              (0x04U << USB_OTG_GNPTXSTS_NPTQXSAV_Pos) /*!< 0x00040000 */
#define USB_OTG_GNPTXSTS_NPTQXSAV_3              (0x08U << USB_OTG_GNPTXSTS_NPTQXSAV_Pos) /*!< 0x00080000 */
#define USB_OTG_GNPTXSTS_NPTQXSAV_4              (0x10U << USB_OTG_GNPTXSTS_NPTQXSAV_Pos) /*!< 0x00100000 */
#define USB_OTG_GNPTXSTS_NPTQXSAV_5              (0x20U << USB_OTG_GNPTXSTS_NPTQXSAV_Pos) /*!< 0x00200000 */
#define USB_OTG_GNPTXSTS_NPTQXSAV_6              (0x40U << USB_OTG_GNPTXSTS_NPTQXSAV_Pos) /*!< 0x00400000 */
#define USB_OTG_GNPTXSTS_NPTQXSAV_7              (0x80U << USB_OTG_GNPTXSTS_NPTQXSAV_Pos) /*!< 0x00800000 */
#define USB_OTG_GNPTXSTS_NPTXQTOP_Pos            (24U)                         
#define USB_OTG_GNPTXSTS_NPTXQTOP_Msk            (0x7FU << USB_OTG_GNPTXSTS_NPTXQTOP_Pos) /*!< 0x7F000000 */
#define USB_OTG_GNPTXSTS_NPTXQTOP                USB_OTG_GNPTXSTS_NPTXQTOP_Msk /*!< Top of the nonperiodic transmit request queue */
#define USB_OTG_GNPTXSTS_NPTXQTOP_0              (0x01U << USB_OTG_GNPTXSTS_NPTXQTOP_Pos) /*!< 0x01000000 */
#define USB_OTG_GNPTXSTS_NPTXQTOP_1              (0x02U << USB_OTG_GNPTXSTS_NPTXQTOP_Pos) /*!< 0x02000000 */
#define USB_OTG_GNPTXSTS_NPTXQTOP_2              (0x04U << USB_OTG_GNPTXSTS_NPTXQTOP_Pos) /*!< 0x04000000 */
#define USB_OTG_GNPTXSTS_NPTXQTOP_3              (0x08U << USB_OTG_GNPTXSTS_NPTXQTOP_Pos) /*!< 0x08000000 */
#define USB_OTG_GNPTXSTS_NPTXQTOP_4              (0x10U << USB_OTG_GNPTXSTS_NPTXQTOP_Pos) /*!< 0x10000000 */
#define USB_OTG_GNPTXSTS_NPTXQTOP_5              (0x20U << USB_OTG_GNPTXSTS_NPTXQTOP_Pos) /*!< 0x20000000 */
#define USB_OTG_GNPTXSTS_NPTXQTOP_6              (0x40U << USB_OTG_GNPTXSTS_NPTXQTOP_Pos) /*!< 0x40000000 */
#define USB_OTG_DTHRCTL_NONISOTHREN_Pos          (0U)                          
#define USB_OTG_DTHRCTL_NONISOTHREN_Msk          (0x1U << USB_OTG_DTHRCTL_NONISOTHREN_Pos) /*!< 0x00000001 */
#define USB_OTG_DTHRCTL_NONISOTHREN              USB_OTG_DTHRCTL_NONISOTHREN_Msk /*!< Nonisochronous IN endpoints threshold enable */
#define USB_OTG_DTHRCTL_ISOTHREN_Pos             (1U)                          
#define USB_OTG_DTHRCTL_ISOTHREN_Msk             (0x1U << USB_OTG_DTHRCTL_ISOTHREN_Pos) /*!< 0x00000002 */
#define USB_OTG_DTHRCTL_ISOTHREN                 USB_OTG_DTHRCTL_ISOTHREN_Msk  /*!< ISO IN endpoint threshold enable */
#define USB_OTG_DTHRCTL_TXTHRLEN_Pos             (2U)                          
#define USB_OTG_DTHRCTL_TXTHRLEN_Msk             (0x1FFU << USB_OTG_DTHRCTL_TXTHRLEN_Pos) /*!< 0x000007FC */
#define USB_OTG_DTHRCTL_TXTHRLEN                 USB_OTG_DTHRCTL_TXTHRLEN_Msk  /*!< Transmit threshold length */
#define USB_OTG_DTHRCTL_TXTHRLEN_0               (0x001U << USB_OTG_DTHRCTL_TXTHRLEN_Pos) /*!< 0x00000004 */
#define USB_OTG_DTHRCTL_TXTHRLEN_1               (0x002U << USB_OTG_DTHRCTL_TXTHRLEN_Pos) /*!< 0x00000008 */
#define USB_OTG_DTHRCTL_TXTHRLEN_2               (0x004U << USB_OTG_DTHRCTL_TXTHRLEN_Pos) /*!< 0x00000010 */
#define USB_OTG_DTHRCTL_TXTHRLEN_3               (0x008U << USB_OTG_DTHRCTL_TXTHRLEN_Pos) /*!< 0x00000020 */
#define USB_OTG_DTHRCTL_TXTHRLEN_4               (0x010U << USB_OTG_DTHRCTL_TXTHRLEN_Pos) /*!< 0x00000040 */
#define USB_OTG_DTHRCTL_TXTHRLEN_5               (0x020U << USB_OTG_DTHRCTL_TXTHRLEN_Pos) /*!< 0x00000080 */
#define USB_OTG_DTHRCTL_TXTHRLEN_6               (0x040U << USB_OTG_DTHRCTL_TXTHRLEN_Pos) /*!< 0x00000100 */
#define USB_OTG_DTHRCTL_TXTHRLEN_7               (0x080U << USB_OTG_DTHRCTL_TXTHRLEN_Pos) /*!< 0x00000200 */
#define USB_OTG_DTHRCTL_TXTHRLEN_8               (0x100U << USB_OTG_DTHRCTL_TXTHRLEN_Pos) /*!< 0x00000400 */
#define USB_OTG_DTHRCTL_RXTHREN_Pos              (16U)                         
#define USB_OTG_DTHRCTL_RXTHREN_Msk              (0x1U << USB_OTG_DTHRCTL_RXTHREN_Pos) /*!< 0x00010000 */
#define USB_OTG_DTHRCTL_RXTHREN                  USB_OTG_DTHRCTL_RXTHREN_Msk   /*!< Receive threshold enable */
#define USB_OTG_DTHRCTL_RXTHRLEN_Pos             (17U)                         
#define USB_OTG_DTHRCTL_RXTHRLEN_Msk             (0x1FFU << USB_OTG_DTHRCTL_RXTHRLEN_Pos) /*!< 0x03FE0000 */
#define USB_OTG_DTHRCTL_RXTHRLEN                 USB_OTG_DTHRCTL_RXTHRLEN_Msk  /*!< Receive threshold length */
#define USB_OTG_DTHRCTL_RXTHRLEN_0               (0x001U << USB_OTG_DTHRCTL_RXTHRLEN_Pos) /*!< 0x00020000 */
#define USB_OTG_DTHRCTL_RXTHRLEN_1               (0x002U << USB_OTG_DTHRCTL_RXTHRLEN_Pos) /*!< 0x00040000 */
#define USB_OTG_DTHRCTL_RXTHRLEN_2               (0x004U << USB_OTG_DTHRCTL_RXTHRLEN_Pos) /*!< 0x00080000 */
#define USB_OTG_DTHRCTL_RXTHRLEN_3               (0x008U << USB_OTG_DTHRCTL_RXTHRLEN_Pos) /*!< 0x00100000 */
#define USB_OTG_DTHRCTL_RXTHRLEN_4               (0x010U << USB_OTG_DTHRCTL_RXTHRLEN_Pos) /*!< 0x00200000 */
#define USB_OTG_DTHRCTL_RXTHRLEN_5               (0x020U << USB_OTG_DTHRCTL_RXTHRLEN_Pos) /*!< 0x00400000 */
#define USB_OTG_DTHRCTL_RXTHRLEN_6               (0x040U << USB_OTG_DTHRCTL_RXTHRLEN_Pos) /*!< 0x00800000 */
#define USB_OTG_DTHRCTL_RXTHRLEN_7               (0x080U << USB_OTG_DTHRCTL_RXTHRLEN_Pos) /*!< 0x01000000 */
#define USB_OTG_DTHRCTL_RXTHRLEN_8               (0x100U << USB_OTG_DTHRCTL_RXTHRLEN_Pos) /*!< 0x02000000 */
#define USB_OTG_DTHRCTL_ARPEN_Pos                (27U)                         
#define USB_OTG_DTHRCTL_ARPEN_Msk                (0x1U << USB_OTG_DTHRCTL_ARPEN_Pos) /*!< 0x08000000 */
#define USB_OTG_DTHRCTL_ARPEN                    USB_OTG_DTHRCTL_ARPEN_Msk     /*!< Arbiter parking enable */
#define USB_OTG_DIEPEMPMSK_INEPTXFEM_Pos         (0U)                          
#define USB_OTG_DIEPEMPMSK_INEPTXFEM_Msk         (0xFFFFU << USB_OTG_DIEPEMPMSK_INEPTXFEM_Pos) /*!< 0x0000FFFF */
#define USB_OTG_DIEPEMPMSK_INEPTXFEM             USB_OTG_DIEPEMPMSK_INEPTXFEM_Msk /*!< IN EP Tx FIFO empty interrupt mask bits */
#define USB_OTG_DEACHINT_IEP1INT_Pos             (1U)                          
#define USB_OTG_DEACHINT_IEP1INT_Msk             (0x1U << USB_OTG_DEACHINT_IEP1INT_Pos) /*!< 0x00000002 */
#define USB_OTG_DEACHINT_IEP1INT                 USB_OTG_DEACHINT_IEP1INT_Msk  /*!< IN endpoint 1interrupt bit   */
#define USB_OTG_DEACHINT_OEP1INT_Pos             (17U)                         
#define USB_OTG_DEACHINT_OEP1INT_Msk             (0x1U << USB_OTG_DEACHINT_OEP1INT_Pos) /*!< 0x00020000 */
#define USB_OTG_DEACHINT_OEP1INT                 USB_OTG_DEACHINT_OEP1INT_Msk  /*!< OUT endpoint 1 interrupt bit */
#define USB_OTG_GCCFG_DCDET_Pos                  (0U)                          
#define USB_OTG_GCCFG_DCDET_Msk                  (0x1U << USB_OTG_GCCFG_DCDET_Pos) /*!< 0x00000001 */
#define USB_OTG_GCCFG_DCDET                      USB_OTG_GCCFG_DCDET_Msk       /*!< Data contact detection (DCD) status */
#define USB_OTG_GCCFG_PDET_Pos                   (1U)                          
#define USB_OTG_GCCFG_PDET_Msk                   (0x1U << USB_OTG_GCCFG_PDET_Pos) /*!< 0x00000002 */
#define USB_OTG_GCCFG_PDET                       USB_OTG_GCCFG_PDET_Msk        /*!< Primary detection (PD) status */
#define USB_OTG_GCCFG_SDET_Pos                   (2U)                          
#define USB_OTG_GCCFG_SDET_Msk                   (0x1U << USB_OTG_GCCFG_SDET_Pos) /*!< 0x00000004 */
#define USB_OTG_GCCFG_SDET                       USB_OTG_GCCFG_SDET_Msk        /*!< Secondary detection (SD) status */
#define USB_OTG_GCCFG_PS2DET_Pos                 (3U)                          
#define USB_OTG_GCCFG_PS2DET_Msk                 (0x1U << USB_OTG_GCCFG_PS2DET_Pos) /*!< 0x00000008 */
#define USB_OTG_GCCFG_PS2DET                     USB_OTG_GCCFG_PS2DET_Msk      /*!< DM pull-up detection status */
#define USB_OTG_GCCFG_PWRDWN_Pos                 (16U)                         
#define USB_OTG_GCCFG_PWRDWN_Msk                 (0x1U << USB_OTG_GCCFG_PWRDWN_Pos) /*!< 0x00010000 */
#define USB_OTG_GCCFG_PWRDWN                     USB_OTG_GCCFG_PWRDWN_Msk      /*!< Power down */
#define USB_OTG_GCCFG_BCDEN_Pos                  (17U)                         
#define USB_OTG_GCCFG_BCDEN_Msk                  (0x1U << USB_OTG_GCCFG_BCDEN_Pos) /*!< 0x00020000 */
#define USB_OTG_GCCFG_BCDEN                      USB_OTG_GCCFG_BCDEN_Msk       /*!< Battery charging detector (BCD) enable */
#define USB_OTG_GCCFG_DCDEN_Pos                  (18U)                         
#define USB_OTG_GCCFG_DCDEN_Msk                  (0x1U << USB_OTG_GCCFG_DCDEN_Pos) /*!< 0x00040000 */
#define USB_OTG_GCCFG_DCDEN                      USB_OTG_GCCFG_DCDEN_Msk       /*!< Data contact detection (DCD) mode enable*/
#define USB_OTG_GCCFG_PDEN_Pos                   (19U)                         
#define USB_OTG_GCCFG_PDEN_Msk                   (0x1U << USB_OTG_GCCFG_PDEN_Pos) /*!< 0x00080000 */
#define USB_OTG_GCCFG_PDEN                       USB_OTG_GCCFG_PDEN_Msk        /*!< Primary detection (PD) mode enable*/
#define USB_OTG_GCCFG_SDEN_Pos                   (20U)                         
#define USB_OTG_GCCFG_SDEN_Msk                   (0x1U << USB_OTG_GCCFG_SDEN_Pos) /*!< 0x00100000 */
#define USB_OTG_GCCFG_SDEN                       USB_OTG_GCCFG_SDEN_Msk        /*!< Secondary detection (SD) mode enable */
#define USB_OTG_GCCFG_VBDEN_Pos                  (21U)                         
#define USB_OTG_GCCFG_VBDEN_Msk                  (0x1U << USB_OTG_GCCFG_VBDEN_Pos) /*!< 0x00200000 */
#define USB_OTG_GCCFG_VBDEN                      USB_OTG_GCCFG_VBDEN_Msk       /*!< Secondary detection (SD) mode enable */
#define USB_OTG_GPWRDN_ADPMEN_Pos                (0U)                          
#define USB_OTG_GPWRDN_ADPMEN_Msk                (0x1U << USB_OTG_GPWRDN_ADPMEN_Pos) /*!< 0x00000001 */
#define USB_OTG_GPWRDN_ADPMEN                    USB_OTG_GPWRDN_ADPMEN_Msk     /*!< ADP module enable */
#define USB_OTG_GPWRDN_ADPIF_Pos                 (23U)                         
#define USB_OTG_GPWRDN_ADPIF_Msk                 (0x1U << USB_OTG_GPWRDN_ADPIF_Pos) /*!< 0x00800000 */
#define USB_OTG_GPWRDN_ADPIF                     USB_OTG_GPWRDN_ADPIF_Msk      /*!< ADP Interrupt flag */
#define USB_OTG_DEACHINTMSK_IEP1INTM_Pos         (1U)                          
#define USB_OTG_DEACHINTMSK_IEP1INTM_Msk         (0x1U << USB_OTG_DEACHINTMSK_IEP1INTM_Pos) /*!< 0x00000002 */
#define USB_OTG_DEACHINTMSK_IEP1INTM             USB_OTG_DEACHINTMSK_IEP1INTM_Msk /*!< IN Endpoint 1 interrupt mask bit  */
#define USB_OTG_DEACHINTMSK_OEP1INTM_Pos         (17U)                         
#define USB_OTG_DEACHINTMSK_OEP1INTM_Msk         (0x1U << USB_OTG_DEACHINTMSK_OEP1INTM_Pos) /*!< 0x00020000 */
#define USB_OTG_DEACHINTMSK_OEP1INTM             USB_OTG_DEACHINTMSK_OEP1INTM_Msk /*!< OUT Endpoint 1 interrupt mask bit */
#define USB_OTG_CID_PRODUCT_ID_Pos               (0U)                          
#define USB_OTG_CID_PRODUCT_ID_Msk               (0xFFFFFFFFU << USB_OTG_CID_PRODUCT_ID_Pos) /*!< 0xFFFFFFFF */
#define USB_OTG_CID_PRODUCT_ID                   USB_OTG_CID_PRODUCT_ID_Msk    /*!< Product ID field */
#define USB_OTG_GLPMCFG_LPMEN_Pos                (0U)                          
#define USB_OTG_GLPMCFG_LPMEN_Msk                (0x1U << USB_OTG_GLPMCFG_LPMEN_Pos) /*!< 0x00000001 */
#define USB_OTG_GLPMCFG_LPMEN                    USB_OTG_GLPMCFG_LPMEN_Msk     /*!< LPM support enable                                     */
#define USB_OTG_GLPMCFG_LPMACK_Pos               (1U)                          
#define USB_OTG_GLPMCFG_LPMACK_Msk               (0x1U << USB_OTG_GLPMCFG_LPMACK_Pos) /*!< 0x00000002 */
#define USB_OTG_GLPMCFG_LPMACK                   USB_OTG_GLPMCFG_LPMACK_Msk    /*!< LPM Token acknowledge enable                           */
#define USB_OTG_GLPMCFG_BESL_Pos                 (2U)                          
#define USB_OTG_GLPMCFG_BESL_Msk                 (0xFU << USB_OTG_GLPMCFG_BESL_Pos) /*!< 0x0000003C */
#define USB_OTG_GLPMCFG_BESL                     USB_OTG_GLPMCFG_BESL_Msk      /*!< BESL value received with last ACKed LPM Token          */
#define USB_OTG_GLPMCFG_REMWAKE_Pos              (6U)                          
#define USB_OTG_GLPMCFG_REMWAKE_Msk              (0x1U << USB_OTG_GLPMCFG_REMWAKE_Pos) /*!< 0x00000040 */
#define USB_OTG_GLPMCFG_REMWAKE                  USB_OTG_GLPMCFG_REMWAKE_Msk   /*!< bRemoteWake value received with last ACKed LPM Token   */
#define USB_OTG_GLPMCFG_L1SSEN_Pos               (7U)                          
#define USB_OTG_GLPMCFG_L1SSEN_Msk               (0x1U << USB_OTG_GLPMCFG_L1SSEN_Pos) /*!< 0x00000080 */
#define USB_OTG_GLPMCFG_L1SSEN                   USB_OTG_GLPMCFG_L1SSEN_Msk    /*!< L1 shallow sleep enable                                */
#define USB_OTG_GLPMCFG_BESLTHRS_Pos             (8U)                          
#define USB_OTG_GLPMCFG_BESLTHRS_Msk             (0xFU << USB_OTG_GLPMCFG_BESLTHRS_Pos) /*!< 0x00000F00 */
#define USB_OTG_GLPMCFG_BESLTHRS                 USB_OTG_GLPMCFG_BESLTHRS_Msk  /*!< BESL threshold                                         */
#define USB_OTG_GLPMCFG_L1DSEN_Pos               (12U)                         
#define USB_OTG_GLPMCFG_L1DSEN_Msk               (0x1U << USB_OTG_GLPMCFG_L1DSEN_Pos) /*!< 0x00001000 */
#define USB_OTG_GLPMCFG_L1DSEN                   USB_OTG_GLPMCFG_L1DSEN_Msk    /*!< L1 deep sleep enable                                   */
#define USB_OTG_GLPMCFG_LPMRSP_Pos               (13U)                         
#define USB_OTG_GLPMCFG_LPMRSP_Msk               (0x3U << USB_OTG_GLPMCFG_LPMRSP_Pos) /*!< 0x00006000 */
#define USB_OTG_GLPMCFG_LPMRSP                   USB_OTG_GLPMCFG_LPMRSP_Msk    /*!< LPM response                                           */
#define USB_OTG_GLPMCFG_SLPSTS_Pos               (15U)                         
#define USB_OTG_GLPMCFG_SLPSTS_Msk               (0x1U << USB_OTG_GLPMCFG_SLPSTS_Pos) /*!< 0x00008000 */
#define USB_OTG_GLPMCFG_SLPSTS                   USB_OTG_GLPMCFG_SLPSTS_Msk    /*!< Port sleep status                                      */
#define USB_OTG_GLPMCFG_L1RSMOK_Pos              (16U)                         
#define USB_OTG_GLPMCFG_L1RSMOK_Msk              (0x1U << USB_OTG_GLPMCFG_L1RSMOK_Pos) /*!< 0x00010000 */
#define USB_OTG_GLPMCFG_L1RSMOK                  USB_OTG_GLPMCFG_L1RSMOK_Msk   /*!< Sleep State Resume OK                                  */
#define USB_OTG_GLPMCFG_LPMCHIDX_Pos             (17U)                         
#define USB_OTG_GLPMCFG_LPMCHIDX_Msk             (0xFU << USB_OTG_GLPMCFG_LPMCHIDX_Pos) /*!< 0x001E0000 */
#define USB_OTG_GLPMCFG_LPMCHIDX                 USB_OTG_GLPMCFG_LPMCHIDX_Msk  /*!< LPM Channel Index                                      */
#define USB_OTG_GLPMCFG_LPMRCNT_Pos              (21U)                         
#define USB_OTG_GLPMCFG_LPMRCNT_Msk              (0x7U << USB_OTG_GLPMCFG_LPMRCNT_Pos) /*!< 0x00E00000 */
#define USB_OTG_GLPMCFG_LPMRCNT                  USB_OTG_GLPMCFG_LPMRCNT_Msk   /*!< LPM retry count                                        */
#define USB_OTG_GLPMCFG_SNDLPM_Pos               (24U)                         
#define USB_OTG_GLPMCFG_SNDLPM_Msk               (0x1U << USB_OTG_GLPMCFG_SNDLPM_Pos) /*!< 0x01000000 */
#define USB_OTG_GLPMCFG_SNDLPM                   USB_OTG_GLPMCFG_SNDLPM_Msk    /*!< Send LPM transaction                                   */
#define USB_OTG_GLPMCFG_LPMRCNTSTS_Pos           (25U)                         
#define USB_OTG_GLPMCFG_LPMRCNTSTS_Msk           (0x7U << USB_OTG_GLPMCFG_LPMRCNTSTS_Pos) /*!< 0x0E000000 */
#define USB_OTG_GLPMCFG_LPMRCNTSTS               USB_OTG_GLPMCFG_LPMRCNTSTS_Msk /*!< LPM retry count status                                 */
#define USB_OTG_GLPMCFG_ENBESL_Pos               (28U)                         
#define USB_OTG_GLPMCFG_ENBESL_Msk               (0x1U << USB_OTG_GLPMCFG_ENBESL_Pos) /*!< 0x10000000 */
#define USB_OTG_GLPMCFG_ENBESL                   USB_OTG_GLPMCFG_ENBESL_Msk    /*!< Enable best effort service latency                     */
#define USB_OTG_DIEPEACHMSK1_XFRCM_Pos           (0U)                          
#define USB_OTG_DIEPEACHMSK1_XFRCM_Msk           (0x1U << USB_OTG_DIEPEACHMSK1_XFRCM_Pos) /*!< 0x00000001 */
#define USB_OTG_DIEPEACHMSK1_XFRCM               USB_OTG_DIEPEACHMSK1_XFRCM_Msk /*!< Transfer completed interrupt mask                 */
#define USB_OTG_DIEPEACHMSK1_EPDM_Pos            (1U)                          
#define USB_OTG_DIEPEACHMSK1_EPDM_Msk            (0x1U << USB_OTG_DIEPEACHMSK1_EPDM_Pos) /*!< 0x00000002 */
#define USB_OTG_DIEPEACHMSK1_EPDM                USB_OTG_DIEPEACHMSK1_EPDM_Msk /*!< Endpoint disabled interrupt mask                  */
#define USB_OTG_DIEPEACHMSK1_TOM_Pos             (3U)                          
#define USB_OTG_DIEPEACHMSK1_TOM_Msk             (0x1U << USB_OTG_DIEPEACHMSK1_TOM_Pos) /*!< 0x00000008 */
#define USB_OTG_DIEPEACHMSK1_TOM                 USB_OTG_DIEPEACHMSK1_TOM_Msk  /*!< Timeout condition mask (nonisochronous endpoints) */
#define USB_OTG_DIEPEACHMSK1_ITTXFEMSK_Pos       (4U)                          
#define USB_OTG_DIEPEACHMSK1_ITTXFEMSK_Msk       (0x1U << USB_OTG_DIEPEACHMSK1_ITTXFEMSK_Pos) /*!< 0x00000010 */
#define USB_OTG_DIEPEACHMSK1_ITTXFEMSK           USB_OTG_DIEPEACHMSK1_ITTXFEMSK_Msk /*!< IN token received when TxFIFO empty mask          */
#define USB_OTG_DIEPEACHMSK1_INEPNMM_Pos         (5U)                          
#define USB_OTG_DIEPEACHMSK1_INEPNMM_Msk         (0x1U << USB_OTG_DIEPEACHMSK1_INEPNMM_Pos) /*!< 0x00000020 */
#define USB_OTG_DIEPEACHMSK1_INEPNMM             USB_OTG_DIEPEACHMSK1_INEPNMM_Msk /*!< IN token received with EP mismatch mask           */
#define USB_OTG_DIEPEACHMSK1_INEPNEM_Pos         (6U)                          
#define USB_OTG_DIEPEACHMSK1_INEPNEM_Msk         (0x1U << USB_OTG_DIEPEACHMSK1_INEPNEM_Pos) /*!< 0x00000040 */
#define USB_OTG_DIEPEACHMSK1_INEPNEM             USB_OTG_DIEPEACHMSK1_INEPNEM_Msk /*!< IN endpoint NAK effective mask                    */
#define USB_OTG_DIEPEACHMSK1_TXFURM_Pos          (8U)                          
#define USB_OTG_DIEPEACHMSK1_TXFURM_Msk          (0x1U << USB_OTG_DIEPEACHMSK1_TXFURM_Pos) /*!< 0x00000100 */
#define USB_OTG_DIEPEACHMSK1_TXFURM              USB_OTG_DIEPEACHMSK1_TXFURM_Msk /*!< FIFO underrun mask */
#define USB_OTG_DIEPEACHMSK1_BIM_Pos             (9U)                          
#define USB_OTG_DIEPEACHMSK1_BIM_Msk             (0x1U << USB_OTG_DIEPEACHMSK1_BIM_Pos) /*!< 0x00000200 */
#define USB_OTG_DIEPEACHMSK1_BIM                 USB_OTG_DIEPEACHMSK1_BIM_Msk  /*!< BNA interrupt mask */
#define USB_OTG_DIEPEACHMSK1_NAKM_Pos            (13U)                         
#define USB_OTG_DIEPEACHMSK1_NAKM_Msk            (0x1U << USB_OTG_DIEPEACHMSK1_NAKM_Pos) /*!< 0x00002000 */
#define USB_OTG_DIEPEACHMSK1_NAKM                USB_OTG_DIEPEACHMSK1_NAKM_Msk /*!< NAK interrupt mask */
#define USB_OTG_HPRT_PCSTS_Pos                   (0U)                          
#define USB_OTG_HPRT_PCSTS_Msk                   (0x1U << USB_OTG_HPRT_PCSTS_Pos) /*!< 0x00000001 */
#define USB_OTG_HPRT_PCSTS                       USB_OTG_HPRT_PCSTS_Msk        /*!< Port connect status        */
#define USB_OTG_HPRT_PCDET_Pos                   (1U)                          
#define USB_OTG_HPRT_PCDET_Msk                   (0x1U << USB_OTG_HPRT_PCDET_Pos) /*!< 0x00000002 */
#define USB_OTG_HPRT_PCDET                       USB_OTG_HPRT_PCDET_Msk        /*!< Port connect detected      */
#define USB_OTG_HPRT_PENA_Pos                    (2U)                          
#define USB_OTG_HPRT_PENA_Msk                    (0x1U << USB_OTG_HPRT_PENA_Pos) /*!< 0x00000004 */
#define USB_OTG_HPRT_PENA                        USB_OTG_HPRT_PENA_Msk         /*!< Port enable                */
#define USB_OTG_HPRT_PENCHNG_Pos                 (3U)                          
#define USB_OTG_HPRT_PENCHNG_Msk                 (0x1U << USB_OTG_HPRT_PENCHNG_Pos) /*!< 0x00000008 */
#define USB_OTG_HPRT_PENCHNG                     USB_OTG_HPRT_PENCHNG_Msk      /*!< Port enable/disable change */
#define USB_OTG_HPRT_POCA_Pos                    (4U)                          
#define USB_OTG_HPRT_POCA_Msk                    (0x1U << USB_OTG_HPRT_POCA_Pos) /*!< 0x00000010 */
#define USB_OTG_HPRT_POCA                        USB_OTG_HPRT_POCA_Msk         /*!< Port overcurrent active    */
#define USB_OTG_HPRT_POCCHNG_Pos                 (5U)                          
#define USB_OTG_HPRT_POCCHNG_Msk                 (0x1U << USB_OTG_HPRT_POCCHNG_Pos) /*!< 0x00000020 */
#define USB_OTG_HPRT_POCCHNG                     USB_OTG_HPRT_POCCHNG_Msk      /*!< Port overcurrent change    */
#define USB_OTG_HPRT_PRES_Pos                    (6U)                          
#define USB_OTG_HPRT_PRES_Msk                    (0x1U << USB_OTG_HPRT_PRES_Pos) /*!< 0x00000040 */
#define USB_OTG_HPRT_PRES                        USB_OTG_HPRT_PRES_Msk         /*!< Port resume   */
#define USB_OTG_HPRT_PSUSP_Pos                   (7U)                          
#define USB_OTG_HPRT_PSUSP_Msk                   (0x1U << USB_OTG_HPRT_PSUSP_Pos) /*!< 0x00000080 */
#define USB_OTG_HPRT_PSUSP                       USB_OTG_HPRT_PSUSP_Msk        /*!< Port suspend  */
#define USB_OTG_HPRT_PRST_Pos                    (8U)                          
#define USB_OTG_HPRT_PRST_Msk                    (0x1U << USB_OTG_HPRT_PRST_Pos) /*!< 0x00000100 */
#define USB_OTG_HPRT_PRST                        USB_OTG_HPRT_PRST_Msk         /*!< Port reset    */
#define USB_OTG_HPRT_PLSTS_Pos                   (10U)                         
#define USB_OTG_HPRT_PLSTS_Msk                   (0x3U << USB_OTG_HPRT_PLSTS_Pos) /*!< 0x00000C00 */
#define USB_OTG_HPRT_PLSTS                       USB_OTG_HPRT_PLSTS_Msk        /*!< Port line status */
#define USB_OTG_HPRT_PLSTS_0                     (0x1U << USB_OTG_HPRT_PLSTS_Pos) /*!< 0x00000400 */
#define USB_OTG_HPRT_PLSTS_1                     (0x2U << USB_OTG_HPRT_PLSTS_Pos) /*!< 0x00000800 */
#define USB_OTG_HPRT_PPWR_Pos                    (12U)                         
#define USB_OTG_HPRT_PPWR_Msk                    (0x1U << USB_OTG_HPRT_PPWR_Pos) /*!< 0x00001000 */
#define USB_OTG_HPRT_PPWR                        USB_OTG_HPRT_PPWR_Msk         /*!< Port power */
#define USB_OTG_HPRT_PTCTL_Pos                   (13U)                         
#define USB_OTG_HPRT_PTCTL_Msk                   (0xFU << USB_OTG_HPRT_PTCTL_Pos) /*!< 0x0001E000 */
#define USB_OTG_HPRT_PTCTL                       USB_OTG_HPRT_PTCTL_Msk        /*!< Port test control */
#define USB_OTG_HPRT_PTCTL_0                     (0x1U << USB_OTG_HPRT_PTCTL_Pos) /*!< 0x00002000 */
#define USB_OTG_HPRT_PTCTL_1                     (0x2U << USB_OTG_HPRT_PTCTL_Pos) /*!< 0x00004000 */
#define USB_OTG_HPRT_PTCTL_2                     (0x4U << USB_OTG_HPRT_PTCTL_Pos) /*!< 0x00008000 */
#define USB_OTG_HPRT_PTCTL_3                     (0x8U << USB_OTG_HPRT_PTCTL_Pos) /*!< 0x00010000 */
#define USB_OTG_HPRT_PSPD_Pos                    (17U)                         
#define USB_OTG_HPRT_PSPD_Msk                    (0x3U << USB_OTG_HPRT_PSPD_Pos) /*!< 0x00060000 */
#define USB_OTG_HPRT_PSPD                        USB_OTG_HPRT_PSPD_Msk         /*!< Port speed */
#define USB_OTG_HPRT_PSPD_0                      (0x1U << USB_OTG_HPRT_PSPD_Pos) /*!< 0x00020000 */
#define USB_OTG_HPRT_PSPD_1                      (0x2U << USB_OTG_HPRT_PSPD_Pos) /*!< 0x00040000 */
#define USB_OTG_DOEPEACHMSK1_XFRCM_Pos           (0U)                          
#define USB_OTG_DOEPEACHMSK1_XFRCM_Msk           (0x1U << USB_OTG_DOEPEACHMSK1_XFRCM_Pos) /*!< 0x00000001 */
#define USB_OTG_DOEPEACHMSK1_XFRCM               USB_OTG_DOEPEACHMSK1_XFRCM_Msk /*!< Transfer completed interrupt mask */
#define USB_OTG_DOEPEACHMSK1_EPDM_Pos            (1U)                          
#define USB_OTG_DOEPEACHMSK1_EPDM_Msk            (0x1U << USB_OTG_DOEPEACHMSK1_EPDM_Pos) /*!< 0x00000002 */
#define USB_OTG_DOEPEACHMSK1_EPDM                USB_OTG_DOEPEACHMSK1_EPDM_Msk /*!< Endpoint disabled interrupt mask */
#define USB_OTG_DOEPEACHMSK1_TOM_Pos             (3U)                          
#define USB_OTG_DOEPEACHMSK1_TOM_Msk             (0x1U << USB_OTG_DOEPEACHMSK1_TOM_Pos) /*!< 0x00000008 */
#define USB_OTG_DOEPEACHMSK1_TOM                 USB_OTG_DOEPEACHMSK1_TOM_Msk  /*!< Timeout condition mask */
#define USB_OTG_DOEPEACHMSK1_ITTXFEMSK_Pos       (4U)                          
#define USB_OTG_DOEPEACHMSK1_ITTXFEMSK_Msk       (0x1U << USB_OTG_DOEPEACHMSK1_ITTXFEMSK_Pos) /*!< 0x00000010 */
#define USB_OTG_DOEPEACHMSK1_ITTXFEMSK           USB_OTG_DOEPEACHMSK1_ITTXFEMSK_Msk /*!< IN token received when TxFIFO empty mask */
#define USB_OTG_DOEPEACHMSK1_INEPNMM_Pos         (5U)                          
#define USB_OTG_DOEPEACHMSK1_INEPNMM_Msk         (0x1U << USB_OTG_DOEPEACHMSK1_INEPNMM_Pos) /*!< 0x00000020 */
#define USB_OTG_DOEPEACHMSK1_INEPNMM             USB_OTG_DOEPEACHMSK1_INEPNMM_Msk /*!< IN token received with EP mismatch mask */
#define USB_OTG_DOEPEACHMSK1_INEPNEM_Pos         (6U)                          
#define USB_OTG_DOEPEACHMSK1_INEPNEM_Msk         (0x1U << USB_OTG_DOEPEACHMSK1_INEPNEM_Pos) /*!< 0x00000040 */
#define USB_OTG_DOEPEACHMSK1_INEPNEM             USB_OTG_DOEPEACHMSK1_INEPNEM_Msk /*!< IN endpoint NAK effective mask */
#define USB_OTG_DOEPEACHMSK1_TXFURM_Pos          (8U)                          
#define USB_OTG_DOEPEACHMSK1_TXFURM_Msk          (0x1U << USB_OTG_DOEPEACHMSK1_TXFURM_Pos) /*!< 0x00000100 */
#define USB_OTG_DOEPEACHMSK1_TXFURM              USB_OTG_DOEPEACHMSK1_TXFURM_Msk /*!< OUT packet error mask */
#define USB_OTG_DOEPEACHMSK1_BIM_Pos             (9U)                          
#define USB_OTG_DOEPEACHMSK1_BIM_Msk             (0x1U << USB_OTG_DOEPEACHMSK1_BIM_Pos) /*!< 0x00000200 */
#define USB_OTG_DOEPEACHMSK1_BIM                 USB_OTG_DOEPEACHMSK1_BIM_Msk  /*!< BNA interrupt mask */
#define USB_OTG_DOEPEACHMSK1_BERRM_Pos           (12U)                         
#define USB_OTG_DOEPEACHMSK1_BERRM_Msk           (0x1U << USB_OTG_DOEPEACHMSK1_BERRM_Pos) /*!< 0x00001000 */
#define USB_OTG_DOEPEACHMSK1_BERRM               USB_OTG_DOEPEACHMSK1_BERRM_Msk /*!< Bubble error interrupt mask */
#define USB_OTG_DOEPEACHMSK1_NAKM_Pos            (13U)                         
#define USB_OTG_DOEPEACHMSK1_NAKM_Msk            (0x1U << USB_OTG_DOEPEACHMSK1_NAKM_Pos) /*!< 0x00002000 */
#define USB_OTG_DOEPEACHMSK1_NAKM                USB_OTG_DOEPEACHMSK1_NAKM_Msk /*!< NAK interrupt mask */
#define USB_OTG_DOEPEACHMSK1_NYETM_Pos           (14U)                         
#define USB_OTG_DOEPEACHMSK1_NYETM_Msk           (0x1U << USB_OTG_DOEPEACHMSK1_NYETM_Pos) /*!< 0x00004000 */
#define USB_OTG_DOEPEACHMSK1_NYETM               USB_OTG_DOEPEACHMSK1_NYETM_Msk /*!< NYET interrupt mask */
#define USB_OTG_HPTXFSIZ_PTXSA_Pos               (0U)                          
#define USB_OTG_HPTXFSIZ_PTXSA_Msk               (0xFFFFU << USB_OTG_HPTXFSIZ_PTXSA_Pos) /*!< 0x0000FFFF */
#define USB_OTG_HPTXFSIZ_PTXSA                   USB_OTG_HPTXFSIZ_PTXSA_Msk    /*!< Host periodic TxFIFO start address */
#define USB_OTG_HPTXFSIZ_PTXFD_Pos               (16U)                         
#define USB_OTG_HPTXFSIZ_PTXFD_Msk               (0xFFFFU << USB_OTG_HPTXFSIZ_PTXFD_Pos) /*!< 0xFFFF0000 */
#define USB_OTG_HPTXFSIZ_PTXFD                   USB_OTG_HPTXFSIZ_PTXFD_Msk    /*!< Host periodic TxFIFO depth */
#define USB_OTG_DIEPCTL_MPSIZ_Pos                (0U)                          
#define USB_OTG_DIEPCTL_MPSIZ_Msk                (0x7FFU << USB_OTG_DIEPCTL_MPSIZ_Pos) /*!< 0x000007FF */
#define USB_OTG_DIEPCTL_MPSIZ                    USB_OTG_DIEPCTL_MPSIZ_Msk     /*!< Maximum packet size */
#define USB_OTG_DIEPCTL_USBAEP_Pos               (15U)                         
#define USB_OTG_DIEPCTL_USBAEP_Msk               (0x1U << USB_OTG_DIEPCTL_USBAEP_Pos) /*!< 0x00008000 */
#define USB_OTG_DIEPCTL_USBAEP                   USB_OTG_DIEPCTL_USBAEP_Msk    /*!< USB active endpoint */
#define USB_OTG_DIEPCTL_EONUM_DPID_Pos           (16U)                         
#define USB_OTG_DIEPCTL_EONUM_DPID_Msk           (0x1U << USB_OTG_DIEPCTL_EONUM_DPID_Pos) /*!< 0x00010000 */
#define USB_OTG_DIEPCTL_EONUM_DPID               USB_OTG_DIEPCTL_EONUM_DPID_Msk /*!< Even/odd frame */
#define USB_OTG_DIEPCTL_NAKSTS_Pos               (17U)                         
#define USB_OTG_DIEPCTL_NAKSTS_Msk               (0x1U << USB_OTG_DIEPCTL_NAKSTS_Pos) /*!< 0x00020000 */
#define USB_OTG_DIEPCTL_NAKSTS                   USB_OTG_DIEPCTL_NAKSTS_Msk    /*!< NAK status */
#define USB_OTG_DIEPCTL_EPTYP_Pos                (18U)                         
#define USB_OTG_DIEPCTL_EPTYP_Msk                (0x3U << USB_OTG_DIEPCTL_EPTYP_Pos) /*!< 0x000C0000 */
#define USB_OTG_DIEPCTL_EPTYP                    USB_OTG_DIEPCTL_EPTYP_Msk     /*!< Endpoint type */
#define USB_OTG_DIEPCTL_EPTYP_0                  (0x1U << USB_OTG_DIEPCTL_EPTYP_Pos) /*!< 0x00040000 */
#define USB_OTG_DIEPCTL_EPTYP_1                  (0x2U << USB_OTG_DIEPCTL_EPTYP_Pos) /*!< 0x00080000 */
#define USB_OTG_DIEPCTL_STALL_Pos                (21U)                         
#define USB_OTG_DIEPCTL_STALL_Msk                (0x1U << USB_OTG_DIEPCTL_STALL_Pos) /*!< 0x00200000 */
#define USB_OTG_DIEPCTL_STALL                    USB_OTG_DIEPCTL_STALL_Msk     /*!< STALL handshake */
#define USB_OTG_DIEPCTL_TXFNUM_Pos               (22U)                         
#define USB_OTG_DIEPCTL_TXFNUM_Msk               (0xFU << USB_OTG_DIEPCTL_TXFNUM_Pos) /*!< 0x03C00000 */
#define USB_OTG_DIEPCTL_TXFNUM                   USB_OTG_DIEPCTL_TXFNUM_Msk    /*!< TxFIFO number */
#define USB_OTG_DIEPCTL_TXFNUM_0                 (0x1U << USB_OTG_DIEPCTL_TXFNUM_Pos) /*!< 0x00400000 */
#define USB_OTG_DIEPCTL_TXFNUM_1                 (0x2U << USB_OTG_DIEPCTL_TXFNUM_Pos) /*!< 0x00800000 */
#define USB_OTG_DIEPCTL_TXFNUM_2                 (0x4U << USB_OTG_DIEPCTL_TXFNUM_Pos) /*!< 0x01000000 */
#define USB_OTG_DIEPCTL_TXFNUM_3                 (0x8U << USB_OTG_DIEPCTL_TXFNUM_Pos) /*!< 0x02000000 */
#define USB_OTG_DIEPCTL_CNAK_Pos                 (26U)                         
#define USB_OTG_DIEPCTL_CNAK_Msk                 (0x1U << USB_OTG_DIEPCTL_CNAK_Pos) /*!< 0x04000000 */
#define USB_OTG_DIEPCTL_CNAK                     USB_OTG_DIEPCTL_CNAK_Msk      /*!< Clear NAK */
#define USB_OTG_DIEPCTL_SNAK_Pos                 (27U)                         
#define USB_OTG_DIEPCTL_SNAK_Msk                 (0x1U << USB_OTG_DIEPCTL_SNAK_Pos) /*!< 0x08000000 */
#define USB_OTG_DIEPCTL_SNAK                     USB_OTG_DIEPCTL_SNAK_Msk      /*!< Set NAK */
#define USB_OTG_DIEPCTL_SD0PID_SEVNFRM_Pos       (28U)                         
#define USB_OTG_DIEPCTL_SD0PID_SEVNFRM_Msk       (0x1U << USB_OTG_DIEPCTL_SD0PID_SEVNFRM_Pos) /*!< 0x10000000 */
#define USB_OTG_DIEPCTL_SD0PID_SEVNFRM           USB_OTG_DIEPCTL_SD0PID_SEVNFRM_Msk /*!< Set DATA0 PID */
#define USB_OTG_DIEPCTL_SODDFRM_Pos              (29U)                         
#define USB_OTG_DIEPCTL_SODDFRM_Msk              (0x1U << USB_OTG_DIEPCTL_SODDFRM_Pos) /*!< 0x20000000 */
#define USB_OTG_DIEPCTL_SODDFRM                  USB_OTG_DIEPCTL_SODDFRM_Msk   /*!< Set odd frame */
#define USB_OTG_DIEPCTL_EPDIS_Pos                (30U)                         
#define USB_OTG_DIEPCTL_EPDIS_Msk                (0x1U << USB_OTG_DIEPCTL_EPDIS_Pos) /*!< 0x40000000 */
#define USB_OTG_DIEPCTL_EPDIS                    USB_OTG_DIEPCTL_EPDIS_Msk     /*!< Endpoint disable */
#define USB_OTG_DIEPCTL_EPENA_Pos                (31U)                         
#define USB_OTG_DIEPCTL_EPENA_Msk                (0x1U << USB_OTG_DIEPCTL_EPENA_Pos) /*!< 0x80000000 */
#define USB_OTG_DIEPCTL_EPENA                    USB_OTG_DIEPCTL_EPENA_Msk     /*!< Endpoint enable */
#define USB_OTG_HCCHAR_MPSIZ_Pos                 (0U)                          
#define USB_OTG_HCCHAR_MPSIZ_Msk                 (0x7FFU << USB_OTG_HCCHAR_MPSIZ_Pos) /*!< 0x000007FF */
#define USB_OTG_HCCHAR_MPSIZ                     USB_OTG_HCCHAR_MPSIZ_Msk      /*!< Maximum packet size */
#define USB_OTG_HCCHAR_EPNUM_Pos                 (11U)                         
#define USB_OTG_HCCHAR_EPNUM_Msk                 (0xFU << USB_OTG_HCCHAR_EPNUM_Pos) /*!< 0x00007800 */
#define USB_OTG_HCCHAR_EPNUM                     USB_OTG_HCCHAR_EPNUM_Msk      /*!< Endpoint number */
#define USB_OTG_HCCHAR_EPNUM_0                   (0x1U << USB_OTG_HCCHAR_EPNUM_Pos) /*!< 0x00000800 */
#define USB_OTG_HCCHAR_EPNUM_1                   (0x2U << USB_OTG_HCCHAR_EPNUM_Pos) /*!< 0x00001000 */
#define USB_OTG_HCCHAR_EPNUM_2                   (0x4U << USB_OTG_HCCHAR_EPNUM_Pos) /*!< 0x00002000 */
#define USB_OTG_HCCHAR_EPNUM_3                   (0x8U << USB_OTG_HCCHAR_EPNUM_Pos) /*!< 0x00004000 */
#define USB_OTG_HCCHAR_EPDIR_Pos                 (15U)                         
#define USB_OTG_HCCHAR_EPDIR_Msk                 (0x1U << USB_OTG_HCCHAR_EPDIR_Pos) /*!< 0x00008000 */
#define USB_OTG_HCCHAR_EPDIR                     USB_OTG_HCCHAR_EPDIR_Msk      /*!< Endpoint direction */
#define USB_OTG_HCCHAR_LSDEV_Pos                 (17U)                         
#define USB_OTG_HCCHAR_LSDEV_Msk                 (0x1U << USB_OTG_HCCHAR_LSDEV_Pos) /*!< 0x00020000 */
#define USB_OTG_HCCHAR_LSDEV                     USB_OTG_HCCHAR_LSDEV_Msk      /*!< Low-speed device */
#define USB_OTG_HCCHAR_EPTYP_Pos                 (18U)                         
#define USB_OTG_HCCHAR_EPTYP_Msk                 (0x3U << USB_OTG_HCCHAR_EPTYP_Pos) /*!< 0x000C0000 */
#define USB_OTG_HCCHAR_EPTYP                     USB_OTG_HCCHAR_EPTYP_Msk      /*!< Endpoint type */
#define USB_OTG_HCCHAR_EPTYP_0                   (0x1U << USB_OTG_HCCHAR_EPTYP_Pos) /*!< 0x00040000 */
#define USB_OTG_HCCHAR_EPTYP_1                   (0x2U << USB_OTG_HCCHAR_EPTYP_Pos) /*!< 0x00080000 */
#define USB_OTG_HCCHAR_MC_Pos                    (20U)                         
#define USB_OTG_HCCHAR_MC_Msk                    (0x3U << USB_OTG_HCCHAR_MC_Pos) /*!< 0x00300000 */
#define USB_OTG_HCCHAR_MC                        USB_OTG_HCCHAR_MC_Msk         /*!< Multi Count (MC) / Error Count (EC) */
#define USB_OTG_HCCHAR_MC_0                      (0x1U << USB_OTG_HCCHAR_MC_Pos) /*!< 0x00100000 */
#define USB_OTG_HCCHAR_MC_1                      (0x2U << USB_OTG_HCCHAR_MC_Pos) /*!< 0x00200000 */
#define USB_OTG_HCCHAR_DAD_Pos                   (22U)                         
#define USB_OTG_HCCHAR_DAD_Msk                   (0x7FU << USB_OTG_HCCHAR_DAD_Pos) /*!< 0x1FC00000 */
#define USB_OTG_HCCHAR_DAD                       USB_OTG_HCCHAR_DAD_Msk        /*!< Device address */
#define USB_OTG_HCCHAR_DAD_0                     (0x01U << USB_OTG_HCCHAR_DAD_Pos) /*!< 0x00400000 */
#define USB_OTG_HCCHAR_DAD_1                     (0x02U << USB_OTG_HCCHAR_DAD_Pos) /*!< 0x00800000 */
#define USB_OTG_HCCHAR_DAD_2                     (0x04U << USB_OTG_HCCHAR_DAD_Pos) /*!< 0x01000000 */
#define USB_OTG_HCCHAR_DAD_3                     (0x08U << USB_OTG_HCCHAR_DAD_Pos) /*!< 0x02000000 */
#define USB_OTG_HCCHAR_DAD_4                     (0x10U << USB_OTG_HCCHAR_DAD_Pos) /*!< 0x04000000 */
#define USB_OTG_HCCHAR_DAD_5                     (0x20U << USB_OTG_HCCHAR_DAD_Pos) /*!< 0x08000000 */
#define USB_OTG_HCCHAR_DAD_6                     (0x40U << USB_OTG_HCCHAR_DAD_Pos) /*!< 0x10000000 */
#define USB_OTG_HCCHAR_ODDFRM_Pos                (29U)                         
#define USB_OTG_HCCHAR_ODDFRM_Msk                (0x1U << USB_OTG_HCCHAR_ODDFRM_Pos) /*!< 0x20000000 */
#define USB_OTG_HCCHAR_ODDFRM                    USB_OTG_HCCHAR_ODDFRM_Msk     /*!< Odd frame */
#define USB_OTG_HCCHAR_CHDIS_Pos                 (30U)                         
#define USB_OTG_HCCHAR_CHDIS_Msk                 (0x1U << USB_OTG_HCCHAR_CHDIS_Pos) /*!< 0x40000000 */
#define USB_OTG_HCCHAR_CHDIS                     USB_OTG_HCCHAR_CHDIS_Msk      /*!< Channel disable */
#define USB_OTG_HCCHAR_CHENA_Pos                 (31U)                         
#define USB_OTG_HCCHAR_CHENA_Msk                 (0x1U << USB_OTG_HCCHAR_CHENA_Pos) /*!< 0x80000000 */
#define USB_OTG_HCCHAR_CHENA                     USB_OTG_HCCHAR_CHENA_Msk      /*!< Channel enable */
#define USB_OTG_HCSPLT_PRTADDR_Pos               (0U)                          
#define USB_OTG_HCSPLT_PRTADDR_Msk               (0x7FU << USB_OTG_HCSPLT_PRTADDR_Pos) /*!< 0x0000007F */
#define USB_OTG_HCSPLT_PRTADDR                   USB_OTG_HCSPLT_PRTADDR_Msk    /*!< Port address */
#define USB_OTG_HCSPLT_PRTADDR_0                 (0x01U << USB_OTG_HCSPLT_PRTADDR_Pos) /*!< 0x00000001 */
#define USB_OTG_HCSPLT_PRTADDR_1                 (0x02U << USB_OTG_HCSPLT_PRTADDR_Pos) /*!< 0x00000002 */
#define USB_OTG_HCSPLT_PRTADDR_2                 (0x04U << USB_OTG_HCSPLT_PRTADDR_Pos) /*!< 0x00000004 */
#define USB_OTG_HCSPLT_PRTADDR_3                 (0x08U << USB_OTG_HCSPLT_PRTADDR_Pos) /*!< 0x00000008 */
#define USB_OTG_HCSPLT_PRTADDR_4                 (0x10U << USB_OTG_HCSPLT_PRTADDR_Pos) /*!< 0x00000010 */
#define USB_OTG_HCSPLT_PRTADDR_5                 (0x20U << USB_OTG_HCSPLT_PRTADDR_Pos) /*!< 0x00000020 */
#define USB_OTG_HCSPLT_PRTADDR_6                 (0x40U << USB_OTG_HCSPLT_PRTADDR_Pos) /*!< 0x00000040 */
#define USB_OTG_HCSPLT_HUBADDR_Pos               (7U)                          
#define USB_OTG_HCSPLT_HUBADDR_Msk               (0x7FU << USB_OTG_HCSPLT_HUBADDR_Pos) /*!< 0x00003F80 */
#define USB_OTG_HCSPLT_HUBADDR                   USB_OTG_HCSPLT_HUBADDR_Msk    /*!< Hub address */
#define USB_OTG_HCSPLT_HUBADDR_0                 (0x01U << USB_OTG_HCSPLT_HUBADDR_Pos) /*!< 0x00000080 */
#define USB_OTG_HCSPLT_HUBADDR_1                 (0x02U << USB_OTG_HCSPLT_HUBADDR_Pos) /*!< 0x00000100 */
#define USB_OTG_HCSPLT_HUBADDR_2                 (0x04U << USB_OTG_HCSPLT_HUBADDR_Pos) /*!< 0x00000200 */
#define USB_OTG_HCSPLT_HUBADDR_3                 (0x08U << USB_OTG_HCSPLT_HUBADDR_Pos) /*!< 0x00000400 */
#define USB_OTG_HCSPLT_HUBADDR_4                 (0x10U << USB_OTG_HCSPLT_HUBADDR_Pos) /*!< 0x00000800 */
#define USB_OTG_HCSPLT_HUBADDR_5                 (0x20U << USB_OTG_HCSPLT_HUBADDR_Pos) /*!< 0x00001000 */
#define USB_OTG_HCSPLT_HUBADDR_6                 (0x40U << USB_OTG_HCSPLT_HUBADDR_Pos) /*!< 0x00002000 */
#define USB_OTG_HCSPLT_XACTPOS_Pos               (14U)                         
#define USB_OTG_HCSPLT_XACTPOS_Msk               (0x3U << USB_OTG_HCSPLT_XACTPOS_Pos) /*!< 0x0000C000 */
#define USB_OTG_HCSPLT_XACTPOS                   USB_OTG_HCSPLT_XACTPOS_Msk    /*!< XACTPOS */
#define USB_OTG_HCSPLT_XACTPOS_0                 (0x1U << USB_OTG_HCSPLT_XACTPOS_Pos) /*!< 0x00004000 */
#define USB_OTG_HCSPLT_XACTPOS_1                 (0x2U << USB_OTG_HCSPLT_XACTPOS_Pos) /*!< 0x00008000 */
#define USB_OTG_HCSPLT_COMPLSPLT_Pos             (16U)                         
#define USB_OTG_HCSPLT_COMPLSPLT_Msk             (0x1U << USB_OTG_HCSPLT_COMPLSPLT_Pos) /*!< 0x00010000 */
#define USB_OTG_HCSPLT_COMPLSPLT                 USB_OTG_HCSPLT_COMPLSPLT_Msk  /*!< Do complete split */
#define USB_OTG_HCSPLT_SPLITEN_Pos               (31U)                         
#define USB_OTG_HCSPLT_SPLITEN_Msk               (0x1U << USB_OTG_HCSPLT_SPLITEN_Pos) /*!< 0x80000000 */
#define USB_OTG_HCSPLT_SPLITEN                   USB_OTG_HCSPLT_SPLITEN_Msk    /*!< Split enable */
#define USB_OTG_HCINT_XFRC_Pos                   (0U)                          
#define USB_OTG_HCINT_XFRC_Msk                   (0x1U << USB_OTG_HCINT_XFRC_Pos) /*!< 0x00000001 */
#define USB_OTG_HCINT_XFRC                       USB_OTG_HCINT_XFRC_Msk        /*!< Transfer completed */
#define USB_OTG_HCINT_CHH_Pos                    (1U)                          
#define USB_OTG_HCINT_CHH_Msk                    (0x1U << USB_OTG_HCINT_CHH_Pos) /*!< 0x00000002 */
#define USB_OTG_HCINT_CHH                        USB_OTG_HCINT_CHH_Msk         /*!< Channel halted */
#define USB_OTG_HCINT_AHBERR_Pos                 (2U)                          
#define USB_OTG_HCINT_AHBERR_Msk                 (0x1U << USB_OTG_HCINT_AHBERR_Pos) /*!< 0x00000004 */
#define USB_OTG_HCINT_AHBERR                     USB_OTG_HCINT_AHBERR_Msk      /*!< AHB error */
#define USB_OTG_HCINT_STALL_Pos                  (3U)                          
#define USB_OTG_HCINT_STALL_Msk                  (0x1U << USB_OTG_HCINT_STALL_Pos) /*!< 0x00000008 */
#define USB_OTG_HCINT_STALL                      USB_OTG_HCINT_STALL_Msk       /*!< STALL response received interrupt */
#define USB_OTG_HCINT_NAK_Pos                    (4U)                          
#define USB_OTG_HCINT_NAK_Msk                    (0x1U << USB_OTG_HCINT_NAK_Pos) /*!< 0x00000010 */
#define USB_OTG_HCINT_NAK                        USB_OTG_HCINT_NAK_Msk         /*!< NAK response received interrupt */
#define USB_OTG_HCINT_ACK_Pos                    (5U)                          
#define USB_OTG_HCINT_ACK_Msk                    (0x1U << USB_OTG_HCINT_ACK_Pos) /*!< 0x00000020 */
#define USB_OTG_HCINT_ACK                        USB_OTG_HCINT_ACK_Msk         /*!< ACK response received/transmitted interrupt */
#define USB_OTG_HCINT_NYET_Pos                   (6U)                          
#define USB_OTG_HCINT_NYET_Msk                   (0x1U << USB_OTG_HCINT_NYET_Pos) /*!< 0x00000040 */
#define USB_OTG_HCINT_NYET                       USB_OTG_HCINT_NYET_Msk        /*!< Response received interrupt */
#define USB_OTG_HCINT_TXERR_Pos                  (7U)                          
#define USB_OTG_HCINT_TXERR_Msk                  (0x1U << USB_OTG_HCINT_TXERR_Pos) /*!< 0x00000080 */
#define USB_OTG_HCINT_TXERR                      USB_OTG_HCINT_TXERR_Msk       /*!< Transaction error */
#define USB_OTG_HCINT_BBERR_Pos                  (8U)                          
#define USB_OTG_HCINT_BBERR_Msk                  (0x1U << USB_OTG_HCINT_BBERR_Pos) /*!< 0x00000100 */
#define USB_OTG_HCINT_BBERR                      USB_OTG_HCINT_BBERR_Msk       /*!< Babble error */
#define USB_OTG_HCINT_FRMOR_Pos                  (9U)                          
#define USB_OTG_HCINT_FRMOR_Msk                  (0x1U << USB_OTG_HCINT_FRMOR_Pos) /*!< 0x00000200 */
#define USB_OTG_HCINT_FRMOR                      USB_OTG_HCINT_FRMOR_Msk       /*!< Frame overrun */
#define USB_OTG_HCINT_DTERR_Pos                  (10U)                         
#define USB_OTG_HCINT_DTERR_Msk                  (0x1U << USB_OTG_HCINT_DTERR_Pos) /*!< 0x00000400 */
#define USB_OTG_HCINT_DTERR                      USB_OTG_HCINT_DTERR_Msk       /*!< Data toggle error */
#define USB_OTG_DIEPINT_XFRC_Pos                 (0U)                          
#define USB_OTG_DIEPINT_XFRC_Msk                 (0x1U << USB_OTG_DIEPINT_XFRC_Pos) /*!< 0x00000001 */
#define USB_OTG_DIEPINT_XFRC                     USB_OTG_DIEPINT_XFRC_Msk      /*!< Transfer completed interrupt */
#define USB_OTG_DIEPINT_EPDISD_Pos               (1U)                          
#define USB_OTG_DIEPINT_EPDISD_Msk               (0x1U << USB_OTG_DIEPINT_EPDISD_Pos) /*!< 0x00000002 */
#define USB_OTG_DIEPINT_EPDISD                   USB_OTG_DIEPINT_EPDISD_Msk    /*!< Endpoint disabled interrupt */
#define USB_OTG_DIEPINT_TOC_Pos                  (3U)                          
#define USB_OTG_DIEPINT_TOC_Msk                  (0x1U << USB_OTG_DIEPINT_TOC_Pos) /*!< 0x00000008 */
#define USB_OTG_DIEPINT_TOC                      USB_OTG_DIEPINT_TOC_Msk       /*!< Timeout condition */
#define USB_OTG_DIEPINT_ITTXFE_Pos               (4U)                          
#define USB_OTG_DIEPINT_ITTXFE_Msk               (0x1U << USB_OTG_DIEPINT_ITTXFE_Pos) /*!< 0x00000010 */
#define USB_OTG_DIEPINT_ITTXFE                   USB_OTG_DIEPINT_ITTXFE_Msk    /*!< IN token received when TxFIFO is empty */
#define USB_OTG_DIEPINT_INEPNE_Pos               (6U)                          
#define USB_OTG_DIEPINT_INEPNE_Msk               (0x1U << USB_OTG_DIEPINT_INEPNE_Pos) /*!< 0x00000040 */
#define USB_OTG_DIEPINT_INEPNE                   USB_OTG_DIEPINT_INEPNE_Msk    /*!< IN endpoint NAK effective */
#define USB_OTG_DIEPINT_TXFE_Pos                 (7U)                          
#define USB_OTG_DIEPINT_TXFE_Msk                 (0x1U << USB_OTG_DIEPINT_TXFE_Pos) /*!< 0x00000080 */
#define USB_OTG_DIEPINT_TXFE                     USB_OTG_DIEPINT_TXFE_Msk      /*!< Transmit FIFO empty */
#define USB_OTG_DIEPINT_TXFIFOUDRN_Pos           (8U)                          
#define USB_OTG_DIEPINT_TXFIFOUDRN_Msk           (0x1U << USB_OTG_DIEPINT_TXFIFOUDRN_Pos) /*!< 0x00000100 */
#define USB_OTG_DIEPINT_TXFIFOUDRN               USB_OTG_DIEPINT_TXFIFOUDRN_Msk /*!< Transmit Fifo Underrun */
#define USB_OTG_DIEPINT_BNA_Pos                  (9U)                          
#define USB_OTG_DIEPINT_BNA_Msk                  (0x1U << USB_OTG_DIEPINT_BNA_Pos) /*!< 0x00000200 */
#define USB_OTG_DIEPINT_BNA                      USB_OTG_DIEPINT_BNA_Msk       /*!< Buffer not available interrupt */
#define USB_OTG_DIEPINT_PKTDRPSTS_Pos            (11U)                         
#define USB_OTG_DIEPINT_PKTDRPSTS_Msk            (0x1U << USB_OTG_DIEPINT_PKTDRPSTS_Pos) /*!< 0x00000800 */
#define USB_OTG_DIEPINT_PKTDRPSTS                USB_OTG_DIEPINT_PKTDRPSTS_Msk /*!< Packet dropped status */
#define USB_OTG_DIEPINT_BERR_Pos                 (12U)                         
#define USB_OTG_DIEPINT_BERR_Msk                 (0x1U << USB_OTG_DIEPINT_BERR_Pos) /*!< 0x00001000 */
#define USB_OTG_DIEPINT_BERR                     USB_OTG_DIEPINT_BERR_Msk      /*!< Babble error interrupt */
#define USB_OTG_DIEPINT_NAK_Pos                  (13U)                         
#define USB_OTG_DIEPINT_NAK_Msk                  (0x1U << USB_OTG_DIEPINT_NAK_Pos) /*!< 0x00002000 */
#define USB_OTG_DIEPINT_NAK                      USB_OTG_DIEPINT_NAK_Msk       /*!< NAK interrupt */
#define USB_OTG_HCINTMSK_XFRCM_Pos               (0U)                          
#define USB_OTG_HCINTMSK_XFRCM_Msk               (0x1U << USB_OTG_HCINTMSK_XFRCM_Pos) /*!< 0x00000001 */
#define USB_OTG_HCINTMSK_XFRCM                   USB_OTG_HCINTMSK_XFRCM_Msk    /*!< Transfer completed mask */
#define USB_OTG_HCINTMSK_CHHM_Pos                (1U)                          
#define USB_OTG_HCINTMSK_CHHM_Msk                (0x1U << USB_OTG_HCINTMSK_CHHM_Pos) /*!< 0x00000002 */
#define USB_OTG_HCINTMSK_CHHM                    USB_OTG_HCINTMSK_CHHM_Msk     /*!< Channel halted mask */
#define USB_OTG_HCINTMSK_AHBERR_Pos              (2U)                          
#define USB_OTG_HCINTMSK_AHBERR_Msk              (0x1U << USB_OTG_HCINTMSK_AHBERR_Pos) /*!< 0x00000004 */
#define USB_OTG_HCINTMSK_AHBERR                  USB_OTG_HCINTMSK_AHBERR_Msk   /*!< AHB error */
#define USB_OTG_HCINTMSK_STALLM_Pos              (3U)                          
#define USB_OTG_HCINTMSK_STALLM_Msk              (0x1U << USB_OTG_HCINTMSK_STALLM_Pos) /*!< 0x00000008 */
#define USB_OTG_HCINTMSK_STALLM                  USB_OTG_HCINTMSK_STALLM_Msk   /*!< STALL response received interrupt mask */
#define USB_OTG_HCINTMSK_NAKM_Pos                (4U)                          
#define USB_OTG_HCINTMSK_NAKM_Msk                (0x1U << USB_OTG_HCINTMSK_NAKM_Pos) /*!< 0x00000010 */
#define USB_OTG_HCINTMSK_NAKM                    USB_OTG_HCINTMSK_NAKM_Msk     /*!< NAK response received interrupt mask */
#define USB_OTG_HCINTMSK_ACKM_Pos                (5U)                          
#define USB_OTG_HCINTMSK_ACKM_Msk                (0x1U << USB_OTG_HCINTMSK_ACKM_Pos) /*!< 0x00000020 */
#define USB_OTG_HCINTMSK_ACKM                    USB_OTG_HCINTMSK_ACKM_Msk     /*!< ACK response received/transmitted interrupt mask */
#define USB_OTG_HCINTMSK_NYET_Pos                (6U)                          
#define USB_OTG_HCINTMSK_NYET_Msk                (0x1U << USB_OTG_HCINTMSK_NYET_Pos) /*!< 0x00000040 */
#define USB_OTG_HCINTMSK_NYET                    USB_OTG_HCINTMSK_NYET_Msk     /*!< response received interrupt mask */
#define USB_OTG_HCINTMSK_TXERRM_Pos              (7U)                          
#define USB_OTG_HCINTMSK_TXERRM_Msk              (0x1U << USB_OTG_HCINTMSK_TXERRM_Pos) /*!< 0x00000080 */
#define USB_OTG_HCINTMSK_TXERRM                  USB_OTG_HCINTMSK_TXERRM_Msk   /*!< Transaction error mask */
#define USB_OTG_HCINTMSK_BBERRM_Pos              (8U)                          
#define USB_OTG_HCINTMSK_BBERRM_Msk              (0x1U << USB_OTG_HCINTMSK_BBERRM_Pos) /*!< 0x00000100 */
#define USB_OTG_HCINTMSK_BBERRM                  USB_OTG_HCINTMSK_BBERRM_Msk   /*!< Babble error mask */
#define USB_OTG_HCINTMSK_FRMORM_Pos              (9U)                          
#define USB_OTG_HCINTMSK_FRMORM_Msk              (0x1U << USB_OTG_HCINTMSK_FRMORM_Pos) /*!< 0x00000200 */
#define USB_OTG_HCINTMSK_FRMORM                  USB_OTG_HCINTMSK_FRMORM_Msk   /*!< Frame overrun mask */
#define USB_OTG_HCINTMSK_DTERRM_Pos              (10U)                         
#define USB_OTG_HCINTMSK_DTERRM_Msk              (0x1U << USB_OTG_HCINTMSK_DTERRM_Pos) /*!< 0x00000400 */
#define USB_OTG_HCINTMSK_DTERRM                  USB_OTG_HCINTMSK_DTERRM_Msk   /*!< Data toggle error mask */
#define USB_OTG_DIEPTSIZ_XFRSIZ_Pos              (0U)                          
#define USB_OTG_DIEPTSIZ_XFRSIZ_Msk              (0x7FFFFU << USB_OTG_DIEPTSIZ_XFRSIZ_Pos) /*!< 0x0007FFFF */
#define USB_OTG_DIEPTSIZ_XFRSIZ                  USB_OTG_DIEPTSIZ_XFRSIZ_Msk   /*!< Transfer size */
#define USB_OTG_DIEPTSIZ_PKTCNT_Pos              (19U)                         
#define USB_OTG_DIEPTSIZ_PKTCNT_Msk              (0x3FFU << USB_OTG_DIEPTSIZ_PKTCNT_Pos) /*!< 0x1FF80000 */
#define USB_OTG_DIEPTSIZ_PKTCNT                  USB_OTG_DIEPTSIZ_PKTCNT_Msk   /*!< Packet count */
#define USB_OTG_DIEPTSIZ_MULCNT_Pos              (29U)                         
#define USB_OTG_DIEPTSIZ_MULCNT_Msk              (0x3U << USB_OTG_DIEPTSIZ_MULCNT_Pos) /*!< 0x60000000 */
#define USB_OTG_DIEPTSIZ_MULCNT                  USB_OTG_DIEPTSIZ_MULCNT_Msk   /*!< Packet count */
#define USB_OTG_HCTSIZ_XFRSIZ_Pos                (0U)                          
#define USB_OTG_HCTSIZ_XFRSIZ_Msk                (0x7FFFFU << USB_OTG_HCTSIZ_XFRSIZ_Pos) /*!< 0x0007FFFF */
#define USB_OTG_HCTSIZ_XFRSIZ                    USB_OTG_HCTSIZ_XFRSIZ_Msk     /*!< Transfer size */
#define USB_OTG_HCTSIZ_PKTCNT_Pos                (19U)                         
#define USB_OTG_HCTSIZ_PKTCNT_Msk                (0x3FFU << USB_OTG_HCTSIZ_PKTCNT_Pos) /*!< 0x1FF80000 */
#define USB_OTG_HCTSIZ_PKTCNT                    USB_OTG_HCTSIZ_PKTCNT_Msk     /*!< Packet count */
#define USB_OTG_HCTSIZ_DOPING_Pos                (31U)                         
#define USB_OTG_HCTSIZ_DOPING_Msk                (0x1U << USB_OTG_HCTSIZ_DOPING_Pos) /*!< 0x80000000 */
#define USB_OTG_HCTSIZ_DOPING                    USB_OTG_HCTSIZ_DOPING_Msk     /*!< Do PING */
#define USB_OTG_HCTSIZ_DPID_Pos                  (29U)                         
#define USB_OTG_HCTSIZ_DPID_Msk                  (0x3U << USB_OTG_HCTSIZ_DPID_Pos) /*!< 0x60000000 */
#define USB_OTG_HCTSIZ_DPID                      USB_OTG_HCTSIZ_DPID_Msk       /*!< Data PID */
#define USB_OTG_HCTSIZ_DPID_0                    (0x1U << USB_OTG_HCTSIZ_DPID_Pos) /*!< 0x20000000 */
#define USB_OTG_HCTSIZ_DPID_1                    (0x2U << USB_OTG_HCTSIZ_DPID_Pos) /*!< 0x40000000 */
#define USB_OTG_DIEPDMA_DMAADDR_Pos              (0U)                          
#define USB_OTG_DIEPDMA_DMAADDR_Msk              (0xFFFFFFFFU << USB_OTG_DIEPDMA_DMAADDR_Pos) /*!< 0xFFFFFFFF */
#define USB_OTG_DIEPDMA_DMAADDR                  USB_OTG_DIEPDMA_DMAADDR_Msk   /*!< DMA address */
#define USB_OTG_HCDMA_DMAADDR_Pos                (0U)                          
#define USB_OTG_HCDMA_DMAADDR_Msk                (0xFFFFFFFFU << USB_OTG_HCDMA_DMAADDR_Pos) /*!< 0xFFFFFFFF */
#define USB_OTG_HCDMA_DMAADDR                    USB_OTG_HCDMA_DMAADDR_Msk     /*!< DMA address */
#define USB_OTG_DTXFSTS_INEPTFSAV_Pos            (0U)                          
#define USB_OTG_DTXFSTS_INEPTFSAV_Msk            (0xFFFFU << USB_OTG_DTXFSTS_INEPTFSAV_Pos) /*!< 0x0000FFFF */
#define USB_OTG_DTXFSTS_INEPTFSAV                USB_OTG_DTXFSTS_INEPTFSAV_Msk /*!< IN endpoint TxFIFO space available */
#define USB_OTG_DIEPTXF_INEPTXSA_Pos             (0U)                          
#define USB_OTG_DIEPTXF_INEPTXSA_Msk             (0xFFFFU << USB_OTG_DIEPTXF_INEPTXSA_Pos) /*!< 0x0000FFFF */
#define USB_OTG_DIEPTXF_INEPTXSA                 USB_OTG_DIEPTXF_INEPTXSA_Msk  /*!< IN endpoint FIFOx transmit RAM start address */
#define USB_OTG_DIEPTXF_INEPTXFD_Pos             (16U)                         
#define USB_OTG_DIEPTXF_INEPTXFD_Msk             (0xFFFFU << USB_OTG_DIEPTXF_INEPTXFD_Pos) /*!< 0xFFFF0000 */
#define USB_OTG_DIEPTXF_INEPTXFD                 USB_OTG_DIEPTXF_INEPTXFD_Msk  /*!< IN endpoint TxFIFO depth */
#define USB_OTG_DOEPCTL_MPSIZ_Pos                (0U)                          
#define USB_OTG_DOEPCTL_MPSIZ_Msk                (0x7FFU << USB_OTG_DOEPCTL_MPSIZ_Pos) /*!< 0x000007FF */
#define USB_OTG_DOEPCTL_MPSIZ                    USB_OTG_DOEPCTL_MPSIZ_Msk     /*!< Maximum packet size */          /*!<Bit 1 */
#define USB_OTG_DOEPCTL_USBAEP_Pos               (15U)                         
#define USB_OTG_DOEPCTL_USBAEP_Msk               (0x1U << USB_OTG_DOEPCTL_USBAEP_Pos) /*!< 0x00008000 */
#define USB_OTG_DOEPCTL_USBAEP                   USB_OTG_DOEPCTL_USBAEP_Msk    /*!< USB active endpoint */
#define USB_OTG_DOEPCTL_NAKSTS_Pos               (17U)                         
#define USB_OTG_DOEPCTL_NAKSTS_Msk               (0x1U << USB_OTG_DOEPCTL_NAKSTS_Pos) /*!< 0x00020000 */
#define USB_OTG_DOEPCTL_NAKSTS                   USB_OTG_DOEPCTL_NAKSTS_Msk    /*!< NAK status */
#define USB_OTG_DOEPCTL_SD0PID_SEVNFRM_Pos       (28U)                         
#define USB_OTG_DOEPCTL_SD0PID_SEVNFRM_Msk       (0x1U << USB_OTG_DOEPCTL_SD0PID_SEVNFRM_Pos) /*!< 0x10000000 */
#define USB_OTG_DOEPCTL_SD0PID_SEVNFRM           USB_OTG_DOEPCTL_SD0PID_SEVNFRM_Msk /*!< Set DATA0 PID */
#define USB_OTG_DOEPCTL_SODDFRM_Pos              (29U)                         
#define USB_OTG_DOEPCTL_SODDFRM_Msk              (0x1U << USB_OTG_DOEPCTL_SODDFRM_Pos) /*!< 0x20000000 */
#define USB_OTG_DOEPCTL_SODDFRM                  USB_OTG_DOEPCTL_SODDFRM_Msk   /*!< Set odd frame */
#define USB_OTG_DOEPCTL_EPTYP_Pos                (18U)                         
#define USB_OTG_DOEPCTL_EPTYP_Msk                (0x3U << USB_OTG_DOEPCTL_EPTYP_Pos) /*!< 0x000C0000 */
#define USB_OTG_DOEPCTL_EPTYP                    USB_OTG_DOEPCTL_EPTYP_Msk     /*!< Endpoint type */
#define USB_OTG_DOEPCTL_EPTYP_0                  (0x1U << USB_OTG_DOEPCTL_EPTYP_Pos) /*!< 0x00040000 */
#define USB_OTG_DOEPCTL_EPTYP_1                  (0x2U << USB_OTG_DOEPCTL_EPTYP_Pos) /*!< 0x00080000 */
#define USB_OTG_DOEPCTL_SNPM_Pos                 (20U)                         
#define USB_OTG_DOEPCTL_SNPM_Msk                 (0x1U << USB_OTG_DOEPCTL_SNPM_Pos) /*!< 0x00100000 */
#define USB_OTG_DOEPCTL_SNPM                     USB_OTG_DOEPCTL_SNPM_Msk      /*!< Snoop mode */
#define USB_OTG_DOEPCTL_STALL_Pos                (21U)                         
#define USB_OTG_DOEPCTL_STALL_Msk                (0x1U << USB_OTG_DOEPCTL_STALL_Pos) /*!< 0x00200000 */
#define USB_OTG_DOEPCTL_STALL                    USB_OTG_DOEPCTL_STALL_Msk     /*!< STALL handshake */
#define USB_OTG_DOEPCTL_CNAK_Pos                 (26U)                         
#define USB_OTG_DOEPCTL_CNAK_Msk                 (0x1U << USB_OTG_DOEPCTL_CNAK_Pos) /*!< 0x04000000 */
#define USB_OTG_DOEPCTL_CNAK                     USB_OTG_DOEPCTL_CNAK_Msk      /*!< Clear NAK */
#define USB_OTG_DOEPCTL_SNAK_Pos                 (27U)                         
#define USB_OTG_DOEPCTL_SNAK_Msk                 (0x1U << USB_OTG_DOEPCTL_SNAK_Pos) /*!< 0x08000000 */
#define USB_OTG_DOEPCTL_SNAK                     USB_OTG_DOEPCTL_SNAK_Msk      /*!< Set NAK */
#define USB_OTG_DOEPCTL_EPDIS_Pos                (30U)                         
#define USB_OTG_DOEPCTL_EPDIS_Msk                (0x1U << USB_OTG_DOEPCTL_EPDIS_Pos) /*!< 0x40000000 */
#define USB_OTG_DOEPCTL_EPDIS                    USB_OTG_DOEPCTL_EPDIS_Msk     /*!< Endpoint disable */
#define USB_OTG_DOEPCTL_EPENA_Pos                (31U)                         
#define USB_OTG_DOEPCTL_EPENA_Msk                (0x1U << USB_OTG_DOEPCTL_EPENA_Pos) /*!< 0x80000000 */
#define USB_OTG_DOEPCTL_EPENA                    USB_OTG_DOEPCTL_EPENA_Msk     /*!< Endpoint enable */
#define USB_OTG_DOEPINT_XFRC_Pos                 (0U)                          
#define USB_OTG_DOEPINT_XFRC_Msk                 (0x1U << USB_OTG_DOEPINT_XFRC_Pos) /*!< 0x00000001 */
#define USB_OTG_DOEPINT_XFRC                     USB_OTG_DOEPINT_XFRC_Msk      /*!< Transfer completed interrupt */
#define USB_OTG_DOEPINT_EPDISD_Pos               (1U)                          
#define USB_OTG_DOEPINT_EPDISD_Msk               (0x1U << USB_OTG_DOEPINT_EPDISD_Pos) /*!< 0x00000002 */
#define USB_OTG_DOEPINT_EPDISD                   USB_OTG_DOEPINT_EPDISD_Msk    /*!< Endpoint disabled interrupt */
#define USB_OTG_DOEPINT_STUP_Pos                 (3U)                          
#define USB_OTG_DOEPINT_STUP_Msk                 (0x1U << USB_OTG_DOEPINT_STUP_Pos) /*!< 0x00000008 */
#define USB_OTG_DOEPINT_STUP                     USB_OTG_DOEPINT_STUP_Msk      /*!< SETUP phase done */
#define USB_OTG_DOEPINT_OTEPDIS_Pos              (4U)                          
#define USB_OTG_DOEPINT_OTEPDIS_Msk              (0x1U << USB_OTG_DOEPINT_OTEPDIS_Pos) /*!< 0x00000010 */
#define USB_OTG_DOEPINT_OTEPDIS                  USB_OTG_DOEPINT_OTEPDIS_Msk   /*!< OUT token received when endpoint disabled */
#define USB_OTG_DOEPINT_OTEPSPR_Pos              (5U)                          
#define USB_OTG_DOEPINT_OTEPSPR_Msk              (0x1U << USB_OTG_DOEPINT_OTEPSPR_Pos) /*!< 0x00000020 */
#define USB_OTG_DOEPINT_OTEPSPR                  USB_OTG_DOEPINT_OTEPSPR_Msk   /*!< OUT Status Phase Received interrupt */
#define USB_OTG_DOEPINT_B2BSTUP_Pos              (6U)                          
#define USB_OTG_DOEPINT_B2BSTUP_Msk              (0x1U << USB_OTG_DOEPINT_B2BSTUP_Pos) /*!< 0x00000040 */
#define USB_OTG_DOEPINT_B2BSTUP                  USB_OTG_DOEPINT_B2BSTUP_Msk   /*!< Back-to-back SETUP packets received */
#define USB_OTG_DOEPINT_NYET_Pos                 (14U)                         
#define USB_OTG_DOEPINT_NYET_Msk                 (0x1U << USB_OTG_DOEPINT_NYET_Pos) /*!< 0x00004000 */
#define USB_OTG_DOEPINT_NYET                     USB_OTG_DOEPINT_NYET_Msk      /*!< NYET interrupt */
#define USB_OTG_DOEPTSIZ_XFRSIZ_Pos              (0U)                          
#define USB_OTG_DOEPTSIZ_XFRSIZ_Msk              (0x7FFFFU << USB_OTG_DOEPTSIZ_XFRSIZ_Pos) /*!< 0x0007FFFF */
#define USB_OTG_DOEPTSIZ_XFRSIZ                  USB_OTG_DOEPTSIZ_XFRSIZ_Msk   /*!< Transfer size */
#define USB_OTG_DOEPTSIZ_PKTCNT_Pos              (19U)                         
#define USB_OTG_DOEPTSIZ_PKTCNT_Msk              (0x3FFU << USB_OTG_DOEPTSIZ_PKTCNT_Pos) /*!< 0x1FF80000 */
#define USB_OTG_DOEPTSIZ_PKTCNT                  USB_OTG_DOEPTSIZ_PKTCNT_Msk   /*!< Packet count */
#define USB_OTG_DOEPTSIZ_STUPCNT_Pos             (29U)                         
#define USB_OTG_DOEPTSIZ_STUPCNT_Msk             (0x3U << USB_OTG_DOEPTSIZ_STUPCNT_Pos) /*!< 0x60000000 */
#define USB_OTG_DOEPTSIZ_STUPCNT                 USB_OTG_DOEPTSIZ_STUPCNT_Msk  /*!< SETUP packet count */
#define USB_OTG_DOEPTSIZ_STUPCNT_0               (0x1U << USB_OTG_DOEPTSIZ_STUPCNT_Pos) /*!< 0x20000000 */
#define USB_OTG_DOEPTSIZ_STUPCNT_1               (0x2U << USB_OTG_DOEPTSIZ_STUPCNT_Pos) /*!< 0x40000000 */
#define USB_OTG_PCGCCTL_STOPCLK_Pos              (0U)                          
#define USB_OTG_PCGCCTL_STOPCLK_Msk              (0x1U << USB_OTG_PCGCCTL_STOPCLK_Pos) /*!< 0x00000001 */
#define USB_OTG_PCGCCTL_STOPCLK                  USB_OTG_PCGCCTL_STOPCLK_Msk   /*!< SETUP packet count */
#define USB_OTG_PCGCCTL_GATECLK_Pos              (1U)                          
#define USB_OTG_PCGCCTL_GATECLK_Msk              (0x1U << USB_OTG_PCGCCTL_GATECLK_Pos) /*!< 0x00000002 */
#define USB_OTG_PCGCCTL_GATECLK                  USB_OTG_PCGCCTL_GATECLK_Msk   /*!<Bit 0 */
#define USB_OTG_PCGCCTL_PHYSUSP_Pos              (4U)                          
#define USB_OTG_PCGCCTL_PHYSUSP_Msk              (0x1U << USB_OTG_PCGCCTL_PHYSUSP_Pos) /*!< 0x00000010 */
#define USB_OTG_PCGCCTL_PHYSUSP                  USB_OTG_PCGCCTL_PHYSUSP_Msk   /*!<Bit 1 */

// ---- OTG low-layer types shared by PCD (device) and HCD (host) ----
// Implemented in lib/cpp/Device/USB/USB-Device.cpp (_MCU_STM32H7x section).
namespace uni {

// AKA USB_OTG_EPTypeDef (endpoint state used by the device controller)
struct OTGEP {
	byte num;
	bool is_in;
	byte type;
	uint16 maxpacket;
	stduint xfer_len;
	stduint xfer_count;
	byte* xfer_buff;
	stduint dma_addr;
	byte tx_fifo_num;
	byte data_pid_start;
	bool is_stall;
};

// AKA USB_OTG_HCTypeDef (host channel state)
struct OTGHC {
	byte ch_num;
	byte ep_num;
	byte ep_is_in;
	byte speed;
	byte ep_type;
	uint16 max_packet;
	stduint xfer_len;
	stduint xfer_count;
	byte* xfer_buff;
	byte data_pid;
	byte do_ping;
	byte process_ping;
	byte dev_addr;
	byte toggle_in;
	byte toggle_out;
	byte urb_state;
	byte state;
	stduint ErrCnt;
};

// OTG core operations shared by PCD (device) and HCD (host).
class OTG : public RuptTrait {
public:
	static bool ResetCore(stduint base);
	static bool Initialize(stduint base, bool ulpi, bool dma_enable, bool use_external_vbus);
	static void setMode(stduint base, byte mode);
	static bool FlushTxFifo(stduint base, stduint num);
	static bool FlushRxFifo(stduint base);
	static byte getMode(stduint base);
	static stduint ReadInterrupts(stduint base);
	static void ClearInterrupts(stduint base, stduint interrupt);
	static bool WritePacket(stduint base, const byte* src, byte ep_num, stduint len);
	static bool ReadPacket(stduint base, byte* dest, stduint len);
	static bool StopDevice(stduint base);
	static bool StopHost(stduint base);

	// device mode
	static bool InitializeDevice(stduint base, bool vbus_sensing_enable, byte speed, byte dev_endpoints, bool dma_enable, bool sof_enable);
	static void setDevSpeed(stduint base, byte speed);
	static byte getDevSpeed(stduint base);
	static void setDevAddress(stduint base, byte address);
	static void DevConnect(stduint base);
	static void DevDisconnect(stduint base);
	static bool ActivateEndpoint(stduint base, OTGEP& ep);
	static bool DeactivateEndpoint(stduint base, OTGEP& ep);
	// AKA USB_ActivateDedicatedEndpoint / USB_DeactivateDedicatedEndpoint
	// (used when the dedicated EP1 interrupt path is enabled)
	static bool ActivateDedicatedEndpoint(stduint base, OTGEP& ep);
	static bool DeactivateDedicatedEndpoint(stduint base, OTGEP& ep);
	static bool StartEndpointXfer(stduint base, OTGEP& ep, bool dma);
	static bool StartEP0Xfer(stduint base, OTGEP& ep, bool dma);
	static bool StartEP0Out(stduint base, bool dma, byte* psetup);
	static bool ConfigStall(stduint base, OTGEP& ep, bool set_or_reset);
	static stduint ReadDevAllOutEpInterrupt(stduint base);
	static stduint ReadDevAllInEpInterrupt(stduint base);
	static stduint ReadDevOutEPInterrupt(stduint base, byte epnum);
	static stduint ReadDevInEPInterrupt(stduint base, byte epnum);

	// host mode
	static bool InitializeHost(stduint base, byte speed, byte host_channels, bool dma_enable);
	static void InitFSLSPClkSel(stduint base, byte freq);
	static bool ResetPort(stduint base);
	static bool InitializeHostChannel(stduint base, byte ch_num, byte epnum, byte dev_address, byte speed, byte ep_type, uint16 mps);
	static bool StartHostChannelXfer(stduint base, OTGHC& hc, bool dma);
	static bool HaltHostChannel(stduint base, byte hc_num);
	static bool DoPing(stduint base, byte ch_num);
	static stduint ReadHostChannelInterrupt(stduint base);

	// RuptTrait: global interrupt (AKA USB_EnableGlobalInt / USB_DisableGlobalInt)
	void enInterrupt(bool enable = true) const override { enGlobalInt(enable); }
	static void enGlobalInt(bool enable) {
		Reference gahbcfg(g_base + 0x008);
		if (enable) gahbcfg.setof(USB_OTG_GAHBCFG_GINT_Pos);
		else gahbcfg.rstof(USB_OTG_GAHBCFG_GINT_Pos);
	}
	void setInterrupt(Handler_t) const override {}
	void setInterruptPriority(byte, byte) const override {}

	static stduint g_base;
};

} // namespace uni

#endif // _MCU_STM32H7x

#endif // _INC_DEVICE_USB_OTG

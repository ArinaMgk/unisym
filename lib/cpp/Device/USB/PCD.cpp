// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Device.USB) Peripheral Controller Driver, PCD
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

#include "../../../../inc/cpp/Device/USB/PCD.hpp"
#include "../../../../inc/cpp/Device/SysTick"
#include "../../../../inc/cpp/Device/RCC/RCC"
// Note: FUNC_OTG_HS/FUNC_OTG_FS are declared extern in PCD.hpp; the single
// definition lives in interrupt_usb.hpp, which the user includes once.

namespace uni {
#if defined(_MCU_STM32H7x)

	// ---- constants (AKA stm32h7xx_ll_usb.h) ----
	#define _USB_OTG_CORE_ID_310A   0x4F54310A
	#define _USBD_HS_TRDT_VALUE     9U

	PCD PCD1;// OTG1_HS @ 0x40040000
	PCD PCD2;// OTG2_FS @ 0x40080000

	// RCC AHB1ENR.OTG1HSEN / OTG2FSEN (AKA HAL_PCD_MspInit clock part)
	void PCD::enClock(bool ena) {
		Reference(_RCC_AHB1ENR_ADDR).setof((base == _OTG1_HS_ADDR)
			? _RCC_AHB1ENR_POSI_ENCLK_OTG1HS
			: _RCC_AHB1ENR_POSI_ENCLK_OTG2FS, ena);
	}

	// AKA HAL_PCD_Init
	bool PCD::setMode() {
		if (base == 0) return false;
		State = PCDState::Busy;
		// MspInit: enable the OTG clock (GPIO/NVIC are handled by the caller)
		enClock(true);
		// Disable the global interrupts
		OTG::enGlobalInt(false);
		// Init the core (common init.)
		if (!OTG::Initialize(base, phy_itface == 1, dma_enable, use_external_vbus)) return false;
		// Force device mode
		OTG::setMode(base, 1);
		// Init endpoint structures
		for (stduint i = 0; i < 15; i++) {
			IN_ep[i].is_in = 1;
			IN_ep[i].num = (byte)i;
			IN_ep[i].tx_fifo_num = (byte)i;
			IN_ep[i].type = 0;// EP_TYPE_CTRL until activated
			IN_ep[i].maxpacket = 0;
			IN_ep[i].xfer_buff = 0;
			IN_ep[i].xfer_len = 0;
			IN_ep[i].xfer_count = 0;
			IN_ep[i].is_stall = false;
		}
		for (stduint i = 0; i < 15; i++) {
			OUT_ep[i].is_in = 0;
			OUT_ep[i].num = (byte)i;
			OUT_ep[i].type = 0;// EP_TYPE_CTRL until activated
			OUT_ep[i].maxpacket = 0;
			OUT_ep[i].xfer_buff = 0;
			OUT_ep[i].xfer_len = 0;
			OUT_ep[i].xfer_count = 0;
			OUT_ep[i].is_stall = false;
			Reference(base + _IMM(OTGGlobalReg::DIEPTXF0) + i * 4) = 0;// DIEPTXF[i]
		}
		// Init device
		if (!OTG::InitializeDevice(base, vbus_sensing_enable, speed, dev_endpoints, dma_enable, sof_enable)) return false;
		State = PCDState::Ready;
		// Activate LPM / BCD
		if (lpm_enable) ActivateLPM();
		if (battery_charging_enable) ActivateBCD();
		// USB_DevDisconnect at the end of init
		OTG::DevDisconnect(base);
		return true;
	}

	// AKA HAL_PCD_DeInit
	bool PCD::canMode() {
		State = PCDState::Busy;
		Stop();
		enClock(false);
		State = PCDState::Reset;
		return true;
	}

	// AKA HAL_PCD_Start
	bool PCD::Start() {
		OTG::DevConnect(base);
		OTG::enGlobalInt(true);
		return true;
	}

	// AKA HAL_PCD_Stop
	bool PCD::Stop() {
		OTG::enGlobalInt(false);
		OTG::StopDevice(base);
		OTG::DevDisconnect(base);
		return true;
	}

	// AKA HAL_PCD_DevConnect / HAL_PCD_DevDisconnect
	bool PCD::Connect() {
		OTG::DevConnect(base);
		return true;
	}
	bool PCD::Disconnect() {
		OTG::DevDisconnect(base);
		return true;
	}

	// AKA HAL_PCD_SetAddress
	bool PCD::setAddress(byte address) {
		OTG::setDevAddress(base, address);
		return true;
	}

	// AKA HAL_PCD_EP_Open
	bool PCD::OpenEndpoint(byte ep_addr, uint16 ep_mps, byte ep_type) {
		OTGEP* ep = (ep_addr & 0x80) ? &IN_ep[ep_addr & 0x7F] : &OUT_ep[ep_addr & 0x7F];
		ep->num = ep_addr & 0x7F;
		ep->is_in = (0x80 & ep_addr) != 0;
		ep->maxpacket = ep_mps;
		ep->type = ep_type;
		if (ep->is_in) ep->tx_fifo_num = ep->num;// assign a Tx FIFO
		if (ep_type == 2) ep->data_pid_start = 0;// EP_TYPE_BULK
		OTG::ActivateEndpoint(base, *ep);
		return true;
	}

	// AKA HAL_PCD_EP_Close
	bool PCD::CloseEndpoint(byte ep_addr) {
		OTGEP* ep = (ep_addr & 0x80) ? &IN_ep[ep_addr & 0x7F] : &OUT_ep[ep_addr & 0x7F];
		ep->num = ep_addr & 0x7F;
		ep->is_in = (0x80 & ep_addr) != 0;
		OTG::DeactivateEndpoint(base, *ep);
		return true;
	}

	// AKA HAL_PCD_EP_Receive
	bool PCD::ReceiveEndpoint(byte ep_addr, byte* pBuf, stduint len) {
		OTGEP* ep = &OUT_ep[ep_addr & 0x7F];
		ep->xfer_buff = pBuf;
		ep->xfer_len = len;
		ep->xfer_count = 0;
		ep->is_in = 0;
		ep->num = ep_addr & 0x7F;
		if (dma_enable) ep->dma_addr = (stduint)pBuf;
		if ((ep_addr & 0x7F) == 0) OTG::StartEP0Xfer(base, *ep, dma_enable);
		else OTG::StartEndpointXfer(base, *ep, dma_enable);
		return true;
	}

	// AKA HAL_PCD_EP_Transmit
	bool PCD::TransmitEndpoint(byte ep_addr, byte* pBuf, stduint len) {
		OTGEP* ep = &IN_ep[ep_addr & 0x7F];
		ep->xfer_buff = pBuf;
		ep->xfer_len = len;
		ep->xfer_count = 0;
		ep->is_in = 1;
		ep->num = ep_addr & 0x7F;
		if (dma_enable) ep->dma_addr = (stduint)pBuf;
		if ((ep_addr & 0x7F) == 0) OTG::StartEP0Xfer(base, *ep, dma_enable);
		else OTG::StartEndpointXfer(base, *ep, dma_enable);
		return true;
	}

	// AKA HAL_PCD_EP_GetRxCount
	uint16 PCD::getRxCount(byte ep_addr) {
		return OUT_ep[ep_addr & 0xF].xfer_count;
	}

	// AKA HAL_PCD_EP_SetStall / HAL_PCD_EP_ClrStall (merged)
	bool PCD::ConfigStall(byte ep_addr, bool set_or_reset) {
		OTGEP* ep = (0x80 & ep_addr) ? &IN_ep[ep_addr & 0x7F] : &OUT_ep[ep_addr & 0x7F];
		ep->is_stall = set_or_reset;
		ep->num = ep_addr & 0x7F;
		ep->is_in = (ep_addr & 0x80) == 0x80;
		OTG::ConfigStall(base, *ep, set_or_reset);
		// SetStall only: after a stall on EP0, re-arm setup reception
		if (set_or_reset && (ep_addr & 0x7F) == 0) {
			OTG::StartEP0Out(base, dma_enable, (byte*)Setup);
		}
		return true;
	}

	// AKA HAL_PCD_EP_Flush
	bool PCD::FlushEndpoint(byte ep_addr) {
		if ((ep_addr & 0x80) == 0x80) OTG::FlushTxFifo(base, ep_addr & 0x7F);
		else OTG::FlushRxFifo(base);
		return true;
	}

	// AKA HAL_PCD_ActivateRemoteWakeup / HAL_PCD_DeActivateRemoteWakeup
	bool PCD::ActivateRemoteWakeup() {
		if (DeviceReg(OTGDeviceReg::DSTS).bitof(USB_OTG_DSTS_SUSPSTS_Pos)) {
			DeviceReg(OTGDeviceReg::DCTL).setof(USB_OTG_DCTL_RWUSIG_Pos);
		}
		return true;
	}
	bool PCD::DeActivateRemoteWakeup() {
		DeviceReg(OTGDeviceReg::DCTL).rstof(USB_OTG_DCTL_RWUSIG_Pos);
		return true;
	}

	// AKA HAL_PCDEx_SetTxFiFo / HAL_PCDEx_SetRxFiFo
	bool PCD::setTxFiFo(byte fifo, uint16 size) {
		stduint Tx_Offset = GlobalReg(OTGGlobalReg::GRXFSIZ);
		if (fifo == 0) {
			GlobalReg(OTGGlobalReg::DIEPTXF0_HNPTXFSIZ) = ((stduint)size << 16) | Tx_Offset;
		}
		else {
			Tx_Offset += GlobalReg(OTGGlobalReg::DIEPTXF0_HNPTXFSIZ) >> 16;
			for (byte i = 0; i < (fifo - 1); i++) {
				Tx_Offset += Reference(base + _IMM(OTGGlobalReg::DIEPTXF0) + i * 4) >> 16;// DIEPTXF[i]
			}
			Reference(base + _IMM(OTGGlobalReg::DIEPTXF0) + (fifo - 1) * 4) = ((stduint)size << 16) | Tx_Offset;
		}
		return true;
	}
	bool PCD::setRxFiFo(uint16 size) {
		GlobalReg(OTGGlobalReg::GRXFSIZ) = size;
		return true;
	}

	// AKA HAL_PCDEx_ActivateLPM / HAL_PCDEx_DeActivateLPM
	bool PCD::ActivateLPM() {
		lpm_active = true;
		LPM_State = PCDLPMState::L0;
		GlobalReg(OTGGlobalReg::GINTMSK).setof(USB_OTG_GINTMSK_LPMINTM_Pos);
		Reference glpmcfg(base + _IMM(OTGGlobalReg::GLPMCFG));
		glpmcfg.setof(USB_OTG_GLPMCFG_LPMEN_Pos);
		glpmcfg.setof(USB_OTG_GLPMCFG_LPMACK_Pos);
		glpmcfg.setof(USB_OTG_GLPMCFG_ENBESL_Pos);
		return true;
	}
	bool PCD::DeActivateLPM() {
		lpm_active = false;
		GlobalReg(OTGGlobalReg::GINTMSK).rstof(USB_OTG_GINTMSK_LPMINTM_Pos);
		Reference glpmcfg(base + _IMM(OTGGlobalReg::GLPMCFG));
		glpmcfg.rstof(USB_OTG_GLPMCFG_LPMEN_Pos);
		glpmcfg.rstof(USB_OTG_GLPMCFG_LPMACK_Pos);
		glpmcfg.rstof(USB_OTG_GLPMCFG_ENBESL_Pos);
		return true;
	}

	// AKA HAL_PCDEx_ActivateBCD / HAL_PCDEx_DeActivateBCD
	bool PCD::ActivateBCD() {
		battery_charging_active = true;
		GlobalReg(OTGGlobalReg::GCCFG).setof(USB_OTG_GCCFG_BCDEN_Pos);
		GlobalReg(OTGGlobalReg::GCCFG).setof(USB_OTG_GCCFG_DCDEN_Pos);// start DCD
		return true;
	}
	bool PCD::DeActivateBCD() {
		battery_charging_active = false;
		GlobalReg(OTGGlobalReg::GCCFG).rstof(USB_OTG_GCCFG_BCDEN_Pos);
		return true;
	}

	// AKA HAL_PCDEx_BCD_VBUSDetect (blocking; runs the BCD discovery sequence)
	void PCD::DetectVBUS() {
		Reference gccfg(base + _IMM(OTGGlobalReg::GCCFG));
		// only while soft-disconnected
		if (DeviceReg(OTGDeviceReg::DCTL).bitof(USB_OTG_DCTL_SDIS_Pos)) {
			uint64 tickstart = SysTick::getTick();
			gccfg.setof(USB_OTG_GCCFG_DCDEN_Pos);// enable DCD
			while (!gccfg.bitof(USB_OTG_GCCFG_DCDET_Pos)) {// wait DCD detect
				if ((SysTick::getTick() - tickstart) > 1000) {
					if (BCDHandler) BCDHandler((pureptr_t)(stduint)PCDBCDMsg::Error);
					return;
				}
			}
			SysDelay_ms(100);
			if (gccfg.bitof(USB_OTG_GCCFG_DCDET_Pos)) {
				if (BCDHandler) BCDHandler((pureptr_t)(stduint)PCDBCDMsg::ContactDetection);
			}
			// primary detection: standard downstream port?
			gccfg.rstof(USB_OTG_GCCFG_DCDEN_Pos);
			gccfg.setof(USB_OTG_GCCFG_PDEN_Pos);
			SysDelay_ms(100);
			if (!gccfg.bitof(USB_OTG_GCCFG_PDET_Pos)) {
				if (BCDHandler) BCDHandler((pureptr_t)(stduint)PCDBCDMsg::StdDownstreamPort);
			}
			else {
				// secondary detection: charging downstream vs dedicated charging port
				gccfg.rstof(USB_OTG_GCCFG_PDEN_Pos);
				gccfg.setof(USB_OTG_GCCFG_SDEN_Pos);
				SysDelay_ms(100);
				if (gccfg.bitof(USB_OTG_GCCFG_SDET_Pos)) {
					if (BCDHandler) BCDHandler((pureptr_t)(stduint)PCDBCDMsg::DedicatedChargingPort);
				}
				else {
					if (BCDHandler) BCDHandler((pureptr_t)(stduint)PCDBCDMsg::ChargingDownstreamPort);
				}
			}
			if (BCDHandler) BCDHandler((pureptr_t)(stduint)PCDBCDMsg::DiscoveryCompleted);
		}
	}

	// AKA PCD_WriteEmptyTxFifo: push the next packet into the TxFIFO when it has room
	static bool PCD_WriteEmptyTxFifo(PCD& pcd, byte epnum) {
		OTGEP* ep = &pcd.IN_ep[epnum];
		stdsint len = (stdsint)(ep->xfer_len - ep->xfer_count);
		if (len > ep->maxpacket) len = ep->maxpacket;
		stduint len32b = (stduint)(len + 3) / 4;
		Reference dtxfsts = pcd.InEndpointReg(epnum, OTGInEPReg::DTXFSTS);
		while (dtxfsts.masof(USB_OTG_DTXFSTS_INEPTFSAV_Pos, 16) > len32b
			&& ep->xfer_count < ep->xfer_len && ep->xfer_len != 0) {
			len = (stdsint)(ep->xfer_len - ep->xfer_count);
			if (len > ep->maxpacket) len = ep->maxpacket;
			len32b = (stduint)(len + 3) / 4;
			OTG::WritePacket(pcd.base, ep->xfer_buff, epnum, (stduint)len);
			ep->xfer_buff += len;
			ep->xfer_count += (stduint)len;
		}
		if (len <= 0) {
			pcd.DeviceReg(OTGDeviceReg::DIEPEMPMSK).rstof(epnum);// clear TXFE mask
		}
		return true;
	}

	// AKA HAL_PCD_IRQHandler
	void PCD::HandleIRQ() {
		stduint i = 0, ep_intr = 0, epint = 0, epnum = 0;
		stduint temp = 0;
		// ensure that we are in device mode
		if (OTG::getMode(base) != 0) return;
		// avoid spurious interrupt
		if (OTG::ReadInterrupts(base) == 0) return;

		Reference gintsts(base + _IMM(OTGGlobalReg::GINTSTS));
		Reference gintmsk(base + _IMM(OTGGlobalReg::GINTMSK));

		if (gintsts.bitof(USB_OTG_GINTSTS_MMIS_Pos)) {
			gintsts.setof(USB_OTG_GINTSTS_MMIS_Pos);// incorrect mode, acknowledge
		}
		// ---- OUT endpoints ----
		if (gintsts.bitof(USB_OTG_GINTSTS_OEPINT_Pos)) {
			epnum = 0;
			ep_intr = OTG::ReadDevAllOutEpInterrupt(base);
			while (ep_intr) {
				if (ep_intr & 0x1) {
					epint = OTG::ReadDevOutEPInterrupt(base, (byte)epnum);
					if (epint & USB_OTG_DOEPINT_XFRC) {
						OutEndpointReg((byte)epnum, OTGOutEPReg::DOEPINT).setof(USB_OTG_DOEPINT_XFRC_Pos);
						// setup/out transaction management for Core ID >= 310A
						if (stduint(GlobalReg(OTGGlobalReg::GSNPSID)) >= _USB_OTG_CORE_ID_310A) {
							if (dma_enable) {
								if (OutEndpointReg(0, OTGOutEPReg::DOEPINT).bitof(15)) {
									OutEndpointReg(0, OTGOutEPReg::DOEPINT).setof(15);
								}
							}
						}
						if (dma_enable) {
							OUT_ep[epnum].xfer_count = OUT_ep[epnum].maxpacket
								- OutEndpointReg((byte)epnum, OTGOutEPReg::DOEPTSIZ).masof(USB_OTG_DOEPTSIZ_XFRSIZ_Pos, 19);
							OUT_ep[epnum].xfer_buff += OUT_ep[epnum].maxpacket;
						}
						if (DataOutStageHandler) DataOutStageHandler((pureptr_t)(stduint)epnum);
						if (dma_enable) {
							if ((epnum == 0) && (OUT_ep[0].xfer_len == 0)) {
								OTG::StartEP0Out(base, true, (byte*)Setup);// ZLP, prepare EP0 for next setup
							}
						}
					}
					if (epint & USB_OTG_DOEPINT_STUP) {
						if (stduint(GlobalReg(OTGGlobalReg::GSNPSID)) >= _USB_OTG_CORE_ID_310A) {
							if (dma_enable) {
								if (OutEndpointReg(0, OTGOutEPReg::DOEPINT).bitof(15)) {
									OutEndpointReg(0, OTGOutEPReg::DOEPINT).setof(15);
								}
							}
						}
						if (SetupStageHandler) SetupStageHandler();
						OutEndpointReg((byte)epnum, OTGOutEPReg::DOEPINT).setof(USB_OTG_DOEPINT_STUP_Pos);
					}
					if (epint & USB_OTG_DOEPINT_OTEPDIS) {
						OutEndpointReg((byte)epnum, OTGOutEPReg::DOEPINT).setof(USB_OTG_DOEPINT_OTEPDIS_Pos);
					}
					if (epint & USB_OTG_DOEPINT_OTEPSPR) {
						OutEndpointReg((byte)epnum, OTGOutEPReg::DOEPINT).setof(USB_OTG_DOEPINT_OTEPSPR_Pos);
					}
				}
				epnum++;
				ep_intr >>= 1;
			}
		}
		// ---- IN endpoints ----
		if (gintsts.bitof(USB_OTG_GINTSTS_IEPINT_Pos)) {
			ep_intr = OTG::ReadDevAllInEpInterrupt(base);
			epnum = 0;
			while (ep_intr) {
				if (ep_intr & 0x1) {
					epint = OTG::ReadDevInEPInterrupt(base, (byte)epnum);
					if (epint & USB_OTG_DIEPINT_XFRC) {
						DeviceReg(OTGDeviceReg::DIEPEMPMSK).rstof(epnum);
						InEndpointReg((byte)epnum, OTGInEPReg::DIEPINT).setof(USB_OTG_DIEPINT_XFRC_Pos);
						if (dma_enable) {
							IN_ep[epnum].xfer_buff += IN_ep[epnum].maxpacket;
						}
						if (DataInStageHandler) DataInStageHandler((pureptr_t)(stduint)epnum);
						if (dma_enable) {
							if ((epnum == 0) && (IN_ep[0].xfer_len == 0)) {
								OTG::StartEP0Out(base, true, (byte*)Setup);// ZLP, prepare for next setup
							}
						}
					}
					if (epint & USB_OTG_DIEPINT_TOC) {
						InEndpointReg((byte)epnum, OTGInEPReg::DIEPINT).setof(USB_OTG_DIEPINT_TOC_Pos);
					}
					if (epint & USB_OTG_DIEPINT_ITTXFE) {
						InEndpointReg((byte)epnum, OTGInEPReg::DIEPINT).setof(USB_OTG_DIEPINT_ITTXFE_Pos);
					}
					if (epint & USB_OTG_DIEPINT_INEPNE) {
						InEndpointReg((byte)epnum, OTGInEPReg::DIEPINT).setof(USB_OTG_DIEPINT_INEPNE_Pos);
					}
					if (epint & USB_OTG_DIEPINT_EPDISD) {
						InEndpointReg((byte)epnum, OTGInEPReg::DIEPINT).setof(USB_OTG_DIEPINT_EPDISD_Pos);
					}
					if (epint & USB_OTG_DIEPINT_TXFE) {
						PCD_WriteEmptyTxFifo(*this, (byte)epnum);
					}
				}
				epnum++;
				ep_intr >>= 1;
			}
		}
		// ---- resume / remote wakeup ----
		if (gintsts.bitof(USB_OTG_GINTSTS_WKUINT_Pos)) {
			DeviceReg(OTGDeviceReg::DCTL).rstof(USB_OTG_DCTL_RWUSIG_Pos);
			if (LPM_State == PCDLPMState::L1) {
				LPM_State = PCDLPMState::L0;
				if (LPMHandler) LPMHandler((pureptr_t)(stduint)PCDLPMMsg::L0Active);
			}
			else {
				if (ResumeHandler) ResumeHandler();
			}
			gintsts.setof(USB_OTG_GINTSTS_WKUINT_Pos);
		}
		// ---- suspend ----
		if (gintsts.bitof(USB_OTG_GINTSTS_USBSUSP_Pos)) {
			if (DeviceReg(OTGDeviceReg::DSTS).bitof(USB_OTG_DSTS_SUSPSTS_Pos)) {
				if (SuspendHandler) SuspendHandler();
			}
			gintsts.setof(USB_OTG_GINTSTS_USBSUSP_Pos);
		}
		// ---- LPM ----
		if (gintsts.bitof(USB_OTG_GINTSTS_LPMINT_Pos)) {
			gintsts.setof(USB_OTG_GINTSTS_LPMINT_Pos);
			if (LPM_State == PCDLPMState::L0) {
				LPM_State = PCDLPMState::L1;
				BESL = GlobalReg(OTGGlobalReg::GLPMCFG).masof(USB_OTG_GLPMCFG_BESL_Pos, 4);
				if (LPMHandler) LPMHandler((pureptr_t)(stduint)PCDLPMMsg::L1Active);
			}
			else {
				if (SuspendHandler) SuspendHandler();
			}
		}
		// ---- USB reset ----
		if (gintsts.bitof(USB_OTG_GINTSTS_USBRST_Pos)) {
			DeviceReg(OTGDeviceReg::DCTL).rstof(USB_OTG_DCTL_RWUSIG_Pos);
			OTG::FlushTxFifo(base, 0x10);
			for (i = 0; i < dev_endpoints; i++) {
				InEndpointReg((byte)i, OTGInEPReg::DIEPINT) = 0xFF;
				OutEndpointReg((byte)i, OTGOutEPReg::DOEPINT) = 0xFF;
			}
			DeviceReg(OTGDeviceReg::DAINT) = 0xFFFFFFFF;
			DeviceReg(OTGDeviceReg::DAINTMSK).setof(0);
			DeviceReg(OTGDeviceReg::DAINTMSK).setof(16);
			if (use_dedicated_ep1) {
				DeviceReg(OTGDeviceReg::DOUTEP1MSK).setof(USB_OTG_DOEPMSK_STUPM_Pos);
				DeviceReg(OTGDeviceReg::DOUTEP1MSK).setof(USB_OTG_DOEPMSK_XFRCM_Pos);
				DeviceReg(OTGDeviceReg::DOUTEP1MSK).setof(USB_OTG_DOEPMSK_EPDM_Pos);
				DeviceReg(OTGDeviceReg::DINEP1MSK).setof(USB_OTG_DIEPMSK_TOM_Pos);
				DeviceReg(OTGDeviceReg::DINEP1MSK).setof(USB_OTG_DIEPMSK_XFRCM_Pos);
				DeviceReg(OTGDeviceReg::DINEP1MSK).setof(USB_OTG_DIEPMSK_EPDM_Pos);
			}
			else {
				DeviceReg(OTGDeviceReg::DOEPMSK).setof(USB_OTG_DOEPMSK_STUPM_Pos);
				DeviceReg(OTGDeviceReg::DOEPMSK).setof(USB_OTG_DOEPMSK_XFRCM_Pos);
				DeviceReg(OTGDeviceReg::DOEPMSK).setof(USB_OTG_DOEPMSK_EPDM_Pos);
				DeviceReg(OTGDeviceReg::DIEPMSK).setof(USB_OTG_DIEPMSK_TOM_Pos);
				DeviceReg(OTGDeviceReg::DIEPMSK).setof(USB_OTG_DIEPMSK_XFRCM_Pos);
				DeviceReg(OTGDeviceReg::DIEPMSK).setof(USB_OTG_DIEPMSK_EPDM_Pos);
			}
			// set default address to 0
			DeviceReg(OTGDeviceReg::DCFG).rstof(USB_OTG_DCFG_DAD_Pos);
			// setup EP0 to receive SETUP packets
			OTG::StartEP0Out(base, dma_enable, (byte*)Setup);
			gintsts.setof(USB_OTG_GINTSTS_USBRST_Pos);
		}
		// ---- enumeration done ----
		if (gintsts.bitof(USB_OTG_GINTSTS_ENUMDNE_Pos)) {
			// activate setup: set IN EP0 MPS per enumerated speed, clear global IN NAK
			Reference diepctl0 = InEndpointReg(0, OTGInEPReg::DIEPCTL);
			diepctl0.rstof(USB_OTG_DIEPCTL_MPSIZ_Pos);
			if (DeviceReg(OTGDeviceReg::DSTS).masof(USB_OTG_DSTS_ENUMSPD_Pos, 2) == 2) {
				diepctl0.maset(USB_OTG_DIEPCTL_MPSIZ_Pos, 11, 3);// LS: MPS=3 (8 bytes)
			}
			DeviceReg(OTGDeviceReg::DCTL).setof(USB_OTG_DCTL_CGINAK_Pos);
			// TRDT
			Reference gusbcfg(base + _IMM(OTGGlobalReg::GUSBCFG));
			gusbcfg &= ~USB_OTG_GUSBCFG_TRDT;
			if (OTG::getDevSpeed(base) == 0) {// HIGH
				speed = 0;
				ep0_mps = 512;
				gusbcfg.maset(USB_OTG_GUSBCFG_TRDT_Pos, 4, _USBD_HS_TRDT_VALUE);
			}
			else {// FULL
				speed = 2;
				ep0_mps = 64;
				// USBTRD table by HCLK frequency (HAL_PCD uses HAL_RCC_GetHCLKFreq)
				stduint hclk = uni::RCC.getFrequencyHCLK();
				stduint trdt = 0x6;// default: >= 32 MHz
				if ((hclk >= 14200000) && (hclk < 15000000)) trdt = 0xF;
				else if ((hclk >= 15000000) && (hclk < 16000000)) trdt = 0xE;
				else if ((hclk >= 16000000) && (hclk < 17200000)) trdt = 0xD;
				else if ((hclk >= 17200000) && (hclk < 18500000)) trdt = 0xC;
				else if ((hclk >= 18500000) && (hclk < 20000000)) trdt = 0xB;
				else if ((hclk >= 20000000) && (hclk < 21800000)) trdt = 0xA;
				else if ((hclk >= 21800000) && (hclk < 24000000)) trdt = 0x9;
				else if ((hclk >= 24000000) && (hclk < 27700000)) trdt = 0x8;
				else if ((hclk >= 27700000) && (hclk < 32000000)) trdt = 0x7;
				gusbcfg.maset(USB_OTG_GUSBCFG_TRDT_Pos, 4, trdt);
			}
			if (ResetHandler) ResetHandler();
			gintsts.setof(USB_OTG_GINTSTS_ENUMDNE_Pos);
		}
		// ---- Rx queue level ----
		if (gintsts.bitof(USB_OTG_GINTSTS_RXFLVL_Pos)) {
			gintmsk.rstof(USB_OTG_GINTMSK_RXFLVLM_Pos);// mask
			temp = GlobalReg(OTGGlobalReg::GRXSTSP);
			OTGEP* ep = &OUT_ep[temp & USB_OTG_GRXSTSP_EPNUM];
			if (((temp & USB_OTG_GRXSTSP_PKTSTS) >> 17) == 2) {// STS_DATA_UPDT
				if ((temp & USB_OTG_GRXSTSP_BCNT) != 0) {
					stduint bcnt = (temp & USB_OTG_GRXSTSP_BCNT) >> 4;
					OTG::ReadPacket(base, ep->xfer_buff, bcnt);
					ep->xfer_buff += bcnt;
					ep->xfer_count += bcnt;
				}
			}
			else if (((temp & USB_OTG_GRXSTSP_PKTSTS) >> 17) == 6) {// STS_SETUP_UPDT
				OTG::ReadPacket(base, (byte*)Setup, 8);
				ep->xfer_count += (temp & USB_OTG_GRXSTSP_BCNT) >> 4;
			}
			gintmsk.setof(USB_OTG_GINTMSK_RXFLVLM_Pos);// unmask
		}
		// ---- SOF ----
		if (gintsts.bitof(USB_OTG_GINTSTS_SOF_Pos)) {
			if (SOFHandler) SOFHandler();
			gintsts.setof(USB_OTG_GINTSTS_SOF_Pos);
		}
		// ---- incomplete ISO IN / OUT ----
		if (gintsts.bitof(USB_OTG_GINTSTS_IISOIXFR_Pos)) {
			if (ISOINIncompleteHandler) ISOINIncompleteHandler((pureptr_t)(stduint)epnum);
			gintsts.setof(USB_OTG_GINTSTS_IISOIXFR_Pos);
		}
		if (gintsts.bitof(USB_OTG_GINTSTS_PXFR_INCOMPISOOUT_Pos)) {
			if (ISOOUTIncompleteHandler) ISOOUTIncompleteHandler((pureptr_t)(stduint)epnum);
			gintsts.setof(USB_OTG_GINTSTS_PXFR_INCOMPISOOUT_Pos);
		}
		// ---- connect / disconnect (session) ----
		if (gintsts.bitof(USB_OTG_GINTSTS_SRQINT_Pos)) {
			if (ConnectHandler) ConnectHandler();
			gintsts.setof(USB_OTG_GINTSTS_SRQINT_Pos);
		}
		if (gintsts.bitof(USB_OTG_GINTSTS_OTGINT_Pos)) {
			temp = GlobalReg(OTGGlobalReg::GOTGINT);
			if (temp & USB_OTG_GOTGINT_SEDET) {
				if (DisconnectHandler) DisconnectHandler();
			}
			GlobalReg(OTGGlobalReg::GOTGINT).setof(USB_OTG_GOTGINT_SEDET_Pos);
		}
	}

	// ---- RuptTrait (NVIC + IRQ_OTG_HS / IRQ_OTG_FS) ----
	void PCD::setInterrupt(Handler_t f) const {
		if (base == _OTG1_HS_ADDR) FUNC_OTG_HS[0] = f;
		else FUNC_OTG_FS[0] = f;
	}
	void PCD::setInterruptPriority(byte preempt, byte sub_priority) const {
		if (base == _OTG1_HS_ADDR) NVIC.setPriority(IRQ_OTG_HS, preempt, sub_priority);
		else NVIC.setPriority(IRQ_OTG_FS, preempt, sub_priority);
	}
	void PCD::enInterrupt(bool enable) const {
		if (base == _OTG1_HS_ADDR) NVIC.setAble(IRQ_OTG_HS, enable);
		else NVIC.setAble(IRQ_OTG_FS, enable);
	}

#endif // _MCU_STM32H7x
}

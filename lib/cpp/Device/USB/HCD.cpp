// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Device.USB) Host Controller Driver, HCD
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

#include "../../../../inc/cpp/Device/USB/HCD.hpp"
#include "../../../../inc/cpp/Device/SysTick"
// Note: FUNC_OTG_HS/FUNC_OTG_FS are declared extern in HCD.hpp; the single
// definition lives in interrupt_usb.hpp, which the user includes once.

namespace uni {
#if defined(_MCU_STM32H7x)

	// ---- constants (AKA stm32h7xx_ll_usb.h) ----
	#define _HCFG_48_MHZ     1
	#define _HCFG_6_MHZ      2
	// GRXSTSP.PKTSTS values (>>17)
	#define _GRXSTS_PKTSTS_IN                2U
	#define _GRXSTS_PKTSTS_IN_XFER_COMP      3U
	#define _GRXSTS_PKTSTS_DATA_TOGGLE_ERR   5U
	#define _GRXSTS_PKTSTS_CH_HALTED         7U
	// HPRT0.PLSTS speed values
	#define _HPRT0_PRTSPD_LOW_SPEED  2U

	HCD HCD1;// OTG1_HS @ 0x40040000
	HCD HCD2;// OTG2_FS @ 0x40080000

	// RCC AHB1ENR.OTG1HSEN / OTG2FSEN (AKA HAL_HCD_MspInit clock part)
	void HCD::enClock(bool ena) {
		Reference(_RCC_AHB1ENR_ADDR).setof((base == _OTG1_HS_ADDR)
			? _RCC_AHB1ENR_POSI_ENCLK_OTG1HS
			: _RCC_AHB1ENR_POSI_ENCLK_OTG2FS, ena);
	}

	// AKA HAL_HCD_Init
	bool HCD::setMode() {
		if (base == 0) return false;
		State = HCDState::Busy;
		// MspInit: enable the OTG clock (GPIO/NVIC are handled by the caller)
		enClock(true);
		// Disable the global interrupts
		OTG::enGlobalInt(false);
		// Init the core (common init.)
		if (!OTG::Initialize(base, phy_itface == 1, dma_enable, use_external_vbus)) return false;
		// Force host mode
		OTG::setMode(base, 0);
		// Init host
		if (!OTG::InitializeHost(base, speed, host_channels, dma_enable)) return false;
		State = HCDState::Ready;
		return true;
	}

	// AKA HAL_HCD_DeInit
	bool HCD::canMode() {
		State = HCDState::Busy;
		enClock(false);
		OTG::enGlobalInt(false);
		State = HCDState::Reset;
		return true;
	}

	// AKA HAL_HCD_Start
	bool HCD::Start() {
		OTG::enGlobalInt(true);
		// AKA USB_DriveVbus(state = 1): drive VBUS via HPRT0.PPWR
		Reference hprt_reg(base + USB_OTG_HOST_PORT_BASE);
		stduint hprt0 = hprt_reg;
		hprt0 &= ~(USB_OTG_HPRT_PENA | USB_OTG_HPRT_PCDET | USB_OTG_HPRT_PENCHNG | USB_OTG_HPRT_POCCHNG);
		if ((hprt0 & USB_OTG_HPRT_PPWR) == 0) {
			hprt_reg = USB_OTG_HPRT_PPWR | hprt0;
		}
		return true;
	}

	// AKA HAL_HCD_Stop
	bool HCD::Stop() {
		OTG::StopHost(base);
		return true;
	}

	// AKA HAL_HCD_ResetPort
	bool HCD::ResetPort() {
		return OTG::ResetPort(base);
	}

	// AKA HAL_HCD_HC_Init
	bool HCD::InitializeHostChannel(byte ch_num, byte epnum, byte dev_address, byte speed, byte ep_type, uint16 mps) {
		hc[ch_num].dev_addr = dev_address;
		hc[ch_num].max_packet = mps;
		hc[ch_num].ch_num = ch_num;
		hc[ch_num].ep_type = ep_type;
		hc[ch_num].ep_num = epnum & 0x7F;
		hc[ch_num].ep_is_in = (epnum & 0x80) == 0x80;
		hc[ch_num].speed = speed;
		hc[ch_num].toggle_in = 0;
		hc[ch_num].toggle_out = 0;
		hc[ch_num].ErrCnt = 0;
		hc[ch_num].urb_state = (byte)URBState::Idle;
		hc[ch_num].state = (byte)HostChannelState::Idle;
		return OTG::InitializeHostChannel(base, ch_num, epnum, dev_address, speed, ep_type, mps);
	}

	// AKA HAL_HCD_HC_Halt
	bool HCD::HaltHostChannel(byte ch_num) {
		return OTG::HaltHostChannel(base, ch_num);
	}

	// AKA HAL_HCD_HC_SubmitRequest
	bool HCD::SubmitRequest(byte ch_num, byte direction, byte ep_type, byte token, byte* pbuff, uint16 length, byte do_ping) {
		OTGHC& hc_ref = hc[ch_num];
		hc_ref.ep_is_in = direction;
		hc_ref.ep_type = ep_type;
		// token: 0 = SETUP, 1 = DATA1
		if (token == 0) hc_ref.data_pid = 3;// HC_PID_SETUP
		else hc_ref.data_pid = 2;// HC_PID_DATA1
		// manage data toggle
		switch (ep_type) {
		case 0:// EP_TYPE_CTRL
			if ((token == 1) && (direction == 0)) {
				if (length == 0) hc_ref.toggle_out = 1;// status OUT stage
				if (hc_ref.toggle_out == 0) hc_ref.data_pid = 0;// DATA0
				else hc_ref.data_pid = 2;// DATA1
				if (hc_ref.urb_state != (byte)URBState::NotReady) hc_ref.do_ping = do_ping;
			}
			break;
		case 2:// EP_TYPE_BULK
			if (direction == 0) {
				if (hc_ref.toggle_out == 0) hc_ref.data_pid = 0;
				else hc_ref.data_pid = 2;
				if (hc_ref.urb_state != (byte)URBState::NotReady) hc_ref.do_ping = do_ping;
			}
			else {
				hc_ref.data_pid = (hc_ref.toggle_in == 0) ? 0 : 2;
			}
			break;
		case 3:// EP_TYPE_INTR
			if (direction == 0) {
				hc_ref.data_pid = (hc_ref.toggle_out == 0) ? 0 : 2;
			}
			else {
				hc_ref.data_pid = (hc_ref.toggle_in == 0) ? 0 : 2;
			}
			break;
		case 1:// EP_TYPE_ISOC
			hc_ref.data_pid = 0;// DATA0
			break;
		default:
			break;
		}
		hc_ref.xfer_buff = pbuff;
		hc_ref.xfer_len = length;
		hc_ref.urb_state = (byte)URBState::Idle;
		hc_ref.xfer_count = 0;
		hc_ref.ch_num = ch_num;
		hc_ref.state = (byte)HostChannelState::Idle;
		return OTG::StartHostChannelXfer(base, hc_ref, dma_enable);
	}

	// AKA HAL_HCD_HC_GetURBState / HAL_HCD_HC_GetXferCount / HAL_HCD_HC_GetState
	URBState HCD::getURBState(byte ch_num) {
		return (URBState)hc[ch_num].urb_state;
	}
	stduint HCD::getXferCount(byte ch_num) {
		return hc[ch_num].xfer_count;
	}
	HostChannelState HCD::getHostChannelState(byte ch_num) {
		return (HostChannelState)hc[ch_num].state;
	}

	// AKA HAL_HCD_GetCurrentFrame / HAL_HCD_GetCurrentSpeed
	stduint HCD::getCurrentFrame() {
		return HostReg(OTGHostReg::HFNUM).masof(USB_OTG_HFNUM_FRNUM_Pos, 16);
	}
	stduint HCD::getCurrentSpeed() {
		// AKA USB_GetHostSpeed: HPRT0.PSPD
		return Reference(base + USB_OTG_HOST_PORT_BASE).masof(USB_OTG_HPRT_PSPD_Pos, 2);
	}

	// ---- private IRQ handlers (AKA HCD_HC_IN_IRQHandler etc., file-local) ----

	// AKA HCD_HC_IN_IRQHandler
	static void HCD_HC_IN_IRQHandler(HCD& hcd, byte chnum) {
		Reference hcint = hcd.ChannelReg(chnum, 0x004);// HCINT
		Reference hcintmsk = hcd.ChannelReg(chnum, 0x008);// HCINTMSK
		Reference hcchar = hcd.ChannelReg(chnum, 0x000);// HCCHAR
		stduint tmpreg = 0;

		if (hcint.bitof(USB_OTG_HCINT_AHBERR_Pos)) {
			hcint.setof(USB_OTG_HCINT_AHBERR_Pos);
			hcintmsk.setof(USB_OTG_HCINTMSK_CHHM_Pos);// unmask halt
		}
		else if (hcint.bitof(USB_OTG_HCINT_ACK_Pos)) {
			hcint.setof(USB_OTG_HCINT_ACK_Pos);
		}
		else if (hcint.bitof(USB_OTG_HCINT_STALL_Pos)) {
			hcintmsk.setof(USB_OTG_HCINTMSK_CHHM_Pos);
			hcd.hc[chnum].state = (byte)HostChannelState::Stall;
			hcint.setof(USB_OTG_HCINT_NAK_Pos);
			hcint.setof(USB_OTG_HCINT_STALL_Pos);
			OTG::HaltHostChannel(hcd.base, chnum);
		}
		else if (hcint.bitof(USB_OTG_HCINT_DTERR_Pos)) {
			hcintmsk.setof(USB_OTG_HCINTMSK_CHHM_Pos);
			OTG::HaltHostChannel(hcd.base, chnum);
			hcint.setof(USB_OTG_HCINT_NAK_Pos);
			hcd.hc[chnum].state = (byte)HostChannelState::DataTglErr;
			hcint.setof(USB_OTG_HCINT_DTERR_Pos);
		}

		if (hcint.bitof(USB_OTG_HCINT_FRMOR_Pos)) {
			hcintmsk.setof(USB_OTG_HCINTMSK_CHHM_Pos);
			OTG::HaltHostChannel(hcd.base, chnum);
			hcint.setof(USB_OTG_HCINT_FRMOR_Pos);
		}
		else if (hcint.bitof(USB_OTG_HCINT_XFRC_Pos)) {
			if (hcd.dma_enable) {
				hcd.hc[chnum].xfer_count = hcd.hc[chnum].xfer_len
					- hcd.ChannelReg(chnum, 0x010).masof(USB_OTG_HCTSIZ_XFRSIZ_Pos, 19);// HCTSIZ
			}
			hcd.hc[chnum].state = (byte)HostChannelState::XFRC;
			hcd.hc[chnum].ErrCnt = 0;
			hcint.setof(USB_OTG_HCINT_XFRC_Pos);
			if ((hcd.hc[chnum].ep_type == 0) || (hcd.hc[chnum].ep_type == 2)) {// CTRL|BULK
				hcintmsk.setof(USB_OTG_HCINTMSK_CHHM_Pos);
				OTG::HaltHostChannel(hcd.base, chnum);
				hcint.setof(USB_OTG_HCINT_NAK_Pos);
			}
			else if (hcd.hc[chnum].ep_type == 3) {// INTR
				hcchar.setof(USB_OTG_HCCHAR_ODDFRM_Pos);
				hcd.hc[chnum].urb_state = (byte)URBState::Done;
				if (hcd.NotifyURBChangeHandler) hcd.NotifyURBChangeHandler((pureptr_t)(stduint)chnum, hcd.hc[chnum].urb_state);
			}
			hcd.hc[chnum].toggle_in ^= 1;
		}
		else if (hcint.bitof(USB_OTG_HCINT_CHH_Pos)) {
			hcintmsk.rstof(USB_OTG_HCINTMSK_CHHM_Pos);// mask halt
			if (hcd.hc[chnum].state == (byte)HostChannelState::XFRC) {
				hcd.hc[chnum].urb_state = (byte)URBState::Done;
			}
			else if (hcd.hc[chnum].state == (byte)HostChannelState::Stall) {
				hcd.hc[chnum].urb_state = (byte)URBState::Stall;
			}
			else if ((hcd.hc[chnum].state == (byte)HostChannelState::Xacterr)
				|| (hcd.hc[chnum].state == (byte)HostChannelState::DataTglErr)) {
				if (hcd.hc[chnum].ErrCnt++ > 3) {
					hcd.hc[chnum].ErrCnt = 0;
					hcd.hc[chnum].urb_state = (byte)URBState::Error;
				}
				else {
					hcd.hc[chnum].urb_state = (byte)URBState::NotReady;
				}
				// re-activate the channel
				tmpreg = hcchar;
				tmpreg &= ~USB_OTG_HCCHAR_CHDIS;
				tmpreg |= USB_OTG_HCCHAR_CHENA;
				hcchar = tmpreg;
			}
			hcint.setof(USB_OTG_HCINT_CHH_Pos);
			if (hcd.NotifyURBChangeHandler) hcd.NotifyURBChangeHandler((pureptr_t)(stduint)chnum, hcd.hc[chnum].urb_state);
		}
		else if (hcint.bitof(USB_OTG_HCINT_TXERR_Pos)) {
			hcintmsk.setof(USB_OTG_HCINTMSK_CHHM_Pos);
			hcd.hc[chnum].ErrCnt++;
			hcd.hc[chnum].state = (byte)HostChannelState::Xacterr;
			OTG::HaltHostChannel(hcd.base, chnum);
			hcint.setof(USB_OTG_HCINT_TXERR_Pos);
		}
		else if (hcint.bitof(USB_OTG_HCINT_NAK_Pos)) {
			if (hcd.hc[chnum].ep_type == 3) {// INTR
				hcintmsk.setof(USB_OTG_HCINTMSK_CHHM_Pos);
				OTG::HaltHostChannel(hcd.base, chnum);
			}
			else if ((hcd.hc[chnum].ep_type == 0) || (hcd.hc[chnum].ep_type == 2)) {// CTRL|BULK
				tmpreg = hcchar;
				tmpreg &= ~USB_OTG_HCCHAR_CHDIS;
				tmpreg |= USB_OTG_HCCHAR_CHENA;
				hcchar = tmpreg;
			}
			hcd.hc[chnum].state = (byte)HostChannelState::NAK;
			hcint.setof(USB_OTG_HCINT_NAK_Pos);
		}
	}

	// AKA HCD_HC_OUT_IRQHandler
	static void HCD_HC_OUT_IRQHandler(HCD& hcd, byte chnum) {
		Reference hcint = hcd.ChannelReg(chnum, 0x004);// HCINT
		Reference hcintmsk = hcd.ChannelReg(chnum, 0x008);// HCINTMSK
		Reference hcchar = hcd.ChannelReg(chnum, 0x000);// HCCHAR
		stduint tmpreg = 0;

		if (hcint.bitof(USB_OTG_HCINT_AHBERR_Pos)) {
			hcint.setof(USB_OTG_HCINT_AHBERR_Pos);
			hcintmsk.setof(USB_OTG_HCINTMSK_CHHM_Pos);
		}
		else if (hcint.bitof(USB_OTG_HCINT_ACK_Pos)) {
			hcint.setof(USB_OTG_HCINT_ACK_Pos);
			if (hcd.hc[chnum].do_ping == 1) {
				hcd.hc[chnum].state = (byte)HostChannelState::NYET;
				hcintmsk.setof(USB_OTG_HCINTMSK_CHHM_Pos);
				OTG::HaltHostChannel(hcd.base, chnum);
				hcd.hc[chnum].urb_state = (byte)URBState::NotReady;
			}
		}
		else if (hcint.bitof(USB_OTG_HCINT_NYET_Pos)) {
			hcd.hc[chnum].state = (byte)HostChannelState::NYET;
			hcd.hc[chnum].ErrCnt = 0;
			hcintmsk.setof(USB_OTG_HCINTMSK_CHHM_Pos);
			OTG::HaltHostChannel(hcd.base, chnum);
			hcint.setof(USB_OTG_HCINT_NYET_Pos);
		}
		else if (hcint.bitof(USB_OTG_HCINT_FRMOR_Pos)) {
			hcintmsk.setof(USB_OTG_HCINTMSK_CHHM_Pos);
			OTG::HaltHostChannel(hcd.base, chnum);
			hcint.setof(USB_OTG_HCINT_FRMOR_Pos);
		}
		else if (hcint.bitof(USB_OTG_HCINT_XFRC_Pos)) {
			hcd.hc[chnum].ErrCnt = 0;
			hcintmsk.setof(USB_OTG_HCINTMSK_CHHM_Pos);
			OTG::HaltHostChannel(hcd.base, chnum);
			hcint.setof(USB_OTG_HCINT_XFRC_Pos);
			hcd.hc[chnum].state = (byte)HostChannelState::XFRC;
		}
		else if (hcint.bitof(USB_OTG_HCINT_STALL_Pos)) {
			hcint.setof(USB_OTG_HCINT_STALL_Pos);
			hcintmsk.setof(USB_OTG_HCINTMSK_CHHM_Pos);
			OTG::HaltHostChannel(hcd.base, chnum);
			hcd.hc[chnum].state = (byte)HostChannelState::Stall;
		}
		else if (hcint.bitof(USB_OTG_HCINT_NAK_Pos)) {
			hcd.hc[chnum].ErrCnt = 0;
			hcintmsk.setof(USB_OTG_HCINTMSK_CHHM_Pos);
			OTG::HaltHostChannel(hcd.base, chnum);
			hcd.hc[chnum].state = (byte)HostChannelState::NAK;
			hcint.setof(USB_OTG_HCINT_NAK_Pos);
		}
		else if (hcint.bitof(USB_OTG_HCINT_TXERR_Pos)) {
			hcintmsk.setof(USB_OTG_HCINTMSK_CHHM_Pos);
			OTG::HaltHostChannel(hcd.base, chnum);
			hcd.hc[chnum].state = (byte)HostChannelState::Xacterr;
			hcint.setof(USB_OTG_HCINT_TXERR_Pos);
		}
		else if (hcint.bitof(USB_OTG_HCINT_DTERR_Pos)) {
			hcintmsk.setof(USB_OTG_HCINTMSK_CHHM_Pos);
			OTG::HaltHostChannel(hcd.base, chnum);
			hcint.setof(USB_OTG_HCINT_NAK_Pos);
			hcint.setof(USB_OTG_HCINT_DTERR_Pos);
			hcd.hc[chnum].state = (byte)HostChannelState::DataTglErr;
		}
		else if (hcint.bitof(USB_OTG_HCINT_CHH_Pos)) {
			hcintmsk.rstof(USB_OTG_HCINTMSK_CHHM_Pos);
			if (hcd.hc[chnum].state == (byte)HostChannelState::XFRC) {
				hcd.hc[chnum].urb_state = (byte)URBState::Done;
				if (hcd.hc[chnum].ep_type == 2) hcd.hc[chnum].toggle_out ^= 1;// BULK
			}
			else if (hcd.hc[chnum].state == (byte)HostChannelState::NAK) {
				hcd.hc[chnum].urb_state = (byte)URBState::NotReady;
			}
			else if (hcd.hc[chnum].state == (byte)HostChannelState::NYET) {
				hcd.hc[chnum].urb_state = (byte)URBState::NotReady;
				hcd.hc[chnum].do_ping = 0;
			}
			else if (hcd.hc[chnum].state == (byte)HostChannelState::Stall) {
				hcd.hc[chnum].urb_state = (byte)URBState::Stall;
			}
			else if ((hcd.hc[chnum].state == (byte)HostChannelState::Xacterr)
				|| (hcd.hc[chnum].state == (byte)HostChannelState::DataTglErr)) {
				if (hcd.hc[chnum].ErrCnt++ > 3) {
					hcd.hc[chnum].ErrCnt = 0;
					hcd.hc[chnum].urb_state = (byte)URBState::Error;
				}
				else {
					hcd.hc[chnum].urb_state = (byte)URBState::NotReady;
				}
				tmpreg = hcchar;
				tmpreg &= ~USB_OTG_HCCHAR_CHDIS;
				tmpreg |= USB_OTG_HCCHAR_CHENA;
				hcchar = tmpreg;
			}
			hcint.setof(USB_OTG_HCINT_CHH_Pos);
			if (hcd.NotifyURBChangeHandler) hcd.NotifyURBChangeHandler((pureptr_t)(stduint)chnum, hcd.hc[chnum].urb_state);
		}
	}

	// AKA HCD_RXQLVL_IRQHandler
	static void HCD_RXQLVL_IRQHandler(HCD& hcd) {
		Reference grxstsp(hcd.base + _IMM(OTGGlobalReg::GRXSTSP));
		stduint temp = grxstsp;
		byte channelnum = (byte)(temp & USB_OTG_GRXSTSP_EPNUM);
		stduint pktsts = (temp & USB_OTG_GRXSTSP_PKTSTS) >> 17;
		stduint pktcnt = (temp & USB_OTG_GRXSTSP_BCNT) >> 4;
		switch (pktsts) {
		case _GRXSTS_PKTSTS_IN:
			if ((pktcnt > 0) && (hcd.hc[channelnum].xfer_buff != 0)) {
				OTG::ReadPacket(hcd.base, hcd.hc[channelnum].xfer_buff, pktcnt);
				hcd.hc[channelnum].xfer_buff += pktcnt;
				hcd.hc[channelnum].xfer_count += pktcnt;
				if (hcd.ChannelReg(channelnum, 0x010).masof(USB_OTG_HCTSIZ_PKTCNT_Pos, 10) > 0) {
					// re-activate the channel when more packets are expected
					stduint tmpreg = hcd.ChannelReg(channelnum, 0x000);
					tmpreg &= ~USB_OTG_HCCHAR_CHDIS;
					tmpreg |= USB_OTG_HCCHAR_CHENA;
					hcd.ChannelReg(channelnum, 0x000) = tmpreg;
					hcd.hc[channelnum].toggle_in ^= 1;
				}
			}
			break;
		case _GRXSTS_PKTSTS_DATA_TOGGLE_ERR:
		case _GRXSTS_PKTSTS_IN_XFER_COMP:
		case _GRXSTS_PKTSTS_CH_HALTED:
		default:
			break;
		}
	}

	// AKA HCD_Port_IRQHandler
	static void HCD_Port_IRQHandler(HCD& hcd) {
		Reference hprt_reg(hcd.base + USB_OTG_HOST_PORT_BASE);
		stduint hprt0 = hprt_reg;
		stduint hprt0_dup = hprt_reg;
		hprt0_dup &= ~(USB_OTG_HPRT_PENA | USB_OTG_HPRT_PCDET | USB_OTG_HPRT_PENCHNG | USB_OTG_HPRT_POCCHNG);

		if ((hprt0 & USB_OTG_HPRT_PCDET) == USB_OTG_HPRT_PCDET) {
			if ((hprt0 & USB_OTG_HPRT_PCSTS) == USB_OTG_HPRT_PCSTS) {
				hcd.GlobalReg(OTGGlobalReg::GINTMSK).rstof(USB_OTG_GINTMSK_DISCINT_Pos);// mask disconnect
				if (hcd.ConnectHandler) hcd.ConnectHandler();
			}
			hprt0_dup |= USB_OTG_HPRT_PCDET;
		}
		if ((hprt0 & USB_OTG_HPRT_PENCHNG) == USB_OTG_HPRT_PENCHNG) {
			hprt0_dup |= USB_OTG_HPRT_PENCHNG;
			if ((hprt0 & USB_OTG_HPRT_PENA) == USB_OTG_HPRT_PENA) {
				if (hcd.phy_itface == 2) {// USB_OTG_EMBEDDED_PHY
					if ((hprt0 & USB_OTG_HPRT_PSPD) == (_HPRT0_PRTSPD_LOW_SPEED << 17)) {
						OTG::InitFSLSPClkSel(hcd.base, _HCFG_6_MHZ);
					}
					else {
						OTG::InitFSLSPClkSel(hcd.base, _HCFG_48_MHZ);
					}
				}
				else {
					if (hcd.speed == 3) {// HCD_SPEED_FULL
						hcd.HostReg(OTGHostReg::HFIR) = 60000;
					}
				}
				if (hcd.ConnectHandler) hcd.ConnectHandler();
				if (hcd.speed == 0) {// HCD_SPEED_HIGH
					hcd.GlobalReg(OTGGlobalReg::GINTMSK).setof(USB_OTG_GINTMSK_DISCINT_Pos);// unmask disconnect
				}
			}
			else {
				// cleanup HPRT
				hprt_reg = hprt_reg & ~(USB_OTG_HPRT_PENA | USB_OTG_HPRT_PCDET | USB_OTG_HPRT_PENCHNG | USB_OTG_HPRT_POCCHNG);
				hcd.GlobalReg(OTGGlobalReg::GINTMSK).setof(USB_OTG_GINTMSK_DISCINT_Pos);
			}
		}
		if ((hprt0 & USB_OTG_HPRT_POCCHNG) == USB_OTG_HPRT_POCCHNG) {
			hprt0_dup |= USB_OTG_HPRT_POCCHNG;
		}
		// clear port interrupts
		hprt_reg = hprt0_dup;
	}

	// AKA HAL_HCD_IRQHandler
	void HCD::HandleIRQ() {
		stduint i = 0, interrupt = 0;
		// ensure that we are in host mode
		if (OTG::getMode(base) != 1) return;
		// avoid spurious interrupt
		if (OTG::ReadInterrupts(base) == 0) return;

		Reference gintsts(base + _IMM(OTGGlobalReg::GINTSTS));

		if (gintsts.bitof(USB_OTG_GINTSTS_PXFR_INCOMPISOOUT_Pos)) {
			gintsts.setof(USB_OTG_GINTSTS_PXFR_INCOMPISOOUT_Pos);
		}
		if (gintsts.bitof(USB_OTG_GINTSTS_IISOIXFR_Pos)) {
			gintsts.setof(USB_OTG_GINTSTS_IISOIXFR_Pos);
		}
		if (gintsts.bitof(USB_OTG_GINTSTS_PTXFE_Pos)) {
			gintsts.setof(USB_OTG_GINTSTS_PTXFE_Pos);
		}
		if (gintsts.bitof(USB_OTG_GINTSTS_MMIS_Pos)) {
			gintsts.setof(USB_OTG_GINTSTS_MMIS_Pos);
		}
		// host disconnect
		if (gintsts.bitof(USB_OTG_GINTSTS_DISCINT_Pos)) {
			// cleanup HPRT
			Reference hprt_reg(base + USB_OTG_HOST_PORT_BASE);
			hprt_reg = hprt_reg & ~(USB_OTG_HPRT_PENA | USB_OTG_HPRT_PCDET | USB_OTG_HPRT_PENCHNG | USB_OTG_HPRT_POCCHNG);
			if (DisconnectHandler) DisconnectHandler();
			OTG::InitFSLSPClkSel(base, _HCFG_48_MHZ);
			gintsts.setof(USB_OTG_GINTSTS_DISCINT_Pos);
		}
		// host port
		if (gintsts.bitof(USB_OTG_GINTSTS_HPRTINT_Pos)) {
			HCD_Port_IRQHandler(*this);
		}
		// SOF
		if (gintsts.bitof(USB_OTG_GINTSTS_SOF_Pos)) {
			if (SOFHandler) SOFHandler();
			gintsts.setof(USB_OTG_GINTSTS_SOF_Pos);
		}
		// host channels
		if (gintsts.bitof(USB_OTG_GINTSTS_HCINT_Pos)) {
			interrupt = OTG::ReadHostChannelInterrupt(base);
			for (i = 0; i < host_channels; i++) {
				if (interrupt & (1U << i)) {
					if (ChannelReg((byte)i, 0x000).bitof(USB_OTG_HCCHAR_EPDIR_Pos)) {
						HCD_HC_IN_IRQHandler(*this, (byte)i);
					}
					else {
						HCD_HC_OUT_IRQHandler(*this, (byte)i);
					}
				}
			}
			gintsts.setof(USB_OTG_GINTSTS_HCINT_Pos);
		}
		// Rx queue level
		if (gintsts.bitof(USB_OTG_GINTSTS_RXFLVL_Pos)) {
			GlobalReg(OTGGlobalReg::GINTMSK).rstof(USB_OTG_GINTMSK_RXFLVLM_Pos);// mask
			HCD_RXQLVL_IRQHandler(*this);
			GlobalReg(OTGGlobalReg::GINTMSK).setof(USB_OTG_GINTMSK_RXFLVLM_Pos);// unmask
		}
	}

	// ---- RuptTrait (NVIC + IRQ_OTG_HS / IRQ_OTG_FS) ----
	void HCD::setInterrupt(Handler_t f) const {
		if (base == _OTG1_HS_ADDR) FUNC_OTG_HS[0] = f;
		else FUNC_OTG_FS[0] = f;
	}
	void HCD::setInterruptPriority(byte preempt, byte sub_priority) const {
		if (base == _OTG1_HS_ADDR) NVIC.setPriority(IRQ_OTG_HS, preempt, sub_priority);
		else NVIC.setPriority(IRQ_OTG_FS, preempt, sub_priority);
	}
	void HCD::enInterrupt(bool enable) const {
		if (base == _OTG1_HS_ADDR) NVIC.setAble(IRQ_OTG_HS, enable);
		else NVIC.setAble(IRQ_OTG_FS, enable);
	}

#endif // _MCU_STM32H7x
}

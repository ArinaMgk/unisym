// UTF-8 CPL TAB4 CRLF
// Docutitle: (Device) Mouse / マウス
// Codifiers: @ArinaMgk
// Attribute: 
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
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

#include <stdlib.h>
#include "../../../inc/c/driver/mouse.h"
#include "../../../inc/c/driver/i8259A.h"
#include "../../../inc/c/board/IBM.h"

#ifdef _SUPPORT_Port8

#define KEYCMD_SENDTO_MOUSE 0xD4
#define MOUSECMD_ENABLE     0xF4

void Mouse_Init()
{
	i8259Master_Enable(2);
	i8259Slaver_Enable(4);// KBD
	//
	// Keyboard_Wait();
	// outpb(PORT_KEYBOARD_CMD, 0xA8); // enable aux device
	//
	Keyboard_Wait();
	outpb(PORT_KEYBOARD_CMD, KEYCMD_SENDTO_MOUSE);
	Keyboard_Wait();
	outpb(PORT_KEYBOARD_DAT, MOUSECMD_ENABLE);
	return; /* うまくいくとACK(0xfa)が送信されてくる */
}

#endif

#if defined(_INC_CPP) && (defined(_UEFI))
#include <algorithm>

//{TEMP} version
uni::PCI::Device* Mouse_Init_USB(uni::PCI& pci, usb::xhci::Controller* xhc) {
	using namespace uni;
	// seek an Intel xHC.
	// VMware: USB 3.1, 显示所有设备
	PCI::Device* xhc_dev = nullptr;// mouse
	uint64_t xhc_mmio_base = nil;
	for0(i, pci.num_device) {
		if (pci.devices[i].class_code.Match(ClassCodeGroup_xHC)) {
			xhc_dev = &pci.devices[i];
			auto vendor_id = pci.read_vendor_id(*xhc_dev);
			if (0x8086 == vendor_id) {
				break;
			}
		}
	}
	if (!xhc_dev) return nullptr;
	pci.enable_MMIO(*xhc_dev);
	auto xhc_bar = pci.ReadBar(*xhc_dev, 0);
	if (!xhc_bar.pvalue) {
		plogerro("xHC BAR0 is not valid.");
	}
	// config MSI
	const uint8_t bsp_local_apic_id = treat<uint32>_IMM(0xFEE00020) >> 24;// or STI is useless -- Phina 20260117
	pci.configure_MSI_fixed_destination(
		*xhc_dev, bsp_local_apic_id,
		PCI::MSITriggerMode::Edge,
		PCI::MSIDeliveryMode::Fixed,
		IRQ_xHCI, 0);
	//
	xhc_mmio_base = *xhc_bar.pvalue & ~_IMM(0xF);
	if (!xhc_mmio_base) return nullptr;
	new (xhc) usb::xhci::Controller(xhc_mmio_base);
	pci.ConvertFromEhci(*xhc_dev);
	if (auto err = xhc->Initialize()) {
		ploginfo("xhc.Initialize: %s", err.Name());
	}
	//
	xhc->Run();
	for1(i, xhc->MaxPorts()) {
		auto port = xhc->PortAt(i);
		// ploginfo("Port %d: IsConnected=%d", i, port.IsConnected());
		if (port.IsConnected()) {
			if (auto err = ConfigurePort(*xhc, port)) {
				plogerro("Failed to configure port: %s at %s:%d", err.Name(), err.File(), err.Line());
				continue;
			}
		}
	}
	//
	return xhc_dev;
}

namespace usb {
	HIDMouseDriver::HIDMouseDriver(Device* dev, int interface_index)
		: HIDBaseDriver{ dev, interface_index, 3 } {
	}

	Error HIDMouseDriver::OnDataReceived() {
		MouseMessage mmsg;
		MemCopyN(&mmsg, (const void*)&Buffer()[0], sizeof(MouseMessage));
		NotifyMouseMove(mmsg);
		return MAKE_ERROR(Error::kSuccess);
	}

	void HIDMouseDriver::SubscribeMouseMove(
		std::function<ObserverType> observer) {
		observers_[num_observers_++] = observer;
	}

	std::function<HIDMouseDriver::ObserverType> HIDMouseDriver::default_observer;

	void HIDMouseDriver::NotifyMouseMove(MouseMessage mmsg) {
		for (int i = 0; i < num_observers_; ++i) {
			observers_[i](mmsg);
		}
	}
}


#endif

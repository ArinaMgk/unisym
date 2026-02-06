// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device.Bus) PCI PCIe 
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

#include "../../../../inc/cpp/Device/Bus/PCI.hpp"


#if (defined(_MCCA) && ((_MCCA & 0xFF00)==0x8600))
// #include <array>
using namespace uni;

int PCI_Init(PCI& pci) {
	auto err = pci.scan_all_bus();
	if (err != PCI_Result::Success) {
		plogwarn("[Devices] (%s) PCI: Detect %u devices.", err.Name(), pci.num_device);
		return pci.num_device;
	}
	for0(i, pci.num_device) {
		const auto& dev = pci.devices[i];
		auto class_code = pci.read_class_code(dev.bus, dev.device, dev.function);
		// vcon0->OutFormat("%[8H].%[8H].%[8H]: vend 0x%[16H], class 0x%[32H], head 0x%[8H]\n\r",
		// 	dev.bus, dev.device, dev.function, vendor_id, class_code, dev.header_type);
	}
	return pci.num_device;
}


PCI_Result PCI::AddDevice(const Device& dev) {
	// ploginfo("PCI::AddDevice");
	if (num_device == devices.size()) {
		return PCI_Result::Full;
	}
	devices[num_device++] = dev;
	return PCI_Result::Success;
}

PCI_Result PCI::ScanFunction(uint8 bus, uint8 device, uint8 function) {
	auto class_code = read_class_code(bus, device, function);
	auto header_type = PCI::read_header_type(bus, device, function);
	Device dev{ bus, device, function, header_type, class_code };
	if (auto err = AddDevice(dev)) {
		return err;
	}
	if (class_code.Match(0x06u, 0x04u)) {
	  // standard PCI-PCI bridge
		auto bus_numbers = read_bus_numbers(bus, device, function);
		uint8_t secondary_bus = (bus_numbers >> 8) & 0xffu;
		return ScanBus(secondary_bus);
	}
	return PCI_Result::Success;
}

PCI_Result PCI::ScanDevice(uint8 bus, uint8 device) {
	if (auto err = ScanFunction(bus, device, 0)) {
		return err;
	}
	if (PCI::IsSingleFunctionDevice(PCI::read_header_type(bus, device, 0))) {
		return PCI_Result::Success;
	}
	for1 (function, 8 - 1) {
		if (PCI::read_vendor_id(bus, device, function) == 0xffffu) {
			continue;
		}
		if (auto err = ScanFunction(bus, device, function)) {
			return err;
		}
	}
	return PCI_Result::Success;
}

PCI_Result PCI::ScanBus(uint8 bus) {
	// ploginfo("PCI::ScanBus bus%u", bus);
	for0(device, 32) {
		if (PCI::read_device_id(bus, device, 0) == 0xFFFFu) {
			continue;
		}
		if (auto err = ScanDevice(bus, device)) {
			return err;
		}
	}
	return PCI_Result::Success;
}

PCI_Result PCI::scan_all_bus() {
	// ploginfo("PCI::scan_all_bus");
	num_device = 0;
	auto header_type = read_header_type(0, 0, 0);
	if (IsSingleFunctionDevice(header_type)) {
		return ScanBus(0);
	}
	for1 (function, 8 - 1) {
		if (read_vendor_id(0, 0, function) == 0xFFFFu) {
			continue;
		}
		if (auto err = ScanBus(function)) {
			return err;
		}
	}
	return PCI_Result::Success;
}

//{unchk}
void PCI::enable_MMIO(const Device& xhc_dev) {
	uint16_t cmd = self.read_config_register(xhc_dev, 0x04);
	bool changed = false;
	if (!(cmd & 0x2)) { cmd |= 0x2; changed = true; } // Memory Space
	if (!(cmd & 0x4)) { cmd |= 0x4; changed = true; } // Bus Master
	if (changed) {
		self.write_config_register(xhc_dev, 0x04, cmd);
		// ploginfo("Enabled PCI Command memory+bus-master: 0x%04x", cmd);
	}
}

expected<uint64, PCI_Result::Code> PCI::ReadBar(Device& device, unsigned bar_index) {
	if (bar_index >= 6) {
		// return std::unexpected(PCI_Result::IndexOutOfRange);
		return unexpected(PCI_Result::IndexOutOfRange);
	}
	const auto addr = calculate_bar_address(bar_index);
	const auto bar = read_config_register(device, addr);
	// 32 bit address
	if ((bar & 4u) == 0) {
		return uint64(bar);
	}
	// 64 bit address
	if (bar_index >= 5) {
		// return std::unexpected(PCI_Result::IndexOutOfRange);
		return unexpected(PCI_Result::IndexOutOfRange);
	}
	const auto bar_upper = read_config_register(device, addr + 4);
	return bar | (static_cast<uint64_t>(bar_upper) << 32);
}

PCI::MSICapability PCI::read_MSI_capability(const Device& dev, uint8 cap_addr) {
	MSICapability msi_cap{};
	msi_cap.header.data = read_config_register(dev, cap_addr);
	msi_cap.msg_addr = read_config_register(dev, cap_addr + 4);
	uint8_t msg_data_addr = cap_addr + 8;
	if (msi_cap.header.bits.addr_64_capable) {
		msi_cap.msg_upper_addr = read_config_register(dev, cap_addr + 8);
		msg_data_addr = cap_addr + 12;
	}
	msi_cap.msg_data = read_config_register(dev, msg_data_addr);
	if (msi_cap.header.bits.per_vector_mask_capable) {
		msi_cap.mask_bits = read_config_register(dev, msg_data_addr + 4);
		msi_cap.pending_bits = read_config_register(dev, msg_data_addr + 8);
	}
	return msi_cap;
}

void PCI::write_MSI_capability(const Device& dev, uint8 cap_addr, const PCI::MSICapability& msi_cap) {
	write_config_register(dev, cap_addr, msi_cap.header.data);
	write_config_register(dev, cap_addr + 4, msi_cap.msg_addr);
	uint8_t msg_data_addr = cap_addr + 8;
	if (msi_cap.header.bits.addr_64_capable) {
		write_config_register(dev, cap_addr + 8, msi_cap.msg_upper_addr);
		msg_data_addr = cap_addr + 12;
	}
	write_config_register(dev, msg_data_addr, msi_cap.msg_data);
	if (msi_cap.header.bits.per_vector_mask_capable) {
		write_config_register(dev, msg_data_addr + 4, msi_cap.mask_bits);
		write_config_register(dev, msg_data_addr + 8, msi_cap.pending_bits);
	}
}

PCI_Result PCI::configure_MSI_register(const Device& dev, uint8_t cap_addr,
	uint32 msg_addr, uint32 msg_data, unsigned num_vector_exponent) {
	auto msi_cap = read_MSI_capability(dev, cap_addr);
	if (msi_cap.header.bits.multi_msg_capable <= num_vector_exponent) {
		msi_cap.header.bits.multi_msg_enable =
			msi_cap.header.bits.multi_msg_capable;
	}
	else {
		msi_cap.header.bits.multi_msg_enable = num_vector_exponent;
	}
	msi_cap.header.bits.msi_enable = 1;
	msi_cap.msg_addr = msg_addr;
	msi_cap.msg_data = msg_data;
	write_MSI_capability(dev, cap_addr, msi_cap);
	return (PCI_Result::Success);
}

PCI_Result PCI::configure_MSIX_register(const Device& dev, uint8_t cap_addr, uint32 msg_addr, uint32 msg_data, unsigned num_vector_exponent) {
	return (PCI_Result::NotImplemented);
}

PCI::CapabilityHeader PCI::read_capability_header(const Device& dev, uint8 addr)
{
	CapabilityHeader header;
	header.data = read_config_register(dev, addr);
	return header;
}

PCI_Result PCI::configure_MSI(const Device& dev, uint32 msg_addr, uint32 msg_data, unsigned num_vector_exponent)
{
	uint8_t cap_addr = read_config_register(dev, 0x34) & 0xffu;
	uint8_t msi_cap_addr = 0, msix_cap_addr = 0;
	while (cap_addr != 0) {
		auto header = read_capability_header(dev, cap_addr);
		if (header.bits.cap_id == kCapabilityMSI) {
			msi_cap_addr = cap_addr;
		}
		else if (header.bits.cap_id == kCapabilityMSIX) {
			msix_cap_addr = cap_addr;
		}
		cap_addr = header.bits.next_ptr;
	}
	if (msi_cap_addr) {
		return configure_MSI_register(dev, msi_cap_addr, msg_addr, msg_data, num_vector_exponent);
	}
	else if (msix_cap_addr) {
		return configure_MSIX_register(dev, msix_cap_addr, msg_addr, msg_data, num_vector_exponent);
	}
	return (PCI_Result::NoPCIMSI);
}

PCI_Result PCI::configure_MSI_fixed_destination(const Device& dev,
	uint8 apic_id,
	MSITriggerMode trigger_mode,
	MSIDeliveryMode delivery_mode,
	uint8 vector,
	unsigned num_vector_exponent
)
{
	uint32_t msg_addr = 0xfee00000u | (_IMM(apic_id) << 12);
	uint32_t msg_data = (static_cast<uint32_t>(delivery_mode) << 8) | vector;
	if (trigger_mode == MSITriggerMode::Level) {
		msg_data |= 0xc000;
	}
	return configure_MSI(dev, msg_addr, msg_data, num_vector_exponent);
}

// To xHCI
//{TEMP} Intel only
bool PCI::ConvertFromEhci(const PCI::Device& xhc_dev) {
	bool intel_ehc_exist = false;
	for0 (i, self.num_device) {
		if (self.devices[i].class_code.Match(0x0Cu, 0x03u, 0x20u) /* EHCI */ &&
			0x8086 == self.read_vendor_id(self.devices[i])) {
			intel_ehc_exist = true;
			break;
		}
	}
	if (!intel_ehc_exist) {
		return false;
	}
	uint32_t superspeed_ports = self.read_config_register(xhc_dev, 0xdc); // USB3PRM
	if (superspeed_ports == 0xFFFFFFFFu) {
		plogwarn("SwitchEhci2Xhci: USB3PRM (0xDC) not present, skipping.");
		return false;
	}
	self.write_config_register(xhc_dev, 0xd8, superspeed_ports); // USB3_PSSEN
	uint32_t ehci2xhci_ports = self.read_config_register(xhc_dev, 0xd4); // XUSB2PRM
	if (ehci2xhci_ports == 0xFFFFFFFFu) {
		plogwarn("SwitchEhci2Xhci: XUSB2PRM (0xD4) not present, skipping.");
		return false;
	}
	self.write_config_register(xhc_dev, 0xd0, ehci2xhci_ports); // XUSB2PR
	// ploginfo("%s: SS = %02, xHCI = %02x\n", __FUNCIDEN__, superspeed_ports, ehci2xhci_ports);
	return true;
}


#endif

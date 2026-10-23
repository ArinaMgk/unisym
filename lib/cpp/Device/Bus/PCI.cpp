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
		auto class_code = pci.read_class_code(dev);
		// vcon0->OutFormat("%[8H].%[8H].%[8H]: vend 0x%[16H], class 0x%[32H], head 0x%[8H]\n\r",
		// 	dev.bus, dev.device, dev.function, vendor_id, class_code, dev.header_type);
	}
	return pci.num_device;
}


PCI_Result PCI::AddDevice(const Device& dev) {
	// ploginfo("PCI::AddDevice");
	for0(index, devices.Count()) {
		const auto& current = devices[index];
		if (current.segment_group == dev.segment_group && current.bus == dev.bus &&
			current.device == dev.device && current.function == dev.function)
			return PCI_Result::Success;
	}
	devices.Append(dev);
	num_device = devices.Count();
	return PCI_Result::Success;
}

PCI_Result PCI::ScanFunction(uint16 segment, uint8 bus, uint8 device, uint8 function) {
	Device dev{};
	dev.segment_group = segment;
	dev.bus = bus;
	dev.device = device;
	dev.function = function;
	dev.class_code = read_class_code(dev);
	dev.header_type = PCI::read_header_type(dev);
	if (auto err = AddDevice(dev)) {
		return err;
	}
	if (dev.class_code.Match(0x06u, 0x04u)) {
		// Standard PCI-to-PCI bridge.
		auto bus_numbers = read_bus_numbers(dev);
		uint8_t primary_bus = bus_numbers & 0xffu;
		uint8_t secondary_bus = (bus_numbers >> 8) & 0xffu;
		uint8_t subordinate_bus = (bus_numbers >> 16) & 0xffu;
		if (secondary_bus == 0 || secondary_bus < primary_bus || secondary_bus > subordinate_bus) {
			return PCI_Result::Success;
		}
		return ScanBus(segment, secondary_bus);
	}
	return PCI_Result::Success;
}

PCI_Result PCI::ScanDevice(uint16 segment, uint8 bus, uint8 device) {
	Device function_zero{};
	function_zero.segment_group = segment;
	function_zero.bus = bus;
	function_zero.device = device;
	if (auto err = ScanFunction(segment, bus, device, 0)) {
		return err;
	}
	if (PCI::IsSingleFunctionDevice(PCI::read_header_type(function_zero))) {
		return PCI_Result::Success;
	}
	for1 (function, 8 - 1) {
		Device dev{};
		dev.segment_group = segment;
		dev.bus = bus;
		dev.device = device;
		dev.function = function;
		if (PCI::read_vendor_id(dev) == 0xffffu) {
			continue;
		}
		if (auto err = ScanFunction(segment, bus, device, function)) {
			return err;
		}
	}
	return PCI_Result::Success;
}

PCI_Result PCI::ScanBus(uint16 segment, uint8 bus) {
	for0(device, 32) {
		Device dev{};
		dev.segment_group = segment;
		dev.bus = bus;
		dev.device = device;
		uint16 device_id = PCI::read_device_id(dev);
		if (device_id == 0xFFFFu) {
			continue;
		}
		if (auto err = ScanDevice(segment, bus, device)) {
			return err;
		}
	}
	return PCI_Result::Success;
}

PCI_Result PCI::scan_all_bus() {
	// ploginfo("PCI::scan_all_bus");
	num_device = 0;
	devices.Clear();
	const stduint access_count = GetConfigSpaceAccessCount();
	if (access_count) {
		bool scanned_segment_zero_bus_zero = false;
		for0(index, access_count) {
			const auto* access = GetConfigSpaceAccess(index);
			if (!access) continue;
			if (access->segment_group == 0 && access->start_bus == 0)
				scanned_segment_zero_bus_zero = true;
			bool already_scanned = false;
			for0(previous, index) {
				const auto* current = GetConfigSpaceAccess(previous);
				if (current && current->segment_group == access->segment_group &&
					current->start_bus == access->start_bus) {
					already_scanned = true;
					break;
				}
			}
			if (already_scanned) continue;
			if (auto err = ScanBus(access->segment_group, access->start_bus)) return err;
		}
		if (scanned_segment_zero_bus_zero) return PCI_Result::Success;
	}
	auto header_type = read_header_type(0, 0, 0);
	if (IsSingleFunctionDevice(header_type)) return ScanBus(0, 0);
	// For multi-function host bridges, each function maps to one PCI bus.
	for0 (function, 8) {
		if (read_vendor_id(0, 0, function) == 0xFFFFu) continue;
		if (auto err = ScanBus(0, function)) return err;
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
	auto disabled_header = msi_cap.header;
	disabled_header.bits.msi_enable = 0;
	write_config_register(dev, cap_addr, disabled_header.data);
	write_config_register(dev, cap_addr + 4, msi_cap.msg_addr);
	uint8_t msg_data_addr = cap_addr + 8;
	if (msi_cap.header.bits.addr_64_capable) {
		write_config_register(dev, cap_addr + 8, msi_cap.msg_upper_addr);
		msg_data_addr = cap_addr + 12;
	}
	write_config_register(dev, msg_data_addr, msi_cap.msg_data);
	if (msi_cap.header.bits.per_vector_mask_capable) {
		write_config_register(dev, msg_data_addr + 4, msi_cap.mask_bits);
	}
	write_config_register(dev, cap_addr, msi_cap.header.data);
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
	(void)cap_addr;
	uint16 count = num_vector_exponent < 11 ? uint16(1u << num_vector_exponent) : 0;
	const uint16 table_size = MSIXTableSize(dev);
	if (count == 0 || table_size == 0) return PCI_Result::NotImplemented;
	if (count > table_size) count = table_size;
	uni::Vector<MSIMessage> messages{};
	for (uint16 index = 0; index < count; ++index) {
		MSIMessage message{};
		message.address = msg_addr;
		message.data = msg_data + index;
		if (!messages.Append(message)) return PCI_Result::Full;
	}
	return ConfigureMSIX(dev, messages.begin(), count);
}

uint16 PCI::MSIXTableSize(const Device& dev) {
	uint8 cap_addr = read_config_register(dev, 0x34) & 0xffu;
	for (uint8 count = 0; cap_addr != 0 && count < 48; ++count) {
		const auto header = read_capability_header(dev, cap_addr);
		if (header.bits.cap_id == kCapabilityMSIX) {
			return static_cast<uint16>(((header.data >> 16) & 0x07ffu) + 1u);
		}
		cap_addr = header.bits.next_ptr;
	}
	return 0;
}

PCI_Result PCI::ConfigureMSIX(const Device& dev, const MSIMessage* messages,
	uint16 message_count) {
	if (messages == nullptr || message_count == 0) return PCI_Result::Empty;
	uint8 cap_addr = read_config_register(dev, 0x34) & 0xffu;
	uint8 msi_cap_addr = 0;
	uint8 msix_cap_addr = 0;
	for (uint8 count = 0; cap_addr != 0 && count < 48; ++count) {
		const auto header = read_capability_header(dev, cap_addr);
		if (header.bits.cap_id == kCapabilityMSI) msi_cap_addr = cap_addr;
		if (header.bits.cap_id == kCapabilityMSIX) msix_cap_addr = cap_addr;
		cap_addr = header.bits.next_ptr;
	}
	if (msix_cap_addr == 0) return PCI_Result::NoPCIMSI;

	uint32 control = read_config_register(dev, msix_cap_addr);
	const uint16 table_size = static_cast<uint16>(((control >> 16) & 0x07ffu) + 1u);
	if (message_count > table_size) return PCI_Result::IndexOutOfRange;
	const uint32 table_descriptor = read_config_register(dev, msix_cap_addr + 4);
	const uint8 bir = table_descriptor & 0x07u;
	if (bir >= 6) return PCI_Result::IndexOutOfRange;
	const uint8 bar_addr = calculate_bar_address(bir);
	const uint32 bar_low = read_config_register(dev, bar_addr);
	if ((bar_low & 0x01u) != 0) return PCI_Result::NotImplemented;
	const uint8 memory_type = uint8((bar_low >> 1) & 0x03u);
	uint64 bar_base = uint64(bar_low & ~0x0fu);
	if (memory_type == 0x02u) {
		if (bir >= 5) return PCI_Result::IndexOutOfRange;
		bar_base |= uint64(read_config_register(dev, bar_addr + 4)) << 32;
	}
	if (bar_base == 0) return PCI_Result::Empty;
	const uint64 table_address = bar_base + uint64(table_descriptor & ~0x07u);
	if (table_address < bar_base || table_address > uint64(~stduint(0))) {
		return PCI_Result::IndexOutOfRange;
	}

	control &= ~(1u << 31);
	control |= 1u << 30;
	write_config_register(dev, msix_cap_addr, control);
	if (msi_cap_addr != 0) {
		uint32 msi_control = read_config_register(dev, msi_cap_addr);
		msi_control &= ~(1u << 16);
		write_config_register(dev, msi_cap_addr, msi_control);
	}
	auto* table = reinterpret_cast<volatile uint32*>(stduint(table_address));
	for (uint16 index = 0; index < table_size; ++index) {
		table[stduint(index) * 4u + 3u] = 1u;
	}
	for (uint16 index = 0; index < message_count; ++index) {
		const stduint offset = stduint(index) * 4u;
		table[offset] = uint32(messages[index].address);
		table[offset + 1] = uint32(messages[index].address >> 32);
		table[offset + 2] = messages[index].data;
		table[offset + 3] = 0;
	}
	control |= 1u << 31;
	control &= ~(1u << 30);
	write_config_register(dev, msix_cap_addr, control);
	return PCI_Result::Success;
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

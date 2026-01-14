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

#endif

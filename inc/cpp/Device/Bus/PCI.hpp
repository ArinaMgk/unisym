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

#ifndef _INCPP_Device_BUS_PCI
#define _INCPP_Device_BUS_PCI
// unicpp
#include "../../../c/stdinc.h"
#include "../../../cpp/port"

#if (defined(_MCCA) && ((_MCCA & 0xFF00)==0x8600))
// ---- unicpp
#include "../../expected"
// ---- stlcpp
#include <array>
// #include <expected>// c++23
// ---- modcpp

#define ClassCodeGroup_xHC 0x0C, 0x03, 0x30

namespace uni {
	class PCI_Result {
	public:
		enum Code {
			Success,
			Full,
			Empty,
			IndexOutOfRange,
			LastOfCode,
		};

		PCI_Result(Code code) : code_{ code } {}

		operator bool() const {
			return this->code_ != Success;
		}

		const char* Name() const {
			return code_names_[static_cast<int>(this->code_)];
		}

	private:
		static constexpr std::array<const char*, 4> code_names_ = {
			"Success",
			"Full",
			"Empty",
			"IndexOutOfRange",
		};
		Code code_;
	};

	class PCI {
	public:
		struct ClassCode {
			uint8_t base, sub, interface;
			// match base
			bool Match(uint8_t b) { return b == base; }
			// match base, sub
			bool Match(uint8_t b, uint8_t s) { return Match(b) && s == sub; }
			// match base, sub, interface
			bool Match(uint8_t b, uint8_t s, uint8_t i) {
				return Match(b, s) && i == interface;
			}
		};
		
		struct Device {
			uint8 bus, device, function, header_type;
			ClassCode class_code;
		};
		std::array<Device, 32> devices;
		int num_device;



		PCI_Result scan_all_bus();

	public:
		inline static uint32 make_port_address(uint8 bus, uint8 dev, uint8 func, uint8 reg) {
			auto shl = [](uint32 x, unsigned bits) {return x << bits;};
			return shl(1, 31) _Comment(enable) | shl(bus, 16) | shl(dev, 11) | shl(func, 8) | (reg & 0xFCu);
		}
		inline static void send_addr(uint32 addr) {
			Port<uint32> paddr(PORT_PCI_CONFIG_ADDR);
			paddr = addr;// outpd(PORT_PCI_CONFIG_ADDR, addr);
		}
		inline static void send_data(uint32 data) {
			Port<uint32> pdata(PORT_PCI_CONFIG_DATA);
			pdata = data;// outpd(PORT_PCI_CONFIG_DATA, data);
		}
		inline static uint32 read_data() {
			return Port<uint32>(PORT_PCI_CONFIG_DATA);// innpd(PORT_PCI_CONFIG_DATA);
		}

		inline static uint16 read_vendor_id(uint8 bus, uint8 dev, uint8 func) {
			send_addr(make_port_address(bus, dev, func, 0x00));
			return read_data() & 0xFFFF;
		}
		inline static uint16 read_vendor_id(const Device& dev) {
			send_addr(make_port_address(dev.bus, dev.device, dev.function, 0x00));
			return read_data() & 0xFFFF;
		}

		inline static uint16 read_device_id(uint8 bus, uint8 dev, uint8 func) {
			send_addr(make_port_address(bus, dev, func, 0x00));
			return read_data() >> 16;
		}

		inline static ClassCode read_class_code(uint8 bus, uint8 dev, uint8 func) {
			send_addr(make_port_address(bus, dev, func, 0x08));
			uint32 class_code = read_data();
			ClassCode cc;
			cc.base = (class_code >> 24);
			cc.sub = (class_code >> 16);
			cc.interface = (class_code >> 8);
			return cc;
		}

		inline static uint8  read_header_type(uint8 bus, uint8 dev, uint8 func) {
			send_addr(make_port_address(bus, dev, func, 0x0C));
			return read_data() >> 16;
		}
		inline static uint32 read_bus_numbers(uint8 bus, uint8 dev, uint8 func) {
			send_addr(make_port_address(bus, dev, func, 0x18));
			return read_data();
		}

		inline static bool IsSingleFunctionDevice(uint8 header_type) {
			return !(header_type & 0x80u);
		}

		inline static uint32 read_config_register(const Device& dev, uint8 reg_addr) {
			send_addr(make_port_address(dev.bus, dev.device, dev.function, reg_addr));
			return read_data();
		}

		inline static void write_config_register(const Device& dev, uint8 reg_addr, uint32 data) {
			send_addr(make_port_address(dev.bus, dev.device, dev.function, reg_addr));
			send_data(data);
		}

		constexpr uint8 calculate_bar_address(unsigned int bar_index) {
			return 0x10 + 4 * bar_index;
		}

		// std::expected<uint64, PCI_Result> ReadBar(Device& device, unsigned bar_index);
		::uni::expected<uint64, PCI_Result::Code> ReadBar(Device& device, unsigned bar_index);

	protected:
		PCI_Result ScanBus(uint8 bus);
		PCI_Result ScanDevice(uint8 bus, uint8 device);
		PCI_Result ScanFunction(uint8 bus, uint8 device, uint8 function);
		//
		PCI_Result AddDevice(const Device& dev);

	};

	
} // namespace uni
#endif

#endif

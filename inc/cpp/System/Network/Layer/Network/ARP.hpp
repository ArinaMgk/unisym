// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [System.Network.Layer.Network] ARP
// Codifiers: @AringMgk
// Attribute: <ArnCovenant> Any-Architect <Environment> <Reference/Dependence>
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

// Funtion: IPv4 -> MAC

#ifndef _INCPP_System_NETWORK_LAYER_NETWORK_ARP
#define _INCPP_System_NETWORK_LAYER_NETWORK_ARP

#include "../../../Network.hpp"
#include "../Link/Ethernet.hpp"

namespace uni {
namespace Network {

	constexpr stduint ArpEthernetIPv4Length = 28;

	enum class ArpHardwareType : uint16 {
		Ethernet = 1,
	};

	enum class ArpOperation : uint16 {
		Request = 1,
		Reply = 2,
	};

	_PACKED(struct) ArpEthernetIPv4Packet {
		uint8 hardware_type[2];
		uint8 protocol_type[2];
		uint8 hardware_length;
		uint8 protocol_length;
		uint8 operation[2];
		uint8 sender_hardware[EthernetAddressLength];
		uint8 sender_protocol[IPv4AddressLength];
		uint8 target_hardware[EthernetAddressLength];
		uint8 target_protocol[IPv4AddressLength];
	};

	struct ArpEthernetIPv4View {
		uint16 hardware_type;
		uint16 protocol_type;
		uint8 hardware_length;
		uint8 protocol_length;
		uint16 operation;
		MacAddress sender_hardware;
		IPv4Address sender_protocol;
		MacAddress target_hardware;
		IPv4Address target_protocol;
	};

	inline bool ParseArpEthernetIPv4(const EthernetFrameView& ethernet, ArpEthernetIPv4View& arp) {
		if (ethernet.type != uint16(EthernetType::ARP)) return false;
		if (!ethernet.payload || ethernet.payload_length < ArpEthernetIPv4Length) return false;
		const auto* packet = reinterpret_cast<const ArpEthernetIPv4Packet*>(ethernet.payload);
		arp.hardware_type = EthernetRead16(packet->hardware_type);
		arp.protocol_type = EthernetRead16(packet->protocol_type);
		arp.hardware_length = packet->hardware_length;
		arp.protocol_length = packet->protocol_length;
		arp.operation = EthernetRead16(packet->operation);
		EthernetCopyAddress(arp.sender_hardware, packet->sender_hardware);
		IPv4CopyAddress(arp.sender_protocol, packet->sender_protocol);
		EthernetCopyAddress(arp.target_hardware, packet->target_hardware);
		IPv4CopyAddress(arp.target_protocol, packet->target_protocol);
		return arp.hardware_type == uint16(ArpHardwareType::Ethernet) &&
			arp.protocol_type == uint16(EthernetType::IPv4) &&
			arp.hardware_length == EthernetAddressLength &&
			arp.protocol_length == IPv4AddressLength;
	}

	inline stduint BuildArpEthernetIPv4Reply(uint8* buffer, stduint capacity,
		const MacAddress& local_mac, const IPv4Address& local_ip,
		const MacAddress& target_mac, const IPv4Address& target_ip) {
		constexpr stduint frame_length = EthernetHeaderLength + ArpEthernetIPv4Length;
		if (!buffer || capacity < frame_length) return 0;
		auto* ethernet = reinterpret_cast<EthernetHeader*>(buffer);
		EthernetWriteAddress(ethernet->destination, target_mac);
		EthernetWriteAddress(ethernet->source, local_mac);
		EthernetWrite16(ethernet->type, uint16(EthernetType::ARP));
		auto* arp = reinterpret_cast<ArpEthernetIPv4Packet*>(buffer + EthernetHeaderLength);
		EthernetWrite16(arp->hardware_type, uint16(ArpHardwareType::Ethernet));
		EthernetWrite16(arp->protocol_type, uint16(EthernetType::IPv4));
		arp->hardware_length = EthernetAddressLength;
		arp->protocol_length = IPv4AddressLength;
		EthernetWrite16(arp->operation, uint16(ArpOperation::Reply));
		EthernetWriteAddress(arp->sender_hardware, local_mac);
		IPv4WriteAddress(arp->sender_protocol, local_ip);
		EthernetWriteAddress(arp->target_hardware, target_mac);
		IPv4WriteAddress(arp->target_protocol, target_ip);
		return frame_length;
	}

}
}

#endif

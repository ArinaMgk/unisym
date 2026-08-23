// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [System.Network.Layer.Network] IPv4
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

#ifndef _INCPP_System_NETWORK_LAYER_NETWORK_IPV4
#define _INCPP_System_NETWORK_LAYER_NETWORK_IPV4

#include "../../../Network.hpp"
#include "../Link/Ethernet.hpp"

namespace uni {
namespace Network {

	constexpr stduint IPv4MinHeaderLength = 20;

	enum class IPv4Protocol : uint8 {
		ICMP = 1,
		TCP = 6,
		UDP = 17,
	};

	_PACKED(struct) IPv4Header {
		uint8 version_ihl;
		uint8 tos;
		uint8 total_length[2];
		uint8 identification[2];
		uint8 flags_fragment[2];
		uint8 ttl;
		uint8 protocol;
		uint8 checksum[2];
		uint8 source[IPv4AddressLength];
		uint8 destination[IPv4AddressLength];
	};

	struct IPv4PacketView {
		const IPv4Header* header;
		uint8 header_length;
		uint16 total_length;
		uint8 protocol;
		IPv4Address source;
		IPv4Address destination;
		const uint8* payload;
		stduint payload_length;
	};

	inline uint16 NetworkChecksum(const void* data, stduint length) {
		const auto* bytes = reinterpret_cast<const uint8*>(data);
		uint32 sum = 0;
		for (; length > 1; length -= 2, bytes += 2) {
			sum += (uint16(bytes[0]) << 8) | uint16(bytes[1]);
		}
		if (length) sum += uint16(bytes[0]) << 8;
		while (sum >> 16) sum = (sum & 0xFFFFu) + (sum >> 16);
		return uint16(~sum);
	}

	inline bool ParseIPv4Packet(const EthernetFrameView& ethernet, IPv4PacketView& packet) {
		if (ethernet.type != uint16(EthernetType::IPv4)) return false;
		if (!ethernet.payload || ethernet.payload_length < IPv4MinHeaderLength) return false;
		const auto* header = reinterpret_cast<const IPv4Header*>(ethernet.payload);
		const uint8 version = header->version_ihl >> 4;
		const uint8 ihl = header->version_ihl & 0x0Fu;
		const stduint header_length = stduint(ihl) * 4;
		if (version != 4 || header_length < IPv4MinHeaderLength || ethernet.payload_length < header_length) return false;
		const uint16 total_length = EthernetRead16(header->total_length);
		if (total_length < header_length || ethernet.payload_length < total_length) return false;
		if (NetworkChecksum(header, header_length) != 0) return false;
		packet.header = header;
		packet.header_length = uint8(header_length);
		packet.total_length = total_length;
		packet.protocol = header->protocol;
		IPv4CopyAddress(packet.source, header->source);
		IPv4CopyAddress(packet.destination, header->destination);
		packet.payload = ethernet.payload + header_length;
		packet.payload_length = total_length - header_length;
		return true;
	}

	inline void BuildIPv4Header(IPv4Header& header,
		const IPv4Address& source, const IPv4Address& destination,
		uint8 protocol, uint16 total_length, uint16 identification, uint8 ttl = 64) {
		header.version_ihl = 0x45u;
		header.tos = 0;
		EthernetWrite16(header.total_length, total_length);
		EthernetWrite16(header.identification, identification);
		EthernetWrite16(header.flags_fragment, 0);
		header.ttl = ttl;
		header.protocol = protocol;
		EthernetWrite16(header.checksum, 0);
		IPv4WriteAddress(header.source, source);
		IPv4WriteAddress(header.destination, destination);
		EthernetWrite16(header.checksum, NetworkChecksum(&header, IPv4MinHeaderLength));
	}

}
}

#endif

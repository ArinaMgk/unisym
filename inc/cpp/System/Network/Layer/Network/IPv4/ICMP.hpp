// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [System.Network.Layer.Network.IPv4] ICMP
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

#ifndef _INCPP_System_NETWORK_LAYER_NETWORK_IPV4_ICMP
#define _INCPP_System_NETWORK_LAYER_NETWORK_IPV4_ICMP

#include "../IPv4.hpp"

namespace uni {
namespace Network {

	constexpr stduint ICMPv4HeaderLength = 8;

	enum class ICMPv4Type : uint8 {
		EchoReply = 0,
		EchoRequest = 8,
	};

	_PACKED(struct) ICMPv4Header {
		uint8 type;
		uint8 code;
		uint8 checksum[2];
		uint8 identifier[2];
		uint8 sequence[2];
	};

	struct ICMPv4EchoView {
		const ICMPv4Header* header;
		const uint8* payload;
		stduint payload_length;
	};

	inline bool ParseICMPv4EchoRequest(const IPv4PacketView& ipv4, ICMPv4EchoView& echo) {
		if (ipv4.protocol != uint8(IPv4Protocol::ICMP)) return false;
		if (!ipv4.payload || ipv4.payload_length < ICMPv4HeaderLength) return false;
		const auto* header = reinterpret_cast<const ICMPv4Header*>(ipv4.payload);
		if (header->type != uint8(ICMPv4Type::EchoRequest) || header->code != 0) return false;
		if (NetworkChecksum(ipv4.payload, ipv4.payload_length) != 0) return false;
		echo.header = header;
		echo.payload = ipv4.payload + ICMPv4HeaderLength;
		echo.payload_length = ipv4.payload_length - ICMPv4HeaderLength;
		return true;
	}

	inline stduint BuildICMPv4EchoReply(uint8* buffer, stduint capacity,
		const MacAddress& local_mac, const MacAddress& target_mac,
		const IPv4Address& local_ip, const IPv4PacketView& request) {
		const stduint icmp_length = request.payload_length;
		const stduint ipv4_length = IPv4MinHeaderLength + icmp_length;
		const stduint frame_length = EthernetHeaderLength + ipv4_length;
		if (!buffer || capacity < frame_length || icmp_length < ICMPv4HeaderLength) return 0;
		auto* ethernet = reinterpret_cast<EthernetHeader*>(buffer);
		EthernetWriteAddress(ethernet->destination, target_mac);
		EthernetWriteAddress(ethernet->source, local_mac);
		EthernetWrite16(ethernet->type, uint16(EthernetType::IPv4));
		auto* ipv4 = reinterpret_cast<IPv4Header*>(buffer + EthernetHeaderLength);
		const uint16 identification = EthernetRead16(request.header->identification);
		BuildIPv4Header(*ipv4, local_ip, request.source, uint8(IPv4Protocol::ICMP),
			uint16(ipv4_length), identification);
		auto* icmp = buffer + EthernetHeaderLength + IPv4MinHeaderLength;
		for0(i, icmp_length) icmp[i] = request.payload[i];
		auto* icmp_header = reinterpret_cast<ICMPv4Header*>(icmp);
		icmp_header->type = uint8(ICMPv4Type::EchoReply);
		icmp_header->code = 0;
		EthernetWrite16(icmp_header->checksum, 0);
		EthernetWrite16(icmp_header->checksum, NetworkChecksum(icmp, icmp_length));
		return frame_length;
	}

}
}

#endif

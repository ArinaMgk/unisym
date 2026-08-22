// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [System.Network.Layer.Link] Ethernet
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

#ifndef _INCPP_System_NETWORK_LAYER_LINK_ETHERNET
#define _INCPP_System_NETWORK_LAYER_LINK_ETHERNET

#include "../Link.hpp"

namespace uni {
namespace Network {

	constexpr stduint EthernetAddressLength = 6;
	constexpr stduint EthernetHeaderLength = 14;
	constexpr stduint EthernetMinFrameLength = 60;
	constexpr stduint EthernetMaxFrameLength = 1518;

	enum class EthernetType : uint16 {
		IPv4 = 0x0800u,
		ARP = 0x0806u,
		IPv6 = 0x86DDu,
		Experiment = 0x88B5u,
	};

	_PACKED(struct) EthernetHeader {
		uint8 destination[EthernetAddressLength];
		uint8 source[EthernetAddressLength];
		uint8 type[2];
	};

	struct EthernetFrameView {
		MacAddress destination;
		MacAddress source;
		uint16 type;
		const uint8* payload;
		stduint payload_length;
	};

	inline uint16 EthernetRead16(const uint8* data) {
		return (uint16(data[0]) << 8) | uint16(data[1]);
	}

	inline void EthernetCopyAddress(MacAddress& destination, const uint8* source) {
		for0(i, EthernetAddressLength) destination.octet[i] = source[i];
	}

	inline bool ParseEthernetFrame(const LinkFrameView& raw, EthernetFrameView& frame) {
		if (!raw.data || raw.length < EthernetHeaderLength) return false;
		const auto* header = reinterpret_cast<const EthernetHeader*>(raw.data);
		EthernetCopyAddress(frame.destination, header->destination);
		EthernetCopyAddress(frame.source, header->source);
		frame.type = EthernetRead16(header->type);
		frame.payload = reinterpret_cast<const uint8*>(raw.data) + EthernetHeaderLength;
		frame.payload_length = raw.length - EthernetHeaderLength;
		return true;
	}

}
}

#endif

// ASCII CPP-ISO11 TAB4 LF
// Docutitle: [System.Network.Layer] Network
// Codifiers: @AringMgk
// Attribute: <ArnCovenant> Any-Architect <Environment> <Reference/Dependence>
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2026 ArinaMgk

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

#ifndef _INCPP_System_NETWORK_LAYER_NETWORK
#define _INCPP_System_NETWORK_LAYER_NETWORK

#include "../../Network.hpp"

namespace uni {
namespace Network {

	enum class NetworkAddressFamily : uint16 {
		Unspec = 0,
		IPv4 = 2,
		IPv6 = 10,
	};

	struct NetworkAddress {
		NetworkAddressFamily family;
		uint8 length;
		uint8 octet[IPv6AddressLength];

		bool isZero() const {
			for0(i, length) if (octet[i]) return false;
			return true;
		}
	};

	struct NetworkPacketContext {
		NetworkAddress source;
		NetworkAddress destination;
		uint8 protocol;
		const void* payload;
		stduint payload_length;
		uint16 identification;
		uint8 ttl;
	};

	class NetworkInterface {
	public:
		virtual ~NetworkInterface() = default;
		virtual NetworkAddress getAddress() const = 0;
		virtual stduint getPayloadMtu() const = 0;
		virtual stdsint SendPacket(const NetworkPacketContext& packet) = 0;
	};

	inline NetworkAddress NetworkAddressIPv4(const IPv4Address& address) {
		NetworkAddress output{};
		output.family = NetworkAddressFamily::IPv4;
		output.length = IPv4AddressLength;
		for0(i, IPv4AddressLength) output.octet[i] = address.octet[i];
		return output;
	}

	inline NetworkAddress NetworkAddressIPv6(const IPv6Address& address) {
		NetworkAddress output{};
		output.family = NetworkAddressFamily::IPv6;
		output.length = IPv6AddressLength;
		for0(i, IPv6AddressLength) output.octet[i] = address.octet[i];
		return output;
	}

	inline bool NetworkReadIPv4Address(const NetworkAddress& input, IPv4Address& output) {
		if (input.family != NetworkAddressFamily::IPv4 || input.length != IPv4AddressLength) return false;
		for0(i, IPv4AddressLength) output.octet[i] = input.octet[i];
		return true;
	}

}
}

#endif

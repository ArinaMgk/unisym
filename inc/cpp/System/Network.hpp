// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [System] Network
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

#ifndef _INCPP_System_NETWORK
#define _INCPP_System_NETWORK

#include "../../c/stdinc.h"

// Application -> Transport -> Network -> Link -> NetworkDevice

namespace uni {
namespace Network {

	constexpr stduint IPv4AddressLength = 4;
	constexpr stduint IPv6AddressLength = 16;

	struct IPv4Address {
		uint8 octet[IPv4AddressLength];

		bool operator==(const IPv4Address& other) const {
			for0(i, IPv4AddressLength) if (octet[i] != other.octet[i]) return false;
			return true;
		}

		bool isZero() const {
			for0(i, IPv4AddressLength) if (octet[i]) return false;
			return true;
		}
	};

	struct IPv6Address {
		uint8 octet[IPv6AddressLength];

		bool operator==(const IPv6Address& other) const {
			for0(i, IPv6AddressLength) if (octet[i] != other.octet[i]) return false;
			return true;
		}

		bool isZero() const {
			for0(i, IPv6AddressLength) if (octet[i]) return false;
			return true;
		}

		bool isMulticast() const {
			return octet[0] == 0xFFu;
		}

		uint16 getGroup(stduint index) const {
			if (index >= 8) return 0;
			return (uint16(octet[index * 2]) << 8) | uint16(octet[index * 2 + 1]);
		}
	};

	inline void IPv4CopyAddress(IPv4Address& destination, const uint8* source) {
		for0(i, IPv4AddressLength) destination.octet[i] = source[i];
	}

	inline void IPv4WriteAddress(uint8* destination, const IPv4Address& source) {
		for0(i, IPv4AddressLength) destination[i] = source.octet[i];
	}

}
}

#endif


// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [System.Network.Layer.Application] Socket
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

#ifndef _INCPP_System_NETWORK_LAYER_APPLICATION_SOCKET
#define _INCPP_System_NETWORK_LAYER_APPLICATION_SOCKET

#include "../../../Network.hpp"

namespace uni {
namespace Network {

	enum class SocketDomain : uint16 {
		Unspec = 0,
		IPv4 = 2,
		IPv6 = 10,
		Link = 17,
	};

	enum class SocketType : uint16 {
		Stream = 1,
		Datagram = 2,
		Raw = 3,
	};

	enum class SocketProtocol : uint16 {
		Default = 0,
		ICMPv4 = 1,
		TCP = 6,
		UDP = 17,
		ICMPv6 = 58,
	};

	enum class SocketShutdown : uint8 {
		Receive = 0,
		Send = 1,
		Both = 2,
	};

	struct SocketAddress {
		uint16 domain;
		uint16 length;
	};

	struct SocketAddressIPv4 {
		SocketAddress head;
		IPv4Address address;
		uint16 port;
	};

	struct SocketAddressIPv6 {
		SocketAddress head;
		IPv6Address address;
		uint16 port;
	};

	struct SocketEndpointIPv4 {
		IPv4Address address;
		uint16 port;

		bool operator==(const SocketEndpointIPv4& other) const {
			return address == other.address && port == other.port;
		}
	};

	struct SocketEndpointIPv6 {
		IPv6Address address;
		uint16 port;

		bool operator==(const SocketEndpointIPv6& other) const {
			return address == other.address && port == other.port;
		}
	};

	inline void SocketWriteAddress(SocketAddress& address, SocketDomain domain, uint16 length) {
		address.domain = uint16(domain);
		address.length = length;
	}

	inline void SocketWriteAddress(SocketAddressIPv4& address, const IPv4Address& ip, uint16 port) {
		SocketWriteAddress(address.head, SocketDomain::IPv4, uint16(sizeof(SocketAddressIPv4)));
		address.address = ip;
		address.port = port;
	}

	inline void SocketWriteAddress(SocketAddressIPv6& address, const IPv6Address& ip, uint16 port) {
		SocketWriteAddress(address.head, SocketDomain::IPv6, uint16(sizeof(SocketAddressIPv6)));
		address.address = ip;
		address.port = port;
	}

}
}

#endif
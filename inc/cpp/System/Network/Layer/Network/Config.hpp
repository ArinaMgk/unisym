// ASCII CPP-ISO11 TAB4 LF
// Docutitle: [System.Network.Layer.Network] Config
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
// Not a Network Instance!

#ifndef _INCPP_System_NETWORK_LAYER_NETWORK_CONFIG
#define _INCPP_System_NETWORK_LAYER_NETWORK_CONFIG

#include "../Network.hpp"

namespace uni {
namespace Network {

	enum class NetworkConfigSource : uint16 {
		None = 0,
		Static = 1,
		DHCP = 2,
		Temporary = 3,
		Failed = 4,
	};

	struct IPv4InterfaceConfig {
		IPv4Address address;
		IPv4Address netmask;
		IPv4Address gateway;
		IPv4Address dns;
		NetworkConfigSource source;
		uint32 lease_seconds;

		bool hasAddress() const {
			return !address.isZero();
		}

		bool hasGateway() const {
			return !gateway.isZero();
		}

		bool hasDNS() const {
			return !dns.isZero();
		}
	};

	struct IPv4RouteConfig {
		IPv4Address destination;
		IPv4Address netmask;
		IPv4Address gateway;
		uint16 interface_index;
		bool up;

		bool isDefault() const {
			return destination.isZero() && netmask.isZero();
		}
	};

	struct NetworkConfigSnapshot {
		IPv4InterfaceConfig ipv4;
		IPv4RouteConfig default_route;
		uint16 interface_index;
		bool link_up;
	};

	class NetworkConfigInterface {
	public:
		virtual ~NetworkConfigInterface() = default;
		virtual stdsint ReadConfig(NetworkConfigSnapshot& config) const = 0;
		virtual stdsint ApplyConfig(const NetworkConfigSnapshot& config) = 0;
	};

}
}

#endif

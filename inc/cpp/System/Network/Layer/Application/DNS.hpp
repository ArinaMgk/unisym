// ASCII CPP-ISO11 TAB4 LF
// Docutitle: [System.Network.Layer.Application] DNS
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

#ifndef _INCPP_System_NETWORK_LAYER_APPLICATION_DNS
#define _INCPP_System_NETWORK_LAYER_APPLICATION_DNS

#include "../Application.hpp"
#include "../Network/IPv4.hpp"

namespace uni {
namespace Network {

	constexpr stduint DNSNameCapacity = 64;
	constexpr stduint DNSStatusTextCapacity = 24;
	constexpr stduint DNSAddressCapacity = 4;

	enum class DNSRecordType : uint16 {
		A = 1,
		CNAME = 5,
		AAAA = 28,
	};

	enum class DNSRecordClass : uint16 {
		IN = 1,
	};

	enum class DNSResolveStatus : uint8 {
		None,
		OK,
		Cached,
		Numeric,
		NoDNSServer,
		Timeout,
		NXDomain,
		ServFail,
		NoAddress,
		BadName,
		BadReply,
		Unsupported,
	};

	struct DNSQuery {
		const char* host;
		DNSRecordType type;
	};

	struct DNSIPv4Result {
		IPv4Address address;
		IPv4Address addresses[DNSAddressCapacity];
		stduint address_count;
		stduint answer_count;
		stduint cname_count;
		stduint non_address_count;
		uint32 ttl;
		DNSResolveStatus status;

		bool hasAddress() const {
			return address_count != 0;
		}
	};

	struct DNSIPv6Result {
		IPv6Address address;
		IPv6Address addresses[DNSAddressCapacity];
		stduint address_count;
		stduint answer_count;
		stduint cname_count;
		stduint non_address_count;
		uint32 ttl;
		DNSResolveStatus status;

		bool hasAddress() const {
			return address_count != 0;
		}
	};

	struct DNSAddressInfo {
		DNSRecordType type;
		uint16 port;
		union {
			IPv4Address ipv4;
			IPv6Address ipv6;
		};
	};

	struct DNSAddressList {
		DNSAddressInfo addresses[DNSAddressCapacity];
		stduint address_count;
		DNSResolveStatus status;
		uint32 ttl;

		bool hasAddress() const {
			return address_count != 0;
		}
	};

	struct DNSCacheEntryIPv4 {
		char host[DNSNameCapacity];
		char status[DNSStatusTextCapacity];
		IPv4Address address;
		IPv4Address addresses[DNSAddressCapacity];
		stduint address_count;
		stduint answer_count;
		uint32 ttl;
		bool negative;
	};

	struct DNSCacheEntryIPv6 {
		char host[DNSNameCapacity];
		char status[DNSStatusTextCapacity];
		IPv6Address address;
		IPv6Address addresses[DNSAddressCapacity];
		stduint address_count;
		stduint answer_count;
		uint32 ttl;
		bool negative;
	};

	class DNSResolverInterface {
	public:
		virtual ~DNSResolverInterface() = default;
		virtual stdsint ResolveIPv4(const char* host, DNSIPv4Result& result) = 0;
		virtual stdsint ResolveIPv6(const char* host, DNSIPv6Result& result) = 0;
		virtual stdsint ResolveAll(const char* host, DNSAddressList& result) = 0;
		virtual stdsint ClearCache() = 0;
	};

	inline void DNSClearIPv4Result(DNSIPv4Result& result, DNSResolveStatus status = DNSResolveStatus::None) {
		result.address = {};
		for0(i, DNSAddressCapacity) result.addresses[i] = {};
		result.address_count = 0;
		result.answer_count = 0;
		result.cname_count = 0;
		result.non_address_count = 0;
		result.ttl = 0;
		result.status = status;
	}

	inline bool DNSAppendIPv4Address(DNSIPv4Result& result, const IPv4Address& address) {
		if (address.isZero()) return false;
		for0(i, result.address_count) {
			if (result.addresses[i] == address) return false;
		}
		if (result.address_count >= DNSAddressCapacity) return false;
		result.addresses[result.address_count++] = address;
		if (result.address_count == 1) result.address = address;
		return true;
	}

	inline void DNSClearAddressList(DNSAddressList& result, DNSResolveStatus status = DNSResolveStatus::None) {
		for0(i, DNSAddressCapacity) result.addresses[i] = {};
		result.address_count = 0;
		result.status = status;
		result.ttl = 0;
	}

	inline const char* DNSStatusName(DNSResolveStatus status) {
		switch (status) {
		case DNSResolveStatus::OK: return "ok";
		case DNSResolveStatus::Cached: return "cached";
		case DNSResolveStatus::Numeric: return "numeric";
		case DNSResolveStatus::NoDNSServer: return "no-dns-server";
		case DNSResolveStatus::Timeout: return "timeout";
		case DNSResolveStatus::NXDomain: return "nxdomain";
		case DNSResolveStatus::ServFail: return "servfail";
		case DNSResolveStatus::NoAddress: return "no-address";
		case DNSResolveStatus::BadName: return "bad-name";
		case DNSResolveStatus::BadReply: return "bad-reply";
		case DNSResolveStatus::Unsupported: return "unsupported";
		default: return "none";
		}
	}

}
}

#endif

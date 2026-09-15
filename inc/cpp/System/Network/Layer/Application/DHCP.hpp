// ASCII CPP-ISO11 TAB4 LF
// Docutitle: [System.Network.Layer.Application] DHCP
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

#ifndef _INCPP_System_NETWORK_LAYER_APPLICATION_DHCP
#define _INCPP_System_NETWORK_LAYER_APPLICATION_DHCP

#include "../Application.hpp"
#include "../Link/Ethernet.hpp"
#include "../Network/IPv4.hpp"

namespace uni {
namespace Network {

	constexpr uint16 DHCPServerPort = 67;
	constexpr uint16 DHCPClientPort = 68;
	constexpr stduint DHCPFixedHeaderLength = 236;
	constexpr stduint DHCPMagicCookieOffset = 236;
	constexpr stduint DHCPOptionsOffset = 240;
	constexpr stduint DHCPMinMessageLength = 240;
	constexpr stduint DHCPDiscoverMinLength = 244;
	constexpr stduint DHCPRequestMinLength = 256;
	constexpr uint32 DHCPMagicCookie = 0x63825363u;

	enum class DHCPMessageType : uint8 {
		Discover = 1,
		Offer = 2,
		Request = 3,
		Decline = 4,
		Ack = 5,
		Nak = 6,
		Release = 7,
		Inform = 8,
	};

	enum class DHCPOption : uint8 {
		Pad = 0,
		SubnetMask = 1,
		Router = 3,
		DomainNameServer = 6,
		RequestedAddress = 50,
		LeaseTime = 51,
		MessageType = 53,
		ServerIdentifier = 54,
		End = 255,
	};

	_PACKED(struct) DHCPHeader {
		uint8 op;
		uint8 htype;
		uint8 hlen;
		uint8 hops;
		uint8 xid[4];
		uint8 secs[2];
		uint8 flags[2];
		uint8 ciaddr[IPv4AddressLength];
		uint8 yiaddr[IPv4AddressLength];
		uint8 siaddr[IPv4AddressLength];
		uint8 giaddr[IPv4AddressLength];
		uint8 chaddr[16];
		uint8 sname[64];
		uint8 file[128];
		uint8 cookie[4];
	};

	struct DHCPOptionView {
		uint8 code;
		const uint8* data;
		stduint length;
	};

	struct DHCPMessageView {
		const DHCPHeader* header;
		const uint8* options;
		stduint options_length;
		uint32 xid;
		IPv4Address client_address;
		IPv4Address your_address;
		IPv4Address server_address;
		IPv4Address relay_address;
	};

	struct DHCPClientConfig {
		IPv4Address address;
		IPv4Address netmask;
		IPv4Address router;
		IPv4Address server;
		IPv4Address dns;
		uint32 lease_time;
		DHCPMessageType message_type;
		bool has_address;
		bool has_netmask;
		bool has_router;
		bool has_server;
		bool has_dns;
		bool has_lease_time;
	};

	enum class DHCPClientState : uint8 {
		Init,
		Discovering,
		Offered,
		Requesting,
		Bound,
		Nak,
	};

	inline void DHCPWrite32(uint8* data, uint32 value) {
		data[0] = uint8(value >> 24);
		data[1] = uint8(value >> 16);
		data[2] = uint8(value >> 8);
		data[3] = uint8(value);
	}

	inline uint32 DHCPRead32(const uint8* data) {
		return (uint32(data[0]) << 24) | (uint32(data[1]) << 16) |
			(uint32(data[2]) << 8) | uint32(data[3]);
	}

	inline void DHCPWriteMagicCookie(uint8* data) {
		DHCPWrite32(data, DHCPMagicCookie);
	}

	inline bool DHCPHasMagicCookie(const uint8* data) {
		return data && DHCPRead32(data) == DHCPMagicCookie;
	}

	inline bool ParseDHCPMessage(const uint8* payload, stduint length, DHCPMessageView& message) {
		if (!payload || length < DHCPMinMessageLength) return false;
		const auto* header = reinterpret_cast<const DHCPHeader*>(payload);
		if (!DHCPHasMagicCookie(header->cookie)) return false;
		message.header = header;
		message.options = payload + DHCPOptionsOffset;
		message.options_length = length - DHCPOptionsOffset;
		message.xid = DHCPRead32(header->xid);
		IPv4CopyAddress(message.client_address, header->ciaddr);
		IPv4CopyAddress(message.your_address, header->yiaddr);
		IPv4CopyAddress(message.server_address, header->siaddr);
		IPv4CopyAddress(message.relay_address, header->giaddr);
		return true;
	}

	inline bool DHCPFindOption(const DHCPMessageView& message, DHCPOption option, DHCPOptionView& view) {
		stduint index = 0;
		while (index < message.options_length) {
			const uint8 code = message.options[index++];
			if (code == uint8(DHCPOption::Pad)) continue;
			if (code == uint8(DHCPOption::End)) break;
			if (index >= message.options_length) break;
			const uint8 length = message.options[index++];
			if (index + length > message.options_length) break;
			if (code == uint8(option)) {
				view.code = code;
				view.data = message.options + index;
				view.length = length;
				return true;
			}
			index += length;
		}
		return false;
	}

	inline bool DHCPReadMessageType(const DHCPMessageView& message, DHCPMessageType& type) {
		DHCPOptionView option{};
		if (!DHCPFindOption(message, DHCPOption::MessageType, option) || option.length != 1) return false;
		type = DHCPMessageType(option.data[0]);
		return true;
	}

	inline bool DHCPReadIPv4Option(const DHCPMessageView& message, DHCPOption option, IPv4Address& address) {
		DHCPOptionView view{};
		if (!DHCPFindOption(message, option, view) || view.length < IPv4AddressLength) return false;
		IPv4CopyAddress(address, view.data);
		return true;
	}

	inline bool DHCPReadU32Option(const DHCPMessageView& message, DHCPOption option, uint32& value) {
		DHCPOptionView view{};
		if (!DHCPFindOption(message, option, view) || view.length != 4) return false;
		value = DHCPRead32(view.data);
		return true;
	}

	inline bool DHCPReadClientConfig(const DHCPMessageView& message, DHCPClientConfig& config) {
		config = {};
		config.address = message.your_address;
		config.has_address = !config.address.isZero();
		if (!DHCPReadMessageType(message, config.message_type)) return false;
		config.has_netmask = DHCPReadIPv4Option(message, DHCPOption::SubnetMask, config.netmask);
		config.has_router = DHCPReadIPv4Option(message, DHCPOption::Router, config.router);
		config.has_server = DHCPReadIPv4Option(message, DHCPOption::ServerIdentifier, config.server);
		config.has_dns = DHCPReadIPv4Option(message, DHCPOption::DomainNameServer, config.dns);
		config.has_lease_time = DHCPReadU32Option(message, DHCPOption::LeaseTime, config.lease_time);
		return true;
	}

	inline uint8* DHCPAppendOption(uint8* cursor, uint8 code, const void* data, uint8 length) {
		*cursor++ = code;
		*cursor++ = length;
		const auto* bytes = reinterpret_cast<const uint8*>(data);
		for0(i, length) *cursor++ = bytes[i];
		return cursor;
	}

	inline uint8* DHCPAppendMessageType(uint8* cursor, DHCPMessageType type) {
		const uint8 value = uint8(type);
		return DHCPAppendOption(cursor, uint8(DHCPOption::MessageType), &value, 1);
	}

	inline uint8* DHCPAppendIPv4Option(uint8* cursor, DHCPOption option, const IPv4Address& address) {
		return DHCPAppendOption(cursor, uint8(option), address.octet, IPv4AddressLength);
	}

	inline stduint BuildDHCPDiscover(uint8* buffer, stduint capacity, const MacAddress& mac, uint32 xid) {
		if (!buffer || capacity < DHCPDiscoverMinLength || mac.isZero()) return 0;
		for0(i, capacity) buffer[i] = 0;
		auto* header = reinterpret_cast<DHCPHeader*>(buffer);
		header->op = 1;
		header->htype = 1;
		header->hlen = EthernetAddressLength;
		DHCPWrite32(header->xid, xid);
		EthernetWrite16(header->flags, 0x8000u);
		for0(i, EthernetAddressLength) header->chaddr[i] = mac.octet[i];
		DHCPWriteMagicCookie(header->cookie);
		auto* option = buffer + DHCPOptionsOffset;
		option = DHCPAppendMessageType(option, DHCPMessageType::Discover);
		*option++ = uint8(DHCPOption::End);
		return stduint(option - buffer);
	}

	inline stduint BuildDHCPRequest(uint8* buffer, stduint capacity, const MacAddress& mac,
		uint32 xid, const IPv4Address& requested_address, const IPv4Address& server_identifier) {
		if (!buffer || capacity < DHCPRequestMinLength || mac.isZero() ||
			requested_address.isZero() || server_identifier.isZero()) return 0;
		for0(i, capacity) buffer[i] = 0;
		auto* header = reinterpret_cast<DHCPHeader*>(buffer);
		header->op = 1;
		header->htype = 1;
		header->hlen = EthernetAddressLength;
		DHCPWrite32(header->xid, xid);
		EthernetWrite16(header->flags, 0x8000u);
		for0(i, EthernetAddressLength) header->chaddr[i] = mac.octet[i];
		DHCPWriteMagicCookie(header->cookie);
		auto* option = buffer + DHCPOptionsOffset;
		option = DHCPAppendMessageType(option, DHCPMessageType::Request);
		option = DHCPAppendIPv4Option(option, DHCPOption::RequestedAddress, requested_address);
		option = DHCPAppendIPv4Option(option, DHCPOption::ServerIdentifier, server_identifier);
		*option++ = uint8(DHCPOption::End);
		return stduint(option - buffer);
	}

	class DHCPClientObject {
	public:
		DHCPClientObject() {
			Reset();
		}

		void Reset() {
			xid_ = 0;
			state_ = DHCPClientState::Init;
			offer_ = {};
			bound_ = {};
		}

		void Begin(uint32 xid) {
			xid_ = xid;
			state_ = DHCPClientState::Discovering;
			offer_ = {};
			bound_ = {};
		}

		DHCPClientState getState() const {
			return state_;
		}

		uint32 getTransactionId() const {
			return xid_;
		}

		const DHCPClientConfig& getOffer() const {
			return offer_;
		}

		const DHCPClientConfig& getBoundConfig() const {
			return bound_;
		}

		bool hasOffer() const {
			return state_ == DHCPClientState::Offered || state_ == DHCPClientState::Requesting ||
				state_ == DHCPClientState::Bound;
		}

		bool isBound() const {
			return state_ == DHCPClientState::Bound;
		}

		stduint BuildDiscover(uint8* buffer, stduint capacity, const MacAddress& mac) const {
			if (!xid_) return 0;
			return BuildDHCPDiscover(buffer, capacity, mac, xid_);
		}

		stduint BuildRequest(uint8* buffer, stduint capacity, const MacAddress& mac) {
			if (!hasOffer() || !offer_.has_address || !offer_.has_server) return 0;
			const stduint length = BuildDHCPRequest(buffer, capacity, mac,
				xid_, offer_.address, offer_.server);
			if (length) state_ = DHCPClientState::Requesting;
			return length;
		}

		bool AcceptMessage(const uint8* payload, stduint length, DHCPClientConfig& config) {
			DHCPMessageView message{};
			if (!ParseDHCPMessage(payload, length, message)) return false;
			if (xid_ && message.xid != xid_) return false;
			if (!DHCPReadClientConfig(message, config)) return false;
			switch (config.message_type) {
			case DHCPMessageType::Offer:
				offer_ = config;
				state_ = DHCPClientState::Offered;
				return true;
			case DHCPMessageType::Ack:
				bound_ = config;
				state_ = DHCPClientState::Bound;
				return true;
			case DHCPMessageType::Nak:
				state_ = DHCPClientState::Nak;
				return true;
			default:
				return true;
			}
		}

	protected:
		uint32 xid_;
		DHCPClientState state_;
		DHCPClientConfig offer_;
		DHCPClientConfig bound_;
	};

}
}

#endif

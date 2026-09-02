// ASCII CPP-ISO11 TAB4 LF
// Docutitle: [System.Network.Layer.Transport] UDP
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

#ifndef _INCPP_System_NETWORK_LAYER_TRANSPORT_UDP
#define _INCPP_System_NETWORK_LAYER_TRANSPORT_UDP

#include "../Transport.hpp"
#include "../Network/IPv4.hpp"

namespace uni {
namespace Network {

	constexpr stduint UDPHeaderLength = 8;

	_PACKED(struct) UDPHeader {
		uint8 source_port[2];
		uint8 destination_port[2];
		uint8 length[2];
		uint8 checksum[2];
	};

	struct UDPDatagramView {
		const UDPHeader* header;
		uint16 source_port;
		uint16 destination_port;
		uint16 length;
		uint16 checksum;
		const uint8* payload;
		stduint payload_length;
	};

	struct UDPEndpoint {
		IPv4Address address;
		uint16 port;
	};

	struct UDPDatagramContext {
		UDPEndpoint source;
		UDPEndpoint destination;
		const uint8* payload;
		stduint payload_length;
	};

	class UDPInterface : public TransportInterface {
	public:
		TransportProtocol getProtocol() const override {
			return TransportProtocol::UDP;
		}
	};

	class UDPObject : public UDPInterface {
	public:
		explicit UDPObject(NetworkInterface* network = nullptr,
			void* packet_buffer = nullptr, stduint packet_capacity = 0);

		NetworkInterface* getNetwork() const override;
		void setNetwork(NetworkInterface* network);
		void setPacketBuffer(void* packet_buffer, stduint packet_capacity);
		void setIdentification(uint16 identification);
		bool isBound() const;
		bool isConnected() const;
		stdsint Bind(const TransportEndpoint& local) override;
		stdsint Connect(const TransportEndpoint& remote) override;
		stdsint Listen(stduint backlog) override;
		stdsint Accept(TransportConnectionContext& connection) override;
		stdsint BuildPacket(NetworkPacketContext& packet, const TransportPayloadContext& payload);
		stdsint Send(const TransportPayloadContext& payload) override;
		stdsint Receive(TransportMutablePayloadContext& payload) override;
		stdsint Control(stduint command, void* args) override;

	protected:
		NetworkInterface* network_;
		uint8* packet_buffer_;
		stduint packet_capacity_;
		TransportEndpoint local_;
		TransportEndpoint remote_;
		uint16 identification_;
		bool bound_;
		bool connected_;
	};

	inline bool ParseUDPDatagram(const IPv4PacketView& ipv4, UDPDatagramView& datagram) {
		if (ipv4.protocol != uint8(IPv4Protocol::UDP)) return false;
		if (!ipv4.payload || ipv4.payload_length < UDPHeaderLength) return false;
		const auto* header = reinterpret_cast<const UDPHeader*>(ipv4.payload);
		const uint16 length = EthernetRead16(header->length);
		if (length < UDPHeaderLength || ipv4.payload_length < length) return false;
		datagram.header = header;
		datagram.source_port = EthernetRead16(header->source_port);
		datagram.destination_port = EthernetRead16(header->destination_port);
		datagram.length = length;
		datagram.checksum = EthernetRead16(header->checksum);
		datagram.payload = ipv4.payload + UDPHeaderLength;
		datagram.payload_length = length - UDPHeaderLength;
		return true;
	}

	inline void BuildUDPHeader(UDPHeader& header, uint16 source_port, uint16 destination_port, uint16 length, uint16 checksum = 0) {
		EthernetWrite16(header.source_port, source_port);
		EthernetWrite16(header.destination_port, destination_port);
		EthernetWrite16(header.length, length);
		EthernetWrite16(header.checksum, checksum);
	}

	inline void UDPChecksumAddBytes(uint32& sum, const uint8* bytes, stduint length) {
		for (; length > 1; length -= 2, bytes += 2) {
			sum += (uint16(bytes[0]) << 8) | uint16(bytes[1]);
		}
		if (length) sum += uint16(bytes[0]) << 8;
	}

	inline uint16 UDPChecksumFold(uint32 sum) {
		while (sum >> 16) sum = (sum & 0xFFFFu) + (sum >> 16);
		const uint16 checksum = uint16(~sum);
		return checksum ? checksum : 0xFFFFu;
	}

	inline uint16 UDPIPv4Checksum(const IPv4Address& source, const IPv4Address& destination,
		const void* udp_packet, stduint udp_length) {
		uint32 sum = 0;
		UDPChecksumAddBytes(sum, source.octet, IPv4AddressLength);
		UDPChecksumAddBytes(sum, destination.octet, IPv4AddressLength);
		sum += uint16(IPv4Protocol::UDP);
		sum += uint16(udp_length);
		UDPChecksumAddBytes(sum, reinterpret_cast<const uint8*>(udp_packet), udp_length);
		return UDPChecksumFold(sum);
	}

	inline bool ValidateUDPIPv4Checksum(const IPv4PacketView& ipv4, const UDPDatagramView& datagram) {
		if (datagram.checksum == 0) return true;
		return UDPIPv4Checksum(ipv4.source, ipv4.destination, datagram.header, datagram.length) == 0xFFFFu;
	}

}
}

#endif

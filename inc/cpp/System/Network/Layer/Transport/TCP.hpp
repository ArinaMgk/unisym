// ASCII CPP-ISO11 TAB4 LF
// Docutitle: [System.Network.Layer.Transport] TCP
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

#ifndef _INCPP_System_NETWORK_LAYER_TRANSPORT_TCP
#define _INCPP_System_NETWORK_LAYER_TRANSPORT_TCP

#include "../Transport.hpp"
#include "../Network/IPv4.hpp"

namespace uni {
namespace Network {

	constexpr stduint TCPMinHeaderLength = 20;

	enum TCPFlag : uint8 {
		TCPFlagFIN = 0x01u,
		TCPFlagSYN = 0x02u,
		TCPFlagRST = 0x04u,
		TCPFlagPSH = 0x08u,
		TCPFlagACK = 0x10u,
		TCPFlagURG = 0x20u,
		TCPFlagECE = 0x40u,
		TCPFlagCWR = 0x80u,
	};

	_PACKED(struct) TCPHeader {
		uint8 source_port[2];
		uint8 destination_port[2];
		uint8 sequence[4];
		uint8 acknowledgment[4];
		uint8 data_offset_reserved;
		uint8 flags;
		uint8 window[2];
		uint8 checksum[2];
		uint8 urgent_pointer[2];
	};

	struct TCPSegmentView {
		const TCPHeader* header;
		uint16 source_port;
		uint16 destination_port;
		uint32 sequence;
		uint32 acknowledgment;
		uint8 header_length;
		uint8 flags;
		uint16 window;
		uint16 checksum;
		uint16 urgent_pointer;
		const uint8* payload;
		stduint payload_length;
	};

	struct TCPEndpoint {
		IPv4Address address;
		uint16 port;
	};

	struct TCPConnectionContext {
		TCPEndpoint local;
		TCPEndpoint remote;
	};

	enum class TCPConnectionState : uint8 {
		SynReceived,
		Established,
		CloseWait,
	};

	struct TCPConnectionControlBlock {
		TCPConnectionContext context;
		uint32 local_next_sequence;
		uint32 remote_next_sequence;
		TCPConnectionState state;
	};

	class TCPInterface : public TransportInterface {
	public:
		TransportProtocol getProtocol() const override {
			return TransportProtocol::TCP;
		}
	};

	class TCPObject : public TCPInterface {
	public:
		explicit TCPObject(NetworkInterface* network = nullptr,
			void* packet_buffer = nullptr, stduint packet_capacity = 0);

		NetworkInterface* getNetwork() const override;
		void Reset(NetworkInterface* network = nullptr,
			void* packet_buffer = nullptr, stduint packet_capacity = 0);
		void setNetwork(NetworkInterface* network);
		void setPacketBuffer(void* packet_buffer, stduint packet_capacity);
		void setAcceptQueue(TCPConnectionContext* pending, stduint capacity);
		void setIdentification(uint16 identification);
		void setWindow(uint16 window);
		TCPConnectionControlBlock& getControl();
		const TCPConnectionControlBlock& getControl() const;
		stduint getPendingAcceptCount() const;
		bool isBound() const;
		bool isConnected() const;
		bool isListening() const;
		stdsint Bind(const TransportEndpoint& local) override;
		stdsint Connect(const TransportEndpoint& remote) override;
		stdsint Listen(stduint backlog) override;
		stdsint Accept(TransportConnectionContext& connection) override;
		stdsint Send(const TransportPayloadContext& payload) override;
		stdsint Receive(TransportMutablePayloadContext& payload) override;
		stdsint Control(stduint command, void* args) override;
		void BeginPassiveConnection(const IPv4Address& local_ip, uint16 local_port,
			const IPv4Address& remote_ip, uint16 remote_port,
			const TCPSegmentView& segment, uint32 initial_sequence);
		bool AcceptHandshakeAck(const TCPSegmentView& segment);
		bool EnqueueAccept(const TCPConnectionContext& connection);
		bool Accept(TCPConnectionContext& connection);
		bool isExpectedSegment(const TCPSegmentView& segment) const;
		bool ConsumeExpectedSegment(const TCPSegmentView& segment);

	protected:
		NetworkInterface* network_;
		uint8* packet_buffer_;
		stduint packet_capacity_;
		TCPConnectionContext* accept_pending_;
		stduint accept_capacity_;
		stduint accept_head_;
		stduint accept_tail_;
		stduint accept_count_;
		TCPConnectionControlBlock control_;
		stduint backlog_;
		uint16 identification_;
		uint16 window_;
		bool bound_;
		bool connected_;
		bool listening_;
	};

	uint32 TCPSequenceLength(const TCPSegmentView& segment);
	void TCPBeginPassiveConnection(TCPConnectionControlBlock& connection,
		const IPv4Address& local_ip, uint16 local_port,
		const IPv4Address& remote_ip, uint16 remote_port,
		const TCPSegmentView& segment, uint32 initial_sequence);
	bool TCPAcceptHandshakeAck(TCPConnectionControlBlock& connection, const TCPSegmentView& segment);
	bool TCPIsExpectedSegment(const TCPConnectionControlBlock& connection, const TCPSegmentView& segment);
	uint32 TCPExpectedAcknowledge(const TCPConnectionControlBlock& connection, const TCPSegmentView& segment);
	bool TCPConsumeExpectedSegment(TCPConnectionControlBlock& connection, const TCPSegmentView& segment);

	inline uint32 TCPRead32(const uint8* data) {
		return (uint32(data[0]) << 24) | (uint32(data[1]) << 16) |
			(uint32(data[2]) << 8) | uint32(data[3]);
	}

	inline void TCPWrite32(uint8* data, uint32 value) {
		data[0] = uint8(value >> 24);
		data[1] = uint8(value >> 16);
		data[2] = uint8(value >> 8);
		data[3] = uint8(value);
	}

	inline bool ParseTCPSegment(const IPv4PacketView& ipv4, TCPSegmentView& segment) {
		if (ipv4.protocol != uint8(IPv4Protocol::TCP)) return false;
		if (!ipv4.payload || ipv4.payload_length < TCPMinHeaderLength) return false;
		const auto* header = reinterpret_cast<const TCPHeader*>(ipv4.payload);
		const uint8 header_length = uint8((header->data_offset_reserved >> 4) * 4);
		if (header_length < TCPMinHeaderLength || ipv4.payload_length < header_length) return false;
		segment.header = header;
		segment.source_port = EthernetRead16(header->source_port);
		segment.destination_port = EthernetRead16(header->destination_port);
		segment.sequence = TCPRead32(header->sequence);
		segment.acknowledgment = TCPRead32(header->acknowledgment);
		segment.header_length = header_length;
		segment.flags = header->flags;
		segment.window = EthernetRead16(header->window);
		segment.checksum = EthernetRead16(header->checksum);
		segment.urgent_pointer = EthernetRead16(header->urgent_pointer);
		segment.payload = ipv4.payload + header_length;
		segment.payload_length = ipv4.payload_length - header_length;
		return true;
	}

	inline void BuildTCPHeader(TCPHeader& header, uint16 source_port, uint16 destination_port,
		uint32 sequence, uint32 acknowledgment, uint8 flags, uint16 window,
		uint16 urgent_pointer = 0, uint16 checksum = 0, uint8 header_length = TCPMinHeaderLength) {
		EthernetWrite16(header.source_port, source_port);
		EthernetWrite16(header.destination_port, destination_port);
		TCPWrite32(header.sequence, sequence);
		TCPWrite32(header.acknowledgment, acknowledgment);
		header.data_offset_reserved = uint8((header_length / 4) << 4);
		header.flags = flags;
		EthernetWrite16(header.window, window);
		EthernetWrite16(header.checksum, checksum);
		EthernetWrite16(header.urgent_pointer, urgent_pointer);
	}

	inline void TCPChecksumAddBytes(uint32& sum, const uint8* bytes, stduint length) {
		for (; length > 1; length -= 2, bytes += 2) {
			sum += (uint16(bytes[0]) << 8) | uint16(bytes[1]);
		}
		if (length) sum += uint16(bytes[0]) << 8;
	}

	inline uint16 TCPChecksumFold(uint32 sum) {
		while (sum >> 16) sum = (sum & 0xFFFFu) + (sum >> 16);
		return uint16(~sum);
	}

	inline uint16 TCPIPv4Checksum(const IPv4Address& source, const IPv4Address& destination,
		const void* tcp_packet, stduint tcp_length) {
		uint32 sum = 0;
		TCPChecksumAddBytes(sum, source.octet, IPv4AddressLength);
		TCPChecksumAddBytes(sum, destination.octet, IPv4AddressLength);
		sum += uint16(IPv4Protocol::TCP);
		sum += uint16(tcp_length);
		TCPChecksumAddBytes(sum, reinterpret_cast<const uint8*>(tcp_packet), tcp_length);
		return TCPChecksumFold(sum);
	}

	inline bool ValidateTCPIPv4Checksum(const IPv4PacketView& ipv4, const TCPSegmentView& segment) {
		return TCPIPv4Checksum(ipv4.source, ipv4.destination, segment.header, ipv4.payload_length) == 0;
	}

	inline stduint BuildTCPIPv4Reset(uint8* buffer, stduint capacity,
		const MacAddress& local_mac, const MacAddress& target_mac,
		const IPv4Address& local_ip, const IPv4PacketView& request,
		const TCPSegmentView& segment, uint16 identification) {
		constexpr stduint tcp_length = TCPMinHeaderLength;
		constexpr stduint ipv4_length = IPv4MinHeaderLength + tcp_length;
		constexpr stduint frame_length = EthernetHeaderLength + ipv4_length;
		if (!buffer || capacity < frame_length) return 0;

		auto* ethernet = reinterpret_cast<EthernetHeader*>(buffer);
		EthernetWriteAddress(ethernet->destination, target_mac);
		EthernetWriteAddress(ethernet->source, local_mac);
		EthernetWrite16(ethernet->type, uint16(EthernetType::IPv4));

		auto* ipv4 = reinterpret_cast<IPv4Header*>(buffer + EthernetHeaderLength);
		BuildIPv4Header(*ipv4, local_ip, request.source, uint8(IPv4Protocol::TCP),
			uint16(ipv4_length), identification);

		uint32 sequence = 0;
		uint32 acknowledgment = 0;
		uint8 flags = TCPFlagRST;
		if (segment.flags & TCPFlagACK) {
			sequence = segment.acknowledgment;
		}
		else {
			flags = TCPFlagRST | TCPFlagACK;
			acknowledgment = segment.sequence + uint32(segment.payload_length);
			if (segment.flags & TCPFlagSYN) acknowledgment++;
			if (segment.flags & TCPFlagFIN) acknowledgment++;
		}

		auto* tcp = reinterpret_cast<TCPHeader*>(
			buffer + EthernetHeaderLength + IPv4MinHeaderLength);
		BuildTCPHeader(*tcp, segment.destination_port, segment.source_port,
			sequence, acknowledgment, flags, 0);
		const uint16 checksum = TCPIPv4Checksum(local_ip, request.source, tcp, tcp_length);
		EthernetWrite16(tcp->checksum, checksum);
		return frame_length;
	}

	inline stduint BuildTCPIPv4SynAck(uint8* buffer, stduint capacity,
		const MacAddress& local_mac, const MacAddress& target_mac,
		const IPv4Address& local_ip, const IPv4PacketView& request,
		const TCPSegmentView& segment, uint32 sequence, uint16 window, uint16 identification) {
		constexpr stduint tcp_length = TCPMinHeaderLength;
		constexpr stduint ipv4_length = IPv4MinHeaderLength + tcp_length;
		constexpr stduint frame_length = EthernetHeaderLength + ipv4_length;
		if (!buffer || capacity < frame_length) return 0;

		auto* ethernet = reinterpret_cast<EthernetHeader*>(buffer);
		EthernetWriteAddress(ethernet->destination, target_mac);
		EthernetWriteAddress(ethernet->source, local_mac);
		EthernetWrite16(ethernet->type, uint16(EthernetType::IPv4));

		auto* ipv4 = reinterpret_cast<IPv4Header*>(buffer + EthernetHeaderLength);
		BuildIPv4Header(*ipv4, local_ip, request.source, uint8(IPv4Protocol::TCP),
			uint16(ipv4_length), identification);

		auto* tcp = reinterpret_cast<TCPHeader*>(
			buffer + EthernetHeaderLength + IPv4MinHeaderLength);
		BuildTCPHeader(*tcp, segment.destination_port, segment.source_port,
			sequence, segment.sequence + 1, TCPFlagSYN | TCPFlagACK, window);
		const uint16 checksum = TCPIPv4Checksum(local_ip, request.source, tcp, tcp_length);
		EthernetWrite16(tcp->checksum, checksum);
		return frame_length;
	}

	inline stduint BuildTCPIPv4Ack(uint8* buffer, stduint capacity,
		const MacAddress& local_mac, const MacAddress& target_mac,
		const IPv4Address& local_ip, const IPv4PacketView& request,
		const TCPSegmentView& segment, uint32 sequence, uint32 acknowledgment,
		uint16 window, uint16 identification) {
		constexpr stduint tcp_length = TCPMinHeaderLength;
		constexpr stduint ipv4_length = IPv4MinHeaderLength + tcp_length;
		constexpr stduint frame_length = EthernetHeaderLength + ipv4_length;
		if (!buffer || capacity < frame_length) return 0;

		auto* ethernet = reinterpret_cast<EthernetHeader*>(buffer);
		EthernetWriteAddress(ethernet->destination, target_mac);
		EthernetWriteAddress(ethernet->source, local_mac);
		EthernetWrite16(ethernet->type, uint16(EthernetType::IPv4));

		auto* ipv4 = reinterpret_cast<IPv4Header*>(buffer + EthernetHeaderLength);
		BuildIPv4Header(*ipv4, local_ip, request.source, uint8(IPv4Protocol::TCP),
			uint16(ipv4_length), identification);

		auto* tcp = reinterpret_cast<TCPHeader*>(
			buffer + EthernetHeaderLength + IPv4MinHeaderLength);
		BuildTCPHeader(*tcp, segment.destination_port, segment.source_port,
			sequence, acknowledgment, TCPFlagACK, window);
		const uint16 checksum = TCPIPv4Checksum(local_ip, request.source, tcp, tcp_length);
		EthernetWrite16(tcp->checksum, checksum);
		return frame_length;
	}

}
}

#endif

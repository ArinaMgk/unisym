// ASCII CPP-ISO11 TAB4 LF
// Docutitle: [System.Network.Layer.Transport] UDP
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

#include "../../../../../../inc/cpp/System/Network/Layer/Transport/UDP.hpp"

namespace uni {
namespace Network {

	UDPObject::UDPObject(NetworkInterface* network, void* packet_buffer, stduint packet_capacity) :
		network_(network),
		packet_buffer_(reinterpret_cast<uint8*>(packet_buffer)),
		packet_capacity_(packet_capacity),
		local_(),
		remote_(),
		identification_(1),
		bound_(false),
		connected_(false) {}

	NetworkInterface* UDPObject::getNetwork() const {
		return network_;
	}

	void UDPObject::setNetwork(NetworkInterface* network) {
		network_ = network;
	}

	void UDPObject::setPacketBuffer(void* packet_buffer, stduint packet_capacity) {
		packet_buffer_ = reinterpret_cast<uint8*>(packet_buffer);
		packet_capacity_ = packet_capacity;
	}

	void UDPObject::setIdentification(uint16 identification) {
		identification_ = identification;
	}

	bool UDPObject::isBound() const {
		return bound_;
	}

	bool UDPObject::isConnected() const {
		return connected_;
	}

	stdsint UDPObject::Bind(const TransportEndpoint& local) {
		if (!local.port || local.address.isZero()) return -1;
		local_ = local;
		bound_ = true;
		return 0;
	}

	stdsint UDPObject::Connect(const TransportEndpoint& remote) {
		if (!remote.port || remote.address.isZero()) return -1;
		remote_ = remote;
		connected_ = true;
		return 0;
	}

	stdsint UDPObject::Listen(stduint backlog) {
		(void)backlog;
		return -1;
	}

	stdsint UDPObject::Accept(TransportConnectionContext& connection) {
		(void)connection;
		return -1;
	}

	stdsint UDPObject::BuildPacket(NetworkPacketContext& packet, const TransportPayloadContext& payload) {
		if (!network_ || !packet_buffer_ || (!payload.payload && payload.payload_length)) return -1;
		TransportEndpoint local = payload.connection.local.port ? payload.connection.local : local_;
		TransportEndpoint remote = payload.connection.remote.port ? payload.connection.remote : remote_;
		if (!local.port || !remote.port || remote.address.isZero()) return -1;
		if (local.address.isZero()) local.address = network_->getAddress();

		IPv4Address source{};
		IPv4Address destination{};
		if (!NetworkReadIPv4Address(local.address, source)) return -1;
		if (!NetworkReadIPv4Address(remote.address, destination)) return -1;

		const stduint udp_length = UDPHeaderLength + payload.payload_length;
		if (udp_length > packet_capacity_ || udp_length > network_->getPayloadMtu()) return -1;

		auto* udp = reinterpret_cast<UDPHeader*>(packet_buffer_);
		BuildUDPHeader(*udp, local.port, remote.port, uint16(udp_length));
		auto* data = packet_buffer_ + UDPHeaderLength;
		const auto* source_data = reinterpret_cast<const uint8*>(payload.payload);
		for0(i, payload.payload_length) data[i] = source_data[i];
		const uint16 checksum = UDPIPv4Checksum(source, destination, udp, udp_length);
		EthernetWrite16(udp->checksum, checksum);

		packet = {
			local.address,
			remote.address,
			uint8(IPv4Protocol::UDP),
			packet_buffer_,
			udp_length,
			identification_++,
			64,
		};
		return stdsint(payload.payload_length);
	}

	stdsint UDPObject::Send(const TransportPayloadContext& payload) {
		NetworkPacketContext packet{};
		const stdsint built = BuildPacket(packet, payload);
		if (built < 0) return built;
		const stdsint sent = network_->SendPacket(packet);
		return sent > 0 ? stdsint(payload.payload_length) : sent;
	}

	stdsint UDPObject::Receive(TransportMutablePayloadContext& payload) {
		(void)payload;
		return -1;
	}

	stdsint UDPObject::Control(stduint command, void* args) {
		(void)command;
		(void)args;
		return -1;
	}

}
}

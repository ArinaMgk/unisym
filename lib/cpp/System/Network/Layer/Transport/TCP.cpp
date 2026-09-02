// ASCII CPP-ISO11 TAB4 LF
// Docutitle: [System.Network.Layer.Transport] TCP
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

#include "../../../../../../inc/cpp/System/Network/Layer/Transport/TCP.hpp"

namespace uni {
namespace Network {

	TCPObject::TCPObject(NetworkInterface* network, void* packet_buffer, stduint packet_capacity) :
		network_(network),
		packet_buffer_(reinterpret_cast<uint8*>(packet_buffer)),
		packet_capacity_(packet_capacity),
		accept_pending_(nullptr),
		accept_capacity_(0),
		accept_head_(0),
		accept_tail_(0),
		accept_count_(0),
		control_(),
		backlog_(0),
		identification_(1),
		window_(0),
		bound_(false),
		connected_(false),
		listening_(false) {}

	NetworkInterface* TCPObject::getNetwork() const {
		return network_;
	}

	void TCPObject::Reset(NetworkInterface* network, void* packet_buffer, stduint packet_capacity) {
		auto* accept_pending = accept_pending_;
		const stduint accept_capacity = accept_capacity_;
		network_ = network;
		packet_buffer_ = reinterpret_cast<uint8*>(packet_buffer);
		packet_capacity_ = packet_capacity;
		accept_pending_ = accept_pending;
		accept_capacity_ = accept_capacity;
		accept_head_ = 0;
		accept_tail_ = 0;
		accept_count_ = 0;
		control_ = {};
		backlog_ = 0;
		identification_ = 1;
		window_ = 0;
		bound_ = false;
		connected_ = false;
		listening_ = false;
	}

	void TCPObject::setNetwork(NetworkInterface* network) {
		network_ = network;
	}

	void TCPObject::setPacketBuffer(void* packet_buffer, stduint packet_capacity) {
		packet_buffer_ = reinterpret_cast<uint8*>(packet_buffer);
		packet_capacity_ = packet_capacity;
	}

	void TCPObject::setAcceptQueue(TCPConnectionContext* pending, stduint capacity) {
		accept_pending_ = pending;
		accept_capacity_ = capacity;
		accept_head_ = 0;
		accept_tail_ = 0;
		accept_count_ = 0;
	}

	void TCPObject::setIdentification(uint16 identification) {
		identification_ = identification;
	}

	void TCPObject::setWindow(uint16 window) {
		window_ = window;
	}

	TCPConnectionControlBlock& TCPObject::getControl() {
		return control_;
	}

	const TCPConnectionControlBlock& TCPObject::getControl() const {
		return control_;
	}

	stduint TCPObject::getPendingAcceptCount() const {
		return accept_count_;
	}

	bool TCPObject::isBound() const {
		return bound_;
	}

	bool TCPObject::isConnected() const {
		return connected_;
	}

	bool TCPObject::isListening() const {
		return listening_;
	}

	stdsint TCPObject::Bind(const TransportEndpoint& local) {
		if (!local.port || local.address.isZero()) return -1;
		IPv4Address address{};
		if (!NetworkReadIPv4Address(local.address, address)) return -1;
		control_.context.local = { address, local.port };
		bound_ = true;
		return 0;
	}

	stdsint TCPObject::Connect(const TransportEndpoint& remote) {
		if (!remote.port || remote.address.isZero()) return -1;
		IPv4Address address{};
		if (!NetworkReadIPv4Address(remote.address, address)) return -1;
		control_.context.remote = { address, remote.port };
		connected_ = true;
		return 0;
	}

	stdsint TCPObject::Listen(stduint backlog) {
		backlog_ = backlog;
		listening_ = true;
		return 0;
	}

	stdsint TCPObject::Accept(TransportConnectionContext& connection) {
		TCPConnectionContext tcp_connection{};
		const stdsint accepted = Accept(tcp_connection);
		if (accepted <= 0) return accepted;
		connection.local = {
			NetworkAddressIPv4(tcp_connection.local.address),
			tcp_connection.local.port,
		};
		connection.remote = {
			NetworkAddressIPv4(tcp_connection.remote.address),
			tcp_connection.remote.port,
		};
		return accepted;
	}

	stdsint TCPObject::Send(const TransportPayloadContext& payload) {
		(void)payload;
		return -1;
	}

	stdsint TCPObject::Receive(TransportMutablePayloadContext& payload) {
		(void)payload;
		return -1;
	}

	stdsint TCPObject::Control(stduint command, void* args) {
		(void)command;
		(void)args;
		return -1;
	}

	void TCPObject::BeginPassiveConnection(const IPv4Address& local_ip, uint16 local_port,
		const IPv4Address& remote_ip, uint16 remote_port,
		const TCPSegmentView& segment, uint32 initial_sequence) {
		TCPBeginPassiveConnection(control_, local_ip, local_port, remote_ip, remote_port,
			segment, initial_sequence);
		bound_ = true;
		connected_ = false;
	}

	bool TCPObject::AcceptHandshakeAck(const TCPSegmentView& segment) {
		const bool accepted = TCPAcceptHandshakeAck(control_, segment);
		if (accepted) connected_ = true;
		return accepted;
	}

	bool TCPObject::EnqueueAccept(const TCPConnectionContext& connection) {
		if (!accept_pending_ || !accept_capacity_) return false;
		const stduint backlog = backlog_ ? backlog_ : 1;
		if (accept_count_ >= backlog || accept_count_ >= accept_capacity_) return false;
		accept_pending_[accept_tail_] = connection;
		accept_tail_ = (accept_tail_ + 1) % accept_capacity_;
		accept_count_++;
		return true;
	}

	bool TCPObject::Accept(TCPConnectionContext& connection) {
		if (!accept_count_) return false;
		connection = accept_pending_[accept_head_];
		accept_pending_[accept_head_] = {};
		accept_head_ = (accept_head_ + 1) % accept_capacity_;
		accept_count_--;
		return true;
	}

	bool TCPObject::isExpectedSegment(const TCPSegmentView& segment) const {
		return TCPIsExpectedSegment(control_, segment);
	}

	bool TCPObject::ConsumeExpectedSegment(const TCPSegmentView& segment) {
		return TCPConsumeExpectedSegment(control_, segment);
	}

	uint32 TCPSequenceLength(const TCPSegmentView& segment) {
		uint32 length = uint32(segment.payload_length);
		if (segment.flags & TCPFlagSYN) length++;
		if (segment.flags & TCPFlagFIN) length++;
		return length;
	}

	void TCPBeginPassiveConnection(TCPConnectionControlBlock& connection,
		const IPv4Address& local_ip, uint16 local_port,
		const IPv4Address& remote_ip, uint16 remote_port,
		const TCPSegmentView& segment, uint32 initial_sequence) {
		connection.context.local = { local_ip, local_port };
		connection.context.remote = { remote_ip, remote_port };
		connection.local_next_sequence = initial_sequence + 1;
		connection.remote_next_sequence = segment.sequence + 1;
		connection.state = TCPConnectionState::SynReceived;
	}

	bool TCPAcceptHandshakeAck(TCPConnectionControlBlock& connection, const TCPSegmentView& segment) {
		if (connection.state != TCPConnectionState::SynReceived) return false;
		if (!(segment.flags & TCPFlagACK)) return false;
		if (segment.acknowledgment != connection.local_next_sequence) return false;
		connection.state = TCPConnectionState::Established;
		return true;
	}

	bool TCPIsExpectedSegment(const TCPConnectionControlBlock& connection, const TCPSegmentView& segment) {
		return segment.sequence == connection.remote_next_sequence;
	}

	uint32 TCPExpectedAcknowledge(const TCPConnectionControlBlock& connection, const TCPSegmentView& segment) {
		return TCPIsExpectedSegment(connection, segment) ?
			connection.remote_next_sequence + TCPSequenceLength(segment) :
			connection.remote_next_sequence;
	}

	bool TCPConsumeExpectedSegment(TCPConnectionControlBlock& connection, const TCPSegmentView& segment) {
		if (!TCPIsExpectedSegment(connection, segment)) return false;
		connection.remote_next_sequence += TCPSequenceLength(segment);
		if (segment.flags & TCPFlagFIN) connection.state = TCPConnectionState::CloseWait;
		return true;
	}

}
}

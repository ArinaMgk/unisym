// ASCII CPP-ISO11 TAB4 LF
// Docutitle: [System.Network.Layer] Transport
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

#ifndef _INCPP_System_NETWORK_LAYER_TRANSPORT
#define _INCPP_System_NETWORK_LAYER_TRANSPORT

#include "Network.hpp"

namespace uni {
namespace Network {

	enum class TransportProtocol : uint8 {
		Default = 0,
		TCP = 6,
		UDP = 17,
	};

	struct TransportEndpoint {
		NetworkAddress address;
		uint16 port;
	};

	struct TransportConnectionContext {
		TransportEndpoint local;
		TransportEndpoint remote;
	};

	struct TransportPayloadContext {
		TransportConnectionContext connection;
		const void* payload;
		stduint payload_length;
	};

	struct TransportMutablePayloadContext {
		TransportConnectionContext connection;
		void* payload;
		stduint capacity;
		stduint payload_length;
	};

	class TransportInterface {
	public:
		virtual ~TransportInterface() = default;
		virtual TransportProtocol getProtocol() const = 0;
		virtual NetworkInterface* getNetwork() const = 0;
		virtual stdsint Bind(const TransportEndpoint& local) = 0;
		virtual stdsint Connect(const TransportEndpoint& remote) = 0;
		virtual stdsint Listen(stduint backlog) = 0;
		virtual stdsint Accept(TransportConnectionContext& connection) = 0;
		virtual stdsint Send(const TransportPayloadContext& payload) = 0;
		virtual stdsint Receive(TransportMutablePayloadContext& payload) = 0;
		virtual stdsint Control(stduint command, void* args) = 0;
	};

}
}

#endif

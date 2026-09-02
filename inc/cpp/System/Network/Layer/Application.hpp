// ASCII CPP-ISO11 TAB4 LF
// Docutitle: [System.Network.Layer] Application
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

#ifndef _INCPP_System_NETWORK_LAYER_APPLICATION
#define _INCPP_System_NETWORK_LAYER_APPLICATION

#include "Transport.hpp"

namespace uni {
namespace Network {

	struct ApplicationPayloadContext {
		const void* payload;
		stduint payload_length;
	};

	struct ApplicationMutablePayloadContext {
		void* payload;
		stduint capacity;
		stduint payload_length;
	};

	class ApplicationInterface {
	public:
		virtual ~ApplicationInterface() = default;
		virtual TransportInterface* getTransport() const = 0;
		virtual stdsint Send(const ApplicationPayloadContext& payload) = 0;
		virtual stdsint Receive(ApplicationMutablePayloadContext& payload) = 0;
		virtual stdsint Control(stduint command, void* args) = 0;
	};

}
}

#endif

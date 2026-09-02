// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [System.Network.Layer] Link
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

#ifndef _INCPP_System_NETWORK_LAYER_LINK
#define _INCPP_System_NETWORK_LAYER_LINK

#include "../../../../c/stdinc.h"
#include "../../../port"

namespace uni {
namespace Network {

	struct MacAddress {
		uint8 octet[6];

		bool isZero() const {
			for0(i, numsof(octet)) if (octet[i]) return false;
			return true;
		}

		bool isBroadcast() const {
			for0(i, numsof(octet)) if (octet[i] != 0xFFu) return false;
			return true;
		}
	};

	enum class LinkState : uint8 {
		Down,
		Up,
		Unknown,
	};

	enum class LinkMedium : uint8 {
		Ethernet,
		Loopback,
		Other,
	};

	struct LinkFrameView {
		const void* data;
		stduint length;
	};

	struct LinkMutableFrameView {
		void* data;
		stduint capacity;
		stduint length;
	};

	struct LinkStatistics {
		uint64 rx_packets;
		uint64 tx_packets;
		uint64 rx_bytes;
		uint64 tx_bytes;
		uint64 rx_drops;
		uint64 tx_drops;
		uint64 rx_errors;
		uint64 tx_errors;
		uint64 interrupts;
		uint64 link_changes;
	};

	class LinkDevice {
	public:
		virtual ~LinkDevice() = default;
		virtual const char* getName() const = 0;
		virtual LinkMedium getMedium() const = 0;
		virtual LinkState getState() const = 0;
		virtual MacAddress getAddress() const = 0;
		virtual stduint getMtu() const = 0;
		virtual stdsint Send(const LinkFrameView& frame) = 0;
		virtual stdsint Receive(LinkMutableFrameView& frame) = 0;
		virtual stdsint Control(stduint command, void* args) = 0;
		virtual void getStatistics(LinkStatistics& statistics) const = 0;
	};

}
}

#endif

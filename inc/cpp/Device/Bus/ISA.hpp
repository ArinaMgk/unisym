// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device.Bus) ISA
// Codifiers: @dosconio
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#ifndef _INCPP_Device_BUS_ISA
#define _INCPP_Device_BUS_ISA

#include "../../../c/stdinc.h"

#if (defined(_MCCA) && ((_MCCA & 0xFF00)==0x8600))

namespace uni {
	namespace ISA {
		enum class BridgeKind : uint8 {
			Unknown = 0,
			IntelPIIX3,
			IntelPIIX4,
		};

		struct BridgeInfo {
			BridgeKind kind;
			uint16 vendor_id;
			uint16 device_id;
			uint8 class_base;
			uint8 class_sub;
			const char* product_name;
		};

		BridgeKind ClassifyBridge(uint16 vendor_id, uint16 device_id, uint8 class_base, uint8 class_sub);
		const BridgeInfo* LookupBridgeInfo(BridgeKind kind);
		bool IsBridgeDevice(uint16 vendor_id, uint16 device_id, uint8 class_base, uint8 class_sub);
		const char* BridgeKindName(BridgeKind kind);
	}
}

#endif

#endif

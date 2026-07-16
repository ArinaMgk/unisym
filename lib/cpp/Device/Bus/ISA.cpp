// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device.Bus) ISA
// Codifiers: @dosconio
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/cpp/Device/Bus/ISA.hpp"

#if (defined(_MCCA) && ((_MCCA & 0xFF00)==0x8600))

namespace {
	using BridgeInfo = uni::ISA::BridgeInfo;
	using BridgeKind = uni::ISA::BridgeKind;

	constexpr BridgeInfo kIsaBridgeTable[] = {
		{BridgeKind::IntelPIIX3, 0x8086u, 0x7000u, 0x06u, 0x01u, "Intel 82371SB PIIX3 ISA bridge"},
		{BridgeKind::IntelPIIX4, 0x8086u, 0x7110u, 0x06u, 0x01u, "Intel 82371AB PIIX4 ISA bridge"},
	};
}

uni::ISA::BridgeKind uni::ISA::ClassifyBridge(uint16 vendor_id, uint16 device_id, uint8 class_base, uint8 class_sub) {
	for (const auto& info : kIsaBridgeTable) {
		if (info.vendor_id != vendor_id) continue;
		if (info.device_id != device_id) continue;
		if (info.class_base != class_base) continue;
		if (info.class_sub != class_sub) continue;
		return info.kind;
	}
	return BridgeKind::Unknown;
}

const uni::ISA::BridgeInfo* uni::ISA::LookupBridgeInfo(BridgeKind kind) {
	for (const auto& info : kIsaBridgeTable) {
		if (info.kind == kind) return &info;
	}
	return nullptr;
}

bool uni::ISA::IsBridgeDevice(uint16 vendor_id, uint16 device_id, uint8 class_base, uint8 class_sub) {
	return ClassifyBridge(vendor_id, device_id, class_base, class_sub) != BridgeKind::Unknown;
}

const char* uni::ISA::BridgeKindName(BridgeKind kind) {
	if (const auto* info = LookupBridgeInfo(kind)) return info->product_name;
	return "ISA bridge";
}

#endif

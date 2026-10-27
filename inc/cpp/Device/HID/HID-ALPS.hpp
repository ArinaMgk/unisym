// UTF-8 CPP-ISO11 TAB4 LF
// Docutitle: ALPS Human Interface Device Protocol
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#ifndef _INCPP_Device_HID_ALPS
#define _INCPP_Device_HID_ALPS

#include "../Bus/Serial/IIC-HID.hpp"

namespace uni {

	constexpr uint16 HIDALPSVendor = 0x044Eu;
	constexpr stduint HIDALPSMaximumContacts = 5u;

	enum class HIDALPSProtocol : uint8 {
		None,
		U1,
	};

	enum class HIDALPSStatus : uint8 {
		Success,
		NotHandled,
		InvalidArgument,
		InvalidReport,
		TransferFailed,
		InvalidResponse,
	};

	struct HIDALPSContact {
		uint16 x;
		uint16 y;
		uint8 pressure;
		bool active;
	};

	struct HIDALPSInputReport {
		uint8 report_id;
		uint8 buttons;
		bool pointing_stick;
		int16 relative_x;
		int16 relative_y;
		uint8 contact_count;
		HIDALPSContact contacts[HIDALPSMaximumContacts];
	};

	struct HIDALPSDeviceState {
		HIDALPSProtocol protocol;
		uint32 x_minimum;
		uint32 x_maximum;
		uint32 y_minimum;
		uint32 y_maximum;
		uint32 x_active_length_mm;
		uint32 y_active_length_mm;
		uint8 button_count;
		uint8 pointing_stick_button_count;
		bool pointing_stick;
	};

	HIDALPSProtocol MatchHIDALPSProtocol(uint16 vendor_id, uint16 product_id);
	HIDALPSStatus InitializeHIDALPS(HIDI2CDevice& device, HIDALPSProtocol protocol, stduint timeout, HIDALPSDeviceState& state, HIDI2CResult& result);
	HIDALPSStatus DecodeHIDALPSInput(const HIDALPSDeviceState& state, const byte* report, stduint length, HIDALPSInputReport& decoded);

}

#endif

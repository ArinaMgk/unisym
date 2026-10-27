// UTF-8 CPP-ISO11 TAB4 LF
// Docutitle: ALPS Human Interface Device Protocol
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/cpp/Device/HID/HID-ALPS.hpp"
#include "../../../../inc/cpp/endian"

namespace uni {

	namespace {
		constexpr uint16 HIDALPSProductU1Dual = 0x120Bu;
		constexpr uint16 HIDALPSProductU1 = 0x1215u;
		constexpr uint16 HIDALPSProductU1UnicornLegacy = 0x121Eu;
		constexpr uint8 HIDALPSU1AbsoluteReport = 0x03u;
		constexpr uint8 HIDALPSU1SecondaryAbsoluteReport = 0x02u;
		constexpr uint8 HIDALPSU1FeatureReport = 0x05u;
		constexpr uint8 HIDALPSU1PointingStickReport = 0x06u;
		constexpr uint8 HIDALPSU1FeatureLength = 0x08u;
		constexpr uint8 HIDALPSU1FeatureTotalLength = 0x0Au;
		constexpr uint8 HIDALPSU1RegisterRead = 0xD1u;
		constexpr uint8 HIDALPSU1RegisterWrite = 0xD2u;
		constexpr uint8 HIDALPSU1DeviceTypePointingStick = 0x10u;
		constexpr uint8 HIDALPSU1DisableDevice = 0x01u;
		constexpr uint8 HIDALPSU1TouchpadAbsoluteMode = 0x02u;
		constexpr uint8 HIDALPSU1PointingStickAbsoluteMode = 0x80u;
		constexpr uint32 HIDALPSU1DeviceControl = 0x00800040u;
		constexpr uint32 HIDALPSU1DeviceType = 0x00800043u;
		constexpr uint32 HIDALPSU1SensorCountX = 0x00800047u;
		constexpr uint32 HIDALPSU1SensorCountY = 0x00800048u;
		constexpr uint32 HIDALPSU1SensorPitchX = 0x00800049u;
		constexpr uint32 HIDALPSU1SensorPitchY = 0x0080004Au;
		constexpr uint32 HIDALPSU1AbsoluteResolution = 0x0080004Eu;
		constexpr uint32 HIDALPSU1TouchpadButtons = 0x00800052u;
		constexpr uint32 HIDALPSU1PointingStickButtons = 0x0080009Fu;

		uint8 HIDALPSU1Checksum(const byte* payload) {
			uint8 checksum = HIDALPSU1FeatureTotalLength + HIDALPSU1FeatureReport;
			for0(index, HIDALPSU1FeatureLength - 2u) checksum += payload[index];
			return checksum;
		}

		HIDALPSStatus WriteHIDALPSU1Register(HIDI2CDevice& device, uint32 address, uint8 value, stduint timeout, HIDI2CResult& result) {
			// Protocol evidence: Linux drivers/hid/hid-alps.c, ALPS U1 feature report commands.
			byte payload[HIDALPSU1FeatureLength - 1u]{};
			payload[0] = HIDALPSU1RegisterWrite;
			reinterpret_cast<LitEndian<uint32, true>*>(payload + 1)->set(address);
			payload[5] = value;
			payload[6] = HIDALPSU1Checksum(payload);
			return device.setReport(HIDI2CReportType::Feature, HIDALPSU1FeatureReport, payload, numsof(payload), timeout, result) == HIDI2CStatus::Success ? HIDALPSStatus::Success : HIDALPSStatus::TransferFailed;
		}

		HIDALPSStatus ReadHIDALPSU1Register(HIDI2CDevice& device, uint32 address, uint8& value, stduint timeout, HIDI2CResult& result) {
			byte payload[HIDALPSU1FeatureLength - 1u]{};
			payload[0] = HIDALPSU1RegisterRead;
			reinterpret_cast<LitEndian<uint32, true>*>(payload + 1)->set(address);
			payload[6] = HIDALPSU1Checksum(payload);
			if (device.setReport(HIDI2CReportType::Feature, HIDALPSU1FeatureReport, payload, numsof(payload), timeout, result) != HIDI2CStatus::Success) return HIDALPSStatus::TransferFailed;
			byte response[HIDALPSU1FeatureLength]{};
			if (device.getReport(HIDI2CReportType::Feature, HIDALPSU1FeatureReport, response, numsof(response), timeout, result) != HIDI2CStatus::Success) return HIDALPSStatus::TransferFailed;
			if (result.length < 7u || response[0] != HIDALPSU1FeatureReport) return HIDALPSStatus::InvalidResponse;
			value = response[6];
			return HIDALPSStatus::Success;
		}

		HIDALPSStatus InitializeHIDALPSU1(HIDI2CDevice& device, stduint timeout, HIDALPSDeviceState& state, HIDI2CResult& result) {
			uint8 device_control = 0;
			uint8 sensor_count_x = 0;
			uint8 sensor_count_y = 0;
			uint8 sensor_pitch_x = 0;
			uint8 sensor_pitch_y = 0;
			uint8 resolution = 0;
			uint8 button_description = 0;
			uint8 device_type = 0;
			if (ReadHIDALPSU1Register(device, HIDALPSU1DeviceControl, device_control, timeout, result) != HIDALPSStatus::Success) return HIDALPSStatus::TransferFailed;
			device_control = uint8((device_control & ~HIDALPSU1DisableDevice) | HIDALPSU1TouchpadAbsoluteMode);
			if (WriteHIDALPSU1Register(device, HIDALPSU1DeviceControl, device_control, timeout, result) != HIDALPSStatus::Success) return HIDALPSStatus::TransferFailed;
			if (ReadHIDALPSU1Register(device, HIDALPSU1SensorCountX, sensor_count_x, timeout, result) != HIDALPSStatus::Success) return HIDALPSStatus::TransferFailed;
			if (ReadHIDALPSU1Register(device, HIDALPSU1SensorCountY, sensor_count_y, timeout, result) != HIDALPSStatus::Success) return HIDALPSStatus::TransferFailed;
			if (ReadHIDALPSU1Register(device, HIDALPSU1SensorPitchX, sensor_pitch_x, timeout, result) != HIDALPSStatus::Success) return HIDALPSStatus::TransferFailed;
			if (ReadHIDALPSU1Register(device, HIDALPSU1SensorPitchY, sensor_pitch_y, timeout, result) != HIDALPSStatus::Success) return HIDALPSStatus::TransferFailed;
			if (ReadHIDALPSU1Register(device, HIDALPSU1AbsoluteResolution, resolution, timeout, result) != HIDALPSStatus::Success) return HIDALPSStatus::TransferFailed;
			if (sensor_count_x < 2u || sensor_count_y < 2u || !resolution) return HIDALPSStatus::InvalidResponse;
			state.x_minimum = 1u;
			state.y_minimum = 1u;
			state.x_maximum = (uint32(resolution) << 2) * (sensor_count_x - 1u);
			state.y_maximum = (uint32(resolution) << 2) * (sensor_count_y - 1u);
			state.x_active_length_mm = uint32(sensor_pitch_x) * (sensor_count_x - 1u) / 10u;
			state.y_active_length_mm = uint32(sensor_pitch_y) * (sensor_count_y - 1u) / 10u;
			if (ReadHIDALPSU1Register(device, HIDALPSU1TouchpadButtons, button_description, timeout, result) != HIDALPSStatus::Success) return HIDALPSStatus::TransferFailed;
			state.button_count = (button_description & 0x0Fu) == ((button_description >> 4) & 0x0Fu) ? button_description & 0x0Fu : 1u;
			if (ReadHIDALPSU1Register(device, HIDALPSU1DeviceType, device_type, timeout, result) != HIDALPSStatus::Success) return HIDALPSStatus::TransferFailed;
			if (device_type & HIDALPSU1DeviceTypePointingStick) {
				device_control |= HIDALPSU1PointingStickAbsoluteMode;
				if (WriteHIDALPSU1Register(device, HIDALPSU1DeviceControl, device_control, timeout, result) != HIDALPSStatus::Success) return HIDALPSStatus::TransferFailed;
				if (ReadHIDALPSU1Register(device, HIDALPSU1PointingStickButtons, state.pointing_stick_button_count, timeout, result) != HIDALPSStatus::Success) return HIDALPSStatus::TransferFailed;
				state.pointing_stick_button_count &= 0x0Fu;
				state.pointing_stick = true;
			}
			return HIDALPSStatus::Success;
		}
	}

	HIDALPSProtocol MatchHIDALPSProtocol(uint16 vendor_id, uint16 product_id) {
		if (vendor_id != HIDALPSVendor) return HIDALPSProtocol::None;
		switch (product_id) {
		case HIDALPSProductU1Dual:
		case HIDALPSProductU1:
		case HIDALPSProductU1UnicornLegacy:
			return HIDALPSProtocol::U1;
		default:
			return HIDALPSProtocol::None;
		}
	}

	HIDALPSStatus InitializeHIDALPS(HIDI2CDevice& device, HIDALPSProtocol protocol, stduint timeout, HIDALPSDeviceState& state, HIDI2CResult& result) {
		state = {};
		state.protocol = protocol;
		if (!timeout) return HIDALPSStatus::InvalidArgument;
		switch (protocol) {
		case HIDALPSProtocol::U1:
			return InitializeHIDALPSU1(device, timeout, state, result);
		default:
			return HIDALPSStatus::NotHandled;
		}
	}

	HIDALPSStatus DecodeHIDALPSInput(const HIDALPSDeviceState& state, const byte* report, stduint length, HIDALPSInputReport& decoded) {
		decoded = {};
		if (!report || !length) return HIDALPSStatus::InvalidArgument;
		if (state.protocol != HIDALPSProtocol::U1) return HIDALPSStatus::NotHandled;
		decoded.report_id = report[0];
		switch (decoded.report_id) {
		case HIDALPSU1AbsoluteReport:
		case HIDALPSU1SecondaryAbsoluteReport:
			// Protocol evidence: Linux drivers/hid/hid-alps.c, u1_raw_event().
			if (length < 28u) return HIDALPSStatus::InvalidReport;
			decoded.buttons = report[1] & 0x07u;
			for0(index, HIDALPSMaximumContacts) {
				const byte* contact = report + index * 5u;
				auto& destination = decoded.contacts[index];
				destination.x = reinterpret_cast<const LitEndian<uint16, true>*>(contact + 3u)->get();
				destination.y = reinterpret_cast<const LitEndian<uint16, true>*>(contact + 5u)->get();
				destination.pressure = contact[7] & 0x7Fu;
				destination.active = destination.pressure != 0;
				if (destination.active) ++decoded.contact_count;
			}
			return HIDALPSStatus::Success;
		case HIDALPSU1PointingStickReport:
			if (length < 6u) return HIDALPSStatus::InvalidReport;
			decoded.buttons = report[1] & 0x07u;
			decoded.pointing_stick = true;
			decoded.relative_x = int16(reinterpret_cast<const LitEndian<uint16, true>*>(report + 2u)->get()) / 8;
			decoded.relative_y = int16(reinterpret_cast<const LitEndian<uint16, true>*>(report + 4u)->get()) / 8;
			return HIDALPSStatus::Success;
		default:
			return HIDALPSStatus::NotHandled;
		}
	}

}

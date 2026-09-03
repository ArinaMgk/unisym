#if (defined(_MCCA) && _MCCA == 0x8664) || defined(_MCU_STM32H7x)

#include "../../../../inc/cpp/Device/USB/USB.hpp"
#include "../../../../inc/c/driver/keyboard.h"
#include "../../../../inc/c/driver/mouse.h"


namespace {
	void DecodeUSBStringDescriptor(const uint8_t* buf, int len, char* out, stduint out_len) {
		if (!out || out_len == 0) return;
		out[0] = 0;
		if (!buf || len < 2 || buf[1] != uni::device::SpaceUSB::descriptor_type::kString) return;
		const int char_count = (len - 2) / 2;
		stduint dst = 0;
		for (int i = 0; i < char_count && dst + 1 < out_len; ++i) {
			const uint16_t code = uint16_t(buf[2 + i * 2]) | (uint16_t(buf[3 + i * 2]) << 8);
			out[dst++] = code < 0x80 ? char(code) : '?';
		}
		out[dst] = 0;
	}

	class ConfigurationDescriptorReader {
	public:
		ConfigurationDescriptorReader(const uint8_t* desc_buf, int len)
			: desc_buf_{ desc_buf },
			desc_buf_len_{ len },
			p_{ desc_buf } {
		}

		const uint8_t* Next() {
			p_ += p_[0];
			if (p_ < desc_buf_ + desc_buf_len_) {
				return p_;
			}
			return nullptr;
		}

		template <class T>
		const T* Next() {
			while (auto n = Next()) {
				if (auto d = uni::device::SpaceUSB::DescriptorDynamicCast<T>(n)) {
					return d;
				}
			}
			return nullptr;
		}

	private:
		const uint8_t* const desc_buf_;
		const int desc_buf_len_;
		const uint8_t* p_;
	};

	uni::device::SpaceUSB::EndpointConfig MakeEPConfig(const uni::device::SpaceUSB::EndpointDescriptor& ep_desc) {
		uni::device::SpaceUSB::EndpointConfig conf;
		conf.ep_id = uni::device::SpaceUSB::EndpointID{
		  ep_desc.endpoint_address.bits.number,
		  ep_desc.endpoint_address.bits.dir_in == 1
		};
		conf.ep_type = static_cast<uni::device::SpaceUSB::EndpointType>(ep_desc.attributes.bits.transfer_type);
		conf.max_packet_size = ep_desc.max_packet_size;
		conf.interval = ep_desc.interval;
		return conf;
	}

	uni::device::SpaceUSB::ClassDriver* NewClassDriver(uni::device::SpaceUSB::DeviceUSB* dev, const uni::device::SpaceUSB::InterfaceDescriptor& if_desc)
	{
#if defined(_MCU_STM32H7x)
		// H7: HID keyboard/mouse and hub drivers stay on x86 for now;
		// enumeration and the ClassDriver core run, but no class driver is bound.
		(void)dev;
		(void)if_desc;
		return nullptr;
#else
		if (dev->DeviceClass() == 0x09u || if_desc.interface_class == 0x09u) {
			return new uni::device::SpaceUSB::USBHubDriver{ dev };
		}
		if (if_desc.interface_class == 3 &&
			if_desc.interface_sub_class == 1) {  // HID boot interface
			if (if_desc.interface_protocol == 1) {  // keyboard
				auto keyboard_driver = new uni::device::SpaceUSB::HIDKeyboardDriver{ dev, if_desc.interface_number };
				if (uni::device::SpaceUSB::HIDKeyboardDriver::default_observer) {
					keyboard_driver->SubscribeKeyPush(uni::device::SpaceUSB::HIDKeyboardDriver::default_observer);
				}
				return keyboard_driver;
			}
			else if (if_desc.interface_protocol == 2) {  // mouse
				auto mouse_driver = new uni::device::SpaceUSB::HIDMouseDriver{ dev, if_desc.interface_number };
				if (uni::device::SpaceUSB::HIDMouseDriver::default_observer) {
					mouse_driver->SubscribeMouseMove(uni::device::SpaceUSB::HIDMouseDriver::default_observer);
				}
				return mouse_driver;
			}
		}
		return nullptr;
#endif
	}

	void Log(LogLevel level, const uni::device::SpaceUSB::InterfaceDescriptor& if_desc) {
		Log(level, "Interface Descriptor: class=%d, sub=%d, protocol=%d\n",
			if_desc.interface_class,
			if_desc.interface_sub_class,
			if_desc.interface_protocol);
	}

	void Log(LogLevel level, const uni::device::SpaceUSB::EndpointConfig& conf) {

		// Log(level, "EndpointConf: ep_id=%d, ep_type=%d, max_packet_size=%d, interval=%d\n",
		// 	conf.ep_id.Address(), conf.ep_type,
		// 	conf.max_packet_size, conf.interval);
	}

	void Log(LogLevel level, const uni::device::SpaceUSB::HIDDescriptor& hid_desc) {
		Log(level, "HID Descriptor: release=0x%02x, num_desc=%d",
			hid_desc.hid_release,
			hid_desc.num_descriptors);
		for (int i = 0; i < hid_desc.num_descriptors; ++i) {
			Log(level, ", desc_type=%d, len=%d",
				hid_desc.GetClassDescriptor(i)->descriptor_type,
				hid_desc.GetClassDescriptor(i)->descriptor_length);
		}
		Log(level, "\n");
	}
}

namespace uni::device::SpaceUSB {
	HubDescriptorCompleteHook g_hub_descriptor_complete_hook = nullptr;
	HubPortStatusHook g_hub_port_status_hook = nullptr;

	DeviceUSB::~DeviceUSB() {
	}

	Error DeviceUSB::ControlIn(EndpointID ep_id, SetupData setup_data,
		void* buf, int len, ClassDriver* issuer) {
		if (issuer) {
			event_waiters_.Put(setup_data, issuer);
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error DeviceUSB::ControlOut(EndpointID ep_id, SetupData setup_data,
		const void* buf, int len, ClassDriver* issuer) {
		if (issuer) {
			event_waiters_.Put(setup_data, issuer);
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error DeviceUSB::InterruptIn(EndpointID ep_id, void* buf, int len) {
		return MAKE_ERROR(Error::kSuccess);
	}

	Error DeviceUSB::InterruptOut(EndpointID ep_id, void* buf, int len) {
		return MAKE_ERROR(Error::kSuccess);
	}

	Error DeviceUSB::OnHubPortStatusReceived(uint8 port_num, uint16 status, uint16 change) {
		(void)port_num;
		(void)status;
		(void)change;
		return MAKE_ERROR(Error::kSuccess);
	}

	Error DeviceUSB::StartInitialize() {
		is_initialized_ = false;
		hub_num_ports_ = 0;
		initialize_phase_ = 1;
		manufacturer_index_ = 0;
		product_index_ = 0;
		serial_index_ = 0;
		string_lang_id_ = 0x0409;
		manufacturer_string_[0] = 0;
		product_string_[0] = 0;
		serial_string_[0] = 0;
		return GetDescriptor(*this, kDefaultControlPipeID, DeviceDescriptor::kType, 0,
			buf_.data(), buf_.size(), true);
	}

	Error DeviceUSB::OnEndpointsConfigured() {
		for (auto class_driver : class_drivers_) {
			if (class_driver != nullptr) {
				if (auto err = class_driver->OnEndpointsConfigured()) {
					return err;
				}
			}
		}
		return MAKE_ERROR(Error::kSuccess);
	}

	Error DeviceUSB::OnControlCompleted(EndpointID ep_id, SetupData setup_data,
		const void* buf, int len) {
		// Log(kDebug, "Device::OnControlCompleted: buf 0x%08x, len %d, dir %d", buf, len, setup_data.request_type.bits.direction);
		if (is_initialized_) {
			if (auto w = event_waiters_.Get(setup_data)) {
				auto* waiter = w.value();
				event_waiters_.Delete(setup_data);
				return waiter->OnControlCompleted(ep_id, setup_data, buf, len);
			}
			plogerro("USB control completion without waiter: req_type=%02x req=%02x value=%04x index=%04x len=%u",
				(unsigned)setup_data.request_type.data,
				(unsigned)setup_data.request,
				(unsigned)setup_data.value,
				(unsigned)setup_data.index,
				(unsigned)setup_data.length);
			return MAKE_ERROR(Error::kNoWaiter);
		}

		const uint8_t* buf8 = reinterpret_cast<const uint8_t*>(buf);
		if (initialize_phase_ == 1) {
			if (setup_data.request == request::kGetDescriptor &&
				DescriptorDynamicCast<DeviceDescriptor>(buf8)) {
				return InitializePhase1(buf8, len);
			}
			return MAKE_ERROR(Error::kInvalidPhase);
		}
		else if (initialize_phase_ == 11) {
			if (setup_data.request == request::kGetDescriptor) {
				return InitializeStringPhase0(buf8, len);
			}
			return MAKE_ERROR(Error::kInvalidPhase);
		}
		else if (initialize_phase_ == 12) {
			if (setup_data.request == request::kGetDescriptor) {
				return InitializeStringPhaseManufacturer(buf8, len);
			}
			return MAKE_ERROR(Error::kInvalidPhase);
		}
		else if (initialize_phase_ == 13) {
			if (setup_data.request == request::kGetDescriptor) {
				return InitializeStringPhaseProduct(buf8, len);
			}
			return MAKE_ERROR(Error::kInvalidPhase);
		}
		else if (initialize_phase_ == 14) {
			if (setup_data.request == request::kGetDescriptor) {
				return InitializeStringPhaseSerial(buf8, len);
			}
			return MAKE_ERROR(Error::kInvalidPhase);
		}
		else if (initialize_phase_ == 2) {
			if (setup_data.request == request::kGetDescriptor &&
				DescriptorDynamicCast<ConfigurationDescriptor>(buf8)) {
				return InitializePhase2(buf8, len);
			}
			return MAKE_ERROR(Error::kInvalidPhase);
		}
		else if (initialize_phase_ == 3) {
			if (setup_data.request == request::kSetConfiguration) {
				return InitializePhase3(setup_data.value & 0xffu);
			}
			return MAKE_ERROR(Error::kInvalidPhase);
		}

		return MAKE_ERROR(Error::kNotImplemented);
	}

	Error DeviceUSB::OnInterruptCompleted(EndpointID ep_id, const void* buf, int len) {
		// Log(kDebug, "Device::OnInterruptCompleted: ep addr %d\n", ep_id.Address());
		if (auto w = class_drivers_[ep_id.Number()]) {
			return w->OnInterruptCompleted(ep_id, buf, len);
		}
		return MAKE_ERROR(Error::kNoWaiter);
	}

	Error DeviceUSB::InitializePhase1(const uint8_t* buf, int len) {
		const auto device_desc = DescriptorDynamicCast<DeviceDescriptor>(buf);
		vendor_id_ = device_desc->vendor_id;
		product_id_ = device_desc->product_id;
		device_class_ = device_desc->device_class;
		device_sub_class_ = device_desc->device_sub_class;
		device_protocol_ = device_desc->device_protocol;
		manufacturer_index_ = device_desc->manufacturer;
		product_index_ = device_desc->product;
		serial_index_ = device_desc->serial_number;
		num_configurations_ = device_desc->num_configurations;
		config_index_ = 0;
		return RequestStringDescriptors();
	}

	Error DeviceUSB::InitializePhase2(const uint8_t* buf, int len) {
		auto conf_desc = DescriptorDynamicCast<ConfigurationDescriptor>(buf);
		if (conf_desc == nullptr) {
			return MAKE_ERROR(Error::kInvalidDescriptor);
		}
		ConfigurationDescriptorReader config_reader{ buf, len };

		ClassDriver* class_driver = nullptr;
		while (auto if_desc = config_reader.Next<InterfaceDescriptor>()) {
			Log(kDebug, *if_desc);

			class_driver = NewClassDriver(this, *if_desc);
			if (class_driver == nullptr) {
			  // Unsupported device. Check the next interface.
				continue;
			}

			num_ep_configs_ = 0;

			while (num_ep_configs_ < if_desc->num_endpoints) {
				auto desc = config_reader.Next();
				if (auto ep_desc = DescriptorDynamicCast<EndpointDescriptor>(desc)) {
					auto conf = MakeEPConfig(*ep_desc);
					Log(kDebug, conf);

					ep_configs_[num_ep_configs_] = conf;
					++num_ep_configs_;
					class_drivers_[conf.ep_id.Number()] = class_driver;
				}
				else if (auto hid_desc = DescriptorDynamicCast<HIDDescriptor>(desc)) {
					Log(kDebug, *hid_desc);
				}
			}

			break;
		}

		if (!class_driver) {
			return MAKE_ERROR(Error::kSuccess);
		}
		initialize_phase_ = 3;
		Log(kDebug, "issuing SetConfiguration: conf_val=%d\n",
			conf_desc->configuration_value);
		return SetConfiguration(*this, kDefaultControlPipeID,
			conf_desc->configuration_value, true);
	}

	Error DeviceUSB::InitializePhase3(uint8_t config_value) {
		for (int i = 0; i < num_ep_configs_; ++i) {
			class_drivers_[ep_configs_[i].ep_id.Number()]->SetEndpoint(ep_configs_[i]);
		}
		initialize_phase_ = 4;
		is_initialized_ = true;
		return MAKE_ERROR(Error::kSuccess);
	}

	Error DeviceUSB::InitializeStringPhase0(const uint8_t* buf, int len) {
		if (len >= 4 && buf[1] == descriptor_type::kString) {
			string_lang_id_ = uint16_t(buf[2]) | (uint16_t(buf[3]) << 8);
		}
		if (manufacturer_index_) {
			initialize_phase_ = 12;
			return GetDescriptor(*this, kDefaultControlPipeID,
				descriptor_type::kString, manufacturer_index_,
				buf_.data(), buf_.size(), true, string_lang_id_);
		}
		if (product_index_) {
			initialize_phase_ = 13;
			return GetDescriptor(*this, kDefaultControlPipeID,
				descriptor_type::kString, product_index_,
				buf_.data(), buf_.size(), true, string_lang_id_);
		}
		if (serial_index_) {
			initialize_phase_ = 14;
			return GetDescriptor(*this, kDefaultControlPipeID,
				descriptor_type::kString, serial_index_,
				buf_.data(), buf_.size(), true, string_lang_id_);
		}
		return BeginConfigurationDescriptorRead();
	}

	Error DeviceUSB::InitializeStringPhaseManufacturer(const uint8_t* buf, int len) {
		DecodeUSBStringDescriptor(buf, len, manufacturer_string_.data(), manufacturer_string_.size());
		if (product_index_) {
			initialize_phase_ = 13;
			return GetDescriptor(*this, kDefaultControlPipeID,
				descriptor_type::kString, product_index_,
				buf_.data(), buf_.size(), true, string_lang_id_);
		}
		if (serial_index_) {
			initialize_phase_ = 14;
			return GetDescriptor(*this, kDefaultControlPipeID,
				descriptor_type::kString, serial_index_,
				buf_.data(), buf_.size(), true, string_lang_id_);
		}
		return BeginConfigurationDescriptorRead();
	}

	Error DeviceUSB::InitializeStringPhaseProduct(const uint8_t* buf, int len) {
		DecodeUSBStringDescriptor(buf, len, product_string_.data(), product_string_.size());
		if (serial_index_) {
			initialize_phase_ = 14;
			return GetDescriptor(*this, kDefaultControlPipeID,
				descriptor_type::kString, serial_index_,
				buf_.data(), buf_.size(), true, string_lang_id_);
		}
		return BeginConfigurationDescriptorRead();
	}

	Error DeviceUSB::InitializeStringPhaseSerial(const uint8_t* buf, int len) {
		DecodeUSBStringDescriptor(buf, len, serial_string_.data(), serial_string_.size());
		return BeginConfigurationDescriptorRead();
	}

	Error DeviceUSB::RequestStringDescriptors() {
		if (!manufacturer_index_ && !product_index_ && !serial_index_) {
			return BeginConfigurationDescriptorRead();
		}
		initialize_phase_ = 11;
		return GetDescriptor(*this, kDefaultControlPipeID,
			descriptor_type::kString, 0,
			buf_.data(), buf_.size(), true, 0);
	}

	Error DeviceUSB::BeginConfigurationDescriptorRead() {
		initialize_phase_ = 2;
		Log(kDebug, "issuing GetDesc(Config): index=%d)\n", config_index_);
		return GetDescriptor(*this, kDefaultControlPipeID,
			ConfigurationDescriptor::kType, config_index_,
			buf_.data(), buf_.size(), true);
	}

	Error GetDescriptor(DeviceUSB& dev, EndpointID ep_id,
		uint8_t desc_type, uint8_t desc_index,
		void* buf, int len, bool debug, uint16_t desc_lang_id) {
		SetupData setup_data{};
		setup_data.request_type.bits.direction = request_type::kIn;
		setup_data.request_type.bits.type = request_type::kStandard;
		setup_data.request_type.bits.recipient = request_type::kDevice;
		setup_data.request = request::kGetDescriptor;
		setup_data.value = (static_cast<uint16_t>(desc_type) << 8) | desc_index;
		setup_data.index = desc_lang_id;
		setup_data.length = len;
		return dev.ControlIn(ep_id, setup_data, buf, len, nullptr);
	}

	Error SetConfiguration(DeviceUSB& dev, EndpointID ep_id,
		uint8_t config_value, bool debug) {
		SetupData setup_data{};
		setup_data.request_type.bits.direction = request_type::kOut;
		setup_data.request_type.bits.type = request_type::kStandard;
		setup_data.request_type.bits.recipient = request_type::kDevice;
		setup_data.request = request::kSetConfiguration;
		setup_data.value = config_value;
		setup_data.index = 0;
		setup_data.length = 0;
		return dev.ControlOut(ep_id, setup_data, nullptr, 0, nullptr);
	}
}

#endif

// ---- STM32H7 OTG low layer (shared by PCD/HCD) ----
#if defined(_MCU_STM32H7x)

#include "../../../../inc/cpp/Device/USB/OTG.hpp"
#include "../../../../inc/cpp/Device/SysTick"
#include "../../../../inc/cpp/interrupt"

namespace uni {

	// OTGEP/OTGHC/OTG declared in OTG.hpp; implementations below.
	stduint OTG::g_base = 0;

	// AKA USB_CoreReset
	bool OTG::ResetCore(stduint base) {
		stduint count = 0;
		Reference grstctl(base + 0x010);
		do {
			if (++count > 200000) return false;
		} while (!grstctl.bitof(USB_OTG_GRSTCTL_AHBIDL_Pos));
		count = 0;
		grstctl.setof(USB_OTG_GRSTCTL_CSRST_Pos);
		do {
			if (++count > 200000) return false;
		} while (grstctl.bitof(USB_OTG_GRSTCTL_CSRST_Pos));
		return true;
	}

	// AKA USB_CoreInit
	bool OTG::Initialize(stduint base, bool ulpi, bool dma_enable, bool use_external_vbus) {
		Reference gccfg(base + 0x038);
		Reference gusbcfg(base + 0x00C);
		Reference gahbcfg(base + 0x008);
		if (ulpi) {
			gccfg.rstof(USB_OTG_GCCFG_PWRDWN_Pos);
			gusbcfg.rstof(USB_OTG_GUSBCFG_TSDPS_Pos);
			gusbcfg.rstof(USB_OTG_GUSBCFG_ULPIFSLS_Pos);
			gusbcfg.rstof(USB_OTG_GUSBCFG_PHYSEL_Pos);
			gusbcfg.rstof(USB_OTG_GUSBCFG_ULPIEVBUSD_Pos);
			gusbcfg.rstof(USB_OTG_GUSBCFG_ULPIEVBUSI_Pos);
			if (use_external_vbus) gusbcfg.setof(USB_OTG_GUSBCFG_ULPIEVBUSD_Pos);
			if (!ResetCore(base)) return false;
		}
		else {
			gusbcfg.setof(USB_OTG_GUSBCFG_PHYSEL_Pos);
			if (!ResetCore(base)) return false;
			gccfg = USB_OTG_GCCFG_PWRDWN;
		}
		if (dma_enable) {
			gahbcfg.maset(USB_OTG_GAHBCFG_HBSTLEN_Pos, 2, 3);
			gahbcfg.setof(USB_OTG_GAHBCFG_DMAEN_Pos);
		}
		return true;
	}

	// AKA USB_SetCurrentMode
	void OTG::setMode(stduint base, byte mode) {
		Reference gusbcfg(base + 0x00C);
		gusbcfg.rstof(USB_OTG_GUSBCFG_FHMOD_Pos);
		gusbcfg.rstof(USB_OTG_GUSBCFG_FDMOD_Pos);
		if (mode == 0) gusbcfg.setof(USB_OTG_GUSBCFG_FHMOD_Pos);
		else if (mode == 1) gusbcfg.setof(USB_OTG_GUSBCFG_FDMOD_Pos);
	}

	// AKA USB_FlushTxFifo
	bool OTG::FlushTxFifo(stduint base, stduint num) {
		stduint count = 0;
		Reference grstctl(base + 0x010);
		grstctl = USB_OTG_GRSTCTL_TXFFLSH | ((num & 0xF) << 6);
		do {
			if (++count > 200000) return false;
		} while (grstctl.bitof(USB_OTG_GRSTCTL_TXFFLSH_Pos));
		return true;
	}

	// AKA USB_FlushRxFifo
	bool OTG::FlushRxFifo(stduint base) {
		stduint count = 0;
		Reference grstctl(base + 0x010);
		grstctl = USB_OTG_GRSTCTL_RXFFLSH;
		do {
			if (++count > 200000) return false;
		} while (grstctl.bitof(USB_OTG_GRSTCTL_RXFFLSH_Pos));
		return true;
	}

	// AKA USB_GetMode
	byte OTG::getMode(stduint base) {
		return (Reference(base + 0x014) & 0x1) ? 1 : 0;
	}

	// AKA USB_ReadInterrupts
	stduint OTG::ReadInterrupts(stduint base) {
		return Reference(base + 0x014) & Reference(base + 0x018);
	}

	// AKA USB_ClearInterrupts
	void OTG::ClearInterrupts(stduint base, stduint interrupt) {
		Reference(base + 0x014) |= interrupt;
	}

	// AKA USB_WritePacket (DFIFO at base + 0x1000 + ep*4)
	bool OTG::WritePacket(stduint base, const byte* src, byte ep_num, stduint len) {
		stduint fifo = base + USB_OTG_FIFO_BASE + ep_num * 4;
		stduint count32b = (len + 3) / 4;
		for (stduint i = 0; i < count32b; i++) {
			stduint w = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);
			Reference(fifo + 0) = w;
			src += 4;
		}
		return true;
	}

	// AKA USB_ReadPacket (DFIFO0 at base + 0x1000)
	bool OTG::ReadPacket(stduint base, byte* dest, stduint len) {
		stduint fifo = base + USB_OTG_FIFO_BASE;
		stduint count32b = (len + 3) / 4;
		for (stduint i = 0; i < count32b; i++) {
			stduint w = Reference(fifo + 0);
			dest[0] = w & 0xFF; dest[1] = (w >> 8) & 0xFF;
			dest[2] = (w >> 16) & 0xFF; dest[3] = (w >> 24) & 0xFF;
			dest += 4;
		}
		return true;
	}

	// AKA USB_StopDevice
	bool OTG::StopDevice(stduint base) {
		for (stduint i = 0; i < 15; i++) {
			Reference(base + USB_OTG_IN_ENDPOINT_BASE + i * USB_OTG_EP_REG_SIZE + 0x004) = 0xFF;// DIEPINT
			Reference(base + USB_OTG_OUT_ENDPOINT_BASE + i * USB_OTG_EP_REG_SIZE + 0x004) = 0xFF;// DOEPINT
		}
		Reference(base + USB_OTG_DEVICE_BASE + 0x18) = 0xFFFFFFFF;// DAINT
		Reference(base + USB_OTG_DEVICE_BASE + 0x10) = 0;// DIEPMSK
		Reference(base + USB_OTG_DEVICE_BASE + 0x14) = 0;// DOEPMSK
		Reference(base + USB_OTG_DEVICE_BASE + 0x1C) = 0;// DAINTMSK
		FlushRxFifo(base);
		FlushTxFifo(base, 0x10);
		return true;
	}

	// AKA USB_StopHost
	bool OTG::StopHost(stduint base) {
		enGlobalInt(false);
		FlushTxFifo(base, 0x10);
		FlushRxFifo(base);
		for (stduint i = 0; i <= 15; i++) {
			Reference hcchar(base + USB_OTG_HOST_CHANNEL_BASE + i * USB_OTG_HOST_CHANNEL_SIZE + 0x000);
			hcchar.setof(USB_OTG_HCCHAR_CHDIS_Pos);
			hcchar.rstof(USB_OTG_HCCHAR_CHENA_Pos);
			hcchar.rstof(USB_OTG_HCCHAR_EPDIR_Pos);
		}
		for (stduint i = 0; i <= 15; i++) {
			Reference hcchar(base + USB_OTG_HOST_CHANNEL_BASE + i * USB_OTG_HOST_CHANNEL_SIZE + 0x000);
			hcchar.setof(USB_OTG_HCCHAR_CHDIS_Pos);
			hcchar.setof(USB_OTG_HCCHAR_CHENA_Pos);
			hcchar.rstof(USB_OTG_HCCHAR_EPDIR_Pos);
			stduint count = 0;
			do {
				if (++count > 1000) break;
			} while (hcchar.bitof(USB_OTG_HCCHAR_CHENA_Pos));
		}
		Reference(base + USB_OTG_HOST_BASE + 0x14) = 0xFFFFFFFF;// HAINT
		Reference(base + 0x014) = 0xFFFFFFFF;// GINTSTS
		enGlobalInt(true);
		return true;
	}

	// AKA USB_DevInit
	bool OTG::InitializeDevice(stduint base, bool vbus_sensing_enable, byte speed, byte dev_endpoints, bool dma_enable, bool sof_enable) {
		Reference gccfg(base + 0x038);
		Reference gotgctl(base + 0x000);
		Reference dcfg(base + USB_OTG_DEVICE_BASE + 0x00);
		Reference dthrctl(base + USB_OTG_DEVICE_BASE + 0x30);
		Reference gintmsk(base + 0x018);
		Reference gintsts(base + 0x014);
		// VBUS sensing
		gccfg.setof(USB_OTG_GCCFG_VBDEN_Pos);
		if (!vbus_sensing_enable) {
			gccfg.rstof(USB_OTG_GCCFG_VBDEN_Pos);
			gotgctl.setof(USB_OTG_GOTGCTL_BVALOEN_Pos);
			gotgctl.setof(USB_OTG_GOTGCTL_BVALOVAL_Pos);
		}
		Reference(base + USB_OTG_PCGCCTL_BASE) = 0;// PCGCCTL
		dcfg |= (0x3U << 11);// DCFG.FRAME_INTERVAL_80 (80% frame interval)
		setDevSpeed(base, speed);
		FlushTxFifo(base, 0x10);
		FlushRxFifo(base);
		Reference(base + USB_OTG_DEVICE_BASE + 0x10) = 0;// DIEPMSK
		Reference(base + USB_OTG_DEVICE_BASE + 0x14) = 0;// DOEPMSK
		Reference(base + USB_OTG_DEVICE_BASE + 0x18) = 0xFFFFFFFF;// DAINT
		Reference(base + USB_OTG_DEVICE_BASE + 0x1C) = 0;// DAINTMSK
		for (stduint i = 0; i < dev_endpoints; i++) {
			Reference diepctl(base + USB_OTG_IN_ENDPOINT_BASE + i * USB_OTG_EP_REG_SIZE + 0x000);
			if (diepctl.bitof(USB_OTG_DIEPCTL_EPENA_Pos)) {
				diepctl.setof(USB_OTG_DIEPCTL_EPDIS_Pos);
				diepctl.setof(USB_OTG_DIEPCTL_SNAK_Pos);
			}
			else diepctl = 0;
			Reference(base + USB_OTG_IN_ENDPOINT_BASE + i * USB_OTG_EP_REG_SIZE + 0x008) = 0;// DIEPTSIZ
			Reference(base + USB_OTG_IN_ENDPOINT_BASE + i * USB_OTG_EP_REG_SIZE + 0x004) = 0xFF;// DIEPINT
		}
		for (stduint i = 0; i < dev_endpoints; i++) {
			Reference doepctl(base + USB_OTG_OUT_ENDPOINT_BASE + i * USB_OTG_EP_REG_SIZE + 0x000);
			if (doepctl.bitof(USB_OTG_DOEPCTL_EPENA_Pos)) {
				doepctl.setof(USB_OTG_DOEPCTL_EPDIS_Pos);
				doepctl.setof(USB_OTG_DOEPCTL_SNAK_Pos);
			}
			else doepctl = 0;
			Reference(base + USB_OTG_OUT_ENDPOINT_BASE + i * USB_OTG_EP_REG_SIZE + 0x008) = 0;// DOEPTSIZ
			Reference(base + USB_OTG_OUT_ENDPOINT_BASE + i * USB_OTG_EP_REG_SIZE + 0x004) = 0xFF;// DOEPINT
		}
		Reference(base + USB_OTG_DEVICE_BASE + 0x10).rstof(USB_OTG_DIEPMSK_TXFURM_Pos);
		if (dma_enable) {
			dthrctl = (0x8U << USB_OTG_DTHRCTL_TXTHRLEN_Pos) | (0x8U << USB_OTG_DTHRCTL_RXTHRLEN_Pos);
			dthrctl.setof(USB_OTG_DTHRCTL_RXTHREN_Pos);
			dthrctl.setof(USB_OTG_DTHRCTL_ISOTHREN_Pos);
			dthrctl.setof(USB_OTG_DTHRCTL_NONISOTHREN_Pos);
			dthrctl |= 0x08000000U;
		}
		gintmsk = 0;
		gintsts = 0xBFFFFFFF;
		if (!dma_enable) gintmsk.setof(USB_OTG_GINTMSK_RXFLVLM_Pos);
		gintmsk.setof(USB_OTG_GINTMSK_USBSUSPM_Pos);
		gintmsk.setof(USB_OTG_GINTMSK_USBRST_Pos);
		gintmsk.setof(USB_OTG_GINTMSK_ENUMDNEM_Pos);
		gintmsk.setof(USB_OTG_GINTMSK_IEPINT_Pos);
		gintmsk.setof(USB_OTG_GINTMSK_OEPINT_Pos);
		gintmsk.setof(USB_OTG_GINTMSK_IISOIXFRM_Pos);
		gintmsk.setof(USB_OTG_GINTMSK_PXFRM_IISOOXFRM_Pos);
		gintmsk.setof(USB_OTG_GINTMSK_WUIM_Pos);
		if (sof_enable) gintmsk.setof(USB_OTG_GINTMSK_SOFM_Pos);
		if (vbus_sensing_enable) {
			gintmsk.setof(USB_OTG_GINTMSK_SRQIM_Pos);
			gintmsk.setof(USB_OTG_GINTMSK_OTGINT_Pos);
		}
		return true;
	}

	// AKA USB_SetDevSpeed / USB_GetDevSpeed
	void OTG::setDevSpeed(stduint base, byte speed) {
		Reference dcfg(base + USB_OTG_DEVICE_BASE + 0x00);
		dcfg.maset(0, 2, speed);// DCFG.DEVSPD[1:0]
	}
	byte OTG::getDevSpeed(stduint base) {
		stduint spd = Reference(base + USB_OTG_DEVICE_BASE + 0x08).masof(USB_OTG_DSTS_ENUMSPD_Pos, 2);
		if (spd == 0) return 0;// HIGH
		if (spd == 1 || spd == 2) return 2;// FULL
		return 3;// LOW
	}

	// AKA USB_SetDevAddress
	void OTG::setDevAddress(stduint base, byte address) {
		Reference dcfg(base + USB_OTG_DEVICE_BASE + 0x00);
		dcfg.maset(USB_OTG_DCFG_DAD_Pos, 7, address);
	}

	// AKA USB_DevConnect / USB_DevDisconnect
	void OTG::DevConnect(stduint base) {
		Reference(base + USB_OTG_DEVICE_BASE + 0x04).rstof(USB_OTG_DCTL_SDIS_Pos);// DCTL
	}
	void OTG::DevDisconnect(stduint base) {
		Reference(base + USB_OTG_DEVICE_BASE + 0x04).setof(USB_OTG_DCTL_SDIS_Pos);
	}

	// AKA USB_ActivateEndpoint
	bool OTG::ActivateEndpoint(stduint base, OTGEP& ep) {
		if (ep.is_in) {
			Reference daintmsk(base + USB_OTG_DEVICE_BASE + 0x1C);
			daintmsk.setof(ep.num & 0xF);// IEPM bit ep
			Reference diepctl(base + USB_OTG_IN_ENDPOINT_BASE + ep.num * USB_OTG_EP_REG_SIZE + 0x000);
			if (!diepctl.bitof(USB_OTG_DIEPCTL_USBAEP_Pos)) {
				diepctl.maset(USB_OTG_DIEPCTL_MPSIZ_Pos, 11, ep.maxpacket);
				diepctl.maset(USB_OTG_DIEPCTL_EPTYP_Pos, 2, ep.type);
				diepctl.maset(22, 4, ep.num);// DIEPCTL.EPNUM
				diepctl.setof(USB_OTG_DIEPCTL_SD0PID_SEVNFRM_Pos);
				diepctl.setof(USB_OTG_DIEPCTL_USBAEP_Pos);
			}
		}
		else {
			Reference daintmsk(base + USB_OTG_DEVICE_BASE + 0x1C);
			daintmsk.setof(16 + (ep.num & 0xF));// OEPM bit 16+ep
			Reference doepctl(base + USB_OTG_OUT_ENDPOINT_BASE + ep.num * USB_OTG_EP_REG_SIZE + 0x000);
			if (!doepctl.bitof(USB_OTG_DOEPCTL_USBAEP_Pos)) {
				doepctl.maset(USB_OTG_DOEPCTL_MPSIZ_Pos, 11, ep.maxpacket);
				doepctl.maset(USB_OTG_DOEPCTL_EPTYP_Pos, 2, ep.type);
				doepctl.setof(USB_OTG_DOEPCTL_SD0PID_SEVNFRM_Pos);
				doepctl.setof(USB_OTG_DOEPCTL_USBAEP_Pos);
			}
		}
		return true;
	}

	// AKA USB_DeactivateEndpoint
	bool OTG::DeactivateEndpoint(stduint base, OTGEP& ep) {
		if (ep.is_in) {
			Reference diepctl(base + USB_OTG_IN_ENDPOINT_BASE + ep.num * USB_OTG_EP_REG_SIZE + 0x000);
			if (!diepctl.bitof(USB_OTG_DIEPCTL_EPENA_Pos)) {
				diepctl.setof(USB_OTG_DIEPCTL_EPDIS_Pos);
				diepctl.setof(USB_OTG_DIEPCTL_SNAK_Pos);
			}
			Reference(base + USB_OTG_IN_ENDPOINT_BASE + ep.num * USB_OTG_EP_REG_SIZE + 0x008) = 0;// DIEPTSIZ
			Reference(base + USB_OTG_IN_ENDPOINT_BASE + ep.num * USB_OTG_EP_REG_SIZE + 0x004) = 0xFF;// DIEPINT
		}
		else {
			Reference doepctl(base + USB_OTG_OUT_ENDPOINT_BASE + ep.num * USB_OTG_EP_REG_SIZE + 0x000);
			if (!doepctl.bitof(USB_OTG_DOEPCTL_EPENA_Pos)) {
				doepctl.setof(USB_OTG_DOEPCTL_EPDIS_Pos);
				doepctl.setof(USB_OTG_DOEPCTL_SNAK_Pos);
			}
			Reference(base + USB_OTG_OUT_ENDPOINT_BASE + ep.num * USB_OTG_EP_REG_SIZE + 0x008) = 0;// DOEPTSIZ
			Reference(base + USB_OTG_OUT_ENDPOINT_BASE + ep.num * USB_OTG_EP_REG_SIZE + 0x004) = 0xFF;// DOEPINT
		}
		return true;
	}

	// AKA USB_ActivateDedicatedEndpoint: like ActivateEndpoint but the EP1
	// interrupt uses the dedicated DEACHMSK path instead of DAINTMSK.
	bool OTG::ActivateDedicatedEndpoint(stduint base, OTGEP& ep) {
		if (ep.is_in) {
			Reference diepctl(base + USB_OTG_IN_ENDPOINT_BASE + ep.num * USB_OTG_EP_REG_SIZE + 0x000);
			if (!diepctl.bitof(USB_OTG_DIEPCTL_USBAEP_Pos)) {
				diepctl.maset(USB_OTG_DIEPCTL_MPSIZ_Pos, 11, ep.maxpacket);
				diepctl.maset(USB_OTG_DIEPCTL_EPTYP_Pos, 2, ep.type);
				diepctl.maset(USB_OTG_DIEPCTL_TXFNUM_Pos, 4, ep.num);// DIEPCTL.TXFNUM
				diepctl.setof(USB_OTG_DIEPCTL_SD0PID_SEVNFRM_Pos);
				diepctl.setof(USB_OTG_DIEPCTL_USBAEP_Pos);
			}
			Reference(base + USB_OTG_DEVICE_BASE + 0x3C).setof(ep.num & 0xF);// DEACHMSK.IEPM
		}
		else {
			Reference doepctl(base + USB_OTG_OUT_ENDPOINT_BASE + ep.num * USB_OTG_EP_REG_SIZE + 0x000);
			if (!doepctl.bitof(USB_OTG_DOEPCTL_USBAEP_Pos)) {
				doepctl.maset(USB_OTG_DOEPCTL_MPSIZ_Pos, 11, ep.maxpacket);
				doepctl.maset(USB_OTG_DOEPCTL_EPTYP_Pos, 2, ep.type);
				doepctl.setof(USB_OTG_DOEPCTL_SD0PID_SEVNFRM_Pos);
				doepctl.setof(USB_OTG_DOEPCTL_USBAEP_Pos);
			}
			Reference(base + USB_OTG_DEVICE_BASE + 0x3C).setof(16 + (ep.num & 0xF));// DEACHMSK.OEPM
		}
		return true;
	}

	// AKA USB_DeactivateDedicatedEndpoint
	bool OTG::DeactivateDedicatedEndpoint(stduint base, OTGEP& ep) {
		if (ep.is_in) {
			Reference diepctl(base + USB_OTG_IN_ENDPOINT_BASE + ep.num * USB_OTG_EP_REG_SIZE + 0x000);
			diepctl.rstof(USB_OTG_DIEPCTL_USBAEP_Pos);
			Reference(base + USB_OTG_DEVICE_BASE + 0x1C).rstof(ep.num & 0xF);// DAINTMSK.IEPM
		}
		else {
			Reference doepctl(base + USB_OTG_OUT_ENDPOINT_BASE + ep.num * USB_OTG_EP_REG_SIZE + 0x000);
			doepctl.rstof(USB_OTG_DOEPCTL_USBAEP_Pos);
			Reference(base + USB_OTG_DEVICE_BASE + 0x1C).rstof(16 + (ep.num & 0xF));// DAINTMSK.OEPM
		}
		return true;
	}

	// AKA USB_EPStartXfer
	bool OTG::StartEndpointXfer(stduint base, OTGEP& ep, bool dma) {
		if (ep.is_in) {
			Reference dieptsiz(base + USB_OTG_IN_ENDPOINT_BASE + ep.num * USB_OTG_EP_REG_SIZE + 0x008);
			Reference diepctl(base + USB_OTG_IN_ENDPOINT_BASE + ep.num * USB_OTG_EP_REG_SIZE + 0x000);
			dieptsiz.maset(USB_OTG_DIEPTSIZ_XFRSIZ_Pos, 19, 0);
			dieptsiz.maset(USB_OTG_DIEPTSIZ_PKTCNT_Pos, 10, 0);
			if (ep.xfer_len == 0) {
				dieptsiz.maset(USB_OTG_DIEPTSIZ_PKTCNT_Pos, 10, 1);
			}
			else {
				stduint pktcnt = (ep.xfer_len + ep.maxpacket - 1) / ep.maxpacket;
				dieptsiz.maset(USB_OTG_DIEPTSIZ_PKTCNT_Pos, 10, pktcnt);
				dieptsiz.maset(USB_OTG_DIEPTSIZ_XFRSIZ_Pos, 19, ep.xfer_len);
				if (ep.type == 1) {// ISOC
					dieptsiz.maset(USB_OTG_DIEPTSIZ_MULCNT_Pos, 2, 1);
				}
			}
			if (dma) Reference(base + USB_OTG_IN_ENDPOINT_BASE + ep.num * USB_OTG_EP_REG_SIZE + 0x00C) = ep.dma_addr;// DIEPDMA
			else if (ep.type != 1 && ep.xfer_len > 0)
				Reference(base + USB_OTG_DEVICE_BASE + 0x34).setof(ep.num & 0xF);// DIEPEMPMSK
			if (ep.type == 1) {// ISOC odd/even
				if (!(Reference(base + USB_OTG_DEVICE_BASE + 0x08) & (1U << 8))) diepctl.setof(USB_OTG_DIEPCTL_SODDFRM_Pos);
				else diepctl.setof(USB_OTG_DIEPCTL_SD0PID_SEVNFRM_Pos);
			}
			diepctl.setof(USB_OTG_DIEPCTL_CNAK_Pos);
			diepctl.setof(USB_OTG_DIEPCTL_EPENA_Pos);
			if (ep.type == 1) WritePacket(base, ep.xfer_buff, ep.num, ep.xfer_len);
		}
		else {
			Reference doetsiz(base + USB_OTG_OUT_ENDPOINT_BASE + ep.num * USB_OTG_EP_REG_SIZE + 0x008);
			Reference doepctl(base + USB_OTG_OUT_ENDPOINT_BASE + ep.num * USB_OTG_EP_REG_SIZE + 0x000);
			doetsiz.maset(USB_OTG_DOEPTSIZ_XFRSIZ_Pos, 19, 0);
			doetsiz.maset(USB_OTG_DOEPTSIZ_PKTCNT_Pos, 10, 0);
			if (ep.xfer_len == 0) {
				doetsiz.maset(USB_OTG_DOEPTSIZ_XFRSIZ_Pos, 19, ep.maxpacket);
				doetsiz.maset(USB_OTG_DOEPTSIZ_PKTCNT_Pos, 10, 1);
			}
			else {
				stduint pktcnt = (ep.xfer_len + ep.maxpacket - 1) / ep.maxpacket;
				doetsiz.maset(USB_OTG_DOEPTSIZ_PKTCNT_Pos, 10, pktcnt);
				doetsiz.maset(USB_OTG_DOEPTSIZ_XFRSIZ_Pos, 19, ep.maxpacket * pktcnt);
			}
			if (dma) Reference(base + USB_OTG_OUT_ENDPOINT_BASE + ep.num * USB_OTG_EP_REG_SIZE + 0x00C) = (stduint)ep.xfer_buff;// DOEPDMA
			if (ep.type == 1) {
				if (!(Reference(base + USB_OTG_DEVICE_BASE + 0x08) & (1U << 8))) doepctl.setof(USB_OTG_DOEPCTL_SODDFRM_Pos);
				else doepctl.setof(USB_OTG_DOEPCTL_SD0PID_SEVNFRM_Pos);
			}
			doepctl.setof(USB_OTG_DOEPCTL_CNAK_Pos);
			doepctl.setof(USB_OTG_DOEPCTL_EPENA_Pos);
		}
		return true;
	}

	// AKA USB_EP0StartXfer
	bool OTG::StartEP0Xfer(stduint base, OTGEP& ep, bool dma) {
		if (ep.is_in) {
			Reference dieptsiz(base + USB_OTG_IN_ENDPOINT_BASE + 0x008);
			Reference diepctl(base + USB_OTG_IN_ENDPOINT_BASE + 0x000);
			dieptsiz.maset(USB_OTG_DIEPTSIZ_XFRSIZ_Pos, 19, 0);
			dieptsiz.maset(USB_OTG_DIEPTSIZ_PKTCNT_Pos, 10, 0);
			if (ep.xfer_len == 0) {
				dieptsiz.maset(USB_OTG_DIEPTSIZ_PKTCNT_Pos, 10, 1);
			}
			else {
				if (ep.xfer_len > ep.maxpacket) ep.xfer_len = ep.maxpacket;
				dieptsiz.maset(USB_OTG_DIEPTSIZ_PKTCNT_Pos, 10, 1);
				dieptsiz.maset(USB_OTG_DIEPTSIZ_XFRSIZ_Pos, 19, ep.xfer_len);
			}
			if (dma) Reference(base + USB_OTG_IN_ENDPOINT_BASE + 0x00C) = ep.dma_addr;
			else if (ep.xfer_len > 0) Reference(base + USB_OTG_DEVICE_BASE + 0x34).setof(0);// DIEPEMPMSK ep0
			diepctl.setof(USB_OTG_DIEPCTL_CNAK_Pos);
			diepctl.setof(USB_OTG_DIEPCTL_EPENA_Pos);
		}
		else {
			Reference doetsiz(base + USB_OTG_OUT_ENDPOINT_BASE + 0x008);
			Reference doepctl(base + USB_OTG_OUT_ENDPOINT_BASE + 0x000);
			doetsiz.maset(USB_OTG_DOEPTSIZ_XFRSIZ_Pos, 19, 0);
			doetsiz.maset(USB_OTG_DOEPTSIZ_PKTCNT_Pos, 10, 0);
			if (ep.xfer_len > 0) ep.xfer_len = ep.maxpacket;
			doetsiz.maset(USB_OTG_DOEPTSIZ_PKTCNT_Pos, 10, 1);
			doetsiz.maset(USB_OTG_DOEPTSIZ_XFRSIZ_Pos, 19, ep.maxpacket);
			if (dma) Reference(base + USB_OTG_OUT_ENDPOINT_BASE + 0x00C) = (stduint)ep.xfer_buff;
			doepctl.setof(USB_OTG_DOEPCTL_CNAK_Pos);
			doepctl.setof(USB_OTG_DOEPCTL_EPENA_Pos);
		}
		return true;
	}

	// AKA USB_EP0_OutStart
	bool OTG::StartEP0Out(stduint base, bool dma, byte* psetup) {
		Reference doetsiz(base + USB_OTG_OUT_ENDPOINT_BASE + 0x008);
		Reference doepctl(base + USB_OTG_OUT_ENDPOINT_BASE + 0x000);
		doetsiz = 0;
		doetsiz.maset(USB_OTG_DOEPTSIZ_PKTCNT_Pos, 10, 1);
		doetsiz.maset(USB_OTG_DOEPTSIZ_XFRSIZ_Pos, 19, 3 * 8);
		doetsiz.maset(USB_OTG_DOEPTSIZ_STUPCNT_Pos, 2, 3);
		if (dma) {
			Reference(base + USB_OTG_OUT_ENDPOINT_BASE + 0x00C) = (stduint)psetup;// DOEPDMA
			doepctl = 0x80008000;
		}
		return true;
	}

	// AKA USB_EPSetStall / USB_EPClearStall
	bool OTG::ConfigStall(stduint base, OTGEP& ep, bool set_or_reset) {
		Reference ctl(ep.is_in
			? base + USB_OTG_IN_ENDPOINT_BASE + ep.num * USB_OTG_EP_REG_SIZE + 0x000
			: base + USB_OTG_OUT_ENDPOINT_BASE + ep.num * USB_OTG_EP_REG_SIZE + 0x000);
		if (set_or_reset) {
			if (!ctl.bitof(USB_OTG_DIEPCTL_EPENA_Pos)) ctl.rstof(USB_OTG_DIEPCTL_EPDIS_Pos);
			ctl.setof(USB_OTG_DIEPCTL_STALL_Pos);
		}
		else {
			ctl.rstof(USB_OTG_DIEPCTL_STALL_Pos);
			if (ep.type == 3 || ep.type == 2) ctl.setof(USB_OTG_DIEPCTL_SD0PID_SEVNFRM_Pos);// INTR|BULK -> DATA0
		}
		return true;
	}

	// AKA USB_ReadDevAllOutEpInterrupt / ReadDevAllInEpInterrupt / ReadDevOutEPInterrupt / ReadDevInEPInterrupt
	stduint OTG::ReadDevAllOutEpInterrupt(stduint base) {
		return (Reference(base + USB_OTG_DEVICE_BASE + 0x18) & Reference(base + USB_OTG_DEVICE_BASE + 0x1C) & 0xFFFF0000U) >> 16;
	}
	stduint OTG::ReadDevAllInEpInterrupt(stduint base) {
		return Reference(base + USB_OTG_DEVICE_BASE + 0x18) & Reference(base + USB_OTG_DEVICE_BASE + 0x1C) & 0xFFFF;
	}
	stduint OTG::ReadDevOutEPInterrupt(stduint base, byte epnum) {
		return Reference(base + USB_OTG_OUT_ENDPOINT_BASE + epnum * USB_OTG_EP_REG_SIZE + 0x004) & Reference(base + USB_OTG_DEVICE_BASE + 0x14);
	}
	stduint OTG::ReadDevInEPInterrupt(stduint base, byte epnum) {
		stduint msk = Reference(base + USB_OTG_DEVICE_BASE + 0x10);
		stduint emp = Reference(base + USB_OTG_DEVICE_BASE + 0x34);
		msk |= ((emp >> epnum) & 0x1) << 7;
		return Reference(base + USB_OTG_IN_ENDPOINT_BASE + epnum * USB_OTG_EP_REG_SIZE + 0x004) & msk;
	}

	// AKA USB_HostInit
	bool OTG::InitializeHost(stduint base, byte speed, byte host_channels, bool dma_enable) {
		Reference gccfg(base + 0x038);
		Reference hcfg(base + USB_OTG_HOST_BASE + 0x00);
		Reference gintmsk(base + 0x018);
		Reference gintsts(base + 0x014);
		Reference grxfsiz(base + 0x024);
		Reference nptxfsiz(base + 0x028);
		Reference hptxfsiz(base + 0x100);
		Reference(base + USB_OTG_PCGCCTL_BASE) = 0;// PCGCCTL
		gccfg.setof(USB_OTG_GCCFG_VBDEN_Pos);
		if (speed == 2 && base != _OTG2_FS_ADDR) hcfg.setof(USB_OTG_HCFG_FSLSS_Pos);// FULL speed, not OTG2_FS
		else hcfg.rstof(USB_OTG_HCFG_FSLSS_Pos);
		FlushTxFifo(base, 0x10);
		FlushRxFifo(base);
		for (stduint i = 0; i < host_channels; i++) {
			Reference(base + USB_OTG_HOST_CHANNEL_BASE + i * USB_OTG_HOST_CHANNEL_SIZE + 0x004) = 0xFFFFFFFF;// HCINT
			Reference(base + USB_OTG_HOST_CHANNEL_BASE + i * USB_OTG_HOST_CHANNEL_SIZE + 0x008) = 0;// HCINTMSK
		}
		// VBUS drive (AKA USB_DriveVbus): GCCFG.NOVBUSSENS, plus HPRT.PPWR set by HCD Start
		gintmsk = 0;
		gintsts = 0xFFFFFFFF;
		if (base == _OTG2_FS_ADDR) {
			grxfsiz = 0x80;
			nptxfsiz = ((0x60 << USB_OTG_NPTXFD_Pos) & USB_OTG_NPTXFD) | 0x80;
			hptxfsiz = ((0x40 << USB_OTG_HPTXFSIZ_PTXFD_Pos) & USB_OTG_HPTXFSIZ_PTXFD) | 0xE0;
		}
		else {
			grxfsiz = 0x200;
			nptxfsiz = ((0x100 << USB_OTG_NPTXFD_Pos) & USB_OTG_NPTXFD) | 0x200;
			hptxfsiz = ((0xE0 << USB_OTG_HPTXFSIZ_PTXFD_Pos) & USB_OTG_HPTXFSIZ_PTXFD) | 0x300;
		}
		if (!dma_enable) gintmsk.setof(USB_OTG_GINTMSK_RXFLVLM_Pos);
		gintmsk.setof(USB_OTG_GINTMSK_PRTIM_Pos);
		gintmsk.setof(USB_OTG_GINTMSK_HCIM_Pos);
		gintmsk.setof(USB_OTG_GINTMSK_SOFM_Pos);
		gintmsk.setof(USB_OTG_GINTSTS_DISCINT_Pos);
		gintmsk.setof(USB_OTG_GINTMSK_PXFRM_IISOOXFRM_Pos);
		gintmsk.setof(USB_OTG_GINTMSK_WUIM_Pos);
		return true;
	}

	// AKA USB_InitFSLSPClkSel
	void OTG::InitFSLSPClkSel(stduint base, byte freq) {
		Reference hcfg(base + USB_OTG_HOST_BASE + 0x00);
		Reference hfir(base + USB_OTG_HOST_BASE + 0x04);
		hcfg.maset(USB_OTG_HCFG_FSLSPCS_Pos, 2, freq);
		if (freq == 1) hfir = 48000;// 48MHz
		else if (freq == 2) hfir = 6000;// 6MHz
	}

	// AKA USB_ResetPort
	bool OTG::ResetPort(stduint base) {
		Reference hprt(base + USB_OTG_HOST_PORT_BASE);
		stduint hprt0 = hprt;
		hprt0 &= ~(USB_OTG_HPRT_PENA | USB_OTG_HPRT_PCDET | USB_OTG_HPRT_PENCHNG | USB_OTG_HPRT_POCCHNG);
		hprt = USB_OTG_HPRT_PRST | hprt0;
		SysDelay_ms(100);
		hprt = (~USB_OTG_HPRT_PRST) & hprt0;
		return true;
	}

	// AKA USB_HC_Init
	bool OTG::InitializeHostChannel(stduint base, byte ch_num, byte epnum, byte dev_address, byte speed, byte ep_type, uint16 mps) {
		Reference hcint(base + USB_OTG_HOST_CHANNEL_BASE + ch_num * USB_OTG_HOST_CHANNEL_SIZE + 0x004);
		Reference hcintmsk(base + USB_OTG_HOST_CHANNEL_BASE + ch_num * USB_OTG_HOST_CHANNEL_SIZE + 0x008);
		Reference hcchar(base + USB_OTG_HOST_CHANNEL_BASE + ch_num * USB_OTG_HOST_CHANNEL_SIZE + 0x000);
		hcint = 0xFFFFFFFF;
		stduint msk = 0;
		if (ep_type == 0 || ep_type == 2) {// CTRL|BULK
			msk = USB_OTG_HCINTMSK_XFRCM | USB_OTG_HCINTMSK_STALLM | USB_OTG_HCINTMSK_TXERRM
				| USB_OTG_HCINTMSK_DTERRM | USB_OTG_HCINTMSK_AHBERR | USB_OTG_HCINTMSK_NAKM;
			if (epnum & 0x80) msk |= USB_OTG_HCINTMSK_BBERRM;
			else if (base != _OTG2_FS_ADDR) msk |= USB_OTG_HCINTMSK_NYET | USB_OTG_HCINTMSK_ACKM;
		}
		else if (ep_type == 3) {// INTR
			msk = USB_OTG_HCINTMSK_XFRCM | USB_OTG_HCINTMSK_STALLM | USB_OTG_HCINTMSK_TXERRM
				| USB_OTG_HCINTMSK_DTERRM | USB_OTG_HCINTMSK_NAKM | USB_OTG_HCINTMSK_AHBERR | USB_OTG_HCINTMSK_FRMORM;
			if (epnum & 0x80) msk |= USB_OTG_HCINTMSK_BBERRM;
		}
		else if (ep_type == 1) {// ISOC
			msk = USB_OTG_HCINTMSK_XFRCM | USB_OTG_HCINTMSK_ACKM | USB_OTG_HCINTMSK_AHBERR | USB_OTG_HCINTMSK_FRMORM;
			if (epnum & 0x80) msk |= USB_OTG_HCINTMSK_TXERRM | USB_OTG_HCINTMSK_BBERRM;
		}
		hcintmsk = msk;
		Reference(base + USB_OTG_HOST_BASE + 0x18).setof(ch_num & 0xF);// HAINTMSK
		Reference(base + 0x018).setof(USB_OTG_GINTMSK_HCIM_Pos);
		hcchar = ((dev_address << USB_OTG_HCCHAR_DAD_Pos) & USB_OTG_HCCHAR_DAD)
			| (((epnum & 0x7F) << USB_OTG_HCCHAR_EPNUM_Pos) & USB_OTG_HCCHAR_EPNUM)
			| ((((epnum & 0x80) == 0x80) ? 1U : 0U) << USB_OTG_HCCHAR_EPDIR_Pos)
			| (((speed == 3) ? 1U : 0U) << USB_OTG_HCCHAR_LSDEV_Pos)
			| ((ep_type << USB_OTG_HCCHAR_EPTYP_Pos) & USB_OTG_HCCHAR_EPTYP)
			| (mps & USB_OTG_HCCHAR_MPSIZ);
		if (ep_type == 3) hcchar.setof(USB_OTG_HCCHAR_ODDFRM_Pos);
		return true;
	}

	// AKA USB_HC_StartXfer
	bool OTG::StartHostChannelXfer(stduint base, OTGHC& hc, bool dma) {
		Reference hcchar(base + USB_OTG_HOST_CHANNEL_BASE + hc.ch_num * USB_OTG_HOST_CHANNEL_SIZE + 0x000);
		Reference hctsiz(base + USB_OTG_HOST_CHANNEL_BASE + hc.ch_num * USB_OTG_HOST_CHANNEL_SIZE + 0x010);
		Reference hcintmsk(base + USB_OTG_HOST_CHANNEL_BASE + hc.ch_num * USB_OTG_HOST_CHANNEL_SIZE + 0x008);
		uint16 num_packets;
		if (base != _OTG2_FS_ADDR && hc.speed == 0) {// HS
			if (!dma && hc.do_ping) return DoPing(base, hc.ch_num);
			else if (dma) {
				hcintmsk &= ~(USB_OTG_HCINTMSK_NYET | USB_OTG_HCINTMSK_ACKM);
				hc.do_ping = 0;
			}
		}
		if (hc.xfer_len > 0) {
			num_packets = (uint16)((hc.xfer_len + hc.max_packet - 1) / hc.max_packet);
			if (num_packets > 256) {
				num_packets = 256;
				hc.xfer_len = num_packets * hc.max_packet;
			}
		}
		else num_packets = 1;
		if (hc.ep_is_in) hc.xfer_len = num_packets * hc.max_packet;
		hctsiz = (hc.xfer_len & USB_OTG_HCTSIZ_XFRSIZ)
			| ((num_packets << USB_OTG_HCTSIZ_PKTCNT_Pos) & USB_OTG_HCTSIZ_PKTCNT)
			| ((hc.data_pid << USB_OTG_HCTSIZ_DPID_Pos) & USB_OTG_HCTSIZ_DPID);
		if (dma) Reference(base + USB_OTG_HOST_CHANNEL_BASE + hc.ch_num * USB_OTG_HOST_CHANNEL_SIZE + 0x014) = (stduint)hc.xfer_buff;// HCDMA
		stduint is_oddframe = (Reference(base + USB_OTG_HOST_BASE + 0x08) & 0x1) ? 0 : 1;// HFNUM
		hcchar.rstof(USB_OTG_HCCHAR_ODDFRM_Pos);
		hcchar.setof(USB_OTG_HCCHAR_ODDFRM_Pos, is_oddframe ? true : false);
		hcchar.rstof(USB_OTG_HCCHAR_CHDIS_Pos);
		hcchar.setof(USB_OTG_HCCHAR_CHENA_Pos);
		if (!dma && !hc.ep_is_in && hc.xfer_len > 0) {
			stduint len_words = (hc.xfer_len + 3) / 4;
			if (hc.ep_type == 0 || hc.ep_type == 2) {
				if (len_words > (Reference(base + 0x02C) & 0xFFFF))// HNPTXSTS
					Reference(base + 0x018).setof(USB_OTG_GINTMSK_NPTXFEM_Pos);
			}
			else {
				if (len_words > (Reference(base + USB_OTG_HOST_BASE + 0x10) & 0xFFFF))// HPTXSTS
					Reference(base + 0x018).setof(USB_OTG_GINTMSK_PTXFEM_Pos);
			}
			WritePacket(base, hc.xfer_buff, hc.ch_num, hc.xfer_len);
		}
		return true;
	}

	// AKA USB_HC_ReadInterrupt
	stduint OTG::ReadHostChannelInterrupt(stduint base) {
		return Reference(base + USB_OTG_HOST_BASE + 0x14) & 0xFFFF;// HAINT
	}

	// AKA USB_HC_Halt
	bool OTG::HaltHostChannel(stduint base, byte hc_num) {
		Reference hcchar(base + USB_OTG_HOST_CHANNEL_BASE + hc_num * USB_OTG_HOST_CHANNEL_SIZE + 0x000);
		hcchar.setof(USB_OTG_HCCHAR_CHDIS_Pos);
		stduint eptyp = hcchar.masof(USB_OTG_HCCHAR_EPTYP_Pos, 2);
		if (eptyp == 0 || eptyp == 2) {// CTRL|BULK
			if ((Reference(base + 0x02C) & 0xFFFF) == 0) {// HNPTXSTS
				hcchar.rstof(USB_OTG_HCCHAR_CHENA_Pos);
				hcchar.setof(USB_OTG_HCCHAR_CHENA_Pos);
				hcchar.rstof(USB_OTG_HCCHAR_EPDIR_Pos);
				stduint count = 0;
				do { if (++count > 1000) break; } while (hcchar.bitof(USB_OTG_HCCHAR_CHENA_Pos));
			}
			else hcchar.setof(USB_OTG_HCCHAR_CHENA_Pos);
		}
		else {
			if ((Reference(base + USB_OTG_HOST_BASE + 0x10) & 0xFFFF) == 0) {// HPTXSTS
				hcchar.rstof(USB_OTG_HCCHAR_CHENA_Pos);
				hcchar.setof(USB_OTG_HCCHAR_CHENA_Pos);
				hcchar.rstof(USB_OTG_HCCHAR_EPDIR_Pos);
				stduint count = 0;
				do { if (++count > 1000) break; } while (hcchar.bitof(USB_OTG_HCCHAR_CHENA_Pos));
			}
			else hcchar.setof(USB_OTG_HCCHAR_CHENA_Pos);
		}
		return true;
	}

	// AKA USB_DoPing
	bool OTG::DoPing(stduint base, byte ch_num) {
		Reference hctsiz(base + USB_OTG_HOST_CHANNEL_BASE + ch_num * USB_OTG_HOST_CHANNEL_SIZE + 0x010);
		Reference hcchar(base + USB_OTG_HOST_CHANNEL_BASE + ch_num * USB_OTG_HOST_CHANNEL_SIZE + 0x000);
		hctsiz = ((1U << USB_OTG_HCTSIZ_PKTCNT_Pos) & USB_OTG_HCTSIZ_PKTCNT) | USB_OTG_HCTSIZ_DOPING;
		hcchar.rstof(USB_OTG_HCCHAR_CHDIS_Pos);
		hcchar.setof(USB_OTG_HCCHAR_CHENA_Pos);
		return true;
	}

}

#endif // _MCU_STM32H7x

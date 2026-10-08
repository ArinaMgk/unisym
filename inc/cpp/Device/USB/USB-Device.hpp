#ifndef _USB_DEVICE_HPP
#define _USB_DEVICE_HPP
namespace uni::device::SpaceUSB {
	class ClassDriver;

	class USBHostDevice {
	public:
		virtual ~USBHostDevice();
		virtual Error ControlIn(EndpointID ep_id, SetupData setup_data,
			void* buf, int len, ClassDriver* issuer);
		virtual Error ControlOut(EndpointID ep_id, SetupData setup_data,
			const void* buf, int len, ClassDriver* issuer);
		virtual Error InterruptIn(EndpointID ep_id, void* buf, int len);
		virtual Error InterruptOut(EndpointID ep_id, void* buf, int len);
		// Bulk transport (AKA a host MSC disk): dir_in selects the IN/OUT direction.
		virtual Error BulkTransfer(EndpointID ep_id, bool dir_in, void* buf, int len);
		virtual Error IsochronousTransfer(EndpointID ep_id, void* buf, int len,
			const IsochronousTransferOptions& options = IsochronousTransferOptions{});
		virtual Error OnHubPortStatusReceived(uint8 port_num, uint16 status,
			uint16 change, uint8 speed_id = 0);
		virtual Error ConfigureHub(uint8 num_ports, uint16 characteristics) {
			(void)num_ports;
			(void)characteristics;
			return MAKE_ERROR(Error::kSuccess);
		}
		virtual uint8 HubDepth() const { return 0; }
		// a hub parent reports the port while a device on it still answers at address 0
		virtual uint8 HubAddressingPort() const { return 0; }
		// a hub parent reports whether one of its downstream devices holds the bus right now
		virtual bool ChildBusy() { return false; }
		// HPRT0.PSPD scale: 0 high, 1 full, 2 low speed
		virtual stduint Speed() const { return 1; }
		// AKA xHCI ConfigureEndpoints: the transport programs its channels here
		virtual Error ConfigureTransportEndpoints() { return MAKE_ERROR(Error::kSuccess); }
		virtual bool RequiresSetAddressRequest() const { return false; }
		// AKA USBH_LL_SetDeviceAddress: the transport follows the new address here
		virtual void OnDeviceAddressChanged(uint8 address) { (void)address; }

		Error StartInitialize();
		bool IsInitialized() { return is_initialized_; }
		// descriptors all read; true even when no class driver claimed the device
		bool IsEnumerated() const { return enumerated_; }
		EndpointConfig* EndpointConfigs() { return ep_configs_.data(); }
		int NumEndpointConfigs() { return num_ep_configs_; }
		// the class driver bound to an endpoint number (AKA class_drivers_[ep_num])
		ClassDriver* ClassDriverOf(int ep_num) {
			return (ep_num >= 0 && ep_num < 16) ? class_drivers_[ep_num] : nullptr;
		}
		Error OnEndpointsConfigured();
		// AKA the periodic tick: hand every class driver its timed work (about once per ms)
		Error ProcessDelayed();
		uint16 VendorID() const { return vendor_id_; }
		uint16 ProductID() const { return product_id_; }
		uint8 DeviceClass() const { return device_class_; }
		uint8 DeviceSubClass() const { return device_sub_class_; }
		uint8 DeviceProtocol() const { return device_protocol_; }
		uint16 USBRelease() const { return usb_release_; }
		bool HasBOS() const { return has_bos_; }
		bool HasUSB20ExtensionCapability() const { return has_usb20_extension_; }
		uint32 USB20ExtensionAttributes() const { return usb20_extension_attributes_; }
		bool HasSuperSpeedUSBCapability() const { return has_superspeed_usb_capability_; }
		const SuperSpeedUSBCapabilityDescriptor& SuperSpeedUSBCapability() const {
			return superspeed_usb_capability_;
		}
		bool HasSuperSpeedPlusUSBCapability() const { return has_superspeed_plus_usb_capability_; }
		const SuperSpeedPlusUSBCapabilityDescriptor& SuperSpeedPlusUSBCapability() const {
			return superspeed_plus_usb_capability_;
		}
		uint8 SuperSpeedPlusSublinkSpeedAttributeCount() const {
			return superspeed_plus_sublink_speed_attribute_count_;
		}
		const SuperSpeedPlusSublinkSpeedAttribute* SuperSpeedPlusSublinkSpeedAttributes() const {
			return superspeed_plus_sublink_speed_attributes_.data();
		}
		bool HasContainerID() const { return has_container_id_; }
		const uint8* ContainerID() const { return container_id_.data(); }
		uint8 HubNumPorts() const { return hub_num_ports_; }
		void SetHubNumPorts(uint8 num_ports) { hub_num_ports_ = num_ports; }
		// bPwrOn2PwrGood: units of 2 ms, from the hub descriptor
		uint8 HubPowerOnToPowerGood() const { return hub_power_on_to_power_good_; }
		void SetHubPowerOnToPowerGood(uint8 val) { hub_power_on_to_power_good_ = val; }
		// the USB address this device is given by SET_ADDRESS
		uint8 AssignedAddress() const { return assigned_address_; }
		void SetAssignedAddress(uint8 val) { assigned_address_ = val; }
		const char* ManufacturerString() const { return manufacturer_string_[0] ? manufacturer_string_.data() : nullptr; }
		const char* ProductString() const { return product_string_[0] ? product_string_.data() : nullptr; }
		const char* SerialString() const { return serial_string_[0] ? serial_string_.data() : nullptr; }

		uint8* Buffer() { return buf_.data(); }

	protected:
		Error OnControlCompleted(EndpointID ep_id, SetupData setup_data,
			const void* buf, int len);
		Error OnInterruptCompleted(EndpointID ep_id, const void* buf, int len);
		Error OnBulkCompleted(EndpointID ep_id, const void* buf, int len);
		Error OnIsochronousCompleted(EndpointID ep_id, const void* buf, int len,
			uint16 frame_id, bool schedule_immediately, int completion_code);

	private:
		void ReleaseClassDrivers();

		uni::Vector<ClassDriver*> class_driver_instances_{};

	 /** @brief Class driver assigned to each endpoint.
		  *
		  * Index is the endpoint number (0 - 15).
		  * Index 0 is always unused since no class driver uses it.
		  */
		uni::Array<ClassDriver*, 16> class_drivers_{};

		uni::Array<uint8, 256> buf_{};

		// following fields are used during initialization
		uint8 num_configurations_ = 0;
		uint8 config_index_ = 0;

		Error OnDeviceDescriptorReceived(const uint8* buf, int len);
		Error OnConfigurationDescriptorReceived(const uint8* buf, int len);
		Error OnSetConfigurationCompleted(uint8 config_value);

		bool is_initialized_ = false;
		bool enumerated_ = false;
		uint16 vendor_id_ = 0;
		uint16 product_id_ = 0;
		uint8 device_class_ = 0;
		uint8 device_sub_class_ = 0;
		uint8 device_protocol_ = 0;
		uint16 usb_release_ = 0;
		uint8* bos_buffer_ = nullptr;
		uint16 bos_buffer_length_ = 0;
		bool has_bos_ = false;
		bool has_usb20_extension_ = false;
		uint32 usb20_extension_attributes_ = 0;
		bool has_superspeed_usb_capability_ = false;
		SuperSpeedUSBCapabilityDescriptor superspeed_usb_capability_{};
		bool has_superspeed_plus_usb_capability_ = false;
		SuperSpeedPlusUSBCapabilityDescriptor superspeed_plus_usb_capability_{};
		uint8 superspeed_plus_sublink_speed_attribute_count_ = 0;
		uni::Array<SuperSpeedPlusSublinkSpeedAttribute, 32>
			superspeed_plus_sublink_speed_attributes_{};
		bool has_container_id_ = false;
		uni::Array<uint8, 16> container_id_{};
		uint8 manufacturer_index_ = 0;
		uint8 product_index_ = 0;
		uint8 serial_index_ = 0;
		uint8 hub_num_ports_ = 0;
		uint8 hub_power_on_to_power_good_ = 0;
		uint8 assigned_address_ = kDefaultDeviceAddress;
		// the enumeration request in flight: a failed control transfer is sent again
		SetupData enum_setup_{};
		void* enum_buf_ = nullptr;
		int enum_len_ = 0;
		int enum_retry_ = 0;
		bool enum_in_ = true;
		bool enum_pending_ = false;
		bool enum_resubmit_ = false;
		uint16 string_lang_id_ = 0x0409;
		uni::Array<char, 64> manufacturer_string_{};
		uni::Array<char, 64> product_string_{};
		uni::Array<char, 64> serial_string_{};
		int initialize_phase_ = 0;
		uni::Array<EndpointConfig, 16> ep_configs_{};
		int num_ep_configs_ = 0;
		Error InitializePhase1(const uint8* buf, int len);
		Error InitializePhase2(const uint8* buf, int len);
		Error InitializePhase3(uint8 config_value);
		Error InitializePhase4();
		Error InitializeStringPhase0(const uint8* buf, int len);
		Error InitializeStringPhaseManufacturer(const uint8* buf, int len);
		Error InitializeStringPhaseProduct(const uint8* buf, int len);
		Error InitializeStringPhaseSerial(const uint8* buf, int len);
		Error InitializeBOSHeader(const uint8* buf, int len);
		Error InitializeBOS(const uint8* buf, int len);
		Error RequestStringDescriptors();
		Error BeginBOSDescriptorRead();
		Error BeginConfigurationDescriptorRead();
		Error InitializeAddressPhase();
		void ReleaseBOSBuffer();

		/** Map structure to identify the issuer of a request within OnControlCompleted.
			 * The issuer is registered when ControlOut or ControlIn is issued.
			 */
		ArrayMap<SetupData, ClassDriver*, 16> event_waiters_{};
	};

	Error GetDescriptor(USBHostDevice& dev, EndpointID ep_id,
		uint8 desc_type, uint8 desc_index,
		void* buf, int len, bool debug = false, uint16 desc_lang_id = 0);
	Error SetConfiguration(USBHostDevice& dev, EndpointID ep_id,
		uint8 config_value, bool debug = false);
	Error SetAddress(USBHostDevice& dev, EndpointID ep_id,
		uint8 address, bool debug = false);

	struct USBHostControllerIdentity {
		const char* driver_name;
		void* driver_data;
		uint8 root_hub_protocol;
	};

	struct USBHostDeviceLocation {
		USBHostDevice* parent_hub;
		uint8 device_id;
		uint8 root_hub_port;
		uint8 upstream_port;
	};

	using HostDeviceConfiguredHook = void (*)(const USBHostControllerIdentity& controller,
		const USBHostDeviceLocation& location, USBHostDevice& dev);
	extern HostDeviceConfiguredHook g_host_device_configured_hook;
	using HostDeviceDisconnectedHook = void (*)(const USBHostControllerIdentity& controller,
		const USBHostDeviceLocation& location, USBHostDevice& dev);
	extern HostDeviceDisconnectedHook g_host_device_disconnected_hook;
	using HostDeviceNotificationHook = void (*)(const USBHostControllerIdentity& controller,
		const USBHostDeviceLocation& location, USBHostDevice& dev,
		uint8 notification_type, uint64 notification_data);
	extern HostDeviceNotificationHook g_host_device_notification_hook;
	using HostBandwidthRequestHook = void (*)(const USBHostControllerIdentity& controller,
		const USBHostDeviceLocation& location, USBHostDevice& dev);
	extern HostBandwidthRequestHook g_host_bandwidth_request_hook;
}
#endif

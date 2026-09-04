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
		virtual Error OnHubPortStatusReceived(uint8 port_num, uint16 status, uint16 change);

		Error StartInitialize();
		bool IsInitialized() { return is_initialized_; }
		EndpointConfig* EndpointConfigs() { return ep_configs_.data(); }
		int NumEndpointConfigs() { return num_ep_configs_; }
		Error OnEndpointsConfigured();
		uint16 VendorID() const { return vendor_id_; }
		uint16 ProductID() const { return product_id_; }
		uint8 DeviceClass() const { return device_class_; }
		uint8 DeviceSubClass() const { return device_sub_class_; }
		uint8 DeviceProtocol() const { return device_protocol_; }
		uint8 HubNumPorts() const { return hub_num_ports_; }
		void SetHubNumPorts(uint8 num_ports) { hub_num_ports_ = num_ports; }
		const char* ManufacturerString() const { return manufacturer_string_[0] ? manufacturer_string_.data() : nullptr; }
		const char* ProductString() const { return product_string_[0] ? product_string_.data() : nullptr; }
		const char* SerialString() const { return serial_string_[0] ? serial_string_.data() : nullptr; }

		uint8* Buffer() { return buf_.data(); }

	protected:
		Error OnControlCompleted(EndpointID ep_id, SetupData setup_data,
			const void* buf, int len);
		Error OnInterruptCompleted(EndpointID ep_id, const void* buf, int len);

	private:
	 /** @brief Class driver assigned to each endpoint.
		  *
		  * Index is the endpoint number (0 - 15).
		  * Index 0 is always unused since no class driver uses it.
		  */
		std::array<ClassDriver*, 16> class_drivers_{};

		std::array<uint8, 256> buf_{};

		// following fields are used during initialization
		uint8 num_configurations_;
		uint8 config_index_;

		Error OnDeviceDescriptorReceived(const uint8* buf, int len);
		Error OnConfigurationDescriptorReceived(const uint8* buf, int len);
		Error OnSetConfigurationCompleted(uint8 config_value);

		bool is_initialized_ = false;
		uint16 vendor_id_ = 0;
		uint16 product_id_ = 0;
		uint8 device_class_ = 0;
		uint8 device_sub_class_ = 0;
		uint8 device_protocol_ = 0;
		uint8 manufacturer_index_ = 0;
		uint8 product_index_ = 0;
		uint8 serial_index_ = 0;
		uint8 hub_num_ports_ = 0;
		uint16 string_lang_id_ = 0x0409;
		std::array<char, 64> manufacturer_string_{};
		std::array<char, 64> product_string_{};
		std::array<char, 64> serial_string_{};
		int initialize_phase_ = 0;
		std::array<EndpointConfig, 16> ep_configs_;
		int num_ep_configs_;
		Error InitializePhase1(const uint8* buf, int len);
		Error InitializePhase2(const uint8* buf, int len);
		Error InitializePhase3(uint8 config_value);
		Error InitializePhase4();
		Error InitializeStringPhase0(const uint8* buf, int len);
		Error InitializeStringPhaseManufacturer(const uint8* buf, int len);
		Error InitializeStringPhaseProduct(const uint8* buf, int len);
		Error InitializeStringPhaseSerial(const uint8* buf, int len);
		Error RequestStringDescriptors();
		Error BeginConfigurationDescriptorRead();

		/** Map structure to identify the issuer of a request within OnControlCompleted.
			 * The issuer is registered when ControlOut or ControlIn is issued.
			 */
		ArrayMap<SetupData, ClassDriver*, 4> event_waiters_{};
	};

	Error GetDescriptor(USBHostDevice& dev, EndpointID ep_id,
		uint8 desc_type, uint8 desc_index,
		void* buf, int len, bool debug = false, uint16 desc_lang_id = 0);
	Error SetConfiguration(USBHostDevice& dev, EndpointID ep_id,
		uint8 config_value, bool debug = false);
}
#endif

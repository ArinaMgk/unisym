#ifndef _USB_DEVICE_HPP
#define _USB_DEVICE_HPP
namespace uni::device::SpaceUSB {
	class ClassDriver;

	class DeviceUSB {
	public:
		virtual ~DeviceUSB();
		virtual Error ControlIn(EndpointID ep_id, SetupData setup_data,
			void* buf, int len, ClassDriver* issuer);
		virtual Error ControlOut(EndpointID ep_id, SetupData setup_data,
			const void* buf, int len, ClassDriver* issuer);
		virtual Error InterruptIn(EndpointID ep_id, void* buf, int len);
		virtual Error InterruptOut(EndpointID ep_id, void* buf, int len);

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

		uint8* Buffer() { return buf_.data(); }

	protected:
		Error OnControlCompleted(EndpointID ep_id, SetupData setup_data,
			const void* buf, int len);
		Error OnInterruptCompleted(EndpointID ep_id, const void* buf, int len);

	private:
	 /** @brief エンドポイントに割り当て済みのクラスドライバ．
	  *
	  * 添字はエンドポイント番号（0 - 15）．
	  * 添字 0 はどのクラスドライバからも使われないため，常に未使用．
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
		int initialize_phase_ = 0;
		std::array<EndpointConfig, 16> ep_configs_;
		int num_ep_configs_;
		Error InitializePhase1(const uint8* buf, int len);
		Error InitializePhase2(const uint8* buf, int len);
		Error InitializePhase3(uint8 config_value);
		Error InitializePhase4();

		/** OnControlCompleted の中で要求の発行元を特定するためのマップ構造．
		 * ControlOut または ControlIn を発行したときに発行元が登録される．
		 */
		ArrayMap<SetupData, ClassDriver*, 4> event_waiters_{};
	};

	Error GetDescriptor(DeviceUSB& dev, EndpointID ep_id,
		uint8 desc_type, uint8 desc_index,
		void* buf, int len, bool debug = false);
	Error SetConfiguration(DeviceUSB& dev, EndpointID ep_id,
		uint8 config_value, bool debug = false);
}
#endif

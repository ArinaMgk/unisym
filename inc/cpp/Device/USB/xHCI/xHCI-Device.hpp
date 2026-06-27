#include "../USB-Device.hpp"

// ---- ---- ---- ---- device.hpp ---- ---- ---- ---- //

namespace uni::device::SpaceUSB3 {
	class HostController;
	class DeviceUSB3 : public ::uni::device::SpaceUSB::DeviceUSB {
	public:
		enum class State {
			kInvalid,
			kBlank,
			kSlotAssigning,
			kSlotAssigned
		};

		using OnTransferredCallbackType = void (
			DeviceUSB3* dev,
			DeviceContextIndex dci,
			int completion_code,
			int trb_transfer_length,
			TRB* issue_trb);

		DeviceUSB3(uint8 slot_id, DoorbellRegister* dbreg, HostController* host);

		Error Initialize();

		DeviceContext* GetDeviceContext() { return &ctx_; }
		const DeviceContext* GetDeviceContext() const { return &ctx_; }
		InputContext* GetInputContext() { return &input_ctx_; }
		//usb::DeviceUSB* USBDevice() { return usb_device_; }
		//void SetUSBDevice(usb::Device* value) { usb_device_ = value; }

		State State() const { return state_; }
		uint8 SlotID() const { return slot_id_; }
		uint8 RootHubPortNum() const { return ctx_.slot_context.bits.root_hub_port_num; }
		uint32 RouteString() const { return ctx_.slot_context.bits.route_string; }
		bool IsHub() const { return ctx_.slot_context.bits.hub != 0; }
		HostController* Controller() const { return host_; }
		uint8 ParentHubSlotID() const { return parent_hub_slot_id_; }
		uint8 UpstreamPortNum() const { return upstream_port_num_; }
		void SetParentHubInfo(uint8 hub_slot_id, uint8 upstream_port_num) {
			parent_hub_slot_id_ = hub_slot_id;
			upstream_port_num_ = upstream_port_num;
		}

		void SelectForSlotAssignment();
		Ring* AllocTransferRing(DeviceContextIndex index, size_t buf_size);

		Error ControlIn(EndpointID ep_id, SetupData setup_data,
			void* buf, int len, ClassDriver* issuer) override;
		Error ControlOut(EndpointID ep_id, SetupData setup_data,
			const void* buf, int len, ClassDriver* issuer) override;
		Error InterruptIn(EndpointID ep_id, void* buf, int len) override;
		Error InterruptOut(EndpointID ep_id, void* buf, int len) override;
		Error OnHubPortStatusReceived(uint8 port_num, uint16 status, uint16 change) override;

		Error OnTransferEventReceived(const TransferEventTRB& trb);

	private:
		alignas(64) struct DeviceContext ctx_;
		alignas(64) struct InputContext input_ctx_;

		const uint8 slot_id_;
		DoorbellRegister* const dbreg_;
		HostController* const host_;

		enum State state_;
		std::array<Ring*, 31> transfer_rings_; // index = dci - 1
		uint8 parent_hub_slot_id_ = 0;
		uint8 upstream_port_num_ = 0;

		/** コントロール転送が完了した際に DataStageTRB や StatusStageTRB
		 * から対応する SetupStageTRB を検索するためのマップ．
		 */
		ArrayMap<const void*, const SetupStageTRB*, 16> setup_stage_map_{};

		//DeviceUSB* usb_device_;
	};
}

// ---- ---- ---- ---- devmgr.hpp ---- ---- ---- ---- //

namespace uni::device::SpaceUSB3 {
	class DoorbellRegister;
	
	class DeviceManager {

	public:
		Error Initialize(size_t max_slots);
		DeviceContext** DeviceContexts() const;
		size_t MaxSlots() const { return max_slots_; }
		DeviceUSB3* FindByPort(uint8 port_num, uint32_t route_string) const;
		DeviceUSB3* FindByState(enum DeviceUSB3::State state) const;
		DeviceUSB3* FindBySlot(uint8 slot_id) const;
		//WithError<Device*> Get(uint8 device_id) const;
		Error AllocDevice(uint8 slot_id, DoorbellRegister* dbreg, HostController* host);
		Error LoadDCBAA(uint8 slot_id);
		Error Remove(uint8 slot_id);

	private:
	 // device_context_pointers_ can be used as DCBAAP's value.
	 // The number of elements is max_slots_ + 1.
		DeviceContext** device_context_pointers_;
		size_t max_slots_;

		// The number of elements is max_slots_ + 1.
		DeviceUSB3** devices_;
	};
}

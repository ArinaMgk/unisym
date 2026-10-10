#include "../USB-Device.hpp"

// ---- ---- ---- ---- device.hpp ---- ---- ---- ---- //

namespace uni::device::SpaceUSB3 {
	class HostController;
	class USBHostDevice_v3 : public ::uni::device::SpaceUSB::USBHostDevice {
	public:
		enum class State {
			Invalid,
			Blank,
			SlotAssigning,
			SlotAssigned
		};

		using OnTransferredCallbackType = void (
			USBHostDevice_v3* dev,
			DeviceContextIndex dci,
			int completion_code,
			int trb_transfer_length,
			TRB* issue_trb);

		USBHostDevice_v3(uint8 slot_id, DoorbellRegister* dbreg, HostController* host,
			uint8 context_size);
		~USBHostDevice_v3();

		Error Initialize();

		SlotContext* GetSlotContext() { return ctx_->Slot(); }
		const SlotContext* GetSlotContext() const { return ctx_->Slot(); }
		SlotContext* GetInputSlotContext() { return input_ctx_->Slot(context_size_); }
		InputControlContext* GetInputControlContext() { return input_ctx_->Control(); }
		void* DeviceContextBuffer() { return ctx_->Buffer(); }
		void* InputContextBuffer() { return input_ctx_->Buffer(); }
		uint8 ContextSize() const { return context_size_; }
		SlotContext* EnableInputSlotContext() {
			return input_ctx_->EnableSlotContext(context_size_);
		}
		EndpointContext* EnableInputEndpoint(DeviceContextIndex dci) {
			return input_ctx_->EnableEndpoint(dci, context_size_);
		}
		//usb::USBHostDevice* USBDevice() { return usb_device_; }
		//void SetUSBDevice(usb::Device* value) { usb_device_ = value; }

		State State() const { return state_; }
		uint8 SlotID() const { return slot_id_; }
		uint8 RootHubPortNum() const { return GetSlotContext()->bits.root_hub_port_num; }
		uint32 RouteString() const { return GetSlotContext()->bits.route_string; }
		bool IsHub() const { return GetSlotContext()->bits.hub != 0; }
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
		Error BulkTransfer(EndpointID ep_id, bool dir_in, void* buf, int len) override;
		bool IsBulkRecoveryPending(EndpointID ep_id) const override;
		Error IsochronousTransfer(EndpointID ep_id, void* buf, int len,
			const IsochronousTransferOptions& options = IsochronousTransferOptions{}) override;
		Error OnHubPortStatusReceived(uint8 port_num, uint16 status,
			uint16 change, uint8 speed_id = 0) override;
		bool ClaimHubPortReset(uint8 port_num) override;
		void ReleaseHubPortReset(uint8 port_num) override;
		Error ConfigureHub(uint8 num_ports, uint16 characteristics) override;
		uint8 HubDepth() const override;

		Error OnTransferEventReceived(const TransferEventTRB& trb);
		Error OnEndpointResetCompleted(EndpointID ep_id, int completion_code);
		Error OnTransferRingDequeueSet(EndpointID ep_id, int completion_code);
		bool IsHubContextUpdatePending() const { return hub_context_update_pending_; }
		void CompleteHubContextUpdate() { hub_context_update_pending_ = false; }

	private:
		enum class BulkRecoveryPhase {
			None,
			ResetEndpoint,
			ClearEndpointHalt,
			ClearTTBuffer,
			SetDequeuePointer,
		};

		// One logical bulk request is represented by one chained Transfer TD.
		struct PendingBulkTransfer {
			void* buffer = nullptr;
			int length = 0;
			int short_transfer_length = 0;
			TRB* first_trb = nullptr;
			TRB* last_trb = nullptr;
			TRB* next_trb = nullptr;
			size_t trb_count = 0;
			bool next_cycle_state = true;
			bool short_event_seen = false;
			bool active = false;
			BulkRecoveryPhase recovery_phase = BulkRecoveryPhase::None;
		};

		struct PendingIsochronousTransfer {
			EndpointID ep_id{};
			void* buffer = nullptr;
			int length = 0;
			TRB* first_trb = nullptr;
			TRB* last_trb = nullptr;
			size_t trb_count = 0;
			uint16 frame_id = 0;
			bool schedule_immediately = true;

			bool operator==(const PendingIsochronousTransfer& rhs) const {
				return ep_id.Address() == rhs.ep_id.Address() &&
					buffer == rhs.buffer && length == rhs.length &&
					first_trb == rhs.first_trb && last_trb == rhs.last_trb &&
					trb_count == rhs.trb_count && frame_id == rhs.frame_id &&
					schedule_immediately == rhs.schedule_immediately;
			}
		};

		struct DeviceContext* ctx_ = nullptr;
		struct InputContext* input_ctx_ = nullptr;

		const uint8 slot_id_;
		DoorbellRegister* const dbreg_;
		HostController* const host_;
		const uint8 context_size_;

		enum State state_ = State::Invalid;
		uni::Array<Ring*, 31> transfer_rings_; // index = dci - 1
		uint8 parent_hub_slot_id_ = 0;
		uint8 upstream_port_num_ = 0;
		bool hub_context_update_pending_ = false;

		/** Map to look up the corresponding SetupStageTRB from DataStageTRB
			 * or StatusStageTRB when a control transfer completes.
			 */
		ArrayMap<const void*, const SetupStageTRB*, 16> setup_stage_map_{};
		uni::Array<PendingBulkTransfer, 31> pending_bulk_transfers_{};
		uni::Vector<PendingIsochronousTransfer> pending_isochronous_transfers_{};
		uni::Array<uint16, 31> next_isochronous_uframe_{};
		byte isochronous_schedule_valid_storage_[4]{};
		byte isochronous_stream_started_storage_[4]{};
		uni::Bitmap isochronous_schedule_valid_;
		uni::Bitmap isochronous_stream_started_;
		const EndpointConfig* EndpointConfigOf(EndpointID ep_id);
		int BulkTransferredLength(EndpointID ep_id, const TRB* issuer_trb,
			int residual_length) const;
		Error BeginBulkRecovery(EndpointID ep_id);
		Error BeginTTBufferClear(EndpointID ep_id);
		Error OnTTBufferClearCompleted(EndpointID ep_id, int completion_code);
		Error SubmitTTBufferClear(USBHostDevice_v3& child, EndpointID ep_id);
		Error QueueControlOut(EndpointID ep_id, SetupData setup_data,
			const void* buf, int len);
		Error QueueBulkDequeuePointer(EndpointID ep_id);
		Error CompleteBulkFailure(EndpointID ep_id);
		PendingIsochronousTransfer* FindIsochronousTransfer(
			EndpointID ep_id, const TRB* issuer_trb);
		int IsochronousTransferredLength(const PendingIsochronousTransfer& pending,
			const TRB* issuer_trb, int residual_length) const;
		size_t ActiveIsochronousTRBs(EndpointID ep_id) const;

		//USBHostDevice* usb_device_;
	};
}

// ---- ---- ---- ---- devmgr.hpp ---- ---- ---- ---- //

namespace uni::device::SpaceUSB3 {
	class DoorbellRegister;
	
	class DeviceManager {

	public:
		Error Initialize(size_t max_slots);
		void** DeviceContexts() const;
		size_t MaxSlots() const { return max_slots_; }
		USBHostDevice_v3* FindByPort(uint8 port_num, uint32_t route_string) const;
		USBHostDevice_v3* FindByState(enum USBHostDevice_v3::State state) const;
		USBHostDevice_v3* FindBySlot(uint8 slot_id) const;
		//WithError<Device*> Get(uint8 device_id) const;
		Error AllocDevice(uint8 slot_id, DoorbellRegister* dbreg, HostController* host);
		Error LoadDCBAA(uint8 slot_id);
		Error Remove(uint8 slot_id);
		void Reset();

	private:
	 // device_context_pointers_ can be used as DCBAAP's value.
	 // The number of elements is max_slots_ + 1.
		void** device_context_pointers_;
		size_t max_slots_;

		// The number of elements is max_slots_ + 1.
		USBHostDevice_v3** devices_;
	};
}

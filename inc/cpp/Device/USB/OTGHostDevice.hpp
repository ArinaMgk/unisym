// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [Device.USB] OTG Host Device Bridge
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#ifndef _INC_DEVICE_USB_OTGHOSTDEVICE
#define _INC_DEVICE_USB_OTGHOSTDEVICE

#include "../../unisym"
#if defined(_MCU_STM32)
#include "USB-Header.hpp"
#include "USB-Device.hpp"
#include "HCD.hpp"
#endif

#if defined(_MCU_STM32H7x)

namespace uni::device::SpaceUSB {

	// AKA USBHostDevice_v3 (xHCI) but on the H7 OTG host controller: bridges the
	class OTGHostDevice : public USBHostDevice {
	public:
		static const int kMaxBridges = 8;
		static const int kMaxHubChildren = 4;

		// the root device takes kDefaultDeviceAddress, a hub child the next free one
		OTGHostDevice(HCD& hcd, byte dev_address, byte speed,
			byte assigned_address = kDefaultDeviceAddress);
		~OTGHostDevice() override;

		// a device behind a hub is made at run time, so a bridge allocates through the host environment
		void* operator new(size_t size);
		void operator delete(void* ptr) noexcept;
		// the root bridge may be placed into a static buffer instead
		void* operator new(size_t size, void* ptr) noexcept { (void)size; return ptr; }
		stduint Speed() const override { return speed_; }
		// diagnostics: which control stage the bridge is sitting on (0 idle, 1 setup, 2/3 data, 4/5 status, 6 done)
		byte CtrlStage() const { return (byte)ctrl_stage_; }

		// transport backends (called by the USBHostDevice protocol stack)
		Error ControlIn(EndpointID ep_id, SetupData setup_data,
			void* buf, int len, ClassDriver* issuer) override;
		Error ControlOut(EndpointID ep_id, SetupData setup_data,
			const void* buf, int len, ClassDriver* issuer) override;
		Error InterruptIn(EndpointID ep_id, void* buf, int len) override;
		Error InterruptOut(EndpointID ep_id, void* buf, int len) override;
		// Bulk transport for class drivers (AKA a host MSC disk)
		Error BulkTransfer(EndpointID ep_id, bool dir_in, void* buf, int len) override;
		Error OnHubPortStatusReceived(uint8 port_num, uint16 status, uint16 change) override;

		// program every non-control channel from the parsed configuration
		Error ConfigureTransportEndpoints() override { return ConfigureEndpoints(); }
		bool RequiresSetAddressRequest() const override { return true; }
		// AKA USBH_LL_SetDeviceAddress: every later transfer uses the new address
		void OnDeviceAddressChanged(uint8 address) override;

		// URB completion dispatcher; called from HCD.NotifyURBChangeHandler
		void OnChannelURBCompleted(byte ch_num, URBState urb_state);
		// Submit every interrupt poll that was held back and whose interval is over
		void ProcessDeferredInterrupts();
		// the channel table: an URB completion goes to the device that took the channel
		static OTGHostDevice* OwnerOfChannel(byte ch_num);
		static void SetChannelOwner(byte ch_num, OTGHostDevice* dev);
		// every control transfer of this device goes out on this very channel
		bool IsControlChannel(byte ch_num) const { return ch_num == ctrl_ch_; }
		byte ControlChannel() const { return ctrl_ch_; }

		// ---- the bus device list: the root device plus everything behind a hub ----
		static OTGHostDevice* BridgeAt(int index);
		static int NumBridges();
		static OTGHostDevice* BridgeOf(const USBHostDevice* dev);
		static void TickAll();// tick every bridge about once per millisecond
		void Tick();
		static void PollBus();// drive the bus transfers, called from the main loop
		void Service();

		// ---- USB address allocation ----
		static byte AllocDeviceAddress();
		static void FreeDeviceAddress(byte address);
		static void ReserveDeviceAddress(byte address);

		// ---- the downstream side of a hub ----
		int NumChildren() const { return num_children_; }
		OTGHostDevice* ChildAt(int index) const {
			return (index >= 0 && index < num_children_) ? children_[index] : nullptr;
		}
		OTGHostDevice* ChildOfPort(byte port) const;
		byte ParentPort() const { return parent_port_; }
		OTGHostDevice* ParentBridge() const { return parent_; }
		bool IsRootDevice() const { return parent_ == nullptr; }
		byte AssignedAddress() const { return assigned_addr_; }
		bool IsHubBridge() const { return HubNumPorts() != 0; }
		byte NextHubPortToReset();
		byte ChildUpCount() const { return child_up_count_; }
		byte ChildDownCount() const { return child_down_count_; }
		byte ChildFailCount() const { return child_fail_count_; }
		byte HubAddressingPort() const override { return hub_addressing_port_; }
		// true while a child of this hub parent has a control transfer in flight or a channel armed
		bool ChildBusy() override;
		stduint HubPortEventCount() const { return hub_port_event_count_; }
		uint16 HubPortStatus(byte port) const {
			return (port >= 1 && port <= 16) ? hub_port_status_[port - 1] : uint16(0);
		}
		uint16 HubPortChange(byte port) const {
			return (port >= 1 && port <= 16) ? hub_port_change_[port - 1] : uint16(0);
		}

		// configure all non-control endpoints from the parsed configuration
		Error ConfigureEndpoints();
		// host channel of an endpoint address, programming the channels on first use
		byte ChannelOfEndpoint(EndpointID ep_id);

		HCD& Controller() { return hcd_; }
		byte DeviceAddress() const { return dev_address_; }

	private:
		// control transfer stage machine (AKA HCD control pipe state)
		enum class ControlStage : byte {
			Idle, Setup, DataIn, DataOut, StatusIn, StatusOut, Done
		};
		ControlStage ctrl_stage_ = ControlStage::Idle;
		bool ctrl_retry_pending_ = false;
		EndpointID ctrl_ep_id_{};
		SetupData ctrl_setup_{};
		void* ctrl_buf_ = nullptr;
		int ctrl_len_ = 0;

		static OTGHostDevice* ch_owner_[16];// who took a host channel
		byte ctrl_ch_ = kNoChannel;// the default control pipe channel of this device

		HCD& hcd_;
		byte dev_address_;
		const byte speed_;

		byte* ch_xfer_base_[16] = {};// what the completion callbacks hand back
		uint16 ch_xfer_len_[16] = {};

		byte ch_nak_retry_[16] = {};// NAK is not a failure: the same URB is re-submitted that many times
		static const int kNakRetryLimit = 200;
		// a transaction error on the default pipe is the bus losing one answer, not the device refusing it
		byte ctrl_err_retry_ = 0;
		static const int kCtrlErrRetryLimit = 5;
		// the core can drop a stage without ever raising an interrupt: watch the armed stage and re-send it
		bool ctrl_waiting_ = false;
		stduint ctrl_submit_tick_ = 0;
		static const stduint kCtrlWatchdogTicks = 10;
		Error ResubmitControlStage();
		byte ch_poll_interval_[16] = {};// bInterval of an interrupt endpoint, in milliseconds on LS/FS
		stduint ch_poll_last_[16] = {};// millisecond the last poll went out at
		// a transfer the core never completes (no interrupt at all) parks its endpoint: watch every channel
		stduint ch_waiting_since_[16] = {};// bus tick of the armed transfer, 0 when nothing is in flight
		static const stduint kChannelWatchdogTicks = 20;
		// a transaction still running at the frame boundary makes the core drop the port enable
		byte* ch_deferred_buf_[16] = {};// poll held back until then
		uint16 ch_deferred_len_[16] = {};

		// endpoint address <-> host channel (AKA USBH_AllocPipe)
		byte ch_of_ep_[32] = {};
		byte ep_of_ch_[16] = {};
		bool endpoints_configured_ = false;

		// EP0 starts at 8 bytes until the first device-descriptor reply says otherwise
		bool mps_known_ = false;
		bool mps_probe_ = false;
		int ctrl_len_full_ = 0;
		int ctrl_xfer_count_ = 0;// bytes the data stage really moved

		// ---- the USB address reserved for this very device ----
		byte assigned_addr_ = kDefaultDeviceAddress;

		// ---- the hub tree: this device's parent hub and its own downstream devices ----
		OTGHostDevice* parent_ = nullptr;
		byte parent_port_ = 0;
		OTGHostDevice* children_[kMaxHubChildren] = {};
		byte child_port_[kMaxHubChildren] = {};// the hub port each child sits on
		int num_children_ = 0;
		uint16 hub_port_status_[16] = {};// the last port status the hub class driver reported
		uint16 hub_port_change_[16] = {};// the change bits of that very report
		stduint hub_port_event_count_ = 0;
		bool hub_addressing_ = false;// a downstream device still answers at address 0
		byte hub_addressing_port_ = 0;
		OTGHostDevice* hub_addressing_child_ = nullptr;
		stduint hub_addressing_since_ = 0;// the bus tick the child was put at address 0
		bool hub_child_started_ = false;// the settle time has passed and enumeration really began
		byte hub_addressing_retry_ = 0;// enumeration retries of the child on this port
		byte child_up_count_ = 0, child_down_count_ = 0, child_fail_count_ = 0;

		// bring the device on one downstream port up
		Error StartChildDevice(byte port, uint16 port_status);
		// give the child's channels and its USB address back
		bool DropChildDevice(byte port);
		// while a child sits at address 0 the hub may not reset another port
		void TickAddressing();
		// the hub class driver asks which port to reset next
		static byte HubResetPortHook(USBHostDevice& dev);
		static void RegisterBridge(OTGHostDevice* dev);
		static void UnregisterBridge(OTGHostDevice* dev);
	};

}

#endif // _MCU_STM32H7x

#endif // _INC_DEVICE_USB_OTGHOSTDEVICE

// ASCII CPP TAB4 CRLF
// Docutitle: (Module) Touch
// Codifiers: @dosconio: 20241216 ~ <Last-check>
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
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

#ifndef _INC_DEVICE_Touch_X
#define _INC_DEVICE_Touch_X

#include "../unisym"
#include "../ISO_IEC_STD/memory"
#include "../../c/graphic.h"
#include "../../c/system/alice.h"// Handler_t
#include "./_Video.hpp"// LayerManager / SheetTrait / SheetEvent

namespace uni {

	constexpr uint16 TouchIndexNone = 0xFFFFu;

	enum TouchContactFields : uint16 {
		TouchContactField_Id = 1u << 0,
		TouchContactField_Size = 1u << 1,
		TouchContactField_Pressure = 1u << 2,
	};

	struct TouchContact {
		Point position;
		uint16 id = 0;
		uint16 valid = 0;
		uint16 size = 0;
		uint16 pressure = 0;
	};

	enum TouchFeatures : uint16 {
		TouchFeature_HardwareId = 1u << 0,
		TouchFeature_ContactSize = 1u << 1,
		TouchFeature_Pressure = 1u << 2,
	};

	struct TouchCapabilities {
		Rectangle bounds;
		uint16 max_contacts = 0;
		uint16 features = 0;
	};

	enum class TouchReadResult : uint8 {
		NoData,
		Frame,
		Retry,
		Invalid,
		Stopped,
		Unsupported,
		InvalidTarget,
	};

	struct TouchFrameInfo {
		uint64 timestamp_us = 0;// monotonic acquisition time, not the interrupt time
		uint32 sequence = 0;
		uint16 count = 0;
		uint16 flags = 0;// reserved, must be zero
	};

	// Borrowed storage: Retry keeps the same target alive; only Frame permits publication.
	struct TouchFrameWriter {
		TouchContact* data = nullptr;
		TouchFrameInfo* info = nullptr;
		uint16 capacity = 0;
	};

	struct TouchFrameView {
		const TouchContact* data = nullptr;
		const TouchFrameInfo* info = nullptr;
	};

	struct TouchFrameToken {
		uint32 generation = 0;
		uint16 slot = TouchIndexNone;
	};

	enum class TouchSlotState : uint8 { Free, Writing, Queued, Reading };

	struct TouchFrameSlot {
		TouchFrameInfo info;
		uint32 generation = 0;
		uint32 epoch = 0;
		uint16 next = TouchIndexNone;
		TouchSlotState state = TouchSlotState::Free;
	};

	struct TouchTrack {
		TouchContact last;
		uint32 lifetime = 0;
		uint16 matched_index = TouchIndexNone;
		bool active = false;
	};

	enum class TouchPhase : uint8 { Down, Move, Up, Cancel };
	enum class TouchCancelReason : uint8 { InvalidFrame, Overflow, Stopped, Detached };

	struct TouchEvent {
		TouchContact contact;
		uint64 timestamp_us = 0;
		uint32 sequence = 0;
		uint32 lifetime = 0;
		uint16 track = TouchIndexNone;
		TouchPhase phase = TouchPhase::Move;
		TouchCancelReason reason = TouchCancelReason::InvalidFrame;
	};

	using TouchEventObserver = void (*)(const TouchEvent& event, void* context);

	class TouchControlBlock;

	// Allocate before binding; destroy only after capture and consumption have drained.
	class TouchStorage {
		friend class TouchControlBlock;
		UniquePtr<TouchFrameSlot[]> slots_;
		UniquePtr<TouchContact[]> contacts_;
		UniquePtr<TouchTrack[]> tracks_;
		UniquePtr<uint16[]> matches_;
		uint16 contact_capacity_ = 0;
		uint16 frame_slots_ = 0;
		TouchControlBlock* owner_ = nullptr;
	public:
		TouchStorage() = default;
		TouchStorage(const TouchStorage&) = delete;
		TouchStorage& operator=(const TouchStorage&) = delete;

		bool Initialize(uint16 contact_capacity, uint16 frame_slots, bool position_matching = false);

		bool isInitialized() const { return contact_capacity_ != 0; }
		uint16 getContactCapacity() const { return contact_capacity_; }
		uint16 getFrameSlotCount() const { return frame_slots_; }
		bool isPositionMatchingEnabled() const { return bool(matches_); }
	};

	// Touch trait: working-state interface for touch controllers (capacitive GT9xx / FT5206 / OTT2001A and resistive XPT2046 etc.), similar to RuptTrait / VideoControlInterface.
	// Initialization (e.g. reset / chip-ID check / calibration) is DECOUPLED from this trait:
	// each driver performs it in its own constructor or a driver-specific Initialize() method before the device is used through this trait.
	// Coordinate convention: drivers return SCREEN coordinates; the panel-orientation transform is done inside each driver (like the HAL does per panel ID).
	class TouchTrait {
	public:
		virtual ~TouchTrait() = default;
		// Scan once; returns the number of valid touch points (0 = no touch)
		// and records each point internally.
		virtual byte Scan() = 0;
		// Number of valid points from the latest Scan().
		virtual byte getPointCount() const = 0;
		// Screen coordinate of the i-th point (0 <= i < getPointCount()).
		virtual Point getPoint(byte i) const = 0;
		// ---- resistive-touch hooks (capacitive drivers may use the defaults) ----
		// true for capacitive controllers; resistive ones return false.
		virtual bool isCapacitive() const { return true; }
		// true if calibration is required before use (resistive only).
		virtual bool NeedsAdjust() const { return false; }
		// Run the calibration procedure (resistive only); true when calibration is done.
		virtual bool Adjust() { return true; }
		// ---- conveniences ----
		Point getPrimary() const { return getPoint(0); }
		bool isTouched() const { return getPointCount() > 0; }
		virtual bool getCapabilities(TouchCapabilities& caps) const { (void)caps; return false; }
		// Unsupported leaves the target untouched; InvalidTarget must be rejected before I/O.
		virtual TouchReadResult ReadFrame(TouchFrameWriter& target) { (void)target; return TouchReadResult::Unsupported; }
	};

	// Touch manager: separates complete-frame acquisition from consumer-side tracking.
	// Wiring:
	//   - the GPIO-EXTI ISR calls HandleInterrupt() (never touch I2C inside the ISR),
	//   - standalone callers use Process(); RTOS producers and consumers use frame slots.
	// Hardware IDs identify contacts when available; otherwise nearest-position matching
	// provides a per-finger track identity, independent of the chip's point ordering (which shifts when fingers
	// are added or removed). Tracking by the raw point index instead would connect
	// different fingers' positions into straight lines (2 points -> line, 3 -> triangle)
	// and falsely lift a still-pressed finger (broken stroke).
	// Each scan dispatches to the top sheet covering the point (LayerManager::getTop),
	// with coordinates converted to sheet-relative ones:
	//   newly-pressed point -> onClick  (0x11 = left-button change and down)
	//   moved point         -> onMoved  (pressure / range radius passed as 0)
	//   lifted point        -> onLeave  (type 0 = moved out)
	// Every dispatched event additionally carries the STABLE track id (0-based) as its
	// LAST variadic argument, so a sheet can track per-finger state (e.g. last
	// position for connecting strokes) across scans. Variadic layout per event
	// (the consumer must read according to the event type):
	//   onClick: (0x11 left-button change/down byte, track id)
	//   onMoved: (pressure 0, range radius 0, track id)
	//   onLeave: (leave type 0, track id)
	using TouchLockEnter = stduint (*)(void* context);
	using TouchLockLeave = void (*)(void* context, stduint state);

	class TouchControlBlock {
		class Guard;
		TouchStorage* storage_ = nullptr;
		UniquePtr<TouchStorage> legacy_storage_;
		TouchFrameToken legacy_token_;
		TouchFrameWriter legacy_writer_;
		bool legacy_capture_ = false;
		TouchCapabilities caps_;
		TouchLockEnter enter_ = nullptr;
		TouchLockLeave leave_ = nullptr;
		void* lock_context_ = nullptr;
		TouchEventObserver observer_ = nullptr, pointer_observer_ = nullptr;
		void* observer_context_ = nullptr;
		void* pointer_context_ = nullptr;
		Atomic<bool> int_pending_{false};
		uint16 free_ = TouchIndexNone, ready_ = TouchIndexNone, tail_ = TouchIndexNone;
		uint16 writing_ = TouchIndexNone, reading_ = TouchIndexNone;
		uint16 primary_ = TouchIndexNone;
		uint32 serial_ = 0, epoch_ = 1, lifetime_ = 0;
		uint32 overflows_ = 0;
		uint32 submitted_sequence_ = 0, event_sequence_ = 0;
		uint64 event_time_ = 0;
		bool have_sequence_ = false, quarantined_ = false, cancel_pending_ = false;
		bool detaching_ = false, wait_empty_ = false, processing_ = false;
		TouchCancelReason cancel_reason_ = TouchCancelReason::InvalidFrame;
		TouchEvent event_;
		bool isToken(const TouchFrameToken& token, TouchSlotState state) const;
		bool isFrameValid(const TouchFrameView& frame) const;
		void FreeSlot(uint16 slot);
		void CancelQueued(TouchCancelReason reason);
		void Emit(uint16 track, TouchPhase phase, TouchCancelReason reason = TouchCancelReason::InvalidFrame);
		void DispatchLegacy(const TouchEvent& event);
	public:
		static constexpr stduint MAX_POINTS = 10;// legacy capacity; new devices supply their own limit
		static constexpr stduint MATCH_MAX_DIST = 160;
		TouchTrait* ptrait = nullptr;
		LayerManager* pmanager = nullptr;
		Handler_t InterruptCallback = nullptr;
		bool use_interrupt = true;

		TouchControlBlock(TouchTrait* device = nullptr, LayerManager* manager = nullptr) : ptrait(device), pmanager(manager) {}
		~TouchControlBlock();
		TouchControlBlock(const TouchControlBlock&) = delete;
		TouchControlBlock& operator=(const TouchControlBlock&) = delete;
		bool Initialize(TouchTrait& device, TouchStorage& storage);
		bool isInitialized() const { return storage_ != nullptr; }
		// Configure only while quiescent, before publishing this block to another thread.
		bool setSynchronization(TouchLockEnter enter, TouchLockLeave leave, void* context);
		// Observers are configured before registration or after the consumer has drained.
		void setObserver(TouchEventObserver observer, void* context = nullptr) { observer_ = observer; observer_context_ = context; }
		void setPointerObserver(TouchEventObserver observer, void* context = nullptr) { pointer_observer_ = observer; pointer_context_ = context; }

		bool BeginCapture(TouchFrameToken& token, TouchFrameWriter& target);
		bool CommitCapture(const TouchFrameToken& token);
		bool AbortCapture(const TouchFrameToken& token);
		bool TakeFrame(TouchFrameToken& token, TouchFrameView& frame);
		bool ReleaseFrame(const TouchFrameToken& token);
		void RequestCancel(TouchCancelReason reason);
		bool TakeCancel(TouchCancelReason& reason);
		void BeginDetach();
		bool isDrained() const;
		bool isDetaching() const;
		uint32 getOverflowCount() const;

		// Consumer only; callbacks run without the handoff lock and must not reenter processing.
		bool ProcessFrame(const TouchFrameView& frame);
		void CancelActiveTouches(TouchCancelReason reason);
		void HandleInterrupt();
		void Process();
		byte Scan() { return ptrait ? ptrait->Scan() : 0; }
		bool isTouched() const { return ptrait && ptrait->isTouched(); }
		Point getPrimary() const { return ptrait ? ptrait->getPrimary() : Point(0, 0); }
	};

}
#endif

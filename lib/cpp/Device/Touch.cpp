// ASCII CPP TAB4 LF
// Docutitle: (Module) Touch
// Codifiers: @ArinaMgk
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


#include "../../../inc/cpp/Device/_Touch.hpp"

namespace uni {
	bool TouchStorage::Initialize(uint16 contact_capacity, uint16 frame_slots, bool position_matching) {
		if (isInitialized() || !contact_capacity || frame_slots < 3) return false;
		const stduint cookie = alignof(TouchFrameSlot) > sizeof(size_t) ? alignof(TouchFrameSlot) : sizeof(size_t);
		const stduint maximum = ~stduint(0) - cookie;
		if (stduint(frame_slots) > maximum / sizeof(TouchFrameSlot) ||
			stduint(contact_capacity) > maximum / sizeof(TouchTrack) ||
			stduint(contact_capacity) > maximum / sizeof(uint16) ||
			stduint(frame_slots) > maximum / sizeof(TouchContact) / contact_capacity) return false;
		const stduint contact_count = stduint(frame_slots) * contact_capacity;
		UniquePtr<TouchFrameSlot[]> slots(new (std::nothrow_t{}) TouchFrameSlot[frame_slots]);
		if (!slots) return false;
		UniquePtr<TouchContact[]> contacts(new (std::nothrow_t{}) TouchContact[contact_count]);
		if (!contacts) return false;
		UniquePtr<TouchTrack[]> tracks(new (std::nothrow_t{}) TouchTrack[contact_capacity]);
		if (!tracks) return false;
		UniquePtr<uint16[]> matches;
		if (position_matching) {
			matches.reset(new (std::nothrow_t{}) uint16[contact_capacity]);
			if (!matches) return false;
			for (uint16 i = 0; i < contact_capacity; ++i) matches[i] = TouchIndexNone;
		}
		slots_ = uni::move(slots);
		contacts_ = uni::move(contacts);
		tracks_ = uni::move(tracks);
		matches_ = uni::move(matches);
		contact_capacity_ = contact_capacity;
		frame_slots_ = frame_slots;
		return true;
	}

	class TouchControlBlock::Guard {
		const TouchControlBlock& owner_;
		stduint state_;
	public:
		explicit Guard(const TouchControlBlock& owner) : owner_(owner), state_(owner.enter_ ? owner.enter_(owner.lock_context_) : 0) {}
		~Guard() { if (owner_.leave_) owner_.leave_(owner_.lock_context_, state_); }
		Guard(const Guard&) = delete;
		Guard& operator=(const Guard&) = delete;
	};

	TouchControlBlock::~TouchControlBlock() {
		if (storage_ && storage_->owner_ == this) storage_->owner_ = nullptr;
	}

	bool TouchControlBlock::Initialize(TouchTrait& device, TouchStorage& storage) {
		if (storage_ || !storage.isInitialized() || storage.owner_) return false;
		TouchCapabilities caps;
		if (device.getCapabilities(caps)) {
			if (!caps.max_contacts || caps.max_contacts > storage.contact_capacity_ ||
				!caps.bounds.width || !caps.bounds.height ||
				caps.bounds.width > ~stduint(0) - caps.bounds.x ||
				caps.bounds.height > ~stduint(0) - caps.bounds.y) return false;
		}
		else caps.max_contacts = storage.contact_capacity_;
		if (!(caps.features & TouchFeature_HardwareId) && !storage.matches_) return false;
		for (uint16 i = 0; i < storage.frame_slots_; ++i) {
			auto& slot = storage.slots_[i];
			slot.next = i + 1 < storage.frame_slots_ ? uint16(i + 1) : TouchIndexNone;
			slot.state = TouchSlotState::Free;
			slot.generation = slot.epoch = 0;
		}
		for (uint16 i = 0; i < storage.contact_capacity_; ++i) {
			storage.tracks_[i].active = false;
			storage.tracks_[i].lifetime = 0;
			storage.tracks_[i].matched_index = TouchIndexNone;
		}
		ptrait = &device;
		caps_ = caps;
		storage_ = &storage;
		storage.owner_ = this;
		free_ = 0;
		return true;
	}

	bool TouchControlBlock::setSynchronization(TouchLockEnter enter, TouchLockLeave leave, void* context) {
		if (processing_ || detaching_ || bool(enter) != bool(leave) || writing_ != TouchIndexNone || reading_ != TouchIndexNone ||
			ready_ != TouchIndexNone || cancel_pending_) return false;
		enter_ = enter;
		leave_ = leave;
		lock_context_ = context;
		return true;
	}

	bool TouchControlBlock::isToken(const TouchFrameToken& token, TouchSlotState state) const {
		return storage_ && token.slot < storage_->frame_slots_ && token.generation &&
			storage_->slots_[token.slot].generation == token.generation && storage_->slots_[token.slot].state == state;
	}

	bool TouchControlBlock::isFrameValid(const TouchFrameView& frame) const {
		if (!storage_ || !frame.info || frame.info->flags || frame.info->count > caps_.max_contacts ||
			(frame.info->count && !frame.data)) return false;
		for (uint16 i = 0; i < frame.info->count; ++i) {
			const auto& point = frame.data[i];
			if (point.valid & ~(TouchContactField_Id | TouchContactField_Size | TouchContactField_Pressure)) return false;
			if ((caps_.features & TouchFeature_HardwareId) && !(point.valid & TouchContactField_Id)) return false;
			if ((point.valid & TouchContactField_Size) && !(caps_.features & TouchFeature_ContactSize)) return false;
			if ((point.valid & TouchContactField_Pressure) && !(caps_.features & TouchFeature_Pressure)) return false;
			if (caps_.bounds.width && !caps_.bounds.ifContain(point.position)) return false;
			if (point.valid & TouchContactField_Id) for (uint16 j = 0; j < i; ++j)
				if ((frame.data[j].valid & TouchContactField_Id) && point.id == frame.data[j].id) return false;
		}
		return true;
	}

	void TouchControlBlock::FreeSlot(uint16 index) {
		auto& slot = storage_->slots_[index];
		slot.state = TouchSlotState::Free;
		slot.next = free_;
		free_ = index;
	}

	void TouchControlBlock::CancelQueued(TouchCancelReason reason) {
		if (!cancel_pending_ || reason == TouchCancelReason::Detached) cancel_reason_ = reason;
		cancel_pending_ = true;
		quarantined_ = true;
		if (!++epoch_) ++epoch_;
		while (ready_ != TouchIndexNone) {
			const uint16 index = ready_;
			ready_ = storage_->slots_[index].next;
			FreeSlot(index);
		}
		tail_ = TouchIndexNone;
	}

	bool TouchControlBlock::BeginCapture(TouchFrameToken& token, TouchFrameWriter& target) {
		Guard guard(*this);
		if (!storage_ || detaching_ || writing_ != TouchIndexNone) return false;
		if (free_ == TouchIndexNone) { ++overflows_; CancelQueued(TouchCancelReason::Overflow); }
		if (free_ == TouchIndexNone) return false;
		const uint16 index = free_;
		auto& slot = storage_->slots_[index];
		free_ = slot.next;
		slot.next = TouchIndexNone;
		slot.state = TouchSlotState::Writing;
		if (!++serial_) ++serial_;
		slot.generation = serial_;
		slot.epoch = epoch_;
		slot.info.count = slot.info.flags = 0;
		slot.info.sequence = 0;
		slot.info.timestamp_us = 0;
		writing_ = index;
		token.slot = index;
		token.generation = slot.generation;
		target.data = storage_->contacts_.get() + stduint(index) * storage_->contact_capacity_;
		target.info = &slot.info;
		target.capacity = storage_->contact_capacity_;
		return true;
	}

	bool TouchControlBlock::CommitCapture(const TouchFrameToken& token) {
		Guard guard(*this);
		if (!isToken(token, TouchSlotState::Writing) || writing_ != token.slot) return false;
		auto& slot = storage_->slots_[token.slot];
		writing_ = TouchIndexNone;
		if (detaching_ || slot.epoch != epoch_) { FreeSlot(token.slot); return false; }
		TouchFrameView frame;
		frame.info = &slot.info;
		frame.data = storage_->contacts_.get() + stduint(token.slot) * storage_->contact_capacity_;
		const uint32 delta = slot.info.sequence - submitted_sequence_;
		if (!isFrameValid(frame) || (have_sequence_ && (!delta || delta > 0x7FFFFFFFu))) {
			FreeSlot(token.slot);
			CancelQueued(TouchCancelReason::InvalidFrame);
			return false;
		}
		submitted_sequence_ = slot.info.sequence;
		have_sequence_ = true;
		if (quarantined_ && slot.info.count) { FreeSlot(token.slot); return true; }
		if (!slot.info.count) quarantined_ = false;
		slot.state = TouchSlotState::Queued;
		if (tail_ == TouchIndexNone) ready_ = token.slot;
		else storage_->slots_[tail_].next = token.slot;
		tail_ = token.slot;
		return true;
	}

	bool TouchControlBlock::AbortCapture(const TouchFrameToken& token) {
		Guard guard(*this);
		if (!isToken(token, TouchSlotState::Writing) || writing_ != token.slot) return false;
		writing_ = TouchIndexNone;
		FreeSlot(token.slot);
		return true;
	}

	bool TouchControlBlock::TakeFrame(TouchFrameToken& token, TouchFrameView& frame) {
		Guard guard(*this);
		if (!storage_ || detaching_ || cancel_pending_ || ready_ == TouchIndexNone || reading_ != TouchIndexNone) return false;
		const uint16 index = ready_;
		auto& slot = storage_->slots_[index];
		ready_ = slot.next;
		if (ready_ == TouchIndexNone) tail_ = TouchIndexNone;
		slot.next = TouchIndexNone;
		slot.state = TouchSlotState::Reading;
		reading_ = index;
		token.slot = index;
		token.generation = slot.generation;
		frame.info = &slot.info;
		frame.data = storage_->contacts_.get() + stduint(index) * storage_->contact_capacity_;
		return true;
	}

	bool TouchControlBlock::ReleaseFrame(const TouchFrameToken& token) {
		Guard guard(*this);
		if (processing_ || !isToken(token, TouchSlotState::Reading) || reading_ != token.slot) return false;
		reading_ = TouchIndexNone;
		FreeSlot(token.slot);
		return true;
	}

	void TouchControlBlock::RequestCancel(TouchCancelReason reason) {
		Guard guard(*this);
		if (storage_) CancelQueued(reason);
	}

	bool TouchControlBlock::TakeCancel(TouchCancelReason& reason) {
		Guard guard(*this);
		if (processing_ || !cancel_pending_) return false;
		reason = cancel_reason_;
		cancel_pending_ = false;
		return true;
	}

	void TouchControlBlock::BeginDetach() {
		Guard guard(*this);
		if (!storage_ || detaching_) return;
		detaching_ = true;
		CancelQueued(TouchCancelReason::Detached);
	}

	bool TouchControlBlock::isDrained() const {
		Guard guard(*this);
		return writing_ == TouchIndexNone && reading_ == TouchIndexNone && ready_ == TouchIndexNone && !cancel_pending_ && !processing_;
	}

	bool TouchControlBlock::isDetaching() const {
		Guard guard(*this);
		return detaching_;
	}

	uint32 TouchControlBlock::getOverflowCount() const {
		Guard guard(*this);
		return overflows_;
	}

	void TouchControlBlock::Emit(uint16 index, TouchPhase phase, TouchCancelReason reason) {
		auto& track = storage_->tracks_[index];
		event_.contact = track.last;
		event_.timestamp_us = event_time_;
		event_.sequence = event_sequence_;
		event_.lifetime = track.lifetime;
		event_.track = index;
		event_.phase = phase;
		event_.reason = reason;
		if (observer_) observer_(event_, observer_context_);
		else if (pmanager) DispatchLegacy(event_);
		if (primary_ == index && pointer_observer_) pointer_observer_(event_, pointer_context_);
	}

	void TouchControlBlock::DispatchLegacy(const TouchEvent& event) {
		SheetTrait* sheet = pmanager->getTop(event.contact.position);
		if (!sheet) return;
		const Point relative = event.contact.position - sheet->sheet_area.getVertex();
		if (event.phase == TouchPhase::Down) sheet->onrupt(SheetEvent::onClick, relative, 0x11, int(event.track));
		else if (event.phase == TouchPhase::Move) sheet->onrupt(SheetEvent::onMoved, relative,
			int(event.contact.valid & TouchContactField_Pressure ? event.contact.pressure : 0),
			int(event.contact.valid & TouchContactField_Size ? event.contact.size : 0), int(event.track));
		else sheet->onrupt(SheetEvent::onLeave, relative, 0, int(event.track));
	}

	bool TouchControlBlock::ProcessFrame(const TouchFrameView& frame) {
		{
			Guard guard(*this);
			if (processing_ || !storage_ || reading_ == TouchIndexNone || cancel_pending_ ||
				frame.info != &storage_->slots_[reading_].info || storage_->slots_[reading_].epoch != epoch_ ||
				frame.data != storage_->contacts_.get() + stduint(reading_) * storage_->contact_capacity_) return false;
			if (!isFrameValid(frame)) { CancelQueued(TouchCancelReason::InvalidFrame); return false; }
			processing_ = true;
		}
		event_time_ = frame.info->timestamp_us;
		event_sequence_ = frame.info->sequence;
		const uint16 count = frame.info->count;
		const uint16 capacity = storage_->contact_capacity_;
		for (uint16 t = 0; t < capacity; ++t) storage_->tracks_[t].matched_index = TouchIndexNone;
		if (caps_.features & TouchFeature_HardwareId) {
			for (uint16 t = 0; t < capacity; ++t) if (storage_->tracks_[t].active)
				for (uint16 i = 0; i < count; ++i) if (storage_->tracks_[t].last.id == frame.data[i].id)
					storage_->tracks_[t].matched_index = i;
		}
		else {
			for (uint16 i = 0; i < count; ++i) storage_->matches_[i] = TouchIndexNone;
			for (;;) {
				uint16 best_track = TouchIndexNone, best_point = TouchIndexNone;
				stduint best_distance = MATCH_MAX_DIST * MATCH_MAX_DIST;
				for (uint16 t = 0; t < capacity; ++t) {
					const auto& track = storage_->tracks_[t];
					if (!track.active || track.matched_index != TouchIndexNone) continue;
					for (uint16 i = 0; i < count; ++i) {
						if (storage_->matches_[i] != TouchIndexNone) continue;
						const auto& a = track.last.position;
						const auto& b = frame.data[i].position;
						const stduint dx = a.x > b.x ? a.x - b.x : b.x - a.x;
						const stduint dy = a.y > b.y ? a.y - b.y : b.y - a.y;
						if (dx > MATCH_MAX_DIST || dy > MATCH_MAX_DIST) continue;
						const stduint distance = dx * dx + dy * dy;
						if (distance <= best_distance) { best_distance = distance; best_track = t; best_point = i; }
					}
				}
				if (best_track == TouchIndexNone) break;
				storage_->tracks_[best_track].matched_index = best_point;
				storage_->matches_[best_point] = best_track;
			}
		}
		// End old contacts before allocating tracks for replacements at full capacity.
		for (uint16 t = 0; t < capacity; ++t) {
			auto& track = storage_->tracks_[t];
			if (!track.active || track.matched_index != TouchIndexNone) continue;
			track.active = false;
			Emit(t, TouchPhase::Up);
			if (primary_ == t) { primary_ = TouchIndexNone; wait_empty_ = true; }
		}
		for (uint16 t = 0; t < capacity; ++t) {
			auto& track = storage_->tracks_[t];
			if (!track.active) continue;
			const auto& point = frame.data[track.matched_index];
			const bool changed = point.position.x != track.last.position.x || point.position.y != track.last.position.y ||
				point.valid != track.last.valid || point.size != track.last.size || point.pressure != track.last.pressure;
			track.last = point;
			if (changed) Emit(t, TouchPhase::Move);
		}
		for (uint16 i = 0; i < count; ++i) {
			bool matched = false;
			for (uint16 t = 0; t < capacity; ++t) if (storage_->tracks_[t].active && storage_->tracks_[t].matched_index == i) { matched = true; break; }
			if (matched) continue;
			for (uint16 t = 0; t < capacity; ++t) {
				auto& track = storage_->tracks_[t];
				if (track.active) continue;
				track.active = true;
				track.last = frame.data[i];
				track.matched_index = i;
				if (!++lifetime_) ++lifetime_;
				track.lifetime = lifetime_;
				if (primary_ == TouchIndexNone && !wait_empty_) primary_ = t;
				Emit(t, TouchPhase::Down);
				break;
			}
		}
		if (!count) wait_empty_ = false;
		{ Guard guard(*this); processing_ = false; }
		return true;
	}

	void TouchControlBlock::CancelActiveTouches(TouchCancelReason reason) {
		{
			Guard guard(*this);
			if (!storage_ || processing_) return;
			processing_ = true;
		}
		for (uint16 t = 0; t < storage_->contact_capacity_; ++t) {
			auto& track = storage_->tracks_[t];
			if (!track.active) continue;
			track.active = false;
			Emit(t, TouchPhase::Cancel, reason);
		}
		primary_ = TouchIndexNone;
		wait_empty_ = true;
		{ Guard guard(*this); processing_ = false; }
	}

	void TouchControlBlock::HandleInterrupt() {
		int_pending_.store(true, MemoryOrder_Release);
		if (InterruptCallback) InterruptCallback();
	}

	void TouchControlBlock::Process() {
		if (!ptrait || isDetaching()) return;
		if (use_interrupt && !legacy_capture_ && !int_pending_.exchange(false, MemoryOrder_Acq_Rel)) return;
		if (!storage_) {
			TouchCapabilities caps;
			const bool known = ptrait->getCapabilities(caps);
			const uint16 capacity = known ? caps.max_contacts : uint16(MAX_POINTS);
			legacy_storage_.reset(new (std::nothrow_t{}) TouchStorage);
			if (!legacy_storage_ || !legacy_storage_->Initialize(capacity, 3, !known || !(caps.features & TouchFeature_HardwareId)) ||
				!Initialize(*ptrait, *legacy_storage_)) return;
		}
		if (!legacy_capture_) {
			if (!BeginCapture(legacy_token_, legacy_writer_)) return;
			legacy_capture_ = true;
		}
		auto result = ptrait->ReadFrame(legacy_writer_);
		if (result == TouchReadResult::Unsupported) {
			legacy_writer_.info->count = ptrait->Scan();
			legacy_writer_.info->sequence = submitted_sequence_ + 1;
			if (legacy_writer_.info->count > legacy_writer_.capacity) result = TouchReadResult::Invalid;
			else {
				for (uint16 i = 0; i < legacy_writer_.info->count; ++i) {
					legacy_writer_.data[i].position = ptrait->getPoint(byte(i));
					legacy_writer_.data[i].valid = 0;
					legacy_writer_.data[i].id = legacy_writer_.data[i].size = legacy_writer_.data[i].pressure = 0;
				}
				result = TouchReadResult::Frame;
			}
		}
		if (result == TouchReadResult::Retry) return;
		if (result == TouchReadResult::Frame) CommitCapture(legacy_token_);
		else {
			AbortCapture(legacy_token_);
			if (result == TouchReadResult::Invalid || result == TouchReadResult::Stopped || result == TouchReadResult::InvalidTarget)
				RequestCancel(result == TouchReadResult::Stopped ? TouchCancelReason::Stopped : TouchCancelReason::InvalidFrame);
		}
		legacy_capture_ = false;
		TouchCancelReason reason;
		if (TakeCancel(reason)) CancelActiveTouches(reason);
		TouchFrameToken token;
		TouchFrameView frame;
		if (TakeFrame(token, frame)) { ProcessFrame(frame); ReleaseFrame(token); }
	}
}

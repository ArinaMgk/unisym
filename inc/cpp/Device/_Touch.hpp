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
#include "../../c/graphic.h"
#include "../../c/system/alice.h"// Handler_t
#include "./_Video.hpp"// LayerManager / SheetTrait / SheetEvent

namespace uni {

	// Touch trait: working-state interface for touch controllers (capacitive GT9xx /
	// FT5206 / OTT2001A and resistive XPT2046 etc.), similar to RuptTrait / VideoControlInterface.
	// Initialization (e.g. reset / chip-ID check / calibration) is DECOUPLED from this trait:
	// each driver performs it in its own constructor or a driver-specific Initialize()
	// method before the device is used through this trait.
	// Coordinate convention: drivers return SCREEN coordinates; the panel-orientation
	// transform is done inside each driver (like the HAL does per panel ID).
	class TouchTrait {
	public:
		virtual ~TouchTrait() = default;
		// Scan once; returns the number of valid touch points (0 = no touch)
		// and records each point internally.
		virtual byte Scan() = 0;
		// Number of valid points from the latest Scan().
		virtual byte GetPointCount() const = 0;
		// Screen coordinate of the i-th point (0 <= i < GetPointCount()).
		virtual Point GetPoint(byte i) const = 0;
		// ---- resistive-touch hooks (capacitive drivers may use the defaults) ----
		// true for capacitive controllers; resistive ones return false.
		virtual bool IsCapacitive() const { return true; }
		// true if calibration is required before use (resistive only).
		virtual bool NeedsAdjust() const { return false; }
		// Run the calibration procedure (resistive only); true when calibration is done.
		virtual bool Adjust() { return true; }
		// ---- conveniences ----
		Point GetPrimary() const { return GetPoint(0); }
		bool isTouched() const { return GetPointCount() > 0; }
	};

	// Touch manager: hosts the INT-driven scan and dispatches Sheet events per point.
	// Wiring:
	//   - the GPIO-EXTI ISR calls HandleInterrupt() (never touch I2C inside the ISR),
	//   - the main loop calls Process() periodically.
	// After each scan the points are matched to the previous frame's tracks by
	// NEAREST POSITION (greedy), so the dispatched track id is a STABLE per-finger
	// identity, independent of the chip's point ordering (which shifts when fingers
	// are added or removed). Tracking by the raw point index instead would connect
	// different fingers' positions into straight lines (2 points -> line, 3 -> triangle)
	// and falsely lift a still-pressed finger (broken stroke).
	// Each scan dispatches to the top sheet covering the point (LayerManager::getTop),
	// with coordinates converted to sheet-relative ones:
	//   newly-pressed point -> onClick  (0x10 = left-button down, same as the mouse)
	//   moved point         -> onMoved  (pressure / range radius passed as 0)
	//   lifted point        -> onLeave  (type 0 = moved out)
	// Every dispatched event additionally carries the STABLE track id (0-based) as its
	// LAST variadic argument, so a sheet can track per-finger state (e.g. last
	// position for connecting strokes) across scans. Variadic layout per event
	// (the consumer must read according to the event type):
	//   onClick: (0x10 left-button-down byte, track id)
	//   onMoved: (pressure 0, range radius 0, track id)
	//   onLeave: (leave type 0, track id)
	class TouchControlBlock {
	public:
		static constexpr stduint MAX_POINTS = 10;// largest known: GT9271 (10-point)
		// Max distance (pixels) for matching a point to a previous-frame track.
		// Too small: fast strokes get split into new/lost fingers (broken strokes);
		// too large: a newly-pressed finger near an old one is mistaken for a move.
		static constexpr stduint MATCH_MAX_DIST = 160;
		TouchTrait* ptrait = nullptr;
		LayerManager* pmanager = nullptr;// sheet-event dispatch target (optional)
		Handler_t InterruptCallback = nullptr;// optional, invoked by HandleInterrupt()
		// true: scan only after HandleInterrupt() recorded a pending event;
		// false: polling mode, scan on every Process() call.
		bool use_interrupt = true;
	private:
		volatile bool int_pending = false;
		Point track_pos[MAX_POINTS] = {};// last dispatched position of each track
		bool track_active[MAX_POINTS] = {};// whether the track currently holds a finger
	public:
		TouchControlBlock(TouchTrait* p = nullptr, LayerManager* pm = nullptr) : ptrait(p), pmanager(pm) {}
		// ISR entry: record the pending flag and invoke the optional callback.
		// The actual I2C scan is deferred to Process() in the main loop.
		void HandleInterrupt() {
			int_pending = true;
			if (InterruptCallback) InterruptCallback();
		}
		inline byte Scan() { return ptrait ? ptrait->Scan() : 0; }
		inline bool isTouched() const { return ptrait && ptrait->isTouched(); }
		inline Point GetPrimary() const { return ptrait ? ptrait->GetPrimary() : Point(0, 0); }
		// Main-loop pump: scan when needed, then match points to tracks and
		// dispatch Sheet events (see class comment).
		void Process() {
			if (!ptrait) return;
			if (use_interrupt && !int_pending) return;
			int_pending = false;
			ptrait->Scan();
			DispatchEvents();
		}
	private:
		byte AllocTrack() {
			for (byte t = 0; t < MAX_POINTS; t++)
				if (!track_active[t]) return t;
			return MAX_POINTS;// no free track (should not happen with current chips)
		}
		void DispatchEvents() {
			byte n = ptrait->GetPointCount();
			if (n > MAX_POINTS) n = MAX_POINTS;
			Point cur[MAX_POINTS];
			bool matched[MAX_POINTS] = {};// current point already paired to a track
			bool trk_used[MAX_POINTS] = {};// track already paired this frame
			bool was_active[MAX_POINTS] = {};// track active at the frame start
			for (byte t = 0; t < MAX_POINTS; t++) was_active[t] = track_active[t];
			for (byte i = 0; i < n; i++) cur[i] = ptrait->GetPoint(i);
			// 1) greedy nearest-neighbor match within MATCH_MAX_DIST; a matched
			//    point continues its track (onMoved when the position changed).
			const stduint max_d2 = (stduint)MATCH_MAX_DIST * MATCH_MAX_DIST;
			for (byte i = 0; i < n; i++) {
				byte best = MAX_POINTS;
				stduint best_d2 = max_d2;
				for (byte t = 0; t < MAX_POINTS; t++) {
					if (!track_active[t] || trk_used[t]) continue;
					stduint dx = cur[i].x > track_pos[t].x ? cur[i].x - track_pos[t].x : track_pos[t].x - cur[i].x;
					stduint dy = cur[i].y > track_pos[t].y ? cur[i].y - track_pos[t].y : track_pos[t].y - cur[i].y;
					stduint d2 = dx * dx + dy * dy;
					if (d2 <= best_d2) { best_d2 = d2; best = t; }
				}
				if (best < MAX_POINTS) {
					matched[i] = true;
					trk_used[best] = true;
					if (cur[i].x != track_pos[best].x || cur[i].y != track_pos[best].y)
						Dispatch(SheetEvent::onMoved, cur[i], best);
					track_pos[best] = cur[i];
				}
			}
			// 2) unmatched current points -> press on a free track.
			for (byte i = 0; i < n; i++) {
				if (matched[i]) continue;
				byte t = AllocTrack();
				if (t >= MAX_POINTS) continue;
				track_pos[t] = cur[i];
				track_active[t] = true;
				Dispatch(SheetEvent::onClick, cur[i], t);
			}
			// 3) tracks that were active at the frame start but unmatched -> lift.
			for (byte t = 0; t < MAX_POINTS; t++) {
				if (was_active[t] && !trk_used[t]) {
					track_active[t] = false;
					Dispatch(SheetEvent::onLeave, track_pos[t], t);
				}
			}
		}
		void Dispatch(SheetEvent evt, const Point& p, byte idx) {
			if (!pmanager) return;
			SheetTrait* sheet = pmanager->getTop(p);
			if (!sheet) return;
			Point rel = p - sheet->sheet_area.getVertex();
			if (evt == SheetEvent::onClick)
				sheet->onrupt(evt, rel, 0x10, idx);// L button down (upper 0RML nibble), track id
			else if (evt == SheetEvent::onMoved)
				sheet->onrupt(evt, rel, 0, 0, idx);// pressure, range radius, track id
			else
				sheet->onrupt(evt, rel, 0, idx);// leave type 0 = moved out, track id
		}
	};

}
#endif

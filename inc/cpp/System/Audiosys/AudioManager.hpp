// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: [Device.Audio] Audio Manager
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#ifndef _INCPP_Device_Audio_AudioManager
#define _INCPP_Device_Audio_AudioManager

#include "../../../c/stdinc.h"
#include "../Audiosys.hpp"
#include "../../vector"

namespace uni {

	// AudioManager: a table of audio cards plus the selected one.
	// Every card is an AudioControlInterface; the manager owns no DMA and no registers.
	class AudioManager {
	public:
		struct CardEntry {
			const char* name;
			AudioControlInterface* device;

			bool operator==(const CardEntry& other) const {
				return name == other.name && device == other.device;
			}
		};

	private:
		Vector<CardEntry> cards;
		stduint selected;// (stduint)-1 when none

	public:
		AudioManager();
		~AudioManager();

		bool                   AppendCard(const char* name, AudioControlInterface* device);
		bool                   RemoveCard(const char* name);
		bool                   SelectCard(stduint index);
		bool                   SelectCard(const char* name);
		stduint                getCardCount() const;
		const char*            getCardName(stduint index) const;
		AudioControlInterface* getCard(stduint index) const;
		stduint                getSelected() const;
		AudioControlInterface* getSelectedCard() const;

	private:
		static bool SameName(const char* a, const char* b);
	};

}

#endif

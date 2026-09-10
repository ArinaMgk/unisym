// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: [Device.Audio] Audio Manager
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/cpp/System/Audiosys/AudioManager.hpp"

namespace uni {

	AudioManager::AudioManager() : selected((stduint)-1) {
	}

	AudioManager::~AudioManager() {
	}

	bool AudioManager::SameName(const char* a, const char* b) {
		if (!a || !b) return false;
		while (*a && *a == *b) { a++; b++; }
		return *a == *b;
	}

	bool AudioManager::AppendCard(const char* name, AudioControlInterface* device) {
		if (!name || !device) return false;
		CardEntry entry{ name, device };
		if (!cards.Append(entry)) return false;
		if (selected == (stduint)-1) selected = cards.Length() - 1;
		return true;
	}

	bool AudioManager::RemoveCard(const char* name) {
		if (!name) return false;
		for (stduint i = 0; i < cards.Length(); i++) {
			if (!SameName(cards[i].name, name)) continue;
			if (!cards.Remove(i)) return false;
			if (selected == i) selected = cards.Length() ? 0 : (stduint)-1;
			else if (selected != (stduint)-1 && selected > i) selected--;
			return true;
		}
		return false;
	}

	bool AudioManager::SelectCard(stduint index) {
		if (index >= cards.Length()) return false;
		selected = index;
		return true;
	}

	bool AudioManager::SelectCard(const char* name) {
		if (!name) return false;
		for (stduint i = 0; i < cards.Length(); i++) {
			if (SameName(cards[i].name, name)) return SelectCard(i);
		}
		return false;
	}

	stduint AudioManager::getCardCount() const {
		return cards.Length();
	}

	const char* AudioManager::getCardName(stduint index) const {
		if (index >= cards.Length()) return nullptr;
		const CardEntry* entry = (const CardEntry*)cards.Locate(index);
		return entry ? entry->name : nullptr;
	}

	AudioControlInterface* AudioManager::getCard(stduint index) const {
		if (index >= cards.Length()) return nullptr;
		const CardEntry* entry = (const CardEntry*)cards.Locate(index);
		return entry ? entry->device : nullptr;
	}

	stduint AudioManager::getSelected() const {
		return selected;
	}

	AudioControlInterface* AudioManager::getSelectedCard() const {
		return getCard(selected);
	}

}

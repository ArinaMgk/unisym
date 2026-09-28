// ASCII CPP-ISO11 TAB4 LF
// Docutitle: [Trait] ConfigText
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
#include "../unisym"
#include "../string"

#ifndef _INCPP_TRAIT_CONFIGTEXT
#define _INCPP_TRAIT_CONFIGTEXT

namespace uni::trait {

	class ConfigText {
	public:
		virtual ~ConfigText() = default;

		// Probe whether the configuration text is valid for this format
		virtual bool Probe() = 0;

		// Set value by hierarchical name path (e.g. "a/b/c")
		virtual bool set(rostr name, rostr val) = 0;

		// Get string value by hierarchical name path (e.g. "a/b/c")
		virtual String get(rostr name) const = 0;

		// Get signed integer number value by hierarchical name path (e.g. "a/b/c")
		virtual stdsint getNumber(rostr name) const = 0;
	};

}

namespace uni { using ConfigTextTrait = uni::trait::ConfigText; }

#endif

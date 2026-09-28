// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [Format.ConfigText] TOML
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

#ifndef _INCPP_FORMAT_CONFIGTEXT_TOML
#define _INCPP_FORMAT_CONFIGTEXT_TOML

#include "../../trait/ConfigText.hpp"
#include "../../../c/file.h"

namespace uni {

	class ConfigTOML : public uni::trait::ConfigText {
	private:
		void* root_node;
		bool is_valid;
		void parse_toml(rostr text);

	public:
		ConfigTOML(HostFile& file);
		virtual ~ConfigTOML();

		virtual bool Probe() override;
		virtual bool set(rostr name, rostr val) override;
		virtual String get(rostr name) const override;
		virtual stdsint getNumber(rostr name) const override;
	};

}

#endif

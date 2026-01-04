// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Stroage)
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

#include "../../../inc/cpp/unisym"
#include "../../../inc/cpp/trait/StorageTrait.hpp"

#define DRV_OF_DEV
namespace uni {
	bool DiscPartition::Read(stduint BlockIden, void* Dest) {
		if (!slice.address && !slice.length) renew_slice();
		// ploginfo("read %u:%u -> %[x]", DRV_OF_DEV(self.device), BlockIden + slice.address, Dest);
		return base->Read(BlockIden + slice.address, Dest);
	}

	bool DiscPartition::Write(stduint BlockIden, const void* Sors) {
		if (!slice.address && !slice.length) renew_slice();
		return base->Write(BlockIden + slice.address, Sors);
	}
}


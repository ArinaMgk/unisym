
// ASCII CPP TAB4 CRLF
// Docutitle: (Module) Screen
// Codifiers: @dosconio: 20240516
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

// Screen = Video + Touch (Do not manage {Backlight} )

#ifndef _INC_DEVICE_Screen
#define _INC_DEVICE_Screen

#include "../../c/nnode.h"
#include "./_Video.hpp"
#include "./_Touch.hpp"
#include "../Witch/Sheet.hpp"

namespace uni {

	#define optional
	class Screen {
		NnodeChain sheet;
		optional VideoControlBlock* vcb;
		optional TouchControlBlock* tcb;
	public:
		
	};
	#undef optional

}


#endif

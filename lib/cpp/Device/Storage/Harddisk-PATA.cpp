// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Storage) Harddisk - PATA Helpers
// Codifiers: @dosconio: 20260717
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/c/storage/harddisk.h"

#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
namespace uni {

	bool PataWaitRetry(Harddisk_PATA* hdd,
		bool (*fn_lup_wait)(Harddisk_PATA*, stduint, stduint, stduint),
		stduint mask, stduint val) {
		if (!fn_lup_wait) return false;
		for (int retry = 0; retry < 5; ++retry) {
			if (fn_lup_wait(hdd, mask, val, HD_TIMEOUT / 1000)) return true;
		}
		return false;
	}

}
#endif

// Docutitle: (Storage) NVMe
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#ifndef _INC_Storage_NVMe
#define _INC_Storage_NVMe

#include "../stdinc.h"

// NVMe PCI programming interface
#define NVME_PCI_CLASS_BASE 0x01
#define NVME_PCI_CLASS_SUB  0x08
#define NVME_PCI_CLASS_IF   0x02

// Controller capability helpers
#define NVME_CAP_MQES_MASK  0xFFFFull
#define NVME_CAP_TO_SHIFT   24
#define NVME_CAP_DSTRD_SHIFT 32

// Controller configuration
#define NVME_CC_EN          0x00000001u

// Controller status
#define NVME_CSTS_RDY       0x00000001u
#define NVME_CSTS_CFS       0x00000002u

#ifdef _DEV_MSVC
#pragma pack(push, 1)
#endif
namespace uni {

	_PACKED(struct) NVME_BAR {
		uint64 cap;
		uint32 vs;
		uint32 intms;
		uint32 intmc;
		uint32 cc;
		uint32 reserved1;
		uint32 csts;
		uint32 nssr;
		uint32 aqa;
		uint64 asq;
		uint64 acq;
		uint8 reserved2[0x1000 - 0x38];
		uint32 doorbells[1];
	};

	static_assert(sizeof(NVME_BAR) == 0x1004, "NVME_BAR size mismatch");

}
#ifdef _DEV_MSVC
#pragma pack(pop)
#endif

#endif

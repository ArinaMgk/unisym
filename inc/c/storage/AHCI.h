// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Storage) AHCI
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2026

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

#ifndef _INC_Storage_AHCI
#define _INC_Storage_AHCI

#include "../stdinc.h"

// AHCI PCI programming interface
#define AHCI_PCI_CLASS_BASE    0x01
#define AHCI_PCI_CLASS_SUB     0x06
#define AHCI_PCI_CLASS_IF      0x01

// Generic Host Control (GHC)
#define AHCI_GHC_HR            0x00000001u
#define AHCI_GHC_IE            0x00000002u
#define AHCI_GHC_MRSM          0x00000004u
#define AHCI_GHC_AE            0x80000000u

// Port command and status
#define AHCI_PxCMD_ST          0x0001u
#define AHCI_PxCMD_SUD         0x0002u
#define AHCI_PxCMD_POD         0x0004u
#define AHCI_PxCMD_CLO         0x0008u
#define AHCI_PxCMD_FRE         0x0010u
#define AHCI_PxCMD_FR          0x4000u
#define AHCI_PxCMD_CR          0x8000u

// Port interrupt status bits used in early bring-up
#define AHCI_PxIS_TFES         (1u << 30)

// SATA status (PxSSTS)
#define AHCI_PxSSTS_DET_MASK   0x0Fu
#define AHCI_PxSSTS_IPM_MASK   0x0F00u
#define AHCI_PxSSTS_DET_OK     0x03u
#define AHCI_PxSSTS_IPM_ACTIVE 0x0100u

// Port signature values
#define AHCI_SIG_ATA           0x00000101u
#define AHCI_SIG_ATAPI         0xEB140101u
#define AHCI_SIG_SEMB          0xC33C0101u
#define AHCI_SIG_PM            0x96690101u

// ATA commands used through AHCI
#define AHCI_ATA_CMD_IDENTIFY      0xEC
#define AHCI_ATA_CMD_READ_DMA_EXT  0x25
#define AHCI_ATA_CMD_WRITE_DMA_EXT 0x35

// FIS types
enum AHCI_FISType : byte {
	AHCI_FIS_TYPE_REG_H2D  = 0x27,
	AHCI_FIS_TYPE_REG_D2H  = 0x34,
	AHCI_FIS_TYPE_DMA_ACT  = 0x39,
	AHCI_FIS_TYPE_DMA_SETUP = 0x41,
	AHCI_FIS_TYPE_DATA     = 0x46,
	AHCI_FIS_TYPE_BIST     = 0x58,
	AHCI_FIS_TYPE_PIO_SETUP = 0x5F,
	AHCI_FIS_TYPE_DEV_BITS = 0xA1,
};

#define AHCI_FIS_PM_MASK       0x0Fu
#define AHCI_FIS_H2D_C         0x80u
#define AHCI_FIS_D2H_I         0x40u
#define AHCI_FIS_PIO_D         0x20u
#define AHCI_FIS_PIO_I         0x40u
#define AHCI_FIS_DMA_D         0x20u
#define AHCI_FIS_DMA_I         0x40u
#define AHCI_FIS_DMA_A         0x80u

#ifdef _DEV_MSVC
#pragma pack(push, 1)
#endif

// Port-to-memory receive FIS area. Total size is 256 bytes.
_PACKED(struct) AHCI_ReceivedFIS {
	byte dsfis[0x1C];
	byte pad0[0x04];
	byte psfis[0x14];
	byte pad1[0x0C];
	byte rfis[0x14];
	byte pad2[0x04];
	byte sdbfis[0x08];
	byte ufis[0x40];
	byte reserved[0x60];
};

// Register Host to Device FIS
_PACKED(struct) AHCI_FIS_REG_H2D {
	byte fis_type;
	byte flags;
	byte command;
	byte featurel;

	byte lba0;
	byte lba1;
	byte lba2;
	byte device;

	byte lba3;
	byte lba4;
	byte lba5;
	byte featureh;

	byte countl;
	byte counth;
	byte icc;
	byte control;

	byte reserved1[4];
};

// Register Device to Host FIS
_PACKED(struct) AHCI_FIS_REG_D2H {
	byte fis_type;
	byte flags;
	byte status;
	byte error;

	byte lba0;
	byte lba1;
	byte lba2;
	byte device;

	byte lba3;
	byte lba4;
	byte lba5;
	byte reserved2;

	byte countl;
	byte counth;
	byte reserved3[2];

	byte reserved4[4];
};

// PIO Setup FIS
_PACKED(struct) AHCI_FIS_PIO_SETUP {
	byte fis_type;
	byte flags;
	byte status;
	byte error;

	byte lba0;
	byte lba1;
	byte lba2;
	byte device;

	byte lba3;
	byte lba4;
	byte lba5;
	byte reserved2;

	byte countl;
	byte counth;
	byte reserved3;
	byte e_status;

	word tc;
	byte reserved4[2];
};

// DMA Setup FIS
_PACKED(struct) AHCI_FIS_DMA_SETUP {
	byte fis_type;
	byte flags;
	uint16 reserved0;
	uint64 dma_buffer_id;
	uint32 reserved1;
	uint32 dma_buffer_offset;
	uint32 transfer_count;
	uint32 reserved2;
};

_PACKED(struct) AHCI_PRDT_ENTRY {
	uint32 dba;
	uint32 dbau;
	uint32 reserved0;
	uint32 dbc : 22;
	uint32 reserved1 : 9;
	uint32 i : 1;
};

_PACKED(struct) AHCI_CMD_HEADER {
	uint16 cfl : 5;
	uint16 a : 1;
	uint16 w : 1;
	uint16 p : 1;
	uint16 r : 1;
	uint16 b : 1;
	uint16 c : 1;
	uint16 reserved0 : 1;
	uint16 pmp : 4;

	uint16 prdtl;
	volatile uint32 prdbc;
	uint32 ctba;
	uint32 ctbau;
	uint32 reserved1[4];
};

// One command table followed by PRDT entries.
_PACKED(struct) AHCI_CMD_TBL {
	byte cfis[64];
	byte acmd[16];
	byte reserved[48];
	AHCI_PRDT_ENTRY prdt_entry[1];
};

_PACKED(struct) AHCI_PORT {
	uint32 clb;
	uint32 clbu;
	uint32 fb;
	uint32 fbu;
	uint32 is;
	uint32 ie;
	uint32 cmd;
	uint32 reserved0;
	uint32 tfd;
	uint32 sig;
	uint32 ssts;
	uint32 sctl;
	uint32 serr;
	uint32 sact;
	uint32 ci;
	uint32 sntf;
	uint32 fbs;
	uint32 reserved1[11];
	uint32 vendor[4];
};

_PACKED(struct) AHCI_MEM {
	uint32 cap;
	uint32 ghc;
	uint32 is;
	uint32 pi;
	uint32 vs;
	uint32 ccc_ctl;
	uint32 ccc_pts;
	uint32 em_loc;
	uint32 em_ctl;
	uint32 cap2;
	uint32 bohc;
	byte reserved[0xA0 - 0x2C];
	byte vendor[0x100 - 0xA0];
	AHCI_PORT ports[1];
};

#ifdef _DEV_MSVC
#pragma pack(pop)
#endif

#ifdef _INC_CPP
static_assert(sizeof(AHCI_FIS_REG_H2D) == 20, "AHCI_FIS_REG_H2D size mismatch");
static_assert(sizeof(AHCI_FIS_REG_D2H) == 20, "AHCI_FIS_REG_D2H size mismatch");
static_assert(sizeof(AHCI_FIS_PIO_SETUP) == 20, "AHCI_FIS_PIO_SETUP size mismatch");
static_assert(sizeof(AHCI_FIS_DMA_SETUP) == 28, "AHCI_FIS_DMA_SETUP size mismatch");
static_assert(sizeof(AHCI_PRDT_ENTRY) == 16, "AHCI_PRDT_ENTRY size mismatch");
static_assert(sizeof(AHCI_CMD_HEADER) == 32, "AHCI_CMD_HEADER size mismatch");
static_assert(sizeof(AHCI_ReceivedFIS) == 256, "AHCI_ReceivedFIS size mismatch");
static_assert(sizeof(AHCI_PORT) == 0x80, "AHCI_PORT size mismatch");
static_assert(sizeof(AHCI_MEM) == 0x100 + sizeof(AHCI_PORT), "AHCI_MEM size mismatch");
#endif

#endif


enum ATARegOffset {
	REG_DATA = 0,
	REG_ERROR = 1,
	REG_FEATURES = 1,
	REG_NSECTOR = 2, // Sector Count
    REG_LBA_LOW  = 3, // LBA Low / Sector Number
    REG_LBA_MID  = 4, // LBA Mid / Cylinder Low
    REG_LBA_HIGH = 5, // LBA High / Cylinder High
    REG_DEVICE   = 6, // Drive / Head
	REG_STATUS = 7, // Read
	#define	STATUS_BSY	0x80
	#define	STATUS_DRDY	0x40
	#define	STATUS_DFSE	0x20
	#define	STATUS_DSC	0x10
	#define	STATUS_DRQ	0x08
	#define	STATUS_CORR	0x04
	#define	STATUS_IDX	0x02
	#define	STATUS_ERR	0x01
	
	REG_CMD = 7  // Write
};
#define	HD_TIMEOUT		10000	/* in millisec */


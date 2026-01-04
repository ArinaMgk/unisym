// INCLUDE FROM {Forrest Y. Yu: OrangeS ...}

// Part Type
#define NO_PART 0x00
#define EX_PART 0x05

#define	HD_TIMEOUT		10000	/* in millisec */
#define	PARTITION_TABLE_OFFSET	0x1BE

/********************************************/
/* I/O Ports used by hard disk controllers. */
/********************************************/
/* slave disk not supported yet, all master registers below */

/* Command Block Registers */
/*	MACRO		PORT			DESCRIPTION			INPUT/OUTPUT	*/
/*	-----		----			-----------			------------	*/
#define REG_DATA	0x1F0		/*	Data				I/O		*/
#define REG_FEATURES	0x1F1		/*	Features			O		*/
#define REG_ERROR	REG_FEATURES	/*	Error				I		*/
/* 	The contents of this register are valid only when the error bit
	(ERR) in the Status Register is set, except at drive power-up or at the
	completion of the drive's internal diagnostics, when the register
	contains a status code.
	When the error bit (ERR) is set, Error Register bits are interpreted as such:
	|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
	+-----+-----+-----+-----+-----+-----+-----+-----+
	| BRK | UNC |     | IDNF|     | ABRT|TKONF| AMNF|
	+-----+-----+-----+-----+-----+-----+-----+-----+
		|     |     |     |     |     |     |     |
		|     |     |     |     |     |     |     `--- 0. Data address mark not found after correct ID field found
		|     |     |     |     |     |     `--------- 1. Track 0 not found during execution of Recalibrate command
		|     |     |     |     |     `--------------- 2. Command aborted due to drive status error or invalid command
		|     |     |     |     `--------------------- 3. Not used
		|     |     |     `--------------------------- 4. Requested sector's ID field not found
		|     |     `--------------------------------- 5. Not used
		|     `--------------------------------------- 6. Uncorrectable data error encountered
		`--------------------------------------------- 7. Bad block mark detected in the requested sector's ID field
*/
#define REG_NSECTOR  0x1F2		/*	Sector Count			I/O		*/
#define REG_LBA_LOW  0x1F3		/*	Sector Number / LBA Bits 0-7	I/O		*/
#define REG_LBA_MID  0x1F4		/*	Cylinder Low / LBA Bits 8-15	I/O		*/
#define REG_LBA_HIGH 0x1F5		/*	Cylinder High / LBA Bits 16-23	I/O		*/
#define REG_DEVICE	0x1F6		/*	Drive | Head | LBA bits 24-27	I/O		*/
/*	|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
	+-----+-----+-----+-----+-----+-----+-----+-----+
	|  1  |  L  |  1  | DRV | HS3 | HS2 | HS1 | HS0 |
	+-----+-----+-----+-----+-----+-----+-----+-----+
				|           |   \_____________________/
				|           |              |
				|           |              `------------ If L=0, Head Select.
				|           |                                    These four bits select the head number.
				|           |                                    HS0 is the least significant.
				|           |                            If L=1, HS0 through HS3 contain bit 24-27 of the LBA.
				|           `--------------------------- Drive. When DRV=0, drive 0 (master) is selected. 
				|                                               When DRV=1, drive 1 (slave) is selected.
				`--------------------------------------- LBA mode. This bit selects the mode of operation.
																When L=0, addressing is by 'CHS' mode.
																When L=1, addressing is by 'LBA' mode.
*/
#define REG_STATUS	0x1F7		/*	Status				I		*/
/* 	Any pending interrupt is cleared whenever this register is read.
	|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
	+-----+-----+-----+-----+-----+-----+-----+-----+
	| BSY | DRDY|DF/SE|  #  | DRQ |     |     | ERR |
	+-----+-----+-----+-----+-----+-----+-----+-----+
		|     |     |     |     |     |     |     |
		|     |     |     |     |     |     |     `--- 0. Error.(an error occurred)
		|     |     |     |     |     |     `--------- 1. Obsolete.
		|     |     |     |     |     `--------------- 2. Obsolete.
		|     |     |     |     `--------------------- 3. Data Request. (ready to transfer data)
		|     |     |     `--------------------------- 4. Command dependent. (formerly DSC bit)
		|     |     `--------------------------------- 5. Device Fault / Stream Error.
		|     `--------------------------------------- 6. Drive Ready.
		`--------------------------------------------- 7. Busy. If BSY=1, no other bits in the register are valid.
*/
#define	STATUS_BSY	0x80
#define	STATUS_DRDY	0x40
#define	STATUS_DFSE	0x20
#define	STATUS_DSC	0x10
#define	STATUS_DRQ	0x08
#define	STATUS_CORR	0x04
#define	STATUS_IDX	0x02
#define	STATUS_ERR	0x01

#define REG_CMD		REG_STATUS	/*	Command				O		*/
/*
	+--------+---------------------------------+-----------------+
	| Command| Command Description             | Parameters Used |
	| Code   |                                 | PC SC SN CY DH  |
	+--------+---------------------------------+-----------------+
	| ECh  @ | Identify Drive                  |             D   |
	| 91h    | Initialize Drive Parameters     |    V        V   |
	| 20h    | Read Sectors With Retry         |    V  V  V  V   |
	| E8h  @ | Write Buffer                    |             D   |
	+--------+---------------------------------+-----------------+

	KEY FOR SYMBOLS IN THE TABLE:
	===========================================-----=========================================================================
	PC    Register 1F1: Write Precompensation	@     These commands are optional and may not be supported by some drives.
	SC    Register 1F2: Sector Count		D     Only DRIVE parameter is valid, HEAD parameter is ignored.
	SN    Register 1F3: Sector Number		D+    Both drives execute this command regardless of the DRIVE parameter.
	CY    Register 1F4+1F5: Cylinder low + high	V     Indicates that the register contains a valid paramterer.
	DH    Register 1F6: Drive / Head
*/

/* Control Block Registers */
/*	MACRO		PORT			DESCRIPTION			INPUT/OUTPUT	*/
/*	-----		----			-----------			------------	*/
#define REG_DEV_CTRL	0x3F6		/*	Device Control			O		*/
//{} Move these EPIC ...
/*	|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
	+-----+-----+-----+-----+-----+-----+-----+-----+
	| HOB |  -  |  -  |  -  |  -  |SRST |-IEN |  0  |
	+-----+-----+-----+-----+-----+-----+-----+-----+
		|                             |     |
		|                             |     `--------- Interrupt Enable.
		|                             |                  - IEN=0, and the drive is selected,
		|                             |                    drive interrupts to the host will be enabled.
		|                             |                  - IEN=1, or the drive is not selected,
		|                             |                    drive interrupts to the host will be disabled.
		|                             `--------------- Software Reset.
		|                                                - The drive is held reset when RST=1.
		|                                                  Setting RST=0 re-enables the drive.
		|                                                - The host must set RST=1 and wait for at least
		|                                                  5 microsecondsbefore setting RST=0, to ensure
		|                                                  that the drive recognizes the reset.
		`--------------------------------------------- HOB (High Order Byte)
														- defined by 48-bit Address feature set.
*/
#define REG_ALT_STATUS	REG_DEV_CTRL	/*	Alternate Status		I		*/
					/*	This register contains the same information as the Status Register.
						The only difference is that reading this register does not imply interrupt acknowledge or clear a pending interrupt.
					*/

#define REG_DRV_ADDR	0x3F7		/*	Drive Address			I		*/






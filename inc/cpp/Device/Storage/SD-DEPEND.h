

// SD_Exported_Types_Group4 Card Specific Data: CSD Register
typedef struct
{
	uint8  CSDStruct;            // CSD structure
	uint8  SysSpecVersion;       // System specification version
	uint8  Reserved1;            // Reserved
	uint8  TAAC;                 // Data read access time 1
	uint8  NSAC;                 // Data read access time 2 in CLK cycles
	uint8  MaxBusClkFrec;        // Max. bus clock frequency
	uint16 CardComdClasses;      // Card command classes
	uint8  RdBlockLen;           // Max. read data block length
	uint8  PartBlockRead;        // Partial blocks for read allowed
	uint8  WrBlockMisalign;      // Write block misalignment
	uint8  RdBlockMisalign;      // Read block misalignment
	uint8  DSRImpl;              // DSR implemented
	uint8  Reserved2;            // Reserved
	uint32 DeviceSize;           // Device Size
	uint8  MaxRdCurrentVDDMin;   // Max. read current @ VDD min
	uint8  MaxRdCurrentVDDMax;   // Max. read current @ VDD max
	uint8  MaxWrCurrentVDDMin;   // Max. write current @ VDD min
	uint8  MaxWrCurrentVDDMax;   // Max. write current @ VDD max
	uint8  DeviceSizeMul;        // Device size multiplier
	uint8  EraseGrSize;          // Erase group size
	uint8  EraseGrMul;           // Erase group size multiplier
	uint8  WrProtectGrSize;      // Write protect group size
	uint8  WrProtectGrEnable;    // Write protect group enable
	uint8  ManDeflECC;           // Manufacturer default ECC
	uint8  WrSpeedFact;          // Write speed factor
	uint8  MaxWrBlockLen;        // Max. write data block length
	uint8  WriteBlockPaPartial;  // Partial blocks for write allowed
	uint8  Reserved3;            // Reserved
	uint8  ContentProtectAppli;  // Content protection application
	uint8  FileFormatGroup;      // File format group
	uint8  CopyFlag;             // Copy flag (OTP)
	uint8  PermWrProtect;        // Permanent write protection
	uint8  TempWrProtect;        // Temporary write protection
	uint8  FileFormat;           // File format
	uint8  ECC;                  // ECC code
	uint8  CSD_CRC;              // CSD CRC
	uint8  Reserved4;            // Always 1
} HAL_SD_CardCSDTypeDef;

// SDMMC Data Control structure
enum class SDMMC_DataTransferDir {
	toCard = 0,
	toSDMMC = 1
};// SDMMC_DCTRLR[bit 1]
enum class SDMMC_DataTransferMode {
	Block = 0,
	Stream = 0b10
};// SDMMC_DCTRLR[bit 2 len 2]
typedef struct
{
	uint32 DataTimeOut;     // data timeout period in card bus clock periods.
	uint32 DataLength;      // number of data bytes to be transferred.
	uint32 DataBlockSize;   // data block size for block transfer.
	SDMMC_DataTransferDir TransferDir;     // data transfer direction, whether the transfer R/W
	SDMMC_DataTransferMode TransferMode;    // whether data transfer is in stream or block mode.
	bool DPSM;            // whether SDMMC Data path state machine (DPSM) O/X
} SDMMC_DataInitTypeDef;

// ---- Link Node ----

// SDEx_Exported_Types_Group1 SD Card Internal DMA Buffer structure
typedef struct
{
	uint32 IDMALAR;  // SDMMC DMA linked list configuration register
	uint32 IDMABASER;// SDMMC DMA buffer base address register
	uint32 IDMABSIZE;// SDMMC DMA buffer size register
} SDMMC_DMALinkNode;// AKA SDMMC_DMALinkNodeTypeDef;
typedef struct
{
	SDMMC_DMALinkNode* pHeadNode;
	SDMMC_DMALinkNode* pTailNode;
	uint32 NodesCounter;// who are ready for execution
#ifdef _INC_CPP

#endif	
} SDMMC_DMALinkedList;// AKA SDMMC_DMALinkedListTypeDef;



// using string::Slice as SDMMC_DMALinkNodeConfTypeDef;






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

// Card Identification Data: CID Register
typedef struct
{
	uint8_t  ManufacturerID;  // Manufacturer ID
	uint16_t OEM_AppliID;     // OEM/Application ID
	uint32_t ProdName1;       // Product Name part1
	uint8_t  ProdName2;       // Product Name part2
	uint8_t  ProdRev;         // Product Revision
	uint32_t ProdSN;          // Product Serial Number
	uint8_t  Reserved1;       // Reserved1
	uint16_t ManufactDate;    // Manufacturing Date
	uint8_t  CID_CRC;         // CID CRC
	uint8_t  Reserved2;       // Always 1
} HAL_SD_CardCIDTypeDef;

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
	//{} Methods
#endif
} SDMMC_DMALinkedList;// AKA SDMMC_DMALinkedListTypeDef;



// using string::Slice as SDMMC_DMALinkNodeConfTypeDef;
//{TODO} Dnode and DChain


// SD_Exported_Types_Group6 SD Card Status returned by ACMD13
enum class SDMMC_BusWidth : uint32 {
	Bits1 = 0b00,
	Bits4 = 0b01,
	Bits8 = 0b10
};
typedef struct {
	SDMMC_BusWidth  DataBusWidth;  // Shows the currently defined data bus width                 */
	uint32 ProtectedAreaSize;      // Carries information about the capacity of protected area   */
	uint16 CardType;               //{TEMP} HALF-DONE
	uint16 EraseSize;              // Determines the number of AUs to be erased in one operation */
	uint8  SecuredMode;            // Card is in secured mode of operation                       */
	uint8  SpeedClass;             // Carries information about the speed class of the card      */
	uint8  PerformanceMove;        // Carries information about the card's performance move      */
	uint8  AllocationUnitSize;     // Carries information about the card's allocation unit size  */
	uint8  EraseTimeout;           // Determines the timeout for any number of AU erase          */
	uint8  EraseOffset;            // Carries information about the erase offset                 */
	uint8  UhsSpeedGrade;          // Carries information about the speed grade of UHS card      */
	uint8  UhsAllocationUnitSize;  // Carries information about the UHS card's allocation unit size  */
	uint8  VideoSpeedClass;        // Carries information about the Video Speed Class of UHS card    */
} HAL_SD_CardStatusTypeDef;

enum class HAL_SD_CardStateTypeDef {
	READY          = 0x00000001U,  /*!< Card state is ready                     */
	IDENTIFICATION = 0x00000002U,  /*!< Card is in identification state         */
	STANDBY        = 0x00000003U,  /*!< Card is in standby state                */
	TRANSFER       = 0x00000004U,  /*!< Card is in transfer state               */
	SENDING        = 0x00000005U,  /*!< Card is sending an operation            */
	RECEIVING      = 0x00000006U,  /*!< Card is receiving operation information */
	PROGRAMMING    = 0x00000007U,  /*!< Card is in programming state            */
	DISCONNECTED   = 0x00000008U,  /*!< Card is disconnected                    */
	ERROR          = 0x000000FFU,  /*!< Card response Error                     */
};

enum class SDMMC_SPEED_MODE {
	AUTO          = (0x00000000U),
	DEFAULT       = (0x00000001U),
	HIGH          = (0x00000002U),
	ULTRA         = (0x00000003U),
	ULTRA_SDR104  = ULTRA,
	DDR           = (0x00000004U),
	ULTRA_SDR50   = (0x00000005U),
};


enum class SDContext {
	NONE                 = 0x00000000U, // None
	IT                   = 0x00000008U, // Process in Interrupt mode
	DMA                  = 0x00000080U, // Process in DMA mode
	// Read single block operation
	READ_SINGLE_BLOCK    = 0x00000001U,
	READ_SINGLE_BLOCK_IT = READ_SINGLE_BLOCK | IT,
	READ_SINGLE_BLOCK_DMA = READ_SINGLE_BLOCK | DMA,
	// Read multiple blocks operatio
	READ_MULTIPLE_BLOCK  = 0x00000002U,
	READ_MULTIPLE_BLOCK_IT  =  READ_MULTIPLE_BLOCK | IT,
	READ_MULTIPLE_BLOCK_DMA  =  READ_MULTIPLE_BLOCK | DMA,
	// Write single block operation
	WRITE_SINGLE_BLOCK   = 0x00000010U,
	WRITE_SINGLE_BLOCK_IT   = WRITE_SINGLE_BLOCK | IT,
	WRITE_SINGLE_BLOCK_DMA   = WRITE_SINGLE_BLOCK | DMA,
	// Write multiple blocks operation
	WRITE_MULTIPLE_BLOCK = 0x00000020U,
	WRITE_MULTIPLE_BLOCK_IT = WRITE_MULTIPLE_BLOCK | IT,
	WRITE_MULTIPLE_BLOCK_DMA = WRITE_MULTIPLE_BLOCK | DMA,
};

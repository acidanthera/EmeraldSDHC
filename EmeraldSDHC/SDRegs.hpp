//
//  SDRegs.hpp
//  SD Host Controller and SD/MMC card register definitions
//
//  Copyright © 2021-2023 Goldfish64. All rights reserved.
//

#ifndef SDRegs_hpp
#define SDRegs_hpp

#define BIT0  (1<<0)
#define BIT1  (1<<1)
#define BIT2  (1<<2)
#define BIT3  (1<<3)
#define BIT4  (1<<4)
#define BIT5  (1<<5)
#define BIT6  (1<<6)
#define BIT7  (1<<7)
#define BIT8  (1<<8)
#define BIT9  (1<<9)
#define BIT10 (1<<10)
#define BIT11 (1<<11)
#define BIT12 (1<<12)
#define BIT13 (1<<13)
#define BIT14 (1<<14)
#define BIT15 (1<<15)
#define BIT16 (1<<16)
#define BIT17 (1<<17)
#define BIT18 (1<<18)
#define BIT19 (1<<19)
#define BIT20 (1<<20)
#define BIT21 (1<<21)
#define BIT22 (1<<22)
#define BIT23 (1<<23)
#define BIT24 (1<<24)
#define BIT25 (1<<25)
#define BIT26 (1<<26)
#define BIT27 (1<<27)
#define BIT28 (1<<28)
#define BIT29 (1<<29)
#define BIT30 (1<<30)
#define BIT31 (1<<31)

//
// Maximum number of slots on PCI-based host controllers.
//
#define kSDHCMaximumSlotCount 6

#define kSDARelativeAddressShift 16

//
// SD Host Controller registers.
//
#define kSDHCRegSDMA                            0x00
#define kSDHCRegBlockSize                       0x04
#define kSDHCRegBlockCount                      0x06
#define kSDHCRegArgument                        0x08
#define kSDHCRegTransferMode                    0x0C
#define kSDHCRegTransferModeDMAEnable           BIT0
#define kSDHCRegTransferModeBlockCountEnable    BIT1
#define kSDHCRegTransferModeAutoCMD12           BIT2
#define kSDHCRegTransferModeDataTransferRead    BIT4
#define kSDHCRegTransferModeMultipleBlock       BIT5

#define kSDHCRegCommand                 0x0E

// 128 bits total
#define kSDHCRegResponse0               0x10
#define kSDHCRegResponse1               0x18

#define kSDHCRegBufferDataPort          0x20

#define kSDHCRegPresentState                  0x24
#define kSDHCRegPresentStateCardCmdInhibit    BIT0
#define kSDHCRegPresentStateCardDatInhibit    BIT1
#define kSDHCRegPresentStateCardInserted      BIT16
#define kSDHCRegPresentStateCardWriteable     BIT19

#define kSDHCRegHostControl1                0x28
#define kSDHCRegHostControl1LEDOn           BIT0
#define kSDHCRegHostControl1DataWidth4Bit   BIT1
#define kSDHCRegHostControl1HighSpeedEnable BIT2
#define kSDHCRegHostControl1DataWidth8Bit   BIT5
#define kSDHCRegHostControl1DataWidthMask   (kSDHCRegHostControl1DataWidth4Bit | kSDHCRegHostControl1DataWidth8Bit)
#define kSDHCRegHostControl1DMA_SDMA        0x00
#define kSDHCRegHostControl1DMA_ADMA2_32Bit BIT4
#define kSDHCRegHostControl1DMA_ADMA2_64Bit (BIT3 | BIT4)
#define kSDHCRegHostControl1DMA_Mask        (BIT3 | BIT4)

#define kSDHCRegPowerControl            0x29
#define kSDHCRegPowerControlVDD1On      BIT0
#define kSDHCRegPowerControlVDD1_3_3    (BIT1 | BIT2 | BIT3)
#define kSDHCRegPowerControlVDD1_3_0    (BIT2 | BIT3)
#define kSDHCRegPowerControlVDD1_1_8    (BIT1 | BIT3)
#define kSDHCRegPowerControlVDD2On      BIT4
#define kSDHCRegPowerControlVDD2_1_8    (BIT5 | BIT7)

#define kSDHCRegBlockGapControl                     0x2A
#define kSDHCRegWakeupControl                       0x2B
#define kSDHCRegClockControl                        0x2C
#define kSDHCRegClockControlIntClockEnable          BIT0
#define kSDHCRegClockControlIntClockStable          BIT1
#define kSDHCRegClockControlSDClockEnable           BIT2
#define kSDHCRegClockControlPLLEnable               BIT3
#define kSDHCRegClockControlFreqSelectLowShift      8
#define kSDHCRegClockControlFreqSelectLowMask       0xFF00
#define kSDHCRegClockControlFreqSelectHighRhShift   2
#define kSDHCRegClockControlFreqSelectHighMask      0xC0


#define kSDHCRegTimeoutControl          0x2E

#define kSDHCRegSoftwareReset           0x2F
#define kSDHCRegSoftwareResetAll        BIT0
#define kSDHCRegSoftwareResetCmd        BIT1
#define kSDHCRegSoftwareResetDat        BIT2

#define kSDHCRegNormalIntStatus                   0x30
#define kSDHCRegNormalIntStatusCommandComplete    BIT0
#define kSDHCRegNormalIntStatusTransferComplete   BIT1
#define kSDHCRegNormalIntStatusBlockGapEvent      BIT2
#define kSDHCRegNormalIntStatusDMAInterrupt       BIT3
#define kSDHCRegNormalIntStatusBufferWriteReady   BIT4
#define kSDHCRegNormalIntStatusBufferReadReady    BIT5
#define kSDHCRegNormalIntStatusCardInsertion      BIT6
#define kSDHCRegNormalIntStatusCardRemoval        BIT7
#define kSDHCRegNormalIntStatusErrorInterrupt     BIT15

#define kSDHCRegErrorIntStatus                  0x32
#define kSDHCRegNormalIntStatusEnable           0x34
#define kSDHCRegErrorIntStatusEnable            0x36
#define kSDHCRegNormalIntSignalEnable           0x38
#define kSDHCRegErrorIntSignalEnable            0x3A
#define kSDHCRegAutoCmdErrorStatus              0x3C
#define kSDHCRegHostControl2                    0x3E
#define kSDHCRegHostControl2UHS_SDR12           0x0
#define kSDHCRegHostControl2UHS_SDR25           0x1
#define kSDHCRegHostControl2UHS_SDR50           0x2
#define kSDHCRegHostControl2UHS_SDR104          0x3
#define kSDHCRegHostControl2UHS_DDR50           0x4
#define kSDHCRegHostControl2UHS_HS400           0x5 // Non-standard, taken from FreeBSD driver
#define kSDHCRegHostControl2UHS_UHSII           (BIT0 | BIT1 | BIT2)
#define kSDHCRegHostControl2UHS_Mask            (BIT0 | BIT1 | BIT2)
#define kSDHCRegHostControl21_8VSignaling       BIT3
#define kSDHCRegHostControl2DriverStrength_A    BIT4
#define kSDHCRegHostControl2DriverStrength_B    0x0
#define kSDHCRegHostControl2DriverStrength_C    BIT5
#define kSDHCRegHostControl2DriverStrength_D    (BIT4 | BIT5)
#define kSDHCRegHostControl2ExecuteTuning       BIT6
#define kSDHCRegHostControl2SamplingClockSelect BIT7
#define kSDHCRegHostControl2UHSIIEnable         BIT8
#define kSDHCRegHostControl2ADMA2_26Bit         BIT10
#define kSDHCRegHostControl2CMD23Enable         BIT11
#define kSDHCRegHostControl2HostVersion4Enable  BIT12
#define kSDHCRegHostControl264BitAddressing     BIT13
#define kSDHCRegHostControl2AsyncIntEnable      BIT14
#define kSDHCRegHostControl2PresetValueEnable   BIT15

#define kSDHCRegCapabilities                      0x40
#define kSDHCRegCapabilitiesTimeoutClockMHz       BIT7
#define kSDHCRegCapabilitiesBaseClockMaskVer1     0x3F00
#define kSDHCRegCapabilitiesBaseClockMaskVer3     0xFF00
#define kSDHCRegCapabilitiesBaseClockShift        8
#define kSDHCRegCapabilitiesMaxBlockLength1024    BIT16
#define kSDHCRegCapabilitiesMaxBlockLength2048    BIT17
#define kSDHCRegCapabilitiesEmbedded8BitSupported BIT18
#define kSDHCRegCapabilitiesADMA2Supported        BIT19
#define kSDHCRegCapabilitiesHighSpeedSupported    BIT21
#define kSDHCRegCapabilitiesSDMASupported         BIT22
#define kSDHCRegCapabilitiesSuspendSupported      BIT23
#define kSDHCRegCapabilitiesVoltage3_3Supported   BIT24
#define kSDHCRegCapabilitiesVoltage3_0Supported   BIT25
#define kSDHCRegCapabilitiesVoltage1_8Supported   BIT26
#define kSDHCRegCapabilitiesSlotTypeEmbedded      BIT30

#define kSDHCRegMaxCurrentCapabilities  0x48

#define kSDHCRegForceEventAutoCmdErrorStatus  0x50
#define kSDHCRegForceEventErrorIntStatus      0x52
#define kSDHCRegADMAErrorStatus         0x54
#define kSDHCRegADMASysAddress          0x58

#define kSDHCRegPresetValue             0x60

#define kSDHCRegPresetValueUHSII        0x74
#define kSDHCRegADMA3IDAddress          0x78

// Common across all slots on host controller
#define kSDHCRegHostControllerSlotIntStatus   0xFC
#define kSDHCRegHostControllerVersion         0xFE
#define kSDHCRegHostControllerVersionMask     0xFF

#define kSDATuneBytes4Bits      64
#define kSDATuneBytes8Bits      128

#define kSDAMaxBlockCount16       UINT16_MAX
#define kSDAMaxBlockCount32       UINT32_MAX

//
// OCR bits
// Initial value indicates support for all voltages and high capacity.
//
#define kSDAOCRCCSHighCapacity    BIT30
#define kSDAOCRCardBusy           BIT31
#define kSDAOCRInitValue          (kSDAOCRCCSHighCapacity | 0xFF8000)

//
// SD Host Controller versions.
//
typedef enum : UInt8 {
  kSDHostControllerVersion1_00  = 0x00,
  kSDHostControllerVersion2_00  = 0x01,
  kSDHostControllerVersion3_00  = 0x02,
  kSDHostControllerVersion4_00  = 0x03,
  kSDHostControllerVersion4_10  = 0x04,
  kSDHostControllerVersion4_20  = 0x05
} SDHostControllerVersion;

//
// SD Host Controller response flags.
//
typedef enum : UInt8 {
  kSDAResponseTypeR0      = 0,
  kSDAResponseTypeR1      = BIT1 | BIT3 | BIT4,
  kSDAResponseTypeR1b     = BIT0 | BIT1 | BIT3 | BIT4,
  kSDAResponseTypeR1d     = BIT1 | BIT3 | BIT4 | BIT5, // Not defined in spec, R1 with data flag set.
  kSDAResponseTypeR2      = BIT0 | BIT3,
  kSDAResponseTypeR3      = BIT1,
  kSDAResponseTypeR4      = BIT1,
  kSDAResponseTypeR5      = BIT1 | BIT3 | BIT4,
  kSDAResponseTypeR5b     = BIT0 | BIT1 | BIT3 | BIT4,
  kSDAResponseTypeR6      = BIT1 | BIT3 | BIT4,
  kSDAResponseTypeR7      = BIT1 | BIT3 | BIT4
} SDAResponseType;

typedef enum : UInt8 {
  kSDHostADMA2DescriptorActionNone      = 0x0,
  kSDHostADMA2DescriptorActionReserved  = 0x2,
  kSDHostADMA2DescriptorActionTransfer  = 0x4,
  kSDHostADMA2DescriptorActionLink      = 0x6
} SDHostADMA2DescriptorAction;

//
// ADMA2 descriptor.
//
typedef struct {
  UInt8 valid : 1;
  UInt8 end : 1;
  UInt8 forceInterrupt : 1;
  SDHostADMA2DescriptorAction action : 3;
  UInt16 length10 : 10;
  UInt16 length16;
  UInt32 address;
} SDHostADMA2Descriptor32;

typedef struct {
  SDHostADMA2Descriptor32 desc;
  SDHostADMA2Descriptor32 link;
} SDHostADMA2Descriptor32WithLink;

//
// SD commands.
//
typedef enum : UInt32 {
  //
  // Basic commands (class 0).
  //
  kSDCommandGoIdleState           = 0,
  kSDCommandAllSendCID            = 2,
  kSDCommandSendRelativeAddress   = 3,
  kSDCommandSetDSR                = 4,
  kSDCommandSelectDeselectCard    = 7,
  kSDCommandSendIfCond            = 8,
  kSDCommandSendCSD               = 9,
  kSDCommandSendCID               = 10,
  kSDCommandVoltageSwitch         = 11,
  kSDCommandStopTransmission      = 12,
  kSDCommandSendStatus            = 13,
  kSDCommandGoInactiveState       = 15,

  //
  // Block read and write commands (class 2 and class 4).
  //
  kSDCommandSetBlockLength        = 16,
  kSDCommandReadSingleBlock       = 17,
  kSDCommandReadMultipleBlock     = 18,
  kSDCommandSendTuningBlock       = 19,
  kSDCommandSpeedClassControl     = 20,
  kSDCommandAddressExtension      = 22,
  kSDCommandSetBlockCount         = 23,
  kSDCommandWriteBlock            = 24,
  kSDCommandWriteMultipleBlock    = 25,
  kSDCommandProgramCSD            = 27,

  //
  // Block write protection commands (class 6).
  //
  kSDCommandSetWriteProtect       = 28,
  kSDCommandClearWriteProtect     = 29,
  kSDCommandSendWriteProtect      = 30,

  //
  // Erase commands (class 5).
  //
  kSDCommandEraseWriteBlockStart  = 32,
  kSDCommandEraseWriteBlockEnd    = 33,
  kSDCommandErase                 = 38,

  //
  // Lock commands (clsas 7).
  //
  kSDCommandLockUnlock            = 42,

  //
  // Application commands (class 8).
  //
  kSDCommandAppCommand            = 55,
  kSDCommandGeneralCommand        = 56,

  kSDCommandInvalid               = UINT32_MAX
} SDCommand;

//
// SD application commands.
// Flag is used to identify these vs regular commands.
//
#define kSDAppCommandFlag         BIT30
typedef enum : UInt32 {
  kSDAppCommandSetBusWidth              = 6  | kSDAppCommandFlag,
  kSDAppCommandSDStatus                 = 13 | kSDAppCommandFlag,
  kSDAppCommandSendNumWrBlocks          = 22 | kSDAppCommandFlag,
  kSDAppCommandSetWrBlkEraseCount       = 23 | kSDAppCommandFlag,
  kSDAppCommandSendOpCond               = 41 | kSDAppCommandFlag,
  kSDAppCommandSetClearCardDetect       = 42 | kSDAppCommandFlag,
  kSDAppCommandSendSCR                  = 51 | kSDAppCommandFlag,

  kSDAppCommandInvalid                  = UINT32_MAX
} SDAppCommand;

//
// MMC commands.
//
typedef enum : UInt32 {
  //
  // Basic commands (class 0).
  //
  kMMCCommandGoIdleState            = 0,
  kMMCCommandSendOpCond             = 1,
  kMMCCommandAllSendCID             = 2,
  kMMCCommandSetRelativeAddress     = 3,
  kMMCCommandSetDSR                 = 4,
  kMMCCommandSleepAwake             = 5,
  kMMCCommandSwitch                 = 6,
  kMMCCommandSelectDeselectCard     = 7,
  kMMCCommandSendExtCSD             = 8,
  kMMCCommandSendCSD                = 9,
  kMMCCommandSendCID                = 10,
  kMMCCommandReadDatUntilStop       = 11,
  kMMCCommandStopTransmission       = 12,
  kMMCCommandSendStatus             = 13,
  kMMCCommandGoInactiveState        = 15,

  //
  // Block read and write commands (class 2, class 3, and class 4).
  //
  kMMCCommandSetBlockLength         = 16,
  kMMCCommandReadSingleBlock        = 17,
  kMMCCommandReadMultipleBlock      = 18,
  kMMCCommandWriteDatUntilStop      = 20,
  kMMCCommandSendTuningBlock        = 21,
  kMMCCommandSetBlockCount          = 23,
  kMMCCommandWriteBlock             = 24,
  kMMCCommandWriteMultipleBlock     = 25,
  kMMCCommandProgramCID             = 26,
  kMMCCommandProgramCSD             = 27,

  //
  // Block write protection commands (class 6).
  //
  kMMCCommandSetWriteProtect        = 28,
  kMMCCommandClearWriteProtect      = 29,
  kMMCCommandSendWriteProtect       = 30,
  kMMCCommandSendWriteProtectType   = 31,

  //
  // Erase commands (class 5).
  //
  kMMCCommandEraseGroupStart        = 35,
  kMMCCommandEraseGroupEnd          = 36,
  kMMCCommandErase                  = 38,

  //
  // Lock commands (clsas 7).
  //
  kMMCCommandLockUnlock             = 42,

  //
  // Application commands (class 8).
  //
  kMMCCommandAppCommand             = 55,
  kMMCCommandGeneralCommand         = 56,

  kMMCCommandInvalid                = UINT32_MAX
} MMCCommand;

//
// MMC Switch command arguments.
//
typedef enum : UInt8 {
  kMMCSwitchAccessCommandSet    = 0x0,
  kMMCSwitchAccessSetBits       = 0x1,
  kMMCSwitchAccessClearBits     = 0x2,
  kMMCSwitchAccessWriteByte     = 0x3
} MMCSwitchAccessBits;

#define kMMCSwitchCommandSetMask  0x7
#define kMMCSwitchValueShift      8
#define kMMCSwitchValueMask       0xFF00
#define kMMCSwitchIndexShift      16
#define kMMCSwitchIndexMask       0xFF0000
#define kMMCSwitchAccessShift     24
#define kMMCSwitchAccessMask      0x3000000

#pragma pack(push, 1)

//
// SD CID register struct.
// CRC excluded as the SD host controller strips this away.
//
typedef struct {
  UInt8   manufactureMonth : 4;
  UInt8   manufactureYear : 8;
  UInt8   reserved : 4;
  UInt32  serialNumber;
  UInt8   revision;
  UInt8   name[5];
  UInt16  oemId;
  UInt8   manufacturerId;
} SDCIDRegister;

//
// SD CSD versions.
//
// V1.0 - SDSC
// V2.0 - SDHC and SDXC
// V3.0 - SDUC
//
#define kSDCSDVersion1_0    0
#define kSDCSDVersion2_0    1
#define kSDCSDVersion3_0    2

//
// SD CSD register v1.0 struct.
//
typedef struct {
  UInt8   reserved1 : 2;
  UInt8   fileFormat : 2;
  UInt8   tmpWriteProtect : 1;
  UInt8   permWriteProtect : 1;
  UInt8   copy : 1;
  UInt8   fileFormatGroup : 1;
  UInt8   reserved2 : 5;

  UInt8   writeBLPartial : 1;
  UInt8   writeBLLength : 4;
  UInt8   writeSpeedFactor : 3;
  UInt8   reserved3 : 2;

  UInt8   writeProtectGroupEnable : 1;
  UInt8   writeProtectGroupSize : 7;
  UInt8   eraseSectorSize : 7;
  UInt8   eraseBlockEnable : 1;
  UInt8   cSizeMultiplier : 3;
  UInt8   vddWriteCurrentMax : 3;
  UInt8   vddWriteCurrentMin : 3;
  UInt8   vddReadCurrentMax : 3;
  UInt8   vddReadCurrentMin : 3;
  UInt16  cSize : 12;
  UInt8   reserved4 : 2;

  UInt8   dsrImplemented : 1;
  UInt8   readBlockMisalign : 1;
  UInt8   writeBlockMisalign : 1;
  UInt8   readBLPartial : 1;
  UInt8   readBLLength : 4;
  UInt16  ccc : 12;
  UInt8   tranSpeed;
  UInt8   nsac;
  UInt8   taac;
  UInt8   reserved5 : 6;
  UInt8   csdStructure : 2;
} SDCSDRegisterV1;

//
// SD CSD register v2.0 struct.
//
typedef struct {
  UInt8   reserved1 : 2;
  UInt8   fileFormat : 2;
  UInt8   tmpWriteProtect : 1;
  UInt8   permWriteProtect : 1;
  UInt8   copy : 1;
  UInt8   fileFormatGroup : 1;
  UInt8   reserved2 : 5;

  UInt8   writeBLPartial : 1;
  UInt8   writeBLLength : 4;
  UInt8   writeSpeedFactor : 3;
  UInt8   reserved3 : 2;

  UInt8   writeProtectGroupEnable : 1;
  UInt8   writeProtectGroupSize : 7;
  UInt8   eraseSectorSize : 7;
  UInt8   eraseBlockEnable : 1;
  UInt8   reserved4 : 1;

  UInt32  cSize : 22;
  UInt8   reserved5 : 6;

  UInt8   dsrImplemented : 1;
  UInt8   readBlockMisalign : 1;
  UInt8   writeBlockMisalign : 1;
  UInt8   readBLPartial : 1;
  UInt8   readBLLength : 4;
  UInt16  ccc : 12;
  UInt8   tranSpeed;
  UInt8   nsac;
  UInt8   taac;
  UInt8   reserved6 : 6;
  UInt8   csdStructure : 2;
} SDCSDRegisterV2;

//
// SD CSD register v3.0 struct.
//
typedef struct {
  UInt8   reserved1 : 2;
  UInt8   fileFormat : 2;
  UInt8   tmpWriteProtect : 1;
  UInt8   permWriteProtect : 1;
  UInt8   copy : 1;
  UInt8   fileFormatGroup : 1;
  UInt8   reserved2 : 5;

  UInt8   writeBLPartial : 1;
  UInt8   writeBLLength : 4;
  UInt8   writeSpeedFactor : 3;
  UInt8   reserved3 : 2;

  UInt8   writeProtectGroupEnable : 1;
  UInt8   writeProtectGroupSize : 7;
  UInt8   eraseSectorSize : 7;
  UInt8   eraseBlockEnable : 1;
  UInt8   reserved4 : 1;

  UInt32  cSize : 28;

  UInt8   dsrImplemented : 1;
  UInt8   readBlockMisalign : 1;
  UInt8   writeBlockMisalign : 1;
  UInt8   readBLPartial : 1;
  UInt8   readBLLength : 4;
  UInt16  ccc : 12;
  UInt8   tranSpeed;
  UInt8   nsac;
  UInt8   taac;
  UInt8   reserved6 : 6;
  UInt8   csdStructure : 2;
} SDCSDRegisterV3;

//
// SD SCR register struct.
//
typedef struct {
  UInt32  manufacturerReserved;

  UInt8   commandSupportBits : 4;
  UInt8   reserved : 2;
  UInt8   sdSpecX : 4;
  UInt8   sdSpec4 : 1;
  UInt8   extendedSecurity : 4;
  UInt8   sdSpec3 : 1;
  UInt8   sdBusWidths : 4;
#define kSDBusWidth1Bit     0
#define kSDBusWidth4Bit     2
  UInt8   sdSecurity : 3;
  UInt8   dataStatusAfterErase : 1;
  UInt8   sdSpec : 4;
  UInt8   scrStructure : 4;
} SDSCRRegister;

//
// MMC CID register struct.
// CRC excluded as the SD host controller strips this away.
//
typedef struct {
  UInt8   manufactureDate;
  UInt32  serialNumber;
  UInt8   revision;
  UInt8   name[6];
  UInt8   oemId;
  UInt8   cbx : 2;
  UInt8   reserved : 6;
  UInt8   manufacturerId;
} MMCCIDRegister;

#define kMMCCID_CBXEmbedded     0x1

//
// MMC CSD structure versions.
//
// V1.0 - V1.0-1.2
// V1.1 - V1.4-2.2
// V1.2 - V3.1-4.1
// EXT  - Version is encoded in extended CSD.
//
#define kMMCCSDVersion1_0   0
#define kMMCCSDVersion1_1   1
#define kMMCCSDVersion1_2   2
#define kMMCCSDVersionEXT   3

//
// MMC specification versions.
//
#define kMMCSpecVersion1_0  0
#define kMMCSpecVersion1_4  1
#define kMMCSpecVersion2_x  2
#define kMMCSpecVersion3_x  3
#define kMMCSpecVersion4_x  4

//
// MMC CSD register struct.
// CRC excluded as the SD host controller strips this away.
//
typedef struct {
  UInt8   ecc : 2;
  UInt8   fileFormat : 2;
  UInt8   tmpWriteProtect : 1;
  UInt8   permWriteProtect : 1;
  UInt8   copy : 1;
  UInt8   fileFormatGroup : 1;
  UInt8   contentProtApp : 1;

  UInt8   reserved1 : 4;
  UInt8   writeBLPartial : 1;
  UInt8   writeBLLength : 4;
  UInt8   writeSpeedFactor : 3;
  UInt8   defaultECC : 2;
  UInt8   writeProtectGroupEnable : 1;
  UInt8   writeProtectGroupSize : 5;
  UInt8   eraseGroupSizeMultiplier : 5;
  UInt8   eraseGroupSize : 5;
  UInt8   cSizeMultiplier : 3;

  UInt8   vddWriteCurrentMax : 3;
  UInt8   vddWriteCurrentMin : 3;
  UInt8   vddReadCurrentMax : 3;
  UInt8   vddReadCurrentMin : 3;
  UInt16  cSize : 12;
  UInt8   reserved2 : 2;

  UInt8   dsrImplemented : 1;
  UInt8   readBlockMisalign : 1;
  UInt8   writeBlockMisalign : 1;
  UInt8   readBLPartial : 1;
  UInt8   readBLLength : 4;
  UInt16  ccc : 12;
  UInt8   tranSpeed;
#define kMMCTranSpeed20MHz  0x2A
#define kMMCTranSpeed26MHz  0x32
  UInt8   nsac;
  UInt8   taac;
  UInt8   reserved3 : 2;
  UInt8   specVersion : 4;
  UInt8   csdStructure : 2;
} MMCCSDRegister;

typedef enum : UInt8 {
  kMMCTimingSpeedDefault    = 0,
  kMMCTimingSpeedHighSpeed  = 1,
  kMMCTimingSpeedHS200      = 2,
  kMMCTimingSpeedHS400      = 3
} MMCTimingSpeed;

//
// MMC Extended CSD register struct.
// MMC v5.1 per spec JESD84-B51
//
typedef struct {
  //
  // Modes segment
  //
  UInt8   reserved1[15];
  UInt8   cmdqModeEnable;
  UInt8   secureRemovalType;
  UInt8   productStateAwarenessEnablement;
  UInt32  maxPreloadingDataSize;
  UInt32  preloadingDataSize;
  UInt8   ffuStatus;
  UInt8   reserved2[2];

  UInt8   modeOperationCodes;
  UInt8   modeConfig;
  UInt8   barrierControl;
  UInt8   flushCache;
  UInt8   cacheControl;
  UInt8   powerOffNotification;
  UInt8   packedCommandFailureIndex;
  UInt8   packetCommandStatus;
  UInt8   contextConfig[15];
  UInt16  extPartitionsAttribute;
  UInt16  exceptionEventsStatus;
  UInt16  exceptionEventsControl;
  UInt8   dyncapNeeded;
  UInt8   class6Control;
  UInt8   iniTimeoutEmulation;
  UInt8   dataSectorSize;
  UInt8   useNativeSectors;
  UInt8   nativeSectorSize;
  UInt8   vendorSpecific[64];
  UInt8   reserved3[2];

  UInt8   programCIDCSDDDRSupport;
  UInt8   periodicWakeup;
  UInt8   tCaseSupport;
  UInt8   productionStateAwareness;
  UInt8   secBadBlkManagement;
  UInt8   reserved4;

  UInt32  enhStartAddress;
  UInt8   enhSizeMult[3];
  UInt8   gpSizeMult[12];
  UInt8   partitionSettingCompleted;
  UInt8   partitionsAttribute;
  UInt8   maxEnhSizeMult[3];
  UInt8   partitioningSupport;
  UInt8   hpiManagement;
  UInt8   rstNFunction;
  UInt8   backgroundOpsEnable;
  UInt8   backgroundOpsStart;
  UInt8   sanitizeStart;
  UInt8   writeReliabilityParam;
  UInt8   writeReliabilitySetting;
  UInt8   rpmbSizeMult;
  UInt8   fwConfig;
  UInt8   reserved5;

  UInt8   userWriteProtection;
  UInt8   reserved6;
  UInt8   bootWriteProtection;
  UInt8   bootWriteProtectionStatus;
  UInt8   eraseGroupDef;
  UInt8   reserved8;
  UInt8   bootBusConditions;
  UInt8   bootConfigProtection;
  UInt8   partitionConfig;

  UInt8   reserved9;
  UInt8   erasedMemoryContent;
  UInt8   reserved10;
  UInt8   busWidth;
#define kMMCBusWidth1Bit    0
#define kMMCBusWidth4Bit    1
#define kMMCBusWidth8Bit    2
#define kMMCBusWidthDDR     4
#define kMMCBusWidth4BitDDR 5
#define kMMCBusWidth8BitDDR 6
  UInt8   strobeSupport;
  UInt8   hsTiming;
#define kMMCHSTimingDriverStrengthShift 4
#define kMMCHSTimingDriverStrengthMask  0xF0
  UInt8   reserved12;
  UInt8   powerClass;
  UInt8   reserved13;
  UInt8   cmdSetRevision;
  UInt8   reserved14;
  UInt8   cmdSet;

  //
  // Properties segment.
  //
  UInt8   extendedCSDRevision;
  UInt8   reserved15;
  UInt8   csdStructure;
  UInt8   reserved16;
  UInt8   deviceType;
#define kMMCDeviceTypeHighSpeed_26MHz                 BIT0
#define kMMCDeviceTypeHighSpeed_52MHz                 BIT1
#define kMMCDeviceTypeHighSpeed_DDR_52MHz_1_8V_3V     BIT2
#define kMMCDeviceTypeHighSpeed_DDR_52MHz_1_2V        BIT3
#define kMMCDeviceTypeHS200_SDR_1_8V                  BIT4
#define kMMCDeviceTypeHS200_SDR_1_2V                  BIT5
#define kMMCDeviceTypeHS400_DDR_1_8V                  BIT6
#define kMMCDeviceTypeHS400_DDR_1_2V                  BIT7

  UInt8   driverStrength;
  UInt8   outOfInterruptTime;
  UInt8   partitionSwitchTime;
  UInt8   powerCL_52_195;
  UInt8   powerCL_26_195;
  UInt8   powerCL_52_360;
  UInt8   powerCL_26_360;
  UInt8   reserved18;

  UInt8   minPerformanceRead_4_26;
  UInt8   minPerformanceWrite_4_26;
  UInt8   minPerformanceRead_8_26_4_52;
  UInt8   minPerformanceWrite_8_26_4_52;
  UInt8   minPerformanceRead_8_52;
  UInt8   minPerformanceWrite_8_52;
  UInt8   secureWriteProtectInfo;
  UInt32  sectorCount;
  UInt8   sleepNotificationTime;
  UInt8   sleepAwakeTimeout;
  UInt8   productionStateAwarenessTimeout;
  UInt8   sleepCurrentVCCQ;
  UInt8   sleepCurrentVCC;
  UInt8   highCapacityWriteProtectGroupSize;
  UInt8   reliableWriteSectorCount;
  UInt8   eraseTimeoutMult;
  UInt8   highCapacityEraseGroupSize;
  UInt8   accessSize;
  UInt8   bootPartitionSize;
  UInt8   reserved22;

  UInt8   bootInfo;
  UInt8   secureTRIMMultiplier;
  UInt8   secureEraseMultiplier;
  UInt8   secureFeatureSupport;
  UInt8   trimMultiplier;
  UInt8   reserved23;

  UInt8   minPerformanceDDRRead_8_52;
  UInt8   minPerformanceDDRWrite_8_52;
  UInt8   powerCL_200_130;
  UInt8   powerCL_200_195;
  UInt8   powerCL_DDR_52_195;
  UInt8   powerCL_DDR_52_360;
  UInt8   cacheFlushPolicy;
  UInt8   iniTimeoutAP;
  UInt32  correctlyProgrammedSectorsCount;
  UInt8   bkOpsStatus;
  UInt8   powerOffLongTimeout;
  UInt8   genericCMD6Timeout;
  UInt32  cacheSize;
  UInt8   powerCL_DDR_200_360;
  UInt64  firmwareVersion;
  UInt16  deviceVersion;
  UInt8   optimalTrimUnitSize;
  UInt8   optimalWriteSize;
  UInt8   optimalReadSize;
  UInt8   preEOLInfo;
  UInt8   deviceLifetimeEstimationTypeA;
  UInt8   deviceLifetimeEstimationTypeB;
  UInt8   vendorHealthReport[32];
  UInt32  numFWSectorsCorrectlyProgrammed;
  UInt8   reserved24;

  UInt8   cmdqDepth;
  UInt8   cmdqSupport;
  UInt8   reserved25[177];

  UInt8   barrierSupport;
  UInt32  ffuArgument;
  UInt8   operationCodeTimeout;
  UInt8   ffuFeatures;
  UInt8   supportedModes;
  UInt8   extSupport;
  UInt8   largeUnitSizeM1;
  UInt8   contextCapabilities;
  UInt8   tagResourcesSize;
  UInt8   tagUnitSize;
  UInt8   dataTagSupport;
  UInt8   maxPackedWrites;
  UInt8   maxPackedReads;
  UInt8   bkOpsSupport;
  UInt8   hpiFeatures;
  UInt8   supportedCommandSets;
  UInt8   extSecurityError;
  UInt8   reserved26[6];
} MMCExtendedCSDRegister;

#pragma pack(pop)

#endif

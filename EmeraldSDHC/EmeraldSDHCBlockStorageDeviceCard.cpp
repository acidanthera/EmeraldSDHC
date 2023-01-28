//
//  EmeraldSDHCBlockStorageDeviceCard.cpp
//  EmeraldSDHC card slot IOBlockStorageDevice implementation
//
//  Card functions
//
//  Copyright © 2021-2023 Goldfish64. All rights reserved.
//

#include "EmeraldSDHCBlockStorageDevice.hpp"
#include <IOKit/storage/IOBlockStorageDriver.h>

//
// Common SD and MMC vendors.
// Taken from https://git.kernel.org/pub/scm/linux/kernel/git/cjb/mmc-utils-old.git/tree/lsmmc.c
//
static const SDAVendor SDVendors[] = {
  { 0x01, "Panasonic" },
  { 0x02, "Toshiba/Kingston/Viking" },
  { 0x03, "SanDisk" },
  { 0x08, "Silicon Power" },
  { 0x18, "Infineon" },
  { 0x1B, "Transcend" },
  { 0x1C, "Transcend" },
  { 0x1D, "Corsair" },
  { 0x1E, "Transcend" },
  { 0x1F, "Kingston" },
  { 0x28, "Lexar" },
  { 0x30, "SanDisk" },
  { 0x33, "STMicroelectronics" },
  { 0x41, "Kingston" },
  { 0x6F, "STMicroelectronics" }
};

static const SDAVendor MMCVendors[] = {
  { 0x00, "SanDisk" },
  { 0x02, "Kingston/SanDisk" },
  { 0x03, "Toshiba" },
  { 0x11, "Toshiba" },
  { 0x13, "Micron" },
  { 0x15, "Samsung/SanDisk/LG" },
  { 0x37, "KingMax" },
  { 0x44, "SanDisk" },
  { 0x2C, "Kingston" },
  { 0x70, "Kingston" },
  { 0x90, "Hynix" }
};

void EmeraldSDHCBlockStorageDevice::handleCardChange() {
  if (_cardSlot->isCardPresent() == _isCardInserted) {
    EMDBGLOG("Card insertion/removal event raised, but state did not change");
    return;
  }

  //
  // Handle card insertion/removal.
  //
  if (_cardSlot->isCardPresent()) {
    EMDBGLOG("Card was inserted");

    initController();
    initCard();
    messageClients(kIOMessageMediaStateHasChanged, reinterpret_cast<void*>(kIOMediaStateOnline), 0);
  } else {
    EMDBGLOG("Card was removed");

    initController();
    initCard();
    messageClients(kIOMessageMediaStateHasChanged, reinterpret_cast<void*>(kIOMediaStateOffline), 0);
  }
}

bool EmeraldSDHCBlockStorageDevice::resetCard() {
  IOReturn status;
  SDACommandResponse resp;
  
  //
  // Assume card installed is a v2 SD card.
  // The first reset command is identical for all types.
  //
  _cardType = kSDACardTypeSD_200;

  //
  // Tell all cards to go to idle state.
  //
  status = doSyncCommand(kSDCommandGoIdleState, 0, kSDATimeout_2sec);
  if (status != kIOReturnSuccess) {
    return false;
  }
  EMDBGLOG("Card has been reset and should be in IDLE status");

  //
  // Issue SEND_IF_COND to card.
  // If no response, this is either is SD 1.0 or MMC.
  //
  status = doSyncCommand(kSDCommandSendIfCond, 0x1AA, kSDATimeout_2sec);
  if (status == kIOReturnTimeout) {
    EMDBGLOG("Card did not respond to SEND_IF_COND, not an SD 2.00 card");
    _cardType = kSDACardTypeSD_Legacy;
  } else if (status != kIOReturnSuccess) {
    return false;
  }

  //
  // Issue SD card initialization command.
  //
  EMDBGLOG("Initializing %s card", _cardType == kSDACardTypeSD_Legacy ? "MMC or legacy SD" : "SD 2.00");
  for (int i = 0; i < 20; i++) {
    status = doSyncCommand(kSDAppCommandSendOpCond, kSDAOCRInitValue, kSDATimeout_2sec, &resp);

    //
    // No response indicates an MMC card.
    //
    if (status == kIOReturnTimeout && _cardType == kSDACardTypeSD_Legacy) {
      EMDBGLOG("Card did not respond to SEND_OP_COND, not an SD card");
      _cardType = kSDACardTypeMMC;
      break;
    } else if (status != kIOReturnSuccess) {
      return false;
    }

    if (resp.bytes4[0] & kSDAOCRCardBusy) {
      break;
    }

    //
    // Spec indicates to wait 1sec between attempts.
    //
    IOSleep(1000);
  }
  
  //
  // TODO: SD card not supported.
  //
  if (isSDCard()) {
    EMSYSLOG("SD cards currently unsupported");
    return false;
  }

  //
  // Check if SD card has started.
  //
  if (isSDCard()) {
    //
    // If card is still not ready, abort.
    //
    if (!(resp.bytes4[0] & kSDAOCRCardBusy)) {
      EMSYSLOG("Timed out initializing card");
      return false;
    }
    _isCardHighCapacity = resp.bytes4[0] & kSDAOCRCCSHighCapacity;

  //
  // Reset and start MMC card.
  //
  } else {
    status = doSyncCommand(kSDCommandGoIdleState, 0, kSDATimeout_10sec);
    if (status != kIOReturnSuccess) {
      return false;
    }
    EMDBGLOG("Card has been reset again and should be in IDLE status");

    //
    // Send OCR to cards until card startup has completed and bit is set.
    //
    do {
      status = doSyncCommand(kMMCCommandSendOpCond, 0x00FF8000 | 0x40000000, kSDATimeout_10sec, &resp); // TODO: fix values here
      if (status != kIOReturnSuccess) {
        return false;
      }
    } while (!(resp.bytes4[0] & BIT31));
  }

  EMDBGLOG("Card type %u has been reset successfully and should now be in READY status", _cardType);
  return true;
}

bool EmeraldSDHCBlockStorageDevice::probeCard() {
  bool cardFound = false;
  UInt16 cardId = kSDACardAddress;
  SDACommandResponse cidResponse;
  SDACommandResponse rcaResponse;
  UInt8 vendorId;

  // Instruct all cards to send over CID.
  // Command will time out after all cards have been identified.
  //
  // Only handle the first card that responds, other cards will be disabled.
  while (doSyncCommand(isSDCard() ? (UInt32) kSDCommandAllSendCID : (UInt32) kMMCCommandAllSendCID, 0, kSDATimeout_10sec, &cidResponse) == kIOReturnSuccess) {
    if (isSDCard()) {
      //
      // Get SD card ID.
      //
      doSyncCommand(kSDCommandSendRelativeAddress, 0, kSDATimeout_10sec, &rcaResponse);
      cardId = rcaResponse.bytes4[0] >> kSDARelativeAddressShift;
    } else {
      //
      // Set MMC card ID.
      //
      doSyncCommand(kMMCCommandSetRelativeAddress, cardId << kSDARelativeAddressShift, kSDATimeout_10sec);
    }
    EMDBGLOG("Card @ 0x%X has CID of 0x%8llX%8llX", cardId, cidResponse.bytes8[1], cidResponse.bytes8[0]);

    //
    // Save and parse returned CID.
    //
    if (!cardFound) {
      _cardAddress = cardId;
      memcpy(&_cardCID, cidResponse.bytes, sizeof (_cardCID));

      //
      // Get card product name, SN, and revision strings from CID.
      //
      if (isSDCard()) {
        _cardProductName[0] = (char)_cardCID.sd.name[4];
        _cardProductName[1] = (char)_cardCID.sd.name[3];
        _cardProductName[2] = (char)_cardCID.sd.name[2];
        _cardProductName[3] = (char)_cardCID.sd.name[1];
        _cardProductName[4] = (char)_cardCID.sd.name[0];
        _cardProductName[5] = '\0';
        _cardProductName[6] = '\0';

        vendorId = _cardCID.sd.manufacturerId;
        snprintf(_cardSN, sizeof (_cardSN), "%u", _cardCID.sd.serialNumber);
        snprintf(_cardRev, sizeof (_cardRev), "%u", _cardCID.sd.revision);
        EMDBGLOG("Mfg Date: %u/%u, SN: %s, Rev: %s, OEM ID: 0x%X, Mfg ID: 0x%X",
                 _cardCID.sd.manufactureMonth, _cardCID.sd.manufactureYear, _cardSN,
                 _cardRev, _cardCID.sd.oemId, _cardCID.sd.manufacturerId);
      } else {
        _cardProductName[0] = (char)_cardCID.mmc.name[5];
        _cardProductName[1] = (char)_cardCID.mmc.name[4];
        _cardProductName[2] = (char)_cardCID.mmc.name[3];
        _cardProductName[3] = (char)_cardCID.mmc.name[2];
        _cardProductName[4] = (char)_cardCID.mmc.name[1];
        _cardProductName[5] = (char)_cardCID.mmc.name[0];
        _cardProductName[6] = '\0';

        vendorId = _cardCID.mmc.manufacturerId;
        snprintf(_cardSN, sizeof (_cardSN), "%u", _cardCID.mmc.serialNumber);
        snprintf(_cardRev, sizeof (_cardRev), "%u", _cardCID.mmc.revision);
        EMDBGLOG("Mfg Date: 0x%X, SN: %s, Rev: %s, OEM ID: 0x%X, Mfg ID: 0x%X, CBX: 0x%X",
                 _cardCID.mmc.manufactureDate, _cardSN, _cardRev,
                 _cardCID.mmc.oemId, _cardCID.mmc.manufacturerId, _cardCID.mmc.cbx);
      }

      //
      // Lookup vendor string from card vendor ID.
      //
      _cardVendorName = "Generic";
      if (isSDCard()) {
        for (int i = 0; i < (sizeof (SDVendors) / sizeof (SDVendors[0])); i++) {
          if (SDVendors[i].manufacturerId == vendorId) {
            _cardVendorName = SDVendors[i].name;
            break;
          }
        }
      } else {
        for (int i = 0; i < (sizeof (MMCVendors) / sizeof (MMCVendors[0])); i++) {
          if (MMCVendors[i].manufacturerId == vendorId) {
            _cardVendorName = MMCVendors[i].name;
            break;
          }
        }
      }

      //
      // If MMC CID specifies BGA, this is eMMC and is a non-removable, embedded card.
      //
      _isCardEmbedded = !isSDCard() && _cardCID.mmc.cbx == kMMCCID_CBXEmbedded;
      EMDBGLOG("Found %s card (embedded: %u) %s %s", isSDCard() ? "SD" : "MMC",
               _isCardEmbedded, _cardVendorName, _cardProductName);
      cardFound = true;
    }

    cardId++;
    break;
  }

  // Tell other cards on bus to go inactive, this driver only supports one card per bus.
 /* if (cardId > cardAddress + 1) {
    DBGLG("Identified %u cards on bus, disabling extra cards", cardId + 1);
    for (int i = kSDACardAddress + 1; i <= cardId; i++) {
      doCommand(kMMCCommandGoInactiveState, i << kSDARelativeAddressShift);
    }
  }*/

  if (!cardFound) {
    return false;
  }

  //
  // Get card CSD structure.
  //
  if (!parseCSD()) {
    return false;
  }

  //
  // Get extended CSD structure for MMC cards, if supported.
  //
  if (!isSDCard() && isExtendedCSDSupported()) {
    if (!parseMMCExtendedCSD()) {
      return false;
    }
  }

  return true;
}

bool EmeraldSDHCBlockStorageDevice::parseCSD() {
  //
  // Get CSD structure from card.
  //
  SDACommandResponse *csdResponse = (SDACommandResponse *) &_cardCSD;
  if (doSyncCommand(isSDCard() ? (UInt32) kSDCommandSendCSD : (UInt32) kMMCCommandSendCSD,
                    _cardAddress << kSDARelativeAddressShift, kSDATimeout_10sec, csdResponse) != kIOReturnSuccess) {
    return false;
  }
  EMDBGLOG("CSD value: 0x%8llX%8llX", csdResponse->bytes8[1], csdResponse->bytes8[0]);

  //
  // Calculate card size.
  //
  if (isSDCard()) {
    EMDBGLOG("CSD struct version: 0x%X", _cardCSD.sd1.csdStructure);
    EMDBGLOG("CSD supported classes: 0x%X", _cardCSD.sd1.ccc);
    EMDBGLOG("CSD max clock rate: 0x%X", _cardCSD.sd1.tranSpeed);

    //
    // Calculate SD block size in bytes and blocks.
    //
    UInt64 cardBlockBytes;
    if (_cardCSD.sd1.csdStructure == kSDCSDVersion2_0) {
      cardBlockBytes = ((UInt64)_cardCSD.sd2.cSize + 1) * (512 * kByte);
    } else if (_cardCSD.sd1.csdStructure == kSDCSDVersion3_0) {
      cardBlockBytes = ((UInt64)_cardCSD.sd3.cSize + 1) * (512 * kByte);
    } else { // Version 1.0
      cardBlockBytes = ((_cardCSD.sd1.cSize + 1) * calcPower(_cardCSD.sd1.cSizeMultiplier + 2)) * calcPower(_cardCSD.sd1.readBLLength);
    }
    _cardBlockCount = (UInt32)(cardBlockBytes / kSDABlockSize);
    EMDBGLOG("Block count: %u (%llu bytes), high capacity: %u", _cardBlockCount, cardBlockBytes, _isCardHighCapacity);

  } else { // TODO: handle small MMC cards.
    //
    // Calculate MMC block size in bytes and blocks.
    //
    UInt32 cardBlockBytes = ((_cardCSD.mmc.cSize + 1) * calcPower(_cardCSD.mmc.cSizeMultiplier + 2)) * calcPower(_cardCSD.mmc.readBLLength);
    _cardBlockCount = cardBlockBytes / kSDABlockSize;
    EMDBGLOG("CSD struct version: 0x%X, spec version: 0x%X", _cardCSD.mmc.csdStructure, _cardCSD.mmc.specVersion);
    EMDBGLOG("CSD CSIZE: 0x%X, CCC: 0x%X", _cardCSD.mmc.cSize, _cardCSD.mmc.ccc);
    EMDBGLOG("CSD max clock rate: 0x%X", _cardCSD.mmc.tranSpeed);
    EMDBGLOG("Block count: %u (%llu bytes), high capacity: %u", _cardBlockCount, cardBlockBytes, _isCardHighCapacity);

    //
    // Calculate max clock speed for standard mode.
    //
    if (_cardCSD.mmc.tranSpeed == kMMCTranSpeed20MHz) {
      _mmcMaxStandardClock = kSDANormalSpeedClock20MHz;
    } else if (_cardCSD.mmc.tranSpeed == kMMCTranSpeed26MHz) {
      _mmcMaxStandardClock = kSDANormalSpeedClock26MHz;
    } else {
      _mmcMaxStandardClock = kSDANormalSpeedClock20MHz; // TODO:
    }
    EMDBGLOG("MMC maximum clock speed is %u Hz", _mmcMaxStandardClock);
  }

  return true;
}

bool EmeraldSDHCBlockStorageDevice::isExtendedCSDSupported() {
  //
  // Extended CSD is only supported on 4.x and newer MMC specifications.
  //
  return !isSDCard() && _cardCSD.mmc.specVersion > kMMCSpecVersion3_x;
}

bool EmeraldSDHCBlockStorageDevice::parseMMCExtendedCSD() {
  //
  // Get extended CSD structure from MMC card.
  //
  IOMemoryDescriptor *memDescriptor = IOMemoryDescriptor::withAddress(&_mmcExtendedCSD, sizeof (_mmcExtendedCSD), kIODirectionIn);
  if (memDescriptor == nullptr) {
    return false;
  }
  memDescriptor->prepare();

  IOReturn status = doSyncCommandWithData(kMMCCommandSendExtCSD, 0, kSDATimeout_10sec, 1, sizeof (_mmcExtendedCSD), memDescriptor, 0);
  memDescriptor->complete();
  memDescriptor->release();

  if (status != kIOReturnSuccess) {
    EMSYSLOG("Failed to get MMC extended CSD with status 0x%X", status);
    return false;
  }

  EMDBGLOG("MMC Extended CSD properties:");
  EMDBGLOG("Supported command sets: 0x%X, HPI features 0x%X", _mmcExtendedCSD.supportedCommandSets, _mmcExtendedCSD.hpiFeatures);
  EMDBGLOG("Wear level type A: 0x%X, type B: 0x%X", _mmcExtendedCSD.deviceLifetimeEstimationTypeA, _mmcExtendedCSD.deviceLifetimeEstimationTypeB);
  EMDBGLOG("Device version: 0x%X, firmware version: 0%llX", _mmcExtendedCSD.deviceVersion, _mmcExtendedCSD.firmwareVersion);
  EMDBGLOG("Sector count %u (%llu bytes total)", _mmcExtendedCSD.sectorCount, (UInt64)_mmcExtendedCSD.sectorCount * 512);

  EMDBGLOG("Supported power classes (200 MHz DDR, 3.6V): 0x%X", _mmcExtendedCSD.powerCL_DDR_200_360);
  EMDBGLOG("Supported power classes (200 MHz, 1.95V): 0x%X", _mmcExtendedCSD.powerCL_200_195);
  EMDBGLOG("Supported power classes (200 MHz, 1.3V): 0x%X", _mmcExtendedCSD.powerCL_200_130);
  EMDBGLOG("Supported power classes (52 MHz DDR, 3.6V): 0x%X", _mmcExtendedCSD.powerCL_DDR_52_360);
  EMDBGLOG("Supported power classes (52 MHz DDR, 1.95V): 0x%X", _mmcExtendedCSD.powerCL_DDR_52_195);
  EMDBGLOG("Supported power classes (52 MHz, 3.6V): 0x%X", _mmcExtendedCSD.powerCL_52_360);
  EMDBGLOG("Supported power classes (52 MHz, 1.95V): 0x%X", _mmcExtendedCSD.powerCL_52_195);
  EMDBGLOG("Supported power classes (26 MHz, 3.6V): 0x%X", _mmcExtendedCSD.powerCL_26_360);
  EMDBGLOG("Supported power classes (26 MHz, 1.95V): 0x%X", _mmcExtendedCSD.powerCL_26_195);
  EMDBGLOG("Current power class: 0x%X", _mmcExtendedCSD.powerClass);

  EMDBGLOG("Minimum performance (8bit 52 MHz DDR) read: 0x%X, write: 0x%X",
           _mmcExtendedCSD.minPerformanceDDRRead_8_52, _mmcExtendedCSD.minPerformanceDDRWrite_8_52);
  EMDBGLOG("Minimum performance (8bit 52 MHz) read: 0x%X, write: 0x%X",
           _mmcExtendedCSD.minPerformanceRead_8_52, _mmcExtendedCSD.minPerformanceWrite_8_52);
  EMDBGLOG("Minimum performance (8bit 26 MHz, 4bit 52 MHz) read: 0x%X, write: 0x%X",
           _mmcExtendedCSD.minPerformanceRead_8_26_4_52, _mmcExtendedCSD.minPerformanceWrite_8_26_4_52);
  EMDBGLOG("Minimum performance (4bit 26 MHz) read: 0x%X, write: 0x%X",
           _mmcExtendedCSD.minPerformanceRead_4_26, _mmcExtendedCSD.minPerformanceWrite_4_26);

  _isCardHighCapacity = true;
  _cardBlockCount = _mmcExtendedCSD.sectorCount;

  EMDBGLOG("Card type: 0x%X, driver strengths: 0x%X", _mmcExtendedCSD.deviceType, _mmcExtendedCSD.driverStrength);
  EMDBGLOG("CSD version: 0x%X, ExtCSD version: 0x%X", _mmcExtendedCSD.csdStructure, _mmcExtendedCSD.extendedCSDRevision);
  EMDBGLOG("Block count: %u (%llu bytes), high capacity: %u", _cardBlockCount, (UInt64)_cardBlockCount * kSDABlockSize, _isCardHighCapacity);
  return true;
}

bool EmeraldSDHCBlockStorageDevice::parseSDSCR() {
  //
  // Get SCR from SD card.
  //
  IOMemoryDescriptor *memDescriptor = IOMemoryDescriptor::withAddress(&_sdSCR, sizeof (_sdSCR), kIODirectionIn);
  if (memDescriptor == nullptr) {
    return false;
  }
  memDescriptor->prepare();

 /* if (prepareCommandDataTransfer(memDescriptor, &blockBuffer) != kIOReturnSuccess) {
    memDescriptor->complete();
    memDescriptor->release();
    return false;
  }*/
 // blockBuffer.blockCount = 1;
//  blockBuffer.blockSize = sizeof (_sdSCR);

  //IOReturn status = doCommandWithData(kSDAppCommandSendSCR, 0, &blockBuffer);
  //completeCommandDataTransfer(&blockBuffer);
  memDescriptor->complete();
  memDescriptor->release();
  //if (status != kIOReturnSuccess) {
  //  return false;
 // }

  EMDBGLOG("SD SCR version: 0x%X, spec: 0x%X, spec3: 0x%X, spec4: 0x%X, specX: 0x%X", _sdSCR.scrStructure,
           _sdSCR.sdSpec, _sdSCR.sdSpec3, _sdSCR.sdSpec4, _sdSCR.sdSpecX);
  EMDBGLOG("SD bus widths supported: 0x%X", _sdSCR.sdBusWidths);
  return true;
}

bool EmeraldSDHCBlockStorageDevice::setCardBusWidth(SDABusWidth busWidth, bool doubleDataRate) {
  UInt8 busWidthBits;
  if (isSDCard()) {
    //
    // Set SD bus width.
    //
    if (busWidth == kSDABusWidth1) {
      busWidthBits = kSDBusWidth1Bit;
    } else if (busWidth == kSDABusWidth4) {
      busWidthBits = kSDBusWidth4Bit;
    } else {
      return false;
    }

    IOReturn status = doSyncCommand(kSDAppCommandSetBusWidth, busWidthBits, kSDATimeout_10sec);
    if (status != kIOReturnSuccess) {
      return false;
    }

  } else {
    //
    // Set MMC bus width.
    //
    if (busWidth == kSDABusWidth1) {
      busWidthBits = kMMCBusWidth1Bit;
    } else if (busWidth == kSDABusWidth4) {
      busWidthBits = kMMCBusWidth4Bit;
    } else if (busWidth == kSDABusWidth8) {
      busWidthBits = kMMCBusWidth8Bit;
    } else {
      return false;
    }
    if (doubleDataRate) {
      busWidthBits |= kMMCBusWidthDDR;
    }

    EMDBGLOG("Setting MMC bus width to 0x%X", busWidthBits);
    if (!switchMMCExtendedCSD (kMMCSwitchAccessWriteByte, __offsetof(MMCExtendedCSDRegister, busWidth), busWidthBits)) {
      EMSYSLOG("Failed to set MMC CSD");
      return false;
    }
  }

  //
  // Controller bus width needs to also be set to match card.
  //
  _cardSlot->setControllerBusWidth(busWidth);
  IOSleep(50);
  return true;
}

bool EmeraldSDHCBlockStorageDevice::switchMMCExtendedCSD(MMCSwitchAccessBits access, UInt8 index, UInt8 value) {
  // [31:26] Set to 0 [25:24] Access [23:16] Index [15:8] Value [7:3] Set to 0 [2:0] Cmd Set
  UInt32 arg = ((access << kMMCSwitchAccessShift) & kMMCSwitchAccessMask)
    | ((index << kMMCSwitchIndexShift) & kMMCSwitchIndexMask)
    | ((value << kMMCSwitchValueShift) & kMMCSwitchValueMask);
  return doSyncCommand(kMMCCommandSwitch, arg, kSDATimeout_10sec) == kIOReturnSuccess;
}

bool EmeraldSDHCBlockStorageDevice::switchMMCSpeed() {
  SDABusWidth busWidth;
  MMCTimingSpeed speed;
  
  //
  // Set speed to max supported.
  //
  if (_mmcExtendedCSD.deviceType & (kMMCDeviceTypeHS400_DDR_1_2V | kMMCDeviceTypeHS400_DDR_1_8V)) {
    busWidth = kSDABusWidth8;
    speed = kMMCTimingSpeedHS200;
  } else if (_mmcExtendedCSD.deviceType & (kMMCDeviceTypeHS200_SDR_1_2V | kMMCDeviceTypeHS200_SDR_1_8V)) {
    busWidth = kSDABusWidth8;
    speed = kMMCTimingSpeedHS200;
  } else if (_mmcExtendedCSD.deviceType & (kMMCDeviceTypeHighSpeed_DDR_52MHz_1_2V | kMMCDeviceTypeHighSpeed_DDR_52MHz_1_8V_3V)) {
    
  }
  
  setCardBusWidth(kSDABusWidth8, false);
  //setControllerClock(26 * MHz);

  setMMCSpeed(kMMCTimingSpeedHS200);
  _cardSlot->setControllerClock(200 * MHz);
  
  tuneCard(kSDABusWidth8);
  //IOSleep(1000);
  
 // parseCSD();
 // IOSleep(1000);
  
  UInt64 hostCaps = _cardSlot->getControllerCapabilities();
  EMDBGLOG("Host controller slot capabilities: 0x%llX", hostCaps);

  //
  // Determine DMA support level.
  //
  if (hostCaps & kSDHCRegCapabilitiesADMA2Supported) {
    EMDBGLOG("ADMA2 supported");
    _hcTransferType = kSDATransferTypeADMA2;
  } else if (hostCaps & kSDHCRegCapabilitiesSDMASupported) {
    EMDBGLOG("SDMA supported");
    _hcTransferType = kSDATransferTypeSDMA;
  } else {
    EMDBGLOG("No DMA support, using PIO");
    _hcTransferType = kSDATransferTypePIO;
  }
  EMDBGLOG("No DMA support, using PIO");
 //_hcTransferType = kSDATransferTypeSDMA;
  _cardSlot->setControllerDMAMode(_hcTransferType);
  
  memset(&_mmcExtendedCSD, 0, sizeof (_mmcExtendedCSD));
  parseMMCExtendedCSD();
  
 /* setControllerClock(52000000);
  setMMCSpeed(kMMCTimingSpeedHighSpeed);
  
  setCardBusWidth(kSDABusWidth8, true);
  
  memset(&_mmcExtendedCSD, 0, sizeof (_mmcExtendedCSD));
  parseMMCExtendedCSD();
  
  setMMCSpeed(kMMCTimingSpeedHS400);
  setControllerClock(200 * MHz);*/
  
  memset(&_mmcExtendedCSD, 0, sizeof (_mmcExtendedCSD));
  parseMMCExtendedCSD();
  
  return true;
}

bool EmeraldSDHCBlockStorageDevice::tuneCard(SDABusWidth busWidth) {
  bool tuningComplete = true;

  UInt16 bytesLength = 0;
  if (busWidth == kSDABusWidth4) {
    bytesLength = kSDATuneBytes4Bits;
  } else if (busWidth == kSDABusWidth8) {
    bytesLength = kSDATuneBytes8Bits;
  } else {
    return false;
  }

  //
  // Setup transfer block for incoming tuning bytes.
  //
  IOBufferMemoryDescriptor *bufDescriptor = IOBufferMemoryDescriptor::withCapacity(bytesLength, kIODirectionIn);
  if (bufDescriptor == nullptr) {
    return false;
  }
  bufDescriptor->prepare();

  //
  // Perform tuning between host controller and card.
  //
  EMDBGLOG("Starting tuning of card using %u bytes", bytesLength);
  _cardSlot->writeReg16(kSDHCRegHostControl2, _cardSlot->readReg16(kSDHCRegHostControl2) | kSDHCRegHostControl2ExecuteTuning);
  while (_cardSlot->readReg16(kSDHCRegHostControl2) & kSDHCRegHostControl2ExecuteTuning) {
    if (doSyncCommandWithData(kMMCCommandSendTuningBlock, 0, kSDATimeout_10sec, 1, bytesLength, bufDescriptor, 0) != kIOReturnSuccess) {
      tuningComplete = false;
      break;
    }
  }
  EMDBGLOG("Tuning complete: %u", tuningComplete); // TODO: handle tuning error interrupts?

  bufDescriptor->complete();
  bufDescriptor->release();

  return tuningComplete;
}

bool EmeraldSDHCBlockStorageDevice::setMMCSpeed(MMCTimingSpeed speed) {
  //
  // Change card mode.
  //
  EMDBGLOG("Host controller slot capabilities: 0x%llX", _cardSlot->readReg64(kSDHCRegCapabilities));
  EMDBGLOG("Setting MMC speed to 0x%X", speed);
  if (!switchMMCExtendedCSD (kMMCSwitchAccessWriteByte, __offsetof(MMCExtendedCSDRegister, hsTiming), (1 << kMMCHSTimingDriverStrengthShift) | speed)) { // TODO: Driver strength defaults to B in the host controller, setting that here for now.
    return false;
  }

  //
  // Set high speed bit only if in high speed mode.
  //
  UInt16 hcControl1 = _cardSlot->readReg16(kSDHCRegHostControl1);
  if (speed != kMMCTimingSpeedDefault) {
    hcControl1 |= kSDHCRegHostControl1HighSpeedEnable;
  } else {
    hcControl1 &= ~kSDHCRegHostControl1HighSpeedEnable;
  }
  _cardSlot->writeReg16(kSDHCRegHostControl1, hcControl1);

  //
  // Set host control 2 register for HS200 and HS400 modes.
  //
  UInt16 hcControl2 = _cardSlot->readReg16(kSDHCRegHostControl2);
  hcControl2 &= ~(kSDHCRegHostControl21_8VSignaling | kSDHCRegHostControl2UHS_Mask);
  if (speed == kMMCTimingSpeedHS200) {
    hcControl2 |= kSDHCRegHostControl21_8VSignaling | kSDHCRegHostControl2UHS_SDR104;
  } else if (speed == kMMCTimingSpeedHS400) {
    hcControl2 |= kSDHCRegHostControl21_8VSignaling | kSDHCRegHostControl2UHS_HS400;
    // TODO: Support HS400 on the host controller side.
  } else {
   // hcControl2 &= ~(kSDHCRegHostControl21_8VSignaling | kSDHCRegHostControl2UHS_Mask);
  }
  _cardSlot->writeReg16(kSDHCRegHostControl2, hcControl2);
  IOSleep(50);
  EMDBGLOG("HC set to 1:0x%X 2:0x%X", _cardSlot->readReg16(kSDHCRegHostControl1), _cardSlot->readReg16(kSDHCRegHostControl2));

  return true;
}

bool EmeraldSDHCBlockStorageDevice::initCard() {
  IOReturn status;
  
  //
  // Check if card is present.
  //
  if (!_cardSlot->isCardPresent()) {
    EMDBGLOG("No card is currently inserted");
    _isCardInserted = false;
    return false;
  }
  _isCardInserted = true;

  //
  // Reset to initialization clock and power on the card.
  //
  if (!_cardSlot->setControllerClock(kSDAInitSpeedClock400kHz)) {
    return false;
  }
  _cardSlot->setControllerPower(true);

  //
  // Reset and probe the card.
  //
  if (!resetCard() || !probeCard()) {
    EMSYSLOG("Failed to initialize card");
    _cardSlot->setControllerPower(false);
    _cardSlot->setControllerClock(0);
    return false;
  }

  //
  // Change clock to run at normal speed.
  //
  if (isSDCard()) {
    
  } else {
    _cardSlot->setControllerClock(_mmcMaxStandardClock);
  }

  //
  // Set standard block length.
  //
  status = doSyncCommand(isSDCard() ? (UInt32) kSDCommandSetBlockLength : (UInt32) kMMCCommandSetBlockLength, kSDABlockSize, kSDATimeout_10sec);
  if (status != kIOReturnSuccess) {
    EMSYSLOG("Failed to set block length with status 0x%X", status);
    _cardSlot->setControllerPower(false);
    _cardSlot->setControllerClock(0);
    return false;
  }

  //
  // Change to higher speed if supported.
  //
  if (isSDCard()) {
    
  } else {
    if (isExtendedCSDSupported()) {
      switchMMCSpeed();
    }
  }

  EMDBGLOG("DAT signal %X", _cardSlot->readReg32(kSDHCRegPresentState));
  return true;
}

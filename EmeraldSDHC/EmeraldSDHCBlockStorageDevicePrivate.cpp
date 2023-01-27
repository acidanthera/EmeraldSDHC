//
//  EmeraldSDHCBlockStorageDevicePrivate.cpp
//  EmeraldSDHC card slot IOBlockStorageDevice implementation
//
//  Misc private functions
//
//  Copyright © 2021-2023 Goldfish64. All rights reserved.
//

#include "EmeraldSDHCBlockStorageDevice.hpp"

#include <IOKit/IOKitKeys.h>
#include <IOKit/scsi/IOSCSIProtocolInterface.h>
#include <IOKit/storage/IOBlockStorageDriver.h>

void EmeraldSDHCBlockStorageDevice::handleInterrupt() {
  UInt16 intStatus = _cardSlot->readReg16(kSDHCRegNormalIntStatus);

  EMIODBGLOG("Interrupt (slot bits 0x%X)! 0x%X", _cardSlot->readReg16(kSDHCRegHostControllerSlotIntStatus), intStatus);
  _cardSlot->writeReg16(kSDHCRegNormalIntStatus, intStatus);

  //
  // Cancel I/O operations for card removal.
  //
  if (intStatus & kSDHCRegNormalIntStatusCardRemoval) {
    if (_currentCommand != nullptr) {
      _currentCommand->result = kIOReturnAborted;
      _currentCommand->state  = kEmeraldSDHCStateComplete;
    }
    flushCommandQueue();
  }

  //
  // Perform async I/O.
  //
  doAsyncIO(intStatus);

  //
  // Handle card insertion/removal on a separate thread.
  // Directly invoking from the interrupt handler will result in a deadlock.
  //
  if (intStatus & (kSDHCRegNormalIntStatusCardInsertion | kSDHCRegNormalIntStatusCardRemoval)) {
    thread_call_enter(_cardChangeThread);
  }
}

void EmeraldSDHCBlockStorageDevice::handleIOTimeout(IOTimerEventSource *sender) {
  EMDBGLOG("Timeout! error bits 0x%X", _cardSlot->readReg16(kSDHCRegErrorIntStatus));
  _cardSlot->resetController(kSDHCRegSoftwareResetCmd);
  _cardSlot->resetController(kSDHCRegSoftwareResetDat);
  _currentCommand->result = kIOReturnTimeout;
  _currentCommand->state = kEmeraldSDHCStateComplete;
  doAsyncIO();
}

bool EmeraldSDHCBlockStorageDevice::initController() {
  EMDBGLOG("Initializing SD host controller version %s", _cardSlot->getControllerVersionString());

  //
  // Completely reset controller.
  //
  if (!_cardSlot->resetController(kSDHCRegSoftwareResetAll)) {
    EMSYSLOG("Failed to reset controller");
    return false;
  }

  //
  // Set controller parameters.
  //
  _isCardSelected = false;

  _cardSlot->writeReg8(kSDHCRegTimeoutControl, 0xE);
  _cardSlot->writeReg16(kSDHCRegNormalIntStatusEnable, -1);
  _cardSlot->writeReg16(kSDHCRegErrorIntStatusEnable, -1);
  _cardSlot->writeReg16(kSDHCRegNormalIntSignalEnable, kSDHCRegNormalIntStatusCommandComplete | kSDHCRegNormalIntStatusTransferComplete
                        | kSDHCRegNormalIntStatusDMAInterrupt | kSDHCRegNormalIntStatusBufferWriteReady | kSDHCRegNormalIntStatusBufferReadReady);
  _cardSlot->setControllerInsertionEvents(true);
  
  return true;
}

void EmeraldSDHCBlockStorageDevice::setStorageProperties() {
  //
  // Set storage properties.
  //
  // eMMC should always be present at startup and
  //   should never disappear during system operation.
  //
  setProperty(kSDAEmbeddedSlotKey, _isCardEmbedded);

  //
  // Build Protocol Characteristics dictionary.
  //
  OSDictionary *protoDict = OSDictionary::withCapacity(2);
  if (protoDict == nullptr) {
    return;
  }

  OSString *physInterconnectString = OSString::withCString(kIOPropertyPhysicalInterconnectTypeSecureDigital);
  OSString *physLocationString = OSString::withCString(_isCardEmbedded ? kIOPropertyInternalKey : kIOPropertyExternalKey);
  if (physInterconnectString == nullptr || physLocationString == nullptr) {
    OSSafeReleaseNULL(physInterconnectString);
    OSSafeReleaseNULL(physLocationString);
    protoDict->release();
    return;
  }

  bool result = protoDict->setObject(kIOPropertyPhysicalInterconnectTypeKey, physInterconnectString)
    && protoDict->setObject(kIOPropertyPhysicalInterconnectLocationKey, physLocationString);
  physInterconnectString->release();
  physLocationString->release();
  if (!result) {
    protoDict->release();
    return;
  }

  setProperty(kIOPropertyProtocolCharacteristicsKey, protoDict);
  protoDict->release();

  //
  // Removable cards need the SD icon.
  //
  if (!_isCardEmbedded) {
    OSDictionary *iconDict = OSDictionary::withCapacity(2);
    if (iconDict == nullptr) {
      return;
    }

    OSString *iconCFBundleString = OSString::withCString("com.apple.iokit.IOSCSIArchitectureModelFamily");
    OSString *iconResourceString = OSString::withCString("SD.icns");
    if (iconCFBundleString == nullptr || iconResourceString == nullptr) {
      OSSafeReleaseNULL(iconCFBundleString);
      OSSafeReleaseNULL(iconResourceString);
      iconDict->release();
      return;
    }

    result = iconDict->setObject(kCFBundleIdentifierKey, iconCFBundleString)
      && iconDict->setObject(kIOBundleResourceFileKey, iconResourceString);
    iconCFBundleString->release();
    iconResourceString->release();
    if (!result) {
      iconDict->release();
      return;
    }

    setProperty(kIOMediaIconKey, iconDict);
    iconDict->release();
  }

  //
  // Populate Device Characteristics for embedded cards or removable cards.
  //
  OSDictionary *devDict = OSDictionary::withCapacity(5);
  if (devDict == nullptr) {
    return;
  }

  if (_isCardEmbedded) {
    OSString *serialNumberString = OSString::withCString(getAdditionalDeviceInfoString());
    OSString *vendorString = OSString::withCString(getVendorString());
    OSString *productString = OSString::withCString(getProductString());
    OSString *revisionString = OSString::withCString(getRevisionString());
    OSString *mediumTypeString = OSString::withCString(kIOPropertyMediumTypeSolidStateKey);
    if (serialNumberString == nullptr || vendorString == nullptr
        || productString == nullptr || revisionString == nullptr
        || mediumTypeString == nullptr) {
      OSSafeReleaseNULL(serialNumberString);
      OSSafeReleaseNULL(vendorString);
      OSSafeReleaseNULL(productString);
      OSSafeReleaseNULL(revisionString);
      OSSafeReleaseNULL(mediumTypeString);
      devDict->release();
      return;
    }

    result = devDict->setObject(kIOPropertyProductSerialNumberKey, serialNumberString)
      && devDict->setObject(kIOPropertyVendorNameKey, vendorString)
      && devDict->setObject(kIOPropertyProductNameKey, productString)
      && devDict->setObject(kIOPropertyProductRevisionLevelKey, revisionString)
      && devDict->setObject(kIOPropertyMediumTypeKey, mediumTypeString);
    serialNumberString->release();
    vendorString->release();
    productString->release();
    revisionString->release();
    mediumTypeString->release();
    if (!result) {
      devDict->release();
      return;
    }
  }

  setProperty(kIOPropertyDeviceCharacteristicsKey, devDict);
  devDict->release();
}

//
//  EmeraldSDHCSlot.hpp
//  EmeraldSDHC card slot driver nub
//
//  Copyright © 2021-2023 Goldfish64. All rights reserved.
//

#ifndef EmeraldSDHCSlot_hpp
#define EmeraldSDHCSlot_hpp

#include <IOKit/IOService.h>

#include "EmeraldSDHC.hpp"
#include "SDMisc.hpp"

class EmeraldSDHCSlot : public IOService {
  OSDeclareDefaultStructors(EmeraldSDHCSlot);
  EMDeclareLogFunctionsHC(EmeraldSDHCSlot);
  typedef IOService super;

private:
  EmeraldSDHC *_hostController = nullptr;
  UInt8       _cardSlotId      = 0;

public:
  //
  // IOService overrides.
  //
  bool attach(IOService *provider) APPLE_KEXT_OVERRIDE;
  void detach(IOService *provider) APPLE_KEXT_OVERRIDE;
  
  //
  // Host controller functions.
  //
  bool waitForBits8(UInt32 offset, UInt8 mask, bool waitClear, bool writeClear);
  bool waitForBits16(UInt32 offset, UInt16 mask, bool waitClear, bool writeClear);
  bool waitForBits32(UInt32 offset, UInt32 mask, bool waitClear, bool writeClear);
  inline SDHostControllerVersion getControllerVersion() {
    return (SDHostControllerVersion) (readReg16(kSDHCRegHostControllerVersion) & kSDHCRegHostControllerVersionMask);
  }
  const char* getControllerVersionString();
  inline UInt64 getControllerCapabilities() {
    return readReg64(kSDHCRegCapabilities);
  }
  inline bool isCardPresent() {
    return readReg32(kSDHCRegPresentState) & kSDHCRegPresentStateCardInserted;
  }
  inline bool isCardWriteProtected() {
    return (readReg32(kSDHCRegPresentState) & kSDHCRegPresentStateCardWriteable) == 0;
  }
  bool resetController(UInt8 bits);
  bool setControllerClock(UInt32 speedHz);
  void setControllerPower(bool enabled);
  void setControllerBusWidth(SDABusWidth busWidth);
  void setControllerDMAMode(SDATransferType type);
  void setControllerInsertionEvents(bool enable);

  //
  // Parent host controller functions.
  //
  inline UInt8 getCardSlotId() { return _cardSlotId; }
  inline void writeReg8(UInt32 offset, UInt8 value) {
    _hostController->writeReg8(_cardSlotId, offset, value);
  }
  inline UInt8 readReg8(UInt32 offset) {
    return _hostController->readReg8(_cardSlotId, offset);
  }
  inline void writeReg16(UInt32 offset, UInt16 value) {
    _hostController->writeReg16(_cardSlotId, offset, value);
  }
  inline UInt16 readReg16(UInt32 offset) {
    return _hostController->readReg16(_cardSlotId, offset);
  }
  inline void writeReg32(UInt32 offset, UInt32 value) {
    _hostController->writeReg32(_cardSlotId, offset, value);
  }
  inline UInt32 readReg32(UInt32 offset) {
    return _hostController->readReg32(_cardSlotId, offset);
  }
  inline void writeReg64(UInt32 offset, UInt64 value) {
    _hostController->writeReg64(_cardSlotId, offset, value);
  }
  inline UInt64 readReg64(UInt32 offset) {
    return _hostController->readReg64(_cardSlotId, offset);
  }
  inline void registerCardSlotInterrupt(OSObject *target, EmeraldSDHCSlotInterruptAction action) {
    _hostController->registerCardSlotInterrupt(_cardSlotId, target, action);
  }
};

#endif

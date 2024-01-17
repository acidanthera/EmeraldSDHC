//
//  EmeraldSDHC.hpp
//  EmeraldSDHC SD host controller driver
//
//  Copyright © 2021-2023 Goldfish64. All rights reserved.
//

#ifndef EmeraldSDHC_hpp
#define EmeraldSDHC_hpp

#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>

#include "SDMisc.hpp"
#include "SDRegs.hpp"

typedef void (*EmeraldSDHCSlotInterruptAction)(void *target);

class EmeraldSDHCSlot;

class EmeraldSDHC : public IOService {
  OSDeclareDefaultStructors(EmeraldSDHC);
  EMDeclareLogFunctionsHC(EmeraldSDHC);
  typedef IOService super;

private:
  IOService              *_device         = nullptr;
  IOWorkLoop             *_workLoop       = nullptr;
  IOInterruptEventSource *_intEventSource = nullptr;

  //
  // Child slots.
  //
  UInt32          _cardSlotCount                              = 0;
  IOMemoryMap     *_cardSlotMemoryMaps[kSDHCMaximumSlotCount] = { };
  volatile void   *_cardSlotBaseMemory[kSDHCMaximumSlotCount] = { };
  EmeraldSDHCSlot *_cardSlotNubs[kSDHCMaximumSlotCount]       = { };

  OSObject                       *_cardSlotTargets[kSDHCMaximumSlotCount] = { };
  EmeraldSDHCSlotInterruptAction _cardSlotActions[kSDHCMaximumSlotCount]  = { };

  void handleInterrupt(OSObject *owner, IOInterruptEventSource *src, int intCount);
  bool probeCardSlots();

public:
  //
  // IOService overrides.
  //
  bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
  void stop(IOService *provider) APPLE_KEXT_OVERRIDE;
  IOWorkLoop *getWorkLoop() const APPLE_KEXT_OVERRIDE;

  //
  // Host controller functions.
  //
  inline void writeReg8(UInt8 slot, UInt32 offset, UInt8 value) {
    *(volatile UInt8 *)((uintptr_t)_cardSlotBaseMemory[slot - 1] + offset) = value;
  }
  inline UInt8 readReg8(UInt8 slot, UInt32 offset) {
    return *(volatile UInt8 *)((uintptr_t)_cardSlotBaseMemory[slot - 1] + offset);
  }
  inline void writeReg16(UInt8 slot, UInt32 offset, UInt16 value) {
    OSWriteLittleInt16(_cardSlotBaseMemory[slot - 1], offset, value);
  }
  inline UInt16 readReg16(UInt8 slot, UInt32 offset) {
    return OSReadLittleInt16(_cardSlotBaseMemory[slot - 1], offset);
  }
  inline void writeReg32(UInt8 slot, UInt32 offset, UInt32 value) {
    OSWriteLittleInt32(_cardSlotBaseMemory[slot - 1], offset, value);
  }
  inline UInt32 readReg32(UInt8 slot, UInt32 offset) {
    return OSReadLittleInt32(_cardSlotBaseMemory[slot - 1], offset);
  }
  inline void writeReg64(UInt8 slot, UInt32 offset, UInt64 value) {
    OSWriteLittleInt64(_cardSlotBaseMemory[slot - 1], offset, value);
  }
  inline UInt64 readReg64(UInt8 slot, UInt32 offset) {
    return OSReadLittleInt64(_cardSlotBaseMemory[slot - 1], offset);
  }
  void registerCardSlotInterrupt(UInt8 slot, OSObject *target, EmeraldSDHCSlotInterruptAction action);
};

#endif

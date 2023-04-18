//
//  EmeraldSDHC.cpp
//  EmeraldSDHC SD host controller driver
//
//  Copyright © 2021-2023 Goldfish64. All rights reserved.
//

#include "EmeraldSDHC.hpp"
#include "EmeraldSDHCSlot.hpp"

OSDefineMetaClassAndStructors(EmeraldSDHC, super);

bool EmeraldSDHC::start(IOService *provider) {
  IOReturn status;
  bool     result = false;

  EMCheckDebugArgs();
  EMSYSLOG("Initializing EmeraldSDHC");

  if (!super::start(provider)) {
    EMSYSLOG("Superclass failed to start");
    return false;
  }

  do {
    //
    // Get IOPCIDevice or IOACPIPlatformDevice provider.
    //
    _acpiDevice = OSDynamicCast(IOACPIPlatformDevice, provider);
    if (_acpiDevice != nullptr) {
      EMSYSLOG("Provider is IOACPIPlatformDevice");
      EmeraldSDHC::isAcpiDevice = true;
      break;
    } else if (_acpiDevice == nullptr) {
      EMSYSLOG("Provider is not IOACPIPlatformDevice");
      break;
    }
    _acpiDevice->retain();

    _pciDevice = OSDynamicCast(IOPCIDevice, provider);
    if (_pciDevice != nullptr) {
      EMSYSLOG("Provider is IOPCIDevice");
      EmeraldSDHC::isAcpiDevice = false;
      break;
    } else if (_pciDevice == nullptr) {
        EMSYSLOG("Provider is not IOPCIDevice");
        break;
    }
    _pciDevice->retain();

    //
    // Create work loop and interrupt source.
    //
    _workLoop = IOWorkLoop::workLoop();
    if (_workLoop == nullptr) {
      EMSYSLOG("Failed to create work loop");
      break;
    }

    _intEventSource = IOInterruptEventSource::interruptEventSource(this,
                                                                   OSMemberFunctionCast(IOInterruptEventAction, this, &EmeraldSDHC::handleInterrupt),
                                                                   provider);
    if (_intEventSource == nullptr) {
      EMSYSLOG("Failed to create interrupt event source");
      break;
    }
    status = _workLoop->addEventSource(_intEventSource);
    if (status != kIOReturnSuccess) {
      EMSYSLOG("Failed to add interrupt event source to work loop with status 0x%X", status);
      break;
    }
    _intEventSource->enable();

    //
    // Probe card slots.
    //
    if (!probeCardSlots()) {
      EMSYSLOG("Failed to probe card slots");
      break;
    }

    result = true;
    EMDBGLOG("Initialized EmeraldSDHC");
  } while (false);

  if (!result) {
    stop(provider);
  }
  return result;
}

void EmeraldSDHC::stop(IOService *provider) {
  if (_intEventSource != nullptr) {
    _workLoop->removeEventSource(_intEventSource);
    OSSafeReleaseNULL(_intEventSource);
  }

  OSSafeReleaseNULL(_workLoop);
  OSSafeReleaseNULL(_acpiDevice);
  OSSafeReleaseNULL(_pciDevice);
}

IOWorkLoop* EmeraldSDHC::getWorkLoop() const {
  return _workLoop;
}

void EmeraldSDHC::handleInterrupt(OSObject *owner, IOInterruptEventSource *src, int intCount) {
  //
  // TODO: handle multiple slots.
  //
  if (_cardSlotActions[0] != nullptr) {
    _cardSlotActions[0](_cardSlotTargets[0]);
  }
}

bool EmeraldSDHC::probeCardSlots() {
  bool         result;
  OSDictionary *cardSlotDict;
  OSNumber     *cardSlotNumber;

  //
  // Get number of active card slots.
  // Each card slot will have exactly one BAR on the PCI device, for a maximum of 6.
  //
  if (EmeraldSDHC::isAcpiDevice == true) {
    _cardSlotCount = _acpiDevice->getDeviceMemoryCount();
  } else if (EmeraldSDHC::isAcpiDevice == false) {
    _cardSlotCount = _pciDevice->getDeviceMemoryCount();
  }
  if (_cardSlotCount > kSDHCMaximumSlotCount) {
    EMSYSLOG("More than %u card slots are present, limiting to %u", kSDHCMaximumSlotCount, kSDHCMaximumSlotCount);
    _cardSlotCount = kSDHCMaximumSlotCount;
  } else if (_cardSlotCount == 0) {
    EMSYSLOG("No card slots are present on SD host controller");
    return false;
  }
  EMDBGLOG("Detected %u card slot(s) on SD host controller", _cardSlotCount);

  bzero(_cardSlotMemoryMaps, sizeof (_cardSlotMemoryMaps));
  bzero(_cardSlotBaseMemory, sizeof (_cardSlotBaseMemory));
  bzero(_cardSlotNubs, sizeof (_cardSlotNubs));
  bzero(_cardSlotTargets, sizeof (_cardSlotTargets));
  bzero(_cardSlotActions, sizeof (_cardSlotActions));

  //
  // Populate information for each card slot and create nubs.
  //
  // Card slot is zero-based for array, but one-based for slot IDs.
  // macOS uses one-based for its slot IDs in AppleSDXC and device path building.
  //
  for (int slot = 0; slot < _cardSlotCount; slot++) {
    //
    // Map memory for card slot.
    //
    if (EmeraldSDHC::isAcpiDevice == true) {
      _cardSlotMemoryMaps[slot] = _acpiDevice->mapDeviceMemoryWithIndex(slot);
    } else if (EmeraldSDHC::isAcpiDevice == false) {
      _cardSlotMemoryMaps[slot] = _pciDevice->mapDeviceMemoryWithIndex(slot);
    }
    if (_cardSlotMemoryMaps[slot] == nullptr) {
      EMSYSLOG("Failed to get memory map for card slot %u", slot + 1);
      return false;
    }
    _cardSlotBaseMemory[slot] = (volatile void *) _cardSlotMemoryMaps[slot]->getVirtualAddress();

    //
    // Allocate card slot nub.
    //
    _cardSlotNubs[slot] = OSTypeAlloc(EmeraldSDHCSlot);
    if (_cardSlotNubs[slot] == nullptr) {
      EMSYSLOG("Failed to allocate nub for card slot %u", slot + 1);
      return false;
    }

    //
    // Add slot number property.
    //
    cardSlotNumber = OSNumber::withNumber(slot + 1, 8);
    if (cardSlotNumber == nullptr) {
      EMSYSLOG("Failed to allocate slot number property for card slot %u", slot + 1);
      return false;
    }

    cardSlotDict = OSDictionary::withCapacity(1);
    if (cardSlotDict == nullptr) {
      EMSYSLOG("Failed to allocate dictionary for card slot %u", slot + 1);
      cardSlotNumber->release();
      return false;
    }

    result = cardSlotDict->setObject(kSDACardSlotNumberKey, cardSlotNumber);
    cardSlotNumber->release();
    if (!result) {
      EMSYSLOG("Failed to add dictionary property for card slot %u", slot + 1);
      cardSlotDict->release();
      return false;
    }

    //
    // Initialize and attach newly created nub.
    //
    result = _cardSlotNubs[slot]->init(cardSlotDict) && _cardSlotNubs[slot]->attach(this);
    cardSlotDict->release();

    if (!result) {
      EMSYSLOG("Failed to attach nub for card slot %u", slot + 1);
      OSSafeReleaseNULL(_cardSlotNubs[slot]);
      return false;
    }
  }

  return true;
}

void EmeraldSDHC::registerCardSlotInterrupt(UInt8 slot, OSObject *target, EmeraldSDHCSlotInterruptAction action) {
  //
  // Register interrupt handler for slot.
  //
  _cardSlotTargets[slot - 1] = target;
  _cardSlotActions[slot - 1] = action;
}

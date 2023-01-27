//
//  EmeraldSDHCBlockStorageDevice.cpp
//  EmeraldSDHC card slot IOBlockStorageDevice implementation
//
//  Public functions
//
//  Copyright © 2021-2023 Goldfish64. All rights reserved.
//

#include "EmeraldSDHCBlockStorageDevice.hpp"

OSDefineMetaClassAndStructors(EmeraldSDHCBlockStorageDevice, super);

bool EmeraldSDHCBlockStorageDevice::start(IOService *provider) {
  IOReturn status;
  bool     result = false;
  bool     poolCommandSuccess = false;

  EMCheckDebugArgs();
  EMDBGLOG("Initializing EmeraldSDHCBlockStorageDevice");

  if (!super::start(provider)) {
    EMSYSLOG("Superclass failed to start");
    return false;
  }

  do {
    //
    // Get card slot nub provider.
    //
    _cardSlot = OSDynamicCast(EmeraldSDHCSlot, provider);
    if (_cardSlot == nullptr) {
      EMSYSLOG("Provider is not EmeraldSDHCSlot");
      break;
    }
    _cardSlot->retain();

    //
    // Initialize timeout timer.
    //
    _timerEventSourceTimeouts = IOTimerEventSource::timerEventSource(this,
                                                                     OSMemberFunctionCast(IOTimerEventSource::Action, this, &EmeraldSDHCBlockStorageDevice::handleIOTimeout));
    if (_timerEventSourceTimeouts == nullptr) {
      EMSYSLOG("Failed to initialize timeout event source");
      break;
    }
    getWorkLoop()->addEventSource(_timerEventSourceTimeouts);
    
    //
    // Initialize command pool and queue.
    //
    _cmdPool = IOCommandPool::withWorkLoop(getWorkLoop());
    if (_cmdPool == nullptr) {
      EMSYSLOG("Failed to initialize command pool");
      break;
    }

    for (int i = 0; i < 10; i++) {
      poolCommandSuccess = allocatePoolCommand() != nullptr;
      if (!poolCommandSuccess) {
        break;
      }
    }
    if (!poolCommandSuccess) {
      EMSYSLOG("Failed to initialize command pool");
      break;
    }
    
    IOBufferMemoryDescriptor *bufDesc;
    
    //
    // Create page-aligned DMA buffer and get physical address.
    //
    bufDesc = IOBufferMemoryDescriptor::inTaskWithPhysicalMask(kernel_task,
                                                               kIODirectionInOut | kIOMemoryPhysicallyContiguous,
                                                               ((kSDAMaxBlocksPerTransfer * kSDABlockSize) / PAGE_SIZE) * sizeof (*descs), 0xFFFFF000ULL);
    bufDesc->prepare();
    
    descAddr = bufDesc->getPhysicalAddress();
    descs   = (SDHostADMA2Descriptor32*) bufDesc->getBytesNoCopy();
    
    bzero(descs, ((kSDAMaxBlocksPerTransfer * kSDABlockSize) / PAGE_SIZE) * sizeof (*descs));

    queue_init(&_cmdQueue);

    //
    // Initialize locks.
    //
    _syncCommandLock = IOLockAlloc();
    if (_syncCommandLock == nullptr) {
      EMSYSLOG("Failed to initialize sync command lock");
      break;
    }

    //
    // Initialize card change thread.
    //
    _cardChangeThread = thread_call_allocate(OSMemberFunctionCast(thread_call_func_t, this, &EmeraldSDHCBlockStorageDevice::handleCardChange), this);
    if (_cardChangeThread == nullptr) {
      EMSYSLOG("Failed to create card change thread");
      break;
    }

    //
    // Register interrupt handler.
    //
    _cardSlot->registerCardSlotInterrupt(this,
                                         OSMemberFunctionCast(EmeraldSDHCSlotInterruptAction, this, &EmeraldSDHCBlockStorageDevice::handleInterrupt));

    //
    // Create command gate.
    //
    _cmdGate = IOCommandGate::commandGate(this);
    if (_cmdGate == nullptr) {
      EMSYSLOG("Failed to initialize command gate");
      break;
    }
    status = getWorkLoop()->addEventSource(_cmdGate);
    if (status != kIOReturnSuccess) {
      EMSYSLOG("Failed to add command gate to work loop");
      break;
    }
    _cmdGate->enable();

    //
    // Initialize controller and card.
    //
    if (!initController()) {
      EMSYSLOG("Failed to initialize SD host controller");
      break;
    }
    if (!initCard()) {
      EMSYSLOG("Failed to initialize card");
    }

    //
    // Register with the power subsystem.
    //
    PMinit();
    provider->joinPMtree(this);

    static const IOPMPowerState powerStates[] = {
      { kIOPMPowerStateVersion1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { kIOPMPowerStateVersion1, kIOPMDeviceUsable, IOPMPowerOn, IOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0 }
    };
    registerPowerDriver(this, const_cast<IOPMPowerState*>(powerStates), sizeof(powerStates) / sizeof(powerStates[0]));

    setStorageProperties();
    registerService();

    result = true;
    EMDBGLOG("Initialized EmeraldSDHCBlockStorageDevice on slot %u", _cardSlot->getCardSlotId());
  } while (false);

  if (!result) {
    stop(provider);
  }
  return result;
}

void EmeraldSDHCBlockStorageDevice::stop(IOService *provider) {
  if (_cmdGate != nullptr) {
    getWorkLoop()->removeEventSource(_cmdGate);
    OSSafeReleaseNULL(_cmdGate);
  }
  OSSafeReleaseNULL(_cardSlot);

  if (_cardChangeThread != nullptr) {
    thread_call_free(_cardChangeThread);
    _cardChangeThread = nullptr;
  }

  if (_syncCommandLock != nullptr) {
    IOLockFree(_syncCommandLock);
    _syncCommandLock = nullptr;
  }

  super::stop(provider);
}

IOReturn EmeraldSDHCBlockStorageDevice::setPowerState(unsigned long powerStateOrdinal, IOService *whatDevice) {
  //
  // Handle power state changes.
  //
  switch (powerStateOrdinal) {
    case 0:
      EMDBGLOG("Sleep request received");
      _isMachineSleeping = true;
      break;

    case 1:
      //
      // macOS will call this function initially, only continue for real wake requests.
      //
      if (_isMachineSleeping) {
        //
        // Reset controller and card.
        //
        EMDBGLOG("Wake request received");
        initController();
        initCard();

        _isMachineSleeping = false;
        EMDBGLOG("Wake request complete");
      }
      break;
  }

  return kIOPMAckImplied;
}

IOReturn EmeraldSDHCBlockStorageDevice::message(UInt32 type, IOService *provider, void *argument) {
  return super::message(type, provider, argument);
}

IOReturn EmeraldSDHCBlockStorageDevice::doEjectMedia() {
  EMDBGLOG("start");
  return kIOReturnSuccess;
}

IOReturn EmeraldSDHCBlockStorageDevice::doFormatMedia(UInt64 byteCapacity) {
  //
  // Low-level formatting not supported.
  //
  return kIOReturnUnsupported;
}

UInt32 EmeraldSDHCBlockStorageDevice::doGetFormatCapacities(UInt64 *capacities, UInt32 capacitiesMaxCount) const {
  //
  // Low-level formatting not supported.
  //
  return 0;
}

IOReturn EmeraldSDHCBlockStorageDevice::doLockUnlockMedia(bool doLock) {
  //
  // Locking not supported.
  //
  return kIOReturnUnsupported;
}

IOReturn EmeraldSDHCBlockStorageDevice::doSynchronizeCache() {
  EMDBGLOG("start");
  return _cardSlot->isCardPresent() ? kIOReturnSuccess : kIOReturnNoMedia;
}

char* EmeraldSDHCBlockStorageDevice::getVendorString() {
  EMDBGLOG("start");
  return (char *) _cardVendorName;
}

char* EmeraldSDHCBlockStorageDevice::getProductString() {
  EMDBGLOG("start");
  return _cardProductName;
}

char* EmeraldSDHCBlockStorageDevice::getRevisionString() {
  EMDBGLOG("start");
  return _cardRev;
}

char* EmeraldSDHCBlockStorageDevice::getAdditionalDeviceInfoString() {
  EMDBGLOG("start");
  return _cardSN;
}

IOReturn EmeraldSDHCBlockStorageDevice::getWriteCacheState(bool *enabled) {
  EMDBGLOG("start");
  return kIOReturnUnsupported;
}

IOReturn EmeraldSDHCBlockStorageDevice::setWriteCacheState(bool enabled) {
  EMDBGLOG("start");
  return kIOReturnUnsupported;
}

IOReturn EmeraldSDHCBlockStorageDevice::reportBlockSize(UInt64 *blockSize) {
  EMDBGLOG("start");
  // Block size used for access is always 512 bytes.
  //
  // Some older cards may have larger block sizes, but those will be used for size
  //   calculation only as 512 bytes is the only size supported by all controllers and cards.
  *blockSize = kSDABlockSize;
  return kIOReturnSuccess;
}

IOReturn EmeraldSDHCBlockStorageDevice::reportEjectability(bool *isEjectable) {
  EMDBGLOG("start");
  *isEjectable = !_isCardEmbedded;
  return kIOReturnSuccess;
}

IOReturn EmeraldSDHCBlockStorageDevice::reportLockability(bool *isLockable) {
  EMDBGLOG("start");
  return kIOReturnUnsupported;
}

IOReturn EmeraldSDHCBlockStorageDevice::reportRemovability(bool *isRemovable) {
  EMDBGLOG("start");
  *isRemovable = !_isCardEmbedded;
  return kIOReturnSuccess;
}

IOReturn EmeraldSDHCBlockStorageDevice::reportMaxValidBlock(UInt64 *maxBlock) {
  EMDBGLOG("start");
  if (!_cardSlot->isCardPresent()) {
    return kIOReturnNoMedia;
  }

  *maxBlock = _cardBlockCount - 1;
  return kIOReturnSuccess;
}

IOReturn EmeraldSDHCBlockStorageDevice::reportMediaState(bool *mediaPresent, bool *changedState) {
  EMDBGLOG("start");
  *mediaPresent = _cardSlot->isCardPresent();
  *changedState = *mediaPresent != _isCardInserted;
  return kIOReturnSuccess;
}

IOReturn EmeraldSDHCBlockStorageDevice::reportPollRequirements(bool *pollRequired, bool *pollIsExpensive) {
  EMDBGLOG("start");
  return kIOReturnUnsupported;
}

IOReturn EmeraldSDHCBlockStorageDevice::reportWriteProtection(bool *isWriteProtected) {
  EMDBGLOG("start");
  if (!_cardSlot->isCardPresent()) {
    return kIOReturnNoMedia;
  }

  *isWriteProtected = _cardSlot->isCardWriteProtected();
  return kIOReturnSuccess;
}

IOReturn EmeraldSDHCBlockStorageDevice::doAsyncReadWrite(IOMemoryDescriptor *buffer, UInt64 block, UInt64 nblks,
                                                         IOStorageAttributes *attributes, IOStorageCompletion *completion) {
  EMIODBGLOG("%s LBA %llu of %llu blocks %llX", buffer->getDirection() == kIODirectionIn ? "Read" : "Write", block, nblks, completion);

  UInt32        blockStart;
  UInt64        blockCountRemaining;
  UInt32        blockCount;
  IOByteCount   offset = 0;

  //
  // Sleep if machine was previously sleeping to allow controller/card to be reset.
  //
  if (_isMachineSleeping) {
    EMDBGLOG("Controller pending resume from sleep power state");
    for (int i = 0; i < kSDATimeout_30sec; i++) {
      if (!_isMachineSleeping) {
        break;
      }
      IOSleep(1);
    }
    
    if (_isMachineSleeping) {
      EMSYSLOG("Timed out while waiting for controller to resume from sleep power state");
      return kIOReturnTimeout;
    }
  }

  bool isRead = buffer->getDirection() == kIODirectionIn;

  //
  // Prepare block data structure.
  // TODO: SDA cannot support more than 32-bit LBA addresses, so for now this cast is safe.
  //
  blockStart = (UInt32) block;
  blockCountRemaining = nblks;

  do {
    //
    // Most SD host controllers have a max possible block count of 65535 per transfer.
    // To meet macOS requirements, the max we can do per command is 61440.
    //
    if (blockCountRemaining > kSDAMaxBlocksPerTransfer) {
      EMIODBGLOG("%s LBA %llu of %llu blocks %llX", buffer->getDirection() == kIODirectionIn ? "Read" : "Write", block, nblks, completion);
      blockCount = kSDAMaxBlocksPerTransfer;
      blockCountRemaining -= blockCount;
      EMIODBGLOG("%u blocks are too big, splitting %x off", blockCountRemaining, offset);
    } else {
      blockCount = (UInt32) blockCountRemaining;
      blockCountRemaining = 0;
    }

    //
    // Invoke command to transfer data.
    // Small capacity cards use byte addresses instead of block addresses.
    //
    UInt32 cmdIndex;
    if (isRead) {
      cmdIndex = isSDCard() ? (UInt32) kSDCommandReadMultipleBlock : (UInt32) kMMCCommandReadMultipleBlock;
    } else {
      cmdIndex = isSDCard() ? (UInt32) kSDCommandWriteMultipleBlock : (UInt32) kMMCCommandWriteMultipleBlock;
    }
    
    if (doAsyncCommandWithData(cmdIndex, _isCardHighCapacity ? blockStart : blockStart * kSDABlockSize,
                               kSDATimeout_120sec, blockCountRemaining == 0 ? completion : nullptr, blockCount, nblks,
                               kSDABlockSize, buffer, offset) != kIOReturnSuccess) {
      panic("error");
      return kIOReturnNoResources;
    }

    //
    // Move to next set of blocks.
    //
    blockStart += blockCount;
    offset += blockCount * kSDABlockSize;
  } while (blockCountRemaining > 0);

  return kIOReturnSuccess;
}

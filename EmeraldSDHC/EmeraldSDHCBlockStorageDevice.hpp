//
//  EmeraldSDHCBlockStorageDevice.hpp
//  EmeraldSDHC card slot IOBlockStorageDevice implementation
//
//  Copyright © 2021-2023 Goldfish64. All rights reserved.
//

#ifndef EmeraldSDHCBlockStorageDevice_hpp
#define EmeraldSDHCBlockStorageDevice_hpp

#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/IODMACommand.h>
#include <IOKit/IOCommandPool.h>
#include <IOKit/storage/IOBlockStorageDevice.h>
#include <IOKit/IOTimerEventSource.h>

#include "EmeraldSDHCSlot.hpp"
#include "EmeraldSDHCCommand.hpp"

typedef struct {
  UInt32              command;
  UInt32              argument;
  SDACommandResponse  *response;
  UInt32              timeout;

  IOStorageCompletion *completion;

  UInt32              blockCount;
  UInt32              blockCountTotal;
  UInt32              blockSize;
  IOMemoryDescriptor  *memoryDescriptor;
  IOByteCount         memoryDescriptorOffset;
} EmeraldSDHCAsyncCommandArgs;

class EmeraldSDHCBlockStorageDevice : public IOBlockStorageDevice {
  OSDeclareDefaultStructors(EmeraldSDHCBlockStorageDevice);
  EMDeclareLogFunctionsCard(EmeraldSDHCBlockStorageDevice);
  typedef IOBlockStorageDevice super;

private:
  EmeraldSDHCSlot *_cardSlot = nullptr;
  IOCommandGate   *_cmdGate  = nullptr;
  IOTimerEventSource *_timerEventSourceTimeouts = nullptr;

  UInt16 _maskWaiting;

  SDHostADMA2Descriptor32 *descs;
  IOPhysicalAddress descAddr;

  
  //
  // Current card properties.
  //
  // Card type.
  SDACardType _cardType           = kSDACardTypeSD_200;
  // Card data transfer type.
  SDATransferType _hcTransferType = kSDATransferTypeSDMA;
  // Card address (fixed for MMC, specified by card for SD).
  UInt16      _cardAddress        = 0;
  // eMMC should show as internal, all others as external.
  bool        _isCardEmbedded     = false;
  // SDSC cards use byte addressing for read/write, all others use 32-bit LBA.
  bool        _isCardHighCapacity = false;
  // Card state changed.
  bool        _isCardInserted     = false;
  // Card selected.
  bool        _isCardSelected     = false;
  // Max MMC clock speed for standard speed mode.
  UInt32      _mmcMaxStandardClock = 0;

  union {
    SDCIDRegister   sd;
    MMCCIDRegister  mmc;
  } _cardCID;
  union {
    SDCSDRegisterV1 sd1;
    SDCSDRegisterV2 sd2;
    SDCSDRegisterV3 sd3;
    MMCCSDRegister  mmc;
  } _cardCSD;
  MMCExtendedCSDRegister _mmcExtendedCSD;
  SDSCRRegister          _sdSCR;

  char        _cardProductName[kSDAProductNameLength] = { };
  const char  *_cardVendorName                        = nullptr;
  char        _cardSN[kSDASerialNumLength]            = { };
  char        _cardRev[kSDARevisionLength]            = { };
  UInt32      _cardBlockCount                         = 0;

  //
  // Commands state structures.
  //
  EmeraldSDHCCommand *_currentCommand = nullptr;
  IOCommandPool      *_cmdPool        = nullptr;
  queue_head_t       _cmdQueue        = { };
  EmeraldSDHCCommand **_initialCommands = nullptr;

  IOLock   *_syncCommandLock      = nullptr;
  bool     _isSleepingSyncCommand = false;
  IOReturn _syncCommandResult;

  //
  // Power management state.
  //
  bool _isMachineSleeping   = false;

  //
  // Internal misc functions.
  //
  void handleInterrupt();
  inline UInt16 calcPower(UInt8 exp) {
    UInt16 value = 1;
    for (int i = 0; i < exp; i++) {
      value *= 2;
    }
    return value;
  }
  inline bool isSDCard() {
    return _cardType != kSDACardTypeMMC;
  }
  

  void handleIOTimeout(IOTimerEventSource *sender);

  //
  // Internal host controller functions.
  //

  bool initController();

  //
  // Thread for card change.
  //
  thread_call_t _cardChangeThread = nullptr;

  //
  // Internal card functions.
  //
  void handleCardChange();
  bool resetCard();
  bool probeCard();
  bool parseCSD();
  bool isExtendedCSDSupported();
  bool parseMMCExtendedCSD();
  bool parseSDSCR();
  bool setCardBusWidth(SDABusWidth busWidth, bool doubleDataRate);
  bool switchMMCExtendedCSD(MMCSwitchAccessBits access, UInt8 index, UInt8 value);
  bool switchMMCSpeed();
  bool tuneCard(SDABusWidth busWidth);
  bool setMMCSpeed(MMCTimingSpeed speed);
  bool initCard();

  //
  // Internal card command functions.
  //
  EmeraldSDHCCommand *allocatePoolCommand();
  void addCommandToQueue(EmeraldSDHCCommand *command);
  EmeraldSDHCCommand *getNextCommandQueue();
  void flushCommandQueue();
  
  void doAsyncIO(UInt16 interruptStatus = 0);
  IOReturn prepareAsyncDataTransfer(EmeraldSDHCCommand *command);
  IOReturn executeAsyncDataTransfer(EmeraldSDHCCommand *command, UInt16 interruptStatus);
  bool sendAsyncCommand(const SDACommandTableEntry *cmdEntry, UInt32 arg);
  bool selectCardAsync(bool selectCard);
  
  inline IOReturn doSyncCommand(UInt32 command, UInt32 argument, UInt32 timeout, SDACommandResponse *response = nullptr) {
    return doSyncCommandWithData(command, argument, timeout, 0, 0, nullptr, 0, response);
  }
  IOReturn doSyncCommandWithData(UInt32 command, UInt32 argument, UInt32 timeout, UInt32 blockCount, UInt32 blockSize,
                                 IOMemoryDescriptor *memoryDescriptor, IOByteCount memoryDescriptorOffset,
                                 SDACommandResponse *response = nullptr);
  void handleSyncCommandCompletion(void *parameter, IOReturn status, UInt64 actualByteCount);
  
  inline IOReturn doAsyncCommand(UInt32 command, UInt32 argument, UInt32 timeout, IOStorageCompletion *completion,
                                 SDACommandResponse *response = nullptr) {
    return doAsyncCommandWithData(command, argument, timeout, completion, 0, 0, 0, nullptr, 0, response);
  }
  IOReturn doAsyncCommandWithData(UInt32 command, UInt32 argument, UInt32 timeout, IOStorageCompletion *completion,
                                  UInt32 blockCount, UInt32 blockCountTotal, UInt32 blockSize,
                                  IOMemoryDescriptor *memoryDescriptor, IOByteCount memoryDescriptorOffset,
                                  SDACommandResponse *response = nullptr);
  IOReturn doAsyncCommandGated(EmeraldSDHCAsyncCommandArgs *args);

  void setStorageProperties();

public:
  //
  // IOService overrides
  //
  bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
  void stop(IOService *provider) APPLE_KEXT_OVERRIDE;
  IOReturn setPowerState(unsigned long powerStateOrdinal, IOService *whatDevice) APPLE_KEXT_OVERRIDE;
  IOReturn message(UInt32 type, IOService *provider, void *argument = nullptr) APPLE_KEXT_OVERRIDE;

  //
  // IOBlockStorageDevice overrides
  //
  IOReturn doEjectMedia() APPLE_KEXT_OVERRIDE;
  IOReturn doFormatMedia(UInt64 byteCapacity) APPLE_KEXT_OVERRIDE;
  UInt32 doGetFormatCapacities(UInt64 *capacities, UInt32 capacitiesMaxCount) const APPLE_KEXT_OVERRIDE;
  IOReturn doLockUnlockMedia(bool doLock) APPLE_KEXT_OVERRIDE;
  IOReturn doSynchronizeCache() APPLE_KEXT_OVERRIDE;
  char *getVendorString() APPLE_KEXT_OVERRIDE;
  char *getProductString() APPLE_KEXT_OVERRIDE;
  char *getRevisionString() APPLE_KEXT_OVERRIDE;
  char *getAdditionalDeviceInfoString() APPLE_KEXT_OVERRIDE;
  IOReturn getWriteCacheState(bool *enabled) APPLE_KEXT_OVERRIDE;
  IOReturn setWriteCacheState(bool enabled) APPLE_KEXT_OVERRIDE;
  IOReturn reportBlockSize(UInt64 *blockSize) APPLE_KEXT_OVERRIDE;
  IOReturn reportEjectability(bool *isEjectable) APPLE_KEXT_OVERRIDE;
  IOReturn reportLockability(bool *isLockable) APPLE_KEXT_OVERRIDE;
  IOReturn reportRemovability(bool *isRemovable) APPLE_KEXT_OVERRIDE;
  IOReturn reportMaxValidBlock(UInt64 *maxBlock) APPLE_KEXT_OVERRIDE;
  IOReturn reportMediaState(bool *mediaPresent, bool *changedState = 0) APPLE_KEXT_OVERRIDE;
  IOReturn reportPollRequirements(bool *pollRequired, bool *pollIsExpensive) APPLE_KEXT_OVERRIDE;
  IOReturn reportWriteProtection(bool *isWriteProtected) APPLE_KEXT_OVERRIDE;
  IOReturn doAsyncReadWrite(IOMemoryDescriptor *buffer, UInt64 block, UInt64 nblks,
                            IOStorageAttributes *attributes, IOStorageCompletion *completion) APPLE_KEXT_OVERRIDE;
};

#endif

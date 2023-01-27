//
//  EmeraldSDHCCommand.hpp
//  EmeraldSDHC IOCommand implementation
//
//  Copyright © 2022-2023 Goldfish64. All rights reserved.
//

#ifndef EmeraldSDHCCommand_hpp
#define EmeraldSDHCCommand_hpp

#include <IOKit/IOCommand.h>
#include <IOKit/IODMACommand.h>
#include <IOKit/IOMemoryDescriptor.h>
#include <IOKit/IOTypes.h>

#include <IOKit/storage/IOBlockStorageDevice.h>

#include "SDMisc.hpp"

typedef enum : UInt32 {
  kEmeraldSDHCStateStart,
  kEmeraldSDHCStateCardSelectionSent,
  kEmeraldSDHCStateAppCommandSent,
  kEmeraldSDHCStateCommandSent,
  kEmeraldSDHCStateDataTransfer,
  kEmeraldSDHCStateComplete,
  kEmeraldSDHCStateDone
} EmeraldSDHCState;

class EmeraldSDHCCommand : public IOCommand {
  OSDeclareDefaultStructors(EmeraldSDHCCommand);
  typedef IOCommand super;

public:
  //
  // Queue link element.
  //
  queue_chain_t queueChain;

private:
  SDACommandTableEntry *_cmdEntry;
  UInt32 _timeoutMS;
  IOMemoryDescriptor *_memoryDescriptor;
  IOByteCount _position;
  IOByteCount _byteCount;
  IOReturn _result;

public:
  IOReturn result;
  EmeraldSDHCState state;
  const SDACommandTableEntry *cmdEntry;
  UInt32               cmdArgument;
  bool needsResponse = false;
  SDACommandResponse *cmdResponse;
  
  IOMemoryDescriptor *memoryDescriptor;
  IOByteCount memoryDescriptorOffset;
  IOByteCount currentDataOffset;
  bool isRead;
  IODMACommand       *dmaCommand;
  UInt32      blockCount;
  UInt32      blockCountTotal;
  UInt32      blockSize;
  
  IOStorageCompletion completion;
  
  bool newCardSelectionState;
  
  UInt64 totalLength = 0;

  //
  // IOCommand overrides.
  //
  bool init() APPLE_KEXT_OVERRIDE;

  //
  // Command functions.
  //
  void zeroCommand();
  void setTimeoutMS(UInt32 timeoutMS);
  void setBuffer(IOMemoryDescriptor *memoryDescriptor);
  void setPosition(IOByteCount position);
  void setByteCount(IOByteCount byteCount);
  
  IOReturn getResult();
  IOMemoryDescriptor *getBuffer();
  
  
};

#endif

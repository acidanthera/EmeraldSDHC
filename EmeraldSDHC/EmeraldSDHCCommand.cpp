//
//  EmeraldSDHCCommand.cpp
//  EmeraldSDHC IOCommand implementation
//
//  Copyright © 2022-2023 Goldfish64. All rights reserved.
//

#include "EmeraldSDHCCommand.hpp"

OSDefineMetaClassAndStructors(EmeraldSDHCCommand, super);

bool EmeraldSDHCCommand::init() {
  if (!super::init()) {
    return false;
  }
  
  dmaCommand = IODMACommand::withSpecification(kIODMACommandOutputHost32, 32, kSDASDMASegmentSize,
                                               IODMACommand::kMapped, 0, kSDASDMASegmentAlignment);
  if (dmaCommand == nullptr) {
    panic("invalid dma command");
  }

  zeroCommand();
  return true;
}

void EmeraldSDHCCommand::zeroCommand() {
  state = kEmeraldSDHCStateDone;
  needsResponse = false;
  memoryDescriptor = nullptr;
  memoryDescriptorOffset = 0;
  currentDataOffset = 0;
  dmaCommand->clearMemoryDescriptor();
  bzero(&completion, sizeof (completion));
}

void EmeraldSDHCCommand::setTimeoutMS(UInt32 timeoutMS) {
  _timeoutMS = timeoutMS;
}

void EmeraldSDHCCommand::setBuffer(IOMemoryDescriptor *memoryDescriptor) {
  _memoryDescriptor = memoryDescriptor;
}

void EmeraldSDHCCommand::setPosition(IOByteCount position) {
  _position = position;
}

void EmeraldSDHCCommand::setByteCount(IOByteCount byteCount) {
  _byteCount = byteCount;
}

IOReturn EmeraldSDHCCommand::getResult() {
  return _result;
}

IOMemoryDescriptor* EmeraldSDHCCommand::getBuffer() {
  return _memoryDescriptor;
}

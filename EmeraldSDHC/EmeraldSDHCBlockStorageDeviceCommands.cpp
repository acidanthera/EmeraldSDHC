//
//  EmeraldSDHCBlockStorageDeviceCommands.cpp
//  EmeraldSDHC card slot IOBlockStorageDevice implementation
//
//  Card command functions
//
//  Copyright © 2021-2023 Goldfish64. All rights reserved.
//

#include "EmeraldSDHCBlockStorageDevice.hpp"

static const SDACommandTableEntry MMCCommandTable[] = {
  // 0 - 9
  { kMMCCommandGoIdleState,         kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kMMCCommandSendOpCond,          kSDAResponseTypeR3,   kSDADataDirectionNone },
  { kMMCCommandAllSendCID,          kSDAResponseTypeR2,   kSDADataDirectionNone },
  { kMMCCommandSetRelativeAddress,  kSDAResponseTypeR1,   kSDADataDirectionNone },
  { kMMCCommandSetDSR,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kMMCCommandSleepAwake,          kSDAResponseTypeR1b,  kSDADataDirectionNone },
  { kMMCCommandSwitch,              kSDAResponseTypeR1b,  kSDADataDirectionNone,        kSDACommandFlagsNeedsSelection },
  { kMMCCommandSelectDeselectCard,  kSDAResponseTypeR1,   kSDADataDirectionNone },
  { kMMCCommandSendExtCSD,          kSDAResponseTypeR1d,  kSDADataDirectionCardToHost,  kSDACommandFlagsNeedsSelection },
  { kMMCCommandSendCSD,             kSDAResponseTypeR2,   kSDADataDirectionNone },

  // 10 - 19
  { kMMCCommandSendCID,             kSDAResponseTypeR2,   kSDADataDirectionNone },
  { kMMCCommandReadDatUntilStop,    kSDAResponseTypeR1,   kSDADataDirectionCardToHost,  kSDACommandFlagsNeedsSelection },
  { kMMCCommandStopTransmission,    kSDAResponseTypeR1,   kSDADataDirectionNone },
  { kMMCCommandSendStatus,          kSDAResponseTypeR1,   kSDADataDirectionNone },
  { kMMCCommandInvalid,             kSDAResponseTypeR0,   kSDADataDirectionNone,        kSDACommandFlagsNeedsSelection },
  { kMMCCommandGoInactiveState,     kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kMMCCommandSetBlockLength,      kSDAResponseTypeR1,   kSDADataDirectionNone,        kSDACommandFlagsNeedsSelection },
  { kMMCCommandReadSingleBlock,     kSDAResponseTypeR1d,  kSDADataDirectionCardToHost,  kSDACommandFlagsNeedsSelection },
  { kMMCCommandReadMultipleBlock,   kSDAResponseTypeR1d,  kSDADataDirectionCardToHost,  kSDACommandFlagsNeedsSelection,
                                    kSDHCRegTransferModeMultipleBlock | kSDHCRegTransferModeAutoCMD12 },
  { kMMCCommandInvalid,             kSDAResponseTypeR0,   kSDADataDirectionNone },

  // 20 - 29
  { kMMCCommandWriteDatUntilStop,   kSDAResponseTypeR1,   kSDADataDirectionHostToCard,  kSDACommandFlagsNeedsSelection },
  { kMMCCommandSendTuningBlock,     kSDAResponseTypeR1d,  kSDADataDirectionCardToHost,
                                    kSDACommandFlagsNeedsSelection | kSDACommandFlagsIgnoreCmdComplete | kSDACommandFlagsIgnoreTransferComplete },
  { kMMCCommandInvalid,             kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kMMCCommandSetBlockCount,       kSDAResponseTypeR1,   kSDADataDirectionNone,        kSDACommandFlagsNeedsSelection },
  { kMMCCommandWriteBlock,          kSDAResponseTypeR1d,  kSDADataDirectionHostToCard,  kSDACommandFlagsNeedsSelection },
  { kMMCCommandWriteMultipleBlock,  kSDAResponseTypeR1d,  kSDADataDirectionHostToCard,  kSDACommandFlagsNeedsSelection,
                                    kSDHCRegTransferModeMultipleBlock | kSDHCRegTransferModeAutoCMD12 },
};

static const SDACommandTableEntry SDCommandTable[] = {
  // 0 - 9
  { kSDCommandGoIdleState,          kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandAllSendCID,           kSDAResponseTypeR2,   kSDADataDirectionNone },
  { kSDCommandSendRelativeAddress,  kSDAResponseTypeR6,   kSDADataDirectionNone },
  { kSDCommandSetDSR,               kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandSelectDeselectCard,   kSDAResponseTypeR1b,  kSDADataDirectionNone }, // TODO:
  { kSDCommandSendIfCond,           kSDAResponseTypeR7,   kSDADataDirectionNone },
  { kSDCommandSendCSD,              kSDAResponseTypeR2,   kSDADataDirectionNone },

  // 10 - 19
  { kSDCommandSendCID,              kSDAResponseTypeR2,   kSDADataDirectionNone },
  { kSDCommandVoltageSwitch,        kSDAResponseTypeR1,   kSDADataDirectionNone,        kSDACommandFlagsNeedsSelection },
  { kSDCommandStopTransmission,     kSDAResponseTypeR1b,  kSDADataDirectionNone,        kSDACommandFlagsNeedsSelection },
  { kSDCommandSendStatus,           kSDAResponseTypeR1,   kSDADataDirectionNone },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandGoInactiveState,      kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandSetBlockLength,       kSDAResponseTypeR1,   kSDADataDirectionNone,        kSDACommandFlagsNeedsSelection },
  { kSDCommandReadSingleBlock,      kSDAResponseTypeR1d,  kSDADataDirectionCardToHost,  kSDACommandFlagsNeedsSelection },
  { kSDCommandReadMultipleBlock,    kSDAResponseTypeR1d,  kSDADataDirectionCardToHost,  kSDACommandFlagsNeedsSelection,
                                    kSDHCRegTransferModeMultipleBlock | kSDHCRegTransferModeAutoCMD12 },
  { kSDCommandSendTuningBlock,      kSDAResponseTypeR1d,  kSDADataDirectionCardToHost,  kSDACommandFlagsNeedsSelection },

  // 20 - 29
  { kSDCommandSpeedClassControl,    kSDAResponseTypeR1b,  kSDADataDirectionNone,        kSDACommandFlagsNeedsSelection },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandAddressExtension,     kSDAResponseTypeR1,   kSDADataDirectionNone,        kSDACommandFlagsNeedsSelection },
  { kSDCommandSetBlockCount,        kSDAResponseTypeR1,   kSDADataDirectionNone,        kSDACommandFlagsNeedsSelection },
  { kSDCommandWriteBlock,           kSDAResponseTypeR1d,  kSDADataDirectionHostToCard,  kSDACommandFlagsNeedsSelection },
  { kSDCommandWriteMultipleBlock,   kSDAResponseTypeR1d,  kSDADataDirectionHostToCard,  kSDACommandFlagsNeedsSelection,
                                    kSDHCRegTransferModeMultipleBlock | kSDHCRegTransferModeAutoCMD12 },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandProgramCSD,           kSDAResponseTypeR1d,  kSDADataDirectionHostToCard,  kSDACommandFlagsNeedsSelection },
  { kSDCommandSetWriteProtect,      kSDAResponseTypeR1b,  kSDADataDirectionNone,        kSDACommandFlagsNeedsSelection },
  { kSDCommandClearWriteProtect,    kSDAResponseTypeR1b,  kSDADataDirectionNone,        kSDACommandFlagsNeedsSelection },

  // 30 - 39
  { kSDCommandSendWriteProtect,     kSDAResponseTypeR1d,  kSDADataDirectionCardToHost,  kSDACommandFlagsNeedsSelection },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandEraseWriteBlockStart, kSDAResponseTypeR1,   kSDADataDirectionNone,        kSDACommandFlagsNeedsSelection },
  { kSDCommandEraseWriteBlockEnd,   kSDAResponseTypeR1,   kSDADataDirectionNone,        kSDACommandFlagsNeedsSelection },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandErase,                kSDAResponseTypeR1b,  kSDADataDirectionNone,        kSDACommandFlagsNeedsSelection },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },

  // 40 - 49
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandLockUnlock,           kSDAResponseTypeR1,   kSDADataDirectionHostToCard,  kSDACommandFlagsNeedsSelection },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },

  // 50 - 59
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandAppCommand,           kSDAResponseTypeR1,   kSDADataDirectionNone,        kSDACommandFlagsIgnoreSelectionState },
  { kSDCommandGeneralCommand,       kSDAResponseTypeR1,   kSDADataDirectionChkBuffer,   kSDACommandFlagsNeedsSelection },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDCommandInvalid,              kSDAResponseTypeR0,   kSDADataDirectionNone },
};

static const SDACommandTableEntry SDAppCommandTable[] = {
  // 0 - 9
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandSetBusWidth,       kSDAResponseTypeR1,   kSDADataDirectionNone,        kSDACommandFlagsIsApplicationCmd },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },

  // 10 - 19
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandSDStatus,          kSDAResponseTypeR1d,  kSDADataDirectionCardToHost,  kSDACommandFlagsIsApplicationCmd },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },

  // 20 - 29
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandSendNumWrBlocks,   kSDAResponseTypeR1d,  kSDADataDirectionCardToHost,  kSDACommandFlagsIsApplicationCmd },
  { kSDAppCommandSetWrBlkEraseCount,kSDAResponseTypeR1,   kSDADataDirectionNone,        kSDACommandFlagsIsApplicationCmd },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },

  // 30 - 39
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },

  // 40 - 49
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandSendOpCond,        kSDAResponseTypeR3,   kSDADataDirectionNone,        kSDACommandFlagsIsApplicationCmd },
  { kSDAppCommandSetClearCardDetect,kSDAResponseTypeR1,   kSDADataDirectionNone,        kSDACommandFlagsIsApplicationCmd },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },

  // 50 - 59
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandSendSCR,           kSDAResponseTypeR1,   kSDADataDirectionCardToHost,  kSDACommandFlagsIsApplicationCmd | kSDACommandFlagsNeedsSelection },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
  { kSDAppCommandInvalid,           kSDAResponseTypeR0,   kSDADataDirectionNone },
};

EmeraldSDHCCommand* EmeraldSDHCBlockStorageDevice::allocatePoolCommand() {
  EMDBGLOG("Allocating pool command");
  EmeraldSDHCCommand *command = OSTypeAlloc(EmeraldSDHCCommand);
  if (command == nullptr) {
    return nullptr;
  }

  if (!command->init()) {
    command->release();
    return nullptr;
  }

  _cmdPool->returnCommand(command);
  return command;
}

void EmeraldSDHCBlockStorageDevice::addCommandToQueue(EmeraldSDHCCommand *command) {
  queue_enter(&_cmdQueue, command, EmeraldSDHCCommand*, queueChain);
}

EmeraldSDHCCommand* EmeraldSDHCBlockStorageDevice::getNextCommandQueue() {
  EmeraldSDHCCommand *command = nullptr;

  if (!queue_empty(&_cmdQueue)) {
    queue_remove_first(&_cmdQueue, command, EmeraldSDHCCommand*, queueChain);
  }

  return command;
}

void EmeraldSDHCBlockStorageDevice::flushCommandQueue() {
  EmeraldSDHCCommand *command;
  while (!queue_empty(&_cmdQueue)) {
    queue_remove_first(&_cmdQueue, command, EmeraldSDHCCommand*, queueChain);
    command->state = kEmeraldSDHCStateDone;
    _cmdPool->returnCommand(command);
  }
}

IOReturn EmeraldSDHCBlockStorageDevice::doSyncCommandWithData(UInt32 command, UInt32 argument, UInt32 timeout, UInt32 blockCount, UInt32 blockSize,
                                                              IOMemoryDescriptor *memoryDescriptor, IOByteCount memoryDescriptorOffset,
                                                              SDACommandResponse *response) {
  IOReturn status;
  
  EMDBGLOG("Doing a sync command 0x%X", command);

  //
  // Create structures for sleeping for async command completion.
  //
  IOStorageCompletion syncCompletion = { };
  syncCompletion.action = OSMemberFunctionCast(IOStorageCompletionAction, this, &EmeraldSDHCBlockStorageDevice::handleSyncCommandCompletion);
  syncCompletion.target = this;
  syncCompletion.parameter = nullptr;
  _isSleepingSyncCommand = true;
  _syncCommandResult = kIOReturnTimeout;

  status = doAsyncCommandWithData(command, argument, timeout, &syncCompletion,
                                  blockCount, blockCount, blockSize, memoryDescriptor, memoryDescriptorOffset, response);
  if (status != kIOReturnSuccess) {
    return status;
  }

  IOLockLock(_syncCommandLock);
  while (_isSleepingSyncCommand) {
    IOLockSleep(_syncCommandLock, &_isSleepingSyncCommand, THREAD_INTERRUPTIBLE);
  }

  _isSleepingSyncCommand = false;
  status = _syncCommandResult;
  IOLockUnlock(_syncCommandLock);

  return status;
}

void EmeraldSDHCBlockStorageDevice::handleSyncCommandCompletion(void *parameter, IOReturn status, UInt64 actualByteCount) {
  EMIODBGLOG("Woken up to complete sync command");

  IOLockLock(_syncCommandLock);
  _syncCommandResult = status;
  _isSleepingSyncCommand  = false;
  IOLockUnlock(_syncCommandLock);
  IOLockWakeup(_syncCommandLock, &_isSleepingSyncCommand, true);
}

IOReturn EmeraldSDHCBlockStorageDevice::doAsyncCommandWithData(UInt32 command, UInt32 argument, UInt32 timeout, IOStorageCompletion *completion,
                                                               UInt32 blockCount, UInt32 blockCountTotal, UInt32 blockSize,
                                                               IOMemoryDescriptor *memoryDescriptor, IOByteCount memoryDescriptorOffset,
                                                               SDACommandResponse *response) {
  EmeraldSDHCAsyncCommandArgs cmdArgs = { };

  cmdArgs.command                = command;
  cmdArgs.argument               = argument;
  cmdArgs.timeout                = timeout;
  cmdArgs.completion             = completion;
  cmdArgs.response               = response;
  cmdArgs.blockCount             = blockCount;
  cmdArgs.blockCountTotal        = blockCountTotal;
  cmdArgs.blockSize              = blockSize;
  cmdArgs.memoryDescriptor       = memoryDescriptor;
  cmdArgs.memoryDescriptorOffset = memoryDescriptorOffset;

  return _cmdGate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this,
                                                  &EmeraldSDHCBlockStorageDevice::doAsyncCommandGated),
                                                  &cmdArgs);
}

IOReturn EmeraldSDHCBlockStorageDevice::doAsyncCommandGated(EmeraldSDHCAsyncCommandArgs *args) {
  EmeraldSDHCCommand *command;
  const SDACommandTableEntry *cmdEntry = nullptr;

  if (!_cardSlot->isCardPresent()) {
    return kIOReturnNoMedia;
  }

  //
  // Get next available command.
  // If none available, fail.
  //
  command = reinterpret_cast<EmeraldSDHCCommand*>(_cmdPool->getCommand());
  if (command == nullptr) {
    return kIOReturnNoResources;
  }
  command->zeroCommand();

  //
  // Get command entry for card.
  //
  if (isSDCard()) {
    if (args->command & kSDAppCommandFlag) {
      cmdEntry = &SDAppCommandTable[args->command  & ~kSDAppCommandFlag];
    } else {
      cmdEntry = &SDCommandTable[args->command ];
    }
  } else {
    cmdEntry = &MMCCommandTable[args->command ];
  }
  
  command->cmdEntry = cmdEntry;
  command->cmdArgument = args->argument;
  
  if (args->completion != nullptr) {
    memcpy(&command->completion, args->completion, sizeof (command->completion));
  }
  command->memoryDescriptor = args->memoryDescriptor;
  command->memoryDescriptorOffset = args->memoryDescriptorOffset;
  command->blockCount = args->blockCount;
  command->blockCountTotal = args->blockCountTotal;
  command->blockSize = args->blockSize;
  command->cmdResponse = args->response;
  command->state = kEmeraldSDHCStateStart;

  addCommandToQueue(command);
  EMIODBGLOG("Added command %u to queue", args->command);
  
  if (_currentCommand == nullptr) {
    
    _currentCommand = getNextCommandQueue();
    _timerEventSourceTimeouts->setTimeoutMS(args->timeout);
    
    doAsyncIO();
  }

  
  return kIOReturnSuccess;
}

void EmeraldSDHCBlockStorageDevice::doAsyncIO(UInt16 interruptStatus) {
  if (_currentCommand == nullptr) {
    EMDBGLOG("Current command invalid for interrupt bits 0x%X", interruptStatus);
    return;
  }

  //
  // Process current command state in state machine.
  //
  EMIODBGLOG("Current command state %u, interrupt bits 0x%X", _currentCommand->state, interruptStatus);
  switch (_currentCommand->state) {
    //
    // Card select/deselect successfully sent.
    //
    case kEmeraldSDHCStateCardSelectionSent:
      _isCardSelected = _currentCommand->newCardSelectionState;
      EMIODBGLOG("Card is now at selection state %u", _isCardSelected);

    case kEmeraldSDHCStateAppCommandSent:
      EMIODBGLOG("Application command sent");

    //
    // Starting of command execution.
    //
    case kEmeraldSDHCStateStart:
      //
      // Select/deselect card if required.
      //
      _currentCommand->newCardSelectionState = _isCardSelected;
      if (!(_currentCommand->cmdEntry->flags & kSDACommandFlagsIgnoreSelectionState)) {
        if ((_currentCommand->cmdEntry->flags & kSDACommandFlagsNeedsSelection)
            && !_isCardSelected) {
          EMIODBGLOG("Card needs to be selected for command 0x%X", _currentCommand->cmdEntry->command);
          _currentCommand->state = kEmeraldSDHCStateCardSelectionSent;
          _currentCommand->newCardSelectionState = true;
          if (selectCardAsync(true)) {
            break;
          }
          _isCardSelected = true;
        } else if (!(_currentCommand->cmdEntry->flags & kSDACommandFlagsNeedsSelection)
                   && _isCardSelected
                   && _currentCommand->cmdEntry->command != kMMCCommandSelectDeselectCard) {
          EMIODBGLOG("Card needs to be deselected for command 0x%X", _currentCommand->cmdEntry->command);
          _currentCommand->state = kEmeraldSDHCStateCardSelectionSent;
          _currentCommand->newCardSelectionState = false;
          if (selectCardAsync(false)) {
            break;
          }
          _isCardSelected = false;
        }
      }

      //
      // Application commands require the application prefix command be sent first.
      //
      if (isSDCard() && _currentCommand->state != kEmeraldSDHCStateAppCommandSent
          && _currentCommand->cmdEntry->flags & kSDACommandFlagsIsApplicationCmd) {
        EMIODBGLOG("Command %u is application command", _currentCommand->cmdEntry->command);
        if (sendAsyncCommand(&SDCommandTable[kSDCommandAppCommand], _cardAddress << kSDARelativeAddressShift)) {
          break;
        }
      }

      if (_currentCommand->memoryDescriptor != nullptr) {
        prepareAsyncDataTransfer(_currentCommand);
      }

      //
      // Send command.
      //
      _currentCommand->state = kEmeraldSDHCStateDataTransfer;
      if (sendAsyncCommand(_currentCommand->cmdEntry, _currentCommand->cmdArgument)) {
        break;
      } else {
        EMIODBGLOG("Not waiting for command to complete");
      }

    //
    // Data transfer in-process and/or transfer complete.
    //
    case kEmeraldSDHCStateDataTransfer:
      if (_currentCommand->memoryDescriptor != nullptr) {
        executeAsyncDataTransfer(_currentCommand, interruptStatus);
        if (_currentCommand->state == kEmeraldSDHCStateDataTransfer) {
          break;
        }
      }

    case kEmeraldSDHCStateCommandSent:
      //
      // Get response data.
      //
      EMIODBGLOG("Command response 0x%08X%08X", _cardSlot->readReg64(kSDHCRegResponse1), _cardSlot->readReg64(kSDHCRegResponse0));
      if (_currentCommand->cmdResponse != nullptr) {
        _currentCommand->cmdResponse->bytes8[0] = _cardSlot->readReg64(kSDHCRegResponse0);
        _currentCommand->cmdResponse->bytes8[1] = _cardSlot->readReg64(kSDHCRegResponse1);
      }

      _currentCommand->result = kIOReturnSuccess;

    //
    // Command execution completed either in failure or successfully.
    //
    case kEmeraldSDHCStateComplete:
      if (_currentCommand->memoryDescriptor != nullptr) {
        _currentCommand->dmaCommand->complete();
        _currentCommand->dmaCommand->clearMemoryDescriptor();
      }
      IOStorage::complete(&_currentCommand->completion, _currentCommand->result,
                          _currentCommand->result == kIOReturnSuccess ? (_currentCommand->blockCountTotal * _currentCommand->blockSize) : 0);

      _timerEventSourceTimeouts->cancelTimeout();

      _currentCommand->state = kEmeraldSDHCStateDone;
      _cmdPool->returnCommand(_currentCommand);

      _currentCommand = getNextCommandQueue();
      if (_currentCommand != nullptr) {
        doAsyncIO();
      }

      break;

    default:
      EMDBGLOG("Unknown state %u", _currentCommand->state);
      break;
  }
}

IOReturn EmeraldSDHCBlockStorageDevice::prepareAsyncDataTransfer(EmeraldSDHCCommand *command) {
  IOReturn status;
  UInt32   numSegments = 1;
  UInt16   transferMode;

  IODMACommand::Segment32 segment;

  //
  // Prepare DMA structures if using DMA.
  // TODO: Support 64-bit on controllers that support it.
  //
  if (_hcTransferType != kSDATransferTypePIO) {
    //
    // Setup IODMACommand to read/write the desired blocks.
    //
    status = command->dmaCommand->setMemoryDescriptor(command->memoryDescriptor, false);
    if (status != kIOReturnSuccess) {
      EMDBGLOG("Failed to set memory descriptor with status 0x%X", status);
      return status;
    }
    status = command->dmaCommand->prepare(command->memoryDescriptorOffset, command->blockCount * command->blockSize, true, true);
    if (status != kIOReturnSuccess) {
      EMDBGLOG("Failed to prepare DMA command with status 0x%X", status);
      command->dmaCommand->clearMemoryDescriptor();
      return status;
    }

    //
    // Generate all segments if using ADMA2, which can be of any length.
    //
    if (_hcTransferType == kSDATransferTypeADMA2) {
      bzero(descs, kSDANumADMA2Descriptors * sizeof (*descs));
      for (int i = 0; i < kSDANumADMA2Descriptors; i++) {
        status = command->dmaCommand->gen32IOVMSegments(&command->currentDataOffset, &segment, &numSegments);
        if (status != kIOReturnSuccess) {
          EMDBGLOG("Failed to generate ADMA segments with status 0x%X", status);
          break;
        }

        descs[i].address  = segment.fIOVMAddr;
        descs[i].length16 = segment.fLength;
        descs[i].action   = kSDHostADMA2DescriptorActionTransfer;
        descs[i].valid    = 1;

        if (command->currentDataOffset >= command->blockCount * command->blockSize) {
          descs[i].end = 1;
          if (command->blockCount != command->blockCountTotal) {
            EMIODBGLOG("All done, got %u bytes for ADMA %u total bl 0x%X",
                       command->currentDataOffset, command->blockCountTotal, command->memoryDescriptorOffset);
          }
          break;
        }
      }

      _cardSlot->writeReg32(kSDHCRegADMASysAddress, descAddr);
      EMIODBGLOG("Using ADMA physical address %p", descAddr);

    //
    // Generate first DMA segment if using SDMA, which will use fixed 4KB segments.
    //
    } else if (_hcTransferType == kSDATransferTypeSDMA) {
      status = command->dmaCommand->gen32IOVMSegments(&command->currentDataOffset, &segment, &numSegments);
      if (status != kIOReturnSuccess) {
        EMDBGLOG("Failed to generate SDMA segment with status 0x%X", status);
        return status;
      }

      _cardSlot->writeReg32(kSDHCRegSDMA, segment.fIOVMAddr);
      EMIODBGLOG("Using SDMA physical address %p for first segment", segment.fIOVMAddr);
    }
  }

  //
  // Set block size and block count.
  //
  _cardSlot->writeReg16(kSDHCRegBlockSize, command->blockSize);
  _cardSlot->writeReg16(kSDHCRegBlockCount, command->blockCount);

  //
  // Set transfer mode.
  //
  transferMode = kSDHCRegTransferModeBlockCountEnable;
  if (command->cmdEntry->dataDirection == kSDADataDirectionCardToHost) {
    transferMode |= kSDHCRegTransferModeDataTransferRead;
  }

  //
  // Enable DMA if using SDMA or ADMA.
  //
  if (_hcTransferType != kSDATransferTypePIO) {
    transferMode |= kSDHCRegTransferModeDMAEnable;
  }

  //
  // Add any command-specific transfer mode flags.
  //
  transferMode |= command->cmdEntry->hostFlags;
  _cardSlot->writeReg16(kSDHCRegTransferMode, transferMode);

  EMIODBGLOG("Preparing to transfer %u blocks total (%u bytes) using %s and transfer mode 0x%X",
             command->blockCount, command->blockCount * command->blockSize,
             _hcTransferType != kSDATransferTypePIO ? "DMA" : "PIO", transferMode);
  EMIODBGLOG("Current data buffer offset: 0x%X", command->currentDataOffset);
  return kIOReturnSuccess;
}

IOReturn EmeraldSDHCBlockStorageDevice::executeAsyncDataTransfer(EmeraldSDHCCommand *command, UInt16 interruptStatus) {
  IOReturn status;
  UInt32   data32;
  UInt32   numSegments = 1;

  IODMACommand::Segment32 segment;

  if ((interruptStatus & (kSDHCRegNormalIntStatusTransferComplete | kSDHCRegNormalIntStatusDMAInterrupt | kSDHCRegNormalIntStatusBufferReadReady | kSDHCRegNormalIntStatusBufferWriteReady)) == 0 && (command->cmdEntry->flags & kSDACommandFlagsIgnoreTransferComplete) == 0) {
    EMIODBGLOG("No data ready yet, breaking out");
    return kIOReturnSuccess;
  }

  EMIODBGLOG("Data transfer, currently %u bytes out of %u", command->currentDataOffset,
             command->blockCount * command->blockSize);
  if (command->currentDataOffset >= (command->blockCount * command->blockSize)) {
    EMIODBGLOG("Data transfer complete");

    if ((interruptStatus & kSDHCRegNormalIntStatusTransferComplete) == 0 && (command->cmdEntry->flags & kSDACommandFlagsIgnoreTransferComplete) == 0) {
      EMIODBGLOG("Data transfer complete no xfer done bit");
      return kIOReturnSuccess;
    }

    command->state = kEmeraldSDHCStateCommandSent;
    return kIOReturnSuccess;
  }

  //
  // Read/write in PIO mode.
  // When the next block is ready to be read or written, a buffer read/write ready interrupt will be raised.
  //
  if (_hcTransferType == kSDATransferTypePIO) {
    //
    // Process next block. Only 32 bits can be read/written at a time.
    //
    for (int i = 0; i < (command->blockSize / sizeof (data32)); i++) {
      if (command->cmdEntry->dataDirection == kSDADataDirectionCardToHost) {
        data32 = _cardSlot->readReg32(kSDHCRegBufferDataPort);
        command->memoryDescriptor->writeBytes(command->memoryDescriptorOffset + command->currentDataOffset, &data32, sizeof (data32));
      } else {
        command->memoryDescriptor->readBytes(command->memoryDescriptorOffset + command->currentDataOffset, &data32, sizeof (data32));
        _cardSlot->writeReg32(kSDHCRegBufferDataPort, data32);
      }
      command->currentDataOffset += sizeof (data32);
    }
    EMIODBGLOG("Transferred %u bytes total in PIO mode", command->currentDataOffset);

  //
  // Read/write in SDMA mode.
  // When the next block is ready to be read or written, a DMA interrupt will be raised.
  //
  } else if (_hcTransferType == kSDATransferTypeSDMA) {
    status = command->dmaCommand->gen32IOVMSegments(&command->currentDataOffset, &segment, &numSegments);
    if (status != kIOReturnSuccess) {
      EMDBGLOG("Failed to generate SDMA segment with status 0x%X", status);
      return status;
    }
    _cardSlot->writeReg32(kSDHCRegSDMA, segment.fIOVMAddr);
    EMIODBGLOG("Processed next SDMA block, current data offset 0x%X", command->currentDataOffset);
  }

  return kIOReturnSuccess;
}

bool EmeraldSDHCBlockStorageDevice::selectCardAsync(bool selectCard) {
  const SDACommandTableEntry *cmdEntry = nullptr;

  EMIODBGLOG("Card selection state will be %u", selectCard);
  if (isSDCard()) {
    cmdEntry = &SDCommandTable[kSDCommandSelectDeselectCard];
  } else {
    cmdEntry = &MMCCommandTable[kMMCCommandSelectDeselectCard];
  }

  return sendAsyncCommand(cmdEntry, selectCard ? _cardAddress << kSDARelativeAddressShift : 0);
}

bool EmeraldSDHCBlockStorageDevice::sendAsyncCommand(const SDACommandTableEntry *cmdEntry, UInt32 arg) {
  //
  // Get command index based on command type.
  //
  UInt8 cmdIndex = (UInt8)(cmdEntry->command & ~kSDAppCommandFlag);
  UInt8 cmdResponse = cmdEntry->response;
  
  int ddd =0;
  if (cmdIndex != kSDCommandGoIdleState) {
    while (_cardSlot->readReg32(kSDHCRegPresentState) & (kSDHCRegPresentStateCardCmdInhibit | kSDHCRegPresentStateCardDatInhibit)) {
      IODelay(1);
      ddd++;

      if (ddd > 5000000) {
       // panic("Timeout waiting for CMD inhibit! state %X int %X err %X", _cardSlot->readReg32(kSDHCRegPresentState), _cardSlot->readReg16(kSDHCRegNormalIntStatus), _cardSlot->readReg16(kSDHCRegErrorIntStatus));
        EMDBGLOG("Timeout waiting for CMD inhibit! state %X int %X err %X adma err %x", _cardSlot->readReg32(kSDHCRegPresentState), _cardSlot->readReg16(kSDHCRegNormalIntStatus), _cardSlot->readReg16(kSDHCRegErrorIntStatus), _cardSlot->readReg16(kSDHCRegADMAErrorStatus));
        //return true;
        while (true);
      }
    }
  }

  //
  // Reset bits.
  //
  _cardSlot->writeReg16(kSDHCRegErrorIntStatus, -1);
  _cardSlot->writeReg16(kSDHCRegNormalIntStatus, -1);

  //
  // If this is a selection command, and cards are being deselected, there will be no response.
  //
  if (cmdIndex == kSDCommandSelectDeselectCard && arg == 0) {
    cmdResponse = kSDAResponseTypeR0;
  }

  //
  // Send command and argument.
  //
  _cardSlot->writeReg32(kSDHCRegArgument, arg);
  _cardSlot->writeReg16(kSDHCRegCommand, (cmdIndex << 8) | cmdResponse);
  EMIODBGLOG("Sent %s command %u with arg 0x%X (response bits 0x%X)", isSDCard() ? "SD" : "MMC", cmdIndex, arg, cmdResponse);
  //return true;// !((cmdResponse == kSDAResponseTypeR0) || (cmdEntry->flags & kSDACommandFlagsIgnoreCmdComplete));
  return (cmdEntry->flags & kSDACommandFlagsIgnoreCmdComplete) == 0;
}

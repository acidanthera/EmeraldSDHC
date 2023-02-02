//
//  SDMisc.hpp
//  SD misc definitions
//
//  Copyright © 2021-2023 Goldfish64. All rights reserved.
//

#ifndef SDMisc_hpp
#define SDMisc_hpp

#include <IOKit/IOLib.h>
#include "SDRegs.hpp"

#define kHz   1000
#define MHz   (kHz * 1000)
#define kByte 1024

#define kSDATimeout_120sec              120000
#define kSDATimeout_30sec               30000
#define kSDATimeout_10sec               10000
#define kSDATimeout_2sec                2000

#define kSDAInitSpeedClock400kHz        (400 * kHz)
#define kSDANormalSpeedClock20MHz       (20 * MHz)
#define kSDANormalSpeedClock25MHz       (25 * MHz)
#define kSDANormalSpeedClock26MHz       (26 * MHz)
#define kSDAHighSpeedClock25MHz         (50 * MHz)

#define kSDAPowerStateOff   0
#define kSDAPowerStateOn    1

// Maximum possible length including null terminator.
#define kSDAProductNameLength     7
#define kSDASerialNumLength       12
#define kSDARevisionLength        2
#define kSDMMCCIDLength           128

#define kSDABlockSize       512

#define kSDACardAddress     0x1

#define kSDACardSlotNumberKey "Slot"
#define kSDAEmbeddedSlotKey   "IsEmbedded"

//
// Used for vendor string lookups.
//
typedef struct {
  UInt8      manufacturerId;
  const char *name;
} SDAVendor;

typedef struct {
  union {
    UInt8  bytes[128];
    UInt32 bytes4[4];
    UInt64 bytes8[2];
  };
} SDACommandResponse;

typedef enum {
  kSDATransferTypePIO,
  kSDATransferTypeSDMA,
  kSDATransferTypeADMA2
} SDATransferType;

#define kSDASDMASegmentSize       0x1000
#define kSDASDMASegmentAlignment  kSDASDMASegmentSize

#define kSDAMaxBlocksPerTransfer  61440

#define kSDANumADMA2Descriptors   ((kSDAMaxBlocksPerTransfer * kSDABlockSize) / PAGE_SIZE)

#define kSDAMaskTimeout           100000

#define kSDAInitialCommandPoolSize 10

typedef enum {
  kSDABusWidth1,
  kSDABusWidth4,
  kSDABusWidth8
} SDABusWidth;

typedef enum {
  // No data transfer.
  kSDADataDirectionNone,
  // Data sent from card to host.
  kSDADataDirectionCardToHost,
  // Data sent to card from host.
  kSDADataDirectionHostToCard,
  // Data may travel either direction, check IOMemoryDescriptor's direction.
  kSDADataDirectionChkBuffer
} SDADataDirection;

typedef enum : UInt8 {
  kSDACommandFlagsNeedsSelection            = BIT0,
  kSDACommandFlagsIsApplicationCmd          = BIT1,
  kSDACommandFlagsIgnoreCmdComplete         = BIT2,
  kSDACommandFlagsIgnoreTransferComplete    = BIT3,
  kSDACommandFlagsIgnoreSelectionState      = BIT4
} SDACommandFlags;

typedef struct {
  // Command index.
  UInt32            command;
  // Response type.
  SDAResponseType   response;
  // Data direction if any data is transferred.
  SDADataDirection  dataDirection;
  // Special command flags.
  UInt8             flags;
  // Additional transfer mode register flags.
  UInt8             hostFlags;
} SDACommandTableEntry;

//
// Card types.
//
typedef enum {
  // SD card compliant with physical layer version 2.00.
  kSDACardTypeSD_200,
  // Legacy SD card.
  kSDACardTypeSD_Legacy,
  // MMC/eMMC card.
  kSDACardTypeMMC
} SDACardType;

//
// Debug printing functions.
//
#if DEBUG
//
// Debug print function.
//
inline void logPrint(const char *className, const char *funcName, bool hasSlotId, UInt32 slotId, const char *format, va_list va) {
  char tmp[256];
  tmp[0] = '\0';
  vsnprintf(tmp, sizeof (tmp), format, va);
  
  if (hasSlotId) {
    IOLog("%s(%u)::%s(): %s\n", className, (unsigned int) slotId, funcName, tmp);
  } else {
    IOLog("%s::%s(): %s\n", className, funcName, tmp);
  }
}

//
// Log functions for host controller.
//
#define EMDeclareLogFunctionsHC(a) \
  private: \
  bool _debugEnabled = false; \
  inline void EMCheckDebugArgs() { \
    int val[16]; \
    _debugEnabled = PE_parse_boot_argn("-emsdhcdbg", val, sizeof (val)); \
  } \
  inline void EMDBGLOG_PRINT(const char *func, const char *str, ...) const { \
    if (this->_debugEnabled) { \
      va_list args; \
      va_start(args, str); \
      logPrint(this->getMetaClass()->getClassName(), func, false, 0, str, args); \
      va_end(args); \
    } \
  } \
    \
  inline void EMSYSLOG_PRINT(const char *func, const char *str, ...) const { \
    va_list args; \
    va_start(args, str); \
    logPrint(this->getMetaClass()->getClassName(), func, false, 0, str, args); \
    va_end(args); \
  } \
    \
  protected:

//
// Log functions for cards.
//
#define EMDeclareLogFunctionsCard(a) \
  private: \
  bool _debugEnabled = false; \
  bool _debugPackets = false; \
  inline void EMCheckDebugArgs() { \
    int val[16]; \
    _debugEnabled = PE_parse_boot_argn("-emsdhcdbg", val, sizeof (val)); \
    _debugPackets = PE_parse_boot_argn("-emsdhcdbgio", val, sizeof (val)); \
  } \
  inline void EMDBGLOG_PRINT(const char *func, const char *str, ...) const { \
    if (this->_debugEnabled) { \
      va_list args; \
      va_start(args, str); \
      logPrint(this->getMetaClass()->getClassName(), func, _cardSlot != nullptr, _cardSlot != nullptr ? _cardSlot->getCardSlotId() : 0, str, args); \
      va_end(args); \
    } \
  } \
    \
  inline void EMIODBGLOG_PRINT(const char *func, const char *str, ...) const { \
  if (this->_debugPackets) { \
    va_list args; \
    va_start(args, str); \
    logPrint(this->getMetaClass()->getClassName(), func, _cardSlot != nullptr, _cardSlot != nullptr ? _cardSlot->getCardSlotId() : 0, str, args); \
    va_end(args); \
  } \
} \
  \
  inline void EMSYSLOG_PRINT(const char *func, const char *str, ...) const { \
    va_list args; \
    va_start(args, str); \
    logPrint(this->getMetaClass()->getClassName(), func, _cardSlot != nullptr, _cardSlot != nullptr ? _cardSlot->getCardSlotId() : 0, str, args); \
    va_end(args); \
  } \
    \
  protected:

//
// Common logging macros to inject function name.
//
#define EMDBGLOG(str, ...)    EMDBGLOG_PRINT(__FUNCTION__, str, ## __VA_ARGS__)
#define EMSYSLOG(str, ...)    EMSYSLOG_PRINT(__FUNCTION__, str, ## __VA_ARGS__)
#define EMIODBGLOG(str, ...)  EMIODBGLOG_PRINT(__FUNCTION__, str, ## __VA_ARGS__)

#else
//
// Release print function.
//
inline void logPrint(const char *className, bool hasSlotId, UInt32 slotId, const char *format, va_list va) {
  char tmp[256];
  tmp[0] = '\0';
  vsnprintf(tmp, sizeof (tmp), format, va);
  
  if (hasSlotId) {
    IOLog("%s(%u): %s\n", className, (unsigned int) slotId, tmp);
  } else {
    IOLog("%s: %s\n", className, tmp);
  }
}

//
// Log functions for host controller.
//
#define EMDeclareLogFunctionsHC(a) \
  private: \
  bool _debugEnabled = false; \
  inline void EMCheckDebugArgs() { } \
  inline void EMDBGLOG(const char *str, ...) const { } \
    \
  inline void EMSYSLOG(const char *str, ...) const { \
    va_list args; \
    va_start(args, str); \
    logPrint(this->getMetaClass()->getClassName(), false, 0, str, args); \
    va_end(args); \
  } \
    \
  protected:

//
// Log functions for cards.
//
#define EMDeclareLogFunctionsCard(a) \
  private: \
  bool _debugEnabled = false; \
  bool _debugPackets = false; \
  inline void EMCheckDebugArgs() { } \
  inline void EMDBGLOG(const char *str, ...) const { } \
  inline void EMIODBGLOG(const char *str, ...) const { } \
    \
  inline void EMSYSLOG(const char *str, ...) const { \
    va_list args; \
    va_start(args, str); \
    logPrint(this->getMetaClass()->getClassName(), true, 0, str, args); \
    va_end(args); \
  } \
    \
  protected:
#endif

#endif

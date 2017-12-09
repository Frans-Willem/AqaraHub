#ifndef _ZNP_SYSTEM_SYSTEM_H_
#define _ZNP_SYSTEM_SYSTEM_H_
#include <iostream>
#include <stlab/concurrency/future.hpp>
#include "znp/znp_sreq_handler.h"

namespace znp {
namespace system {
enum class SystemCommand : uint8_t {
  RESET = 0x00,
  PING = 0x01,
  VERSION = 0x02,
  SET_EXTADDR = 0x03,
  GET_EXTADDR = 0x04,
  RAM_READ = 0x05,
  RAM_WRITE = 0x06,
  OSAL_NV_ITEM_INIT = 0x07,
  OSAL_NV_READ = 0x08,
  OSAL_NV_WRITE = 0x09,
  OSAL_START_TIMER = 0x0A,
  OSAL_STOP_TIMER = 0x0B,
  RANDOM = 0x0C,
  ADC_READ = 0x0D,
  GPIO = 0x0E,
  STACK_TUNE = 0x0F,
  SET_TIME = 0x10,
  GET_TIME = 0x11,
  OSAL_NV_DELETE = 0x12,
  OSAL_NV_LENGTH = 0x13,
  TEST_RF = 0x40,
  TEST_LOOPBACK = 0x41,
  RESET_IND = 0x80,
  OSAL_TIMER_EXPIRED = 0x81,
};

enum class Capability : uint16_t {
  SYS = 0x0001,
  MAC = 0x0002,
  NWK = 0x0004,
  AF = 0x0008,
  ZDO = 0x0010,
  SAPI = 0x0020,
  UTIL = 0x0040,
  DEBUG = 0x0080,
  APP = 0x0100,
  ZOAD = 0x1000
};

std::ostream& operator<<(std::ostream& stream, const Capability& cap);

enum class ResetReason : uint8_t { PowerUp = 0, External = 1, Watchdog = 2 };
std::ostream& operator<<(std::ostream& stream, const ResetReason& reason);

struct ResetInfo {
  ResetReason reason;
  uint8_t TransportRev;
  uint8_t ProductId;
  uint8_t MajorRel;
  uint8_t MinorRel;
  uint8_t HwRev;
};
std::ostream& operator<<(std::ostream& stream, const ResetInfo& info);

struct VersionInfo {
  uint8_t TransportRev;
  uint8_t Product;
  uint8_t MajorRel;
  uint8_t MinorRel;
  uint8_t MaintRel;
};

}  // namespace system
}  // namespace znp
#endif  // _ZNP_SYSTEM_SYSTEM_H_

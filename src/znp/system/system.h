#ifndef _ZNP_SYSTEM_SYSTEM_H_
#define _ZNP_SYSTEM_SYSTEM_H_
#include <iostream>

namespace znp {
namespace system {
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

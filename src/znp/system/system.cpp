#include "znp/system/system.h"
#include "asio_executor.h"

namespace znp {
namespace system {

std::ostream& operator<<(std::ostream& stream, const Capability& cap) {
  uint16_t unhandled = (uint16_t)cap;
  bool first = true;
  std::map<uint16_t, std::string> names{{(uint16_t)Capability::SYS, "SYS"},
                                        {(uint16_t)Capability::MAC, "MAC"},
                                        {(uint16_t)Capability::NWK, "NWK"},
                                        {(uint16_t)Capability::AF, "AF"},
                                        {(uint16_t)Capability::ZDO, "ZDO"},
                                        {(uint16_t)Capability::SAPI, "SAPI"},
                                        {(uint16_t)Capability::UTIL, "UTIL"},
                                        {(uint16_t)Capability::DEBUG, "DEBUG"},
                                        {(uint16_t)Capability::APP, "APP"},
                                        {(uint16_t)Capability::ZOAD, "ZOAD"}};
  for (auto name : names) {
    if (unhandled & name.first) {
      if (!first) stream << "|";
      first = false;
      stream << name.second;
      unhandled &= ~name.first;
    }
  }
  if (unhandled != 0) {
    if (!first) stream << "|";
    first = false;
    stream << std::hex << (unsigned int)unhandled;
  }
  return stream;
}

std::ostream& operator<<(std::ostream& stream, const ResetReason& reason) {
  switch (reason) {
    case ResetReason::PowerUp:
      return stream << "Power-up";
    case ResetReason::External:
      return stream << "External";
    case ResetReason::Watchdog:
      return stream << "Watch-dog";
    default:
      return stream << "UNK(" << (unsigned int)reason << ")";
  }
}

std::ostream& operator<<(std::ostream& stream, const ResetInfo& info) {
  return stream << "[" << info.reason << " " << (unsigned int)info.TransportRev
                << " " << (unsigned int)info.ProductId << " "
                << (unsigned int)info.MajorRel << "."
                << (unsigned int)info.MinorRel << "."
                << (unsigned int)info.HwRev << "]";
}

}  // namespace system
}  // namespace znp

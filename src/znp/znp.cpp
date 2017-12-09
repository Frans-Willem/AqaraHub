#include "znp/znp.h"

namespace znp {
std::ostream& operator<<(std::ostream& stream, const ZnpSubsystem& subsys) {
  switch (subsys) {
    case ZnpSubsystem::RPC_Error:
      return stream << "RPC_Error";
    case ZnpSubsystem::SYS:
      return stream << "SYS";
    case ZnpSubsystem::MAC:
      return stream << "MAC";
    case ZnpSubsystem::NWK:
      return stream << "NWK";
    case ZnpSubsystem::AF:
      return stream << "AF";
    case ZnpSubsystem::ZDO:
      return stream << "ZDO";
    case ZnpSubsystem::SAPI:
      return stream << "SAPI";
    case ZnpSubsystem::UTIL:
      return stream << "UTIL";
    case ZnpSubsystem::DEBUG:
      return stream << "DEBUG";
    case ZnpSubsystem::APP:
      return stream << "APP";
    default:
      return stream << "UNK(" << (int)subsys << ")";
  }
}

std::ostream& operator<<(std::ostream& stream, const ZnpCommandType& type) {
  switch (type) {
    case ZnpCommandType::POLL:
      return stream << "POLL";
    case ZnpCommandType::SREQ:
      return stream << "SREQ";
    case ZnpCommandType::AREQ:
      return stream << "AREQ";
    case ZnpCommandType::SRSP:
      return stream << "SRSP";
    default:
      return stream << "UNK(" << (int)type << ")";
  }
}
}  // namespace znp

#ifndef _ZNP_H_
#define _ZNP_H_
#include <iostream>

namespace znp {
enum class ZnpCommandType { POLL = 0, SREQ = 2, AREQ = 4, SRSP = 6 };

enum class ZnpSubsystem {
  RPC_Error = 0,
  SYS = 1,
  MAC = 2,
  NWK = 3,
  AF = 4,
  ZDO = 5,
  SAPI = 6,
  UTIL = 7,
  DEBUG = 8,
  APP = 9
};

std::ostream& operator<<(std::ostream& stream, const ZnpSubsystem& subsys);
std::ostream& operator<<(std::ostream& stream, const ZnpCommandType& type);
}  // namespace znp
#endif  //_ZNP_H_

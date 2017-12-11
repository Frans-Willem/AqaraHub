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

enum class ZnpStatus : uint8_t {
	Success = 0x00,
	Failure = 0x01,
	InvalidParameter = 0x02,
	MemError = 0x03,
	BufferFull = 0x11
};

typedef uint64_t IEEEAddress;
typedef uint16_t ShortAddress;

enum class AddrMode : uint8_t {
	NotPresent = 0,
	Group = 1,
	ShortAddress = 2,
	IEEEAddress = 3,
	Broadcast = 0xFF
};

std::ostream& operator<<(std::ostream& stream, const ZnpSubsystem& subsys);
std::ostream& operator<<(std::ostream& stream, const ZnpCommandType& type);
}  // namespace znp
#endif  //_ZNP_H_

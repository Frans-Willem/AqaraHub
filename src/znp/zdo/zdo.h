#ifndef _ZNP_ZDO_ZDO_H_
#define _ZNP_ZDO_ZDO_H_
#include <cstdint>
#include <iostream>

namespace znp {
namespace zdo {

enum StartupFromAppResponse : uint8_t {
	Restored = 0,
	New = 1,
	Leave = 2
};
}  // namespace zdo
}  // namespace znp
#endif  //_ZNP_ZDO_ZDO_H_

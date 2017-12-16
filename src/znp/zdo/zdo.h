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
enum DeviceState : uint8_t {
	HOLD = 0,
	INIT = 1,
	NWK_DISC = 2,
	NWK_JOINING = 3,
	NWK_REJOIN = 4,
	END_DEVICE_UNAUTH = 5,
	END_DEVICE = 6,
	ROUTER = 7,
	COORD_STARTING = 8,
	ZB_COORD = 9,
	NWK_ORPHAN = 10
};
}  // namespace zdo
}  // namespace znp
#endif  //_ZNP_ZDO_ZDO_H_

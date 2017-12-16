#ifndef _ZNP_SIMPLEAPI_SIMPLEAPI_H_
#define _ZNP_SIMPLEAPI_SIMPLEAPI_H_
#include <cstdint>
#include <iostream>

namespace znp {
namespace simpleapi {

enum class ConfigurationOption : uint16_t {
  STARTUP_OPTION = 0x0003,
  POLL_RATE = 0x0024,
  QUEUED_POLL_RATE = 0x0025,
  RESPONSE_POLL_RATE = 0x026,
  POLL_FAILURE_RETRIES = 0x0029,
  INDIRECT_MSG_TIMEOUT = 0x002B,
  ROUTE_EXPIRY_TIME = 0x002C,
  EXTENDED_PAN_ID = 0x002D,
  BCAST_RETRIES = 0x002E,
  PASSIVE_ACK_TIMEOUT = 0x002F,
  BCAST_DELIVERY_TIME = 0x0030,
  APS_FRAME_RETRIES = 0x0043,
  APS_ACK_WAIT_DURATION = 0x0044,
  BINDING_TIME = 0x0046,
  PRECFGKEY = 0x0062,
  PRECFGKEYS_ENABLE = 0x0063,
  SECURITY_MODE = 0x0064,
  USERDESC = 0x0081,
  PANID = 0x0083,
  CHANLIST = 0x0084,
  LOGICAL_TYPE = 0x0087,
  ZDO_DIRECT_CB = 0x008F
};

enum class LogicalType : uint8_t {
	Coordinator = 0,
	Router = 1,
	EndDevice = 2
};

enum class StartupOption : uint8_t {
	ClearConfig = 1,
	ClearState = 2
};

inline StartupOption operator|(StartupOption a, StartupOption b) {
	return (StartupOption)(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

enum class DeviceInfo : uint8_t {
	DeviceState = 0,
	DeviceIEEEAddress = 1,
	DeviceShortAddress = 2,
	ParentDeviceShortAddress = 3,
	ParentDeviceIEEEAddress = 4,
	Channel = 5,
	PanId = 6,
	ExtendedPanId = 7
};

}  // namespace simpleapi
}  // namespace znp
#endif  // _ZNP_SIMPLEAPI_SIMPLEAPI_H_

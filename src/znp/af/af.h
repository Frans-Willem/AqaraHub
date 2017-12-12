#ifndef _ZNP_AF_AF_H_
#define _ZNP_AF_AF_H_
#include <boost/fusion/adapted/struct/define_struct.hpp>
#include <boost/fusion/include/define_struct.hpp>
#include <cstdint>
#include <iostream>
#include <vector>

namespace znp {
namespace af {
enum class AfCommand : uint8_t {
  REGISTER = 0x00,
  DATA_REQUEST = 0x01,
  DATA_REQUEST_EXT = 0x02,
  DATA_REQUEST_SRC_RTG = 0x03,
  INTER_PAN_CTL = 0x10,
  DATA_STORE = 0x11,
  DATA_RETRIEVE = 0x12,
  APSF_CONFIG_SET = 0x13,
  DATA_CONFIRM = 0x80,
  REFLECT_ERROR = 0x83,
  INCOMING_MSG = 0x81,
  INCOMING_MSG_EXT = 0x82
};

std::ostream& operator<<(std::ostream& stream, AfCommand command);
enum class Latency : uint8_t {
  NoLatency = 0,
  FastBeacons = 1,
  SlowBeacons = 2
};

}  // namespace af
}  // namespace znp

// Define this as a boost fusion structure so we can automatically generate
// Encode/Decode pairs.
BOOST_FUSION_DEFINE_STRUCT(
    (znp)(af), IncomingMsg,
    (uint16_t, GroupId)(uint16_t, ClusterId)(uint16_t, SrcAddr)(
        uint8_t, SrcEndpoint)(uint8_t, DstEndpoint)(uint8_t, WasBroadcast)(
        uint8_t, LinkQuality)(uint8_t, SecurityUse)(uint32_t, TimeStamp)(
        uint8_t, TransSeqNumber)(std::vector<uint8_t>, Data))
#endif  // _ZNP_AF_AF_H_

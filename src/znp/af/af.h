#ifndef _ZNP_AF_AF_H_
#define _ZNP_AF_AF_H_
#include <boost/fusion/adapted/struct/define_struct.hpp>
#include <boost/fusion/include/define_struct.hpp>
#include <cstdint>
#include <iostream>
#include <vector>

namespace znp {
namespace af {

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

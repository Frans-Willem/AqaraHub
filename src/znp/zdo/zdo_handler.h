#ifndef _ZNP_ZDO_ZDO_HANDLER_H_
#define _ZNP_ZDO_ZDO_HANDLER_H_
#include <set>
#include <stlab/concurrency/future.hpp>
#include "znp/zdo/zdo.h"
#include "znp/znp_api.h"
#include "znp/znp_sreq_handler.h"

namespace znp {
namespace zdo {
class ZdoHandler {
 public:
  ZdoHandler(std::shared_ptr<ZnpPort> port,
             std::shared_ptr<ZnpSreqHandler> sreq_handler,
             std::shared_ptr<ZnpApi> api);
  ~ZdoHandler() = default;
  stlab::future<StartupFromAppResponse> StartupFromApp(uint16_t start_delay);
  stlab::future<DeviceState> WaitForState(std::set<DeviceState> end_states,
                                          std::set<DeviceState> allowed_states);
  stlab::future<uint16_t> PermitJoin(AddrMode addr_mode, uint16_t dst_address,
                                     uint8_t duration, uint8_t tc_significance);
 private:
	 std::shared_ptr<ZnpPort> port_;
	 std::shared_ptr<ZnpSreqHandler> sreq_handler_;
         std::shared_ptr<ZnpApi> api_;
};
}  // namespace zdo
}  // namespace znp
#endif  // _ZNP_ZDO_ZDO_HANDLER_H_

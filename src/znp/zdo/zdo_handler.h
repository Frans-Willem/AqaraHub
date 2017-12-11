#ifndef _ZNP_ZDO_ZDO_HANDLER_H_
#define _ZNP_ZDO_ZDO_HANDLER_H_
#include "znp/zdo/zdo.h"
#include "znp/znp_sreq_handler.h"
#include "znp/simpleapi/simpleapi_handler.h"
#include <stlab/concurrency/future.hpp>
#include <set>

namespace znp {
namespace zdo {
class ZdoHandler {
 public:
	 ZdoHandler(std::shared_ptr<ZnpPort> port, std::shared_ptr<ZnpSreqHandler> sreq_handler, std::shared_ptr<simpleapi::SimpleAPIHandler> simpleapi_handler);
	 ~ZdoHandler() = default;
	 stlab::future<StartupFromAppResponse> StartupFromApp(uint16_t start_delay);
	 stlab::future<DeviceState> WaitForState(std::set<DeviceState> end_states, std::set<DeviceState> allowed_states);
 private:
	 std::shared_ptr<ZnpPort> port_;
	 std::shared_ptr<ZnpSreqHandler> sreq_handler_;
	 std::shared_ptr<simpleapi::SimpleAPIHandler> simpleapi_handler_;
};
}  // namespace zdo
}  // namespace znp
#endif  // _ZNP_ZDO_ZDO_HANDLER_H_

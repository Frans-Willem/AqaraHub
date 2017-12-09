#ifndef _ZNP_SIMPLEAPI_SIMPLEAPI_HANDLER_H_
#define _ZNP_SIMPLEAPI_SIMPLEAPI_HANDLER_H_
#include "znp/simpleapi/simpleapi.h"
#include "znp/znp_sreq_handler.h"
#include <stlab/concurrency/future.hpp>

namespace znp {
namespace simpleapi {
class SimpleAPIHandler {
 public:
  SimpleAPIHandler(std::shared_ptr<ZnpSreqHandler> sreq_handler);
  ~SimpleAPIHandler() = default;

  stlab::future<std::vector<uint8_t>> ReadRawConfiguration(ConfigurationOption option);
  stlab::future<void> WriteRawConfiguration(ConfigurationOption option, std::vector<uint8_t> data);
  stlab::future<void> WriteStartupOption(StartupOption option);
  stlab::future<void> PermitJoiningRequest(uint16_t destination, uint8_t timeout);

 private:
  std::shared_ptr<ZnpSreqHandler> sreq_handler_;
};
}  // namespace simpleapi
}  // namespace znp
#endif  // _ZNP_SIMPLEAPI_SIMPLEAPI_HANDLER_H_

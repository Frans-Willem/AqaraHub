#ifndef _ZNP_SIMPLEAPI_SIMPLEAPI_HANDLER_H_
#define _ZNP_SIMPLEAPI_SIMPLEAPI_HANDLER_H_
#include <stlab/concurrency/future.hpp>
#include "znp/encoding.h"
#include "znp/simpleapi/simpleapi.h"
#include "znp/zdo/zdo.h"
#include "znp/znp_sreq_handler.h"

namespace znp {
namespace simpleapi {
class SimpleAPIHandler {
 public:
  SimpleAPIHandler(std::shared_ptr<ZnpSreqHandler> sreq_handler);
  ~SimpleAPIHandler() = default;

  // Configuration getters/setters
  template <typename T>
  stlab::future<T> ReadConfiguration(ConfigurationOption option) {
    return ReadRawConfiguration(option, znp::EncodedSize<T>())
        .then(znp::Decode<T>);
  }
  template <typename T>
  stlab::future<void> WriteConfiguration(ConfigurationOption option,
                                         const T& value) {
    return WriteRawConfiguration(option, znp::Encode(value));
  }
  // Configuration getters/setters for certain enum configurations
  stlab::future<void> WriteStartupOption(StartupOption startup_option);
  stlab::future<void> WriteLogicalType(LogicalType logical_type);
  // Other SimpleAPI commands
  stlab::future<void> PermitJoiningRequest(uint16_t destination,
                                           uint8_t timeout);
  template <typename T>
  stlab::future<T> GetDeviceInfo(DeviceInfo info) {
    // Use DecodePartial as GetDeviceInfo always returns 8 bytes, even if less
    // are used.
    return GetRawDeviceInfo(info).then(znp::DecodePartial<T>);
  }
  stlab::future<zdo::DeviceState> GetDeviceState();
  stlab::future<IEEEAddress> GetDeviceIEEEAddress();
  stlab::future<ShortAddress> GetDeviceShortAddress();

 private:
  std::shared_ptr<ZnpSreqHandler> sreq_handler_;
  // Private getters/setters for raw configuration
  stlab::future<std::vector<uint8_t>> ReadRawConfiguration(
      ConfigurationOption option);
  stlab::future<void> WriteRawConfiguration(ConfigurationOption option,
                                            std::vector<uint8_t> data);
  stlab::future<std::vector<uint8_t>> GetRawDeviceInfo(DeviceInfo info);
};
}  // namespace simpleapi
}  // namespace znp
#endif  // _ZNP_SIMPLEAPI_SIMPLEAPI_HANDLER_H_

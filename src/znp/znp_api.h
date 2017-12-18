#ifndef _ZNP_API_H_
#define _ZNP_API_H_
#include <boost/signals2/signal.hpp>
#include <map>
#include <queue>
#include <stlab/concurrency/future.hpp>
#include <vector>
#include "logging.h"
#include "polyfill/apply.h"
#include "znp/encoding.h"
#include "znp/znp.h"
#include "znp/znp_raw_interface.h"

namespace znp {
class ZnpApi {
 public:
  ZnpApi(std::shared_ptr<ZnpRawInterface> interface);
  ~ZnpApi() = default;

  // SYS commands
  stlab::future<ResetInfo> SysReset(bool soft_reset);
  stlab::future<Capability> SysPing();

  // SYS events
  boost::signals2::signal<void(ResetInfo)> on_reset_;

  // AF commands
  // AF events

  // ZDO commands
  // ZDO events

  // SAPI commands
  stlab::future<std::vector<uint8_t>> SapiReadConfigurationRaw(
      ConfigurationOption option);
  template <ConfigurationOption O>
  stlab::future<typename ConfigurationOptionInfo<O>::Type>
  SapiReadConfiguration() {
    return SapiReadConfigurationRaw(O).then(
        &znp::Decode<ConfigurationOptionInfo<O>::Type>);
  }
  stlab::future<void> SapiWriteConfigurationRaw(
      ConfigurationOption option, const std::vector<uint8_t>& value);
  template <ConfigurationOption O>
  stlab::future<void> SapiWriteConfiguration(
      const typename ConfigurationOptionInfo<O>::Type& value) {
    return SapiWriteConfigurationRaw(O, znp::Encode(value));
  }

  stlab::future<std::vector<uint8_t>> SapiGetDeviceInfoRaw(DeviceInfo info);
  template <DeviceInfo I>
  stlab::future<typename DeviceInfoInfo<I>::Type> SapiGetDeviceInfo() {
    // DecodePartial because GetDeviceInfo will always return 8 bytes, even if
    // less are needed.
    return SapiGetDeviceInfoRaw(I).then(
        &znp::DecodePartial<typename DeviceInfoInfo<I>::Type>);
  }

  // SAPI events

 private:
  std::shared_ptr<ZnpRawInterface> raw_;
  boost::signals2::scoped_connection on_frame_connection_;

  typedef std::function<void(std::exception_ptr, std::vector<uint8_t>)>
      QueueCallback;
  std::map<std::pair<ZnpCommandType, ZnpCommand>, std::queue<QueueCallback>>
      queue_;

  typedef std::function<void(const std::vector<uint8_t>&)> DefaultHandler;
  std::map<std::pair<ZnpCommandType, ZnpCommand>, std::list<DefaultHandler>>
      handlers_;

  void OnFrame(ZnpCommandType type, ZnpCommand command,
               const std::vector<uint8_t>& payload);
  stlab::future<std::vector<uint8_t>> WaitFor(ZnpCommandType type,
                                              ZnpCommand command);
  stlab::future<std::vector<uint8_t>> RawSReq(
      ZnpCommand command, const std::vector<uint8_t>& payload);
  static std::vector<uint8_t> CheckStatus(const std::vector<uint8_t>& response);
  static void CheckOnlyStatus(const std::vector<uint8_t>& response);

  template <typename... Args>
  void AddSimpleEventHandler(ZnpCommandType type, ZnpCommand command,
                             boost::signals2::signal<void(Args...)>& signal,
                             bool allow_partial) {
    auto& handler_list = handlers_[std::make_pair(type, command)];
    handler_list.push_back(
        [&signal, allow_partial](const std::vector<uint8_t>& data) {
          std::tuple<Args...> arguments;
          try {
            if (allow_partial) {
              arguments = znp::DecodePartial<std::tuple<Args...>>(data);
            } else {
              arguments = znp::Decode<std::tuple<Args...>>(data);
            }
          } catch (const std::exception& exc) {
            LOG("ZnpApi", warning)
                << "Exception while decoding event: " << exc.what();
            return;
          }
          polyfill::apply(signal, arguments);
        });
  }
};
}  // namespace znp
#endif  //_ZNP_API_H_

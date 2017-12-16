#ifndef _ZNP_API_H_
#define _ZNP_API_H_
#include <boost/signals2/signal.hpp>
#include <map>
#include <queue>
#include <stlab/concurrency/future.hpp>
#include <vector>
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

 private:
  std::shared_ptr<ZnpRawInterface> raw_;
  boost::signals2::scoped_connection on_frame_connection_;

  typedef std::function<void(std::exception_ptr, std::vector<uint8_t>)>
      QueueCallback;
  std::map<std::pair<ZnpCommandType, ZnpCommand>, std::queue<QueueCallback>>
      queue_;

  void OnFrame(ZnpCommandType type, ZnpCommand command,
               const std::vector<uint8_t>& payload);
  stlab::future<std::vector<uint8_t>> WaitFor(ZnpCommandType type,
                                              ZnpCommand command);
  stlab::future<std::vector<uint8_t>> RawSReq(
      ZnpCommand command, const std::vector<uint8_t>& payload);
  static std::vector<uint8_t> CheckStatus(const std::vector<uint8_t>& response);
  static void CheckOnlyStatus(const std::vector<uint8_t>& response);
};
}  // namespace znp
#endif  //_ZNP_API_H_

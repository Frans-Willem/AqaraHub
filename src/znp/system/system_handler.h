#ifndef _ZNP_SYSTEM_SYSTEM_HANDLER_H_
#define _ZNP_SYSTEM_SYSTEM_HANDLER_H_
#include "znp/system/system.h"

namespace znp {
namespace system {
class SystemHandler {
 public:
  SystemHandler(boost::asio::io_service& io_service,
                std::shared_ptr<ZnpPort> port,
                std::shared_ptr<ZnpSreqHandler> sreq_handler);
  ~SystemHandler() = default;
  stlab::future<ResetInfo> Reset(bool soft_reset);
  stlab::future<Capability> Ping();

  boost::signals2::signal<void(ResetInfo)> on_reset_;

 private:
  boost::asio::io_service& io_service_;
  std::shared_ptr<ZnpPort> port_;
  std::shared_ptr<ZnpSreqHandler> sreq_handler_;
  boost::signals2::scoped_connection on_frame_connection_;

  void OnFrame(ZnpCommandType type, ZnpSubsystem subsys, uint8_t command,
               boost::asio::const_buffer payload);
};
}  // namespace system
}  // namespace znp
#endif  // _ZNP_SYSTEM_SYSTEM_HANDLER_H_

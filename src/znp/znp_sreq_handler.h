#ifndef _ZNP_SREQ_HANDLER_H_
#define _ZNP_SREQ_HANDLER_H_
#include <boost/signals2/signal.hpp>
#include <stlab/concurrency/future.hpp>
#include "znp/znp.h"
#include "znp/znp_port.h"

namespace znp {
class ZnpSreqHandler {
 public:
  ZnpSreqHandler(boost::asio::io_service& io_service,
                 std::shared_ptr<ZnpPort> port);
  ~ZnpSreqHandler() = default;

  stlab::future<std::vector<uint8_t>> SReq(ZnpSubsystem subsys, uint8_t command,
                                           const std::vector<uint8_t>& payload);

 private:
  boost::asio::io_service& io_service_;
  std::shared_ptr<ZnpPort> port_;
  boost::signals2::scoped_connection on_frame_connection_;

  typedef std::pair<ZnpSubsystem, uint8_t> SreqIdentifier;
  typedef std::function<void(std::exception_ptr, std::vector<uint8_t>)>
      SreqCallback;
  std::map<SreqIdentifier, std::queue<SreqCallback>> srsp_queue_;

  void OnFrame(ZnpCommandType type, ZnpSubsystem subsys, uint8_t command,
               boost::asio::const_buffer payload);
};
}  // namespace znp
#endif  //_ZNP_SREQ_HANDLER_H

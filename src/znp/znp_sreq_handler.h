#ifndef _ZNP_SREQ_HANDLER_H_
#define _ZNP_SREQ_HANDLER_H_
#include <boost/signals2/signal.hpp>
#include <stlab/concurrency/future.hpp>
#include "znp/znp.h"
#include "znp/znp_port.h"

namespace znp {
class ZnpSreqHandler {
 public:
  ZnpSreqHandler(std::shared_ptr<ZnpPort> port);
  ~ZnpSreqHandler() = default;

  /**
   * Sends a raw SREQ, and waits for a matching SRSP.
   * The raw payload of the SRSP is returned in the future.
   */
  stlab::future<std::vector<uint8_t>> SReq(ZnpSubsystem subsys, uint8_t command,
                                           const std::vector<uint8_t>& payload);
  /**
   * Similar to SReq, but the first byte of the response is checked against ZnpStatus::Success.
   * The raw payload, minus the first byte, is returned in the future.
   */
  stlab::future<std::vector<uint8_t>> SReqStatus(ZnpSubsystem subsys, uint8_t command, const std::vector<uint8_t>& payload);

  stlab::future<std::vector<uint8_t>> WaitForAReq(ZnpSubsystem subsys, uint8_t command);

 private:
  std::shared_ptr<ZnpPort> port_;
  boost::signals2::scoped_connection on_frame_connection_;

  typedef std::pair<ZnpSubsystem, uint8_t> SreqIdentifier;
  typedef std::function<void(std::exception_ptr, std::vector<uint8_t>)>
      SreqCallback;
  std::map<SreqIdentifier, std::queue<SreqCallback>> srsp_queue_;

  typedef std::pair<ZnpSubsystem, uint8_t> AreqIdentifier;
  typedef std::function<void(std::exception_ptr, std::vector<uint8_t>)> AreqCallback;
  std::map<AreqIdentifier, std::queue<AreqCallback>> areq_queue_;

  void OnFrame(ZnpCommandType type, ZnpSubsystem subsys, uint8_t command,
               boost::asio::const_buffer payload);
};
}  // namespace znp
#endif  //_ZNP_SREQ_HANDLER_H

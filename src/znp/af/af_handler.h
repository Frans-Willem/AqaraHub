#ifndef _ZNP_AF_AF_HANDLER_H_
#define _ZNP_AF_AF_HANDLER_H_
#include "znp/af/af.h"
#include "znp/znp_sreq_handler.h"

namespace znp {
namespace af {
class AfHandler {
 public:
  AfHandler(std::shared_ptr<ZnpPort> port,
            std::shared_ptr<ZnpSreqHandler> sreq_handler);
  ~AfHandler() = default;
  stlab::future<void> Register(uint8_t endpoint, uint16_t profile_id,
                               uint16_t device_id, uint8_t version,
                               Latency latency,
                               std::vector<uint16_t> input_clusters,
                               std::vector<uint16_t> output_clusters);

  boost::signals2::signal<void(const IncomingMsg&)> on_incoming_message_;

 private:
  std::shared_ptr<ZnpPort> port_;
  std::shared_ptr<ZnpSreqHandler> sreq_handler_;
  boost::signals2::scoped_connection on_frame_connection_;

  void OnFrame(ZnpCommandType cmdtype, ZnpSubsystem subsys, uint8_t command,
               const std::vector<uint8_t>& payload);
  void OnIncomingMsg(const std::vector<uint8_t>& payload);
};
}  // namespace af
}  // namespace znp
#endif  // _ZNP_AF_AF_HANDLER_H_

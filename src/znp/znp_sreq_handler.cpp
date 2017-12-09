#include "znp/znp_sreq_handler.h"
#include "asio_executor.h"

namespace znp {
ZnpSreqHandler::ZnpSreqHandler(boost::asio::io_service& io_service,
                               std::shared_ptr<ZnpPort> port)
    : io_service_(io_service),
      port_(port),
      on_frame_connection_(port_->on_frame_.connect(
          std::bind(&ZnpSreqHandler::OnFrame, this, std::placeholders::_1,
                    std::placeholders::_2, std::placeholders::_3,
                    std::placeholders::_4))) {}

stlab::future<std::vector<uint8_t>> ZnpSreqHandler::SReq(
    ZnpSubsystem subsys, uint8_t command, const std::vector<uint8_t>& payload) {
  auto package = stlab::package<std::vector<uint8_t>(std::exception_ptr,
                                                     std::vector<uint8_t>)>(
      AsioExecutor(io_service_),
      [](std::exception_ptr exc, std::vector<uint8_t> retval) {
        if (exc != nullptr) {
          std::rethrow_exception(exc);
        }
        return std::move(retval);
      });
  srsp_queue_[std::make_pair(subsys, command)].push(package.first);
  port_->SendFrame(ZnpCommandType::SREQ, subsys, (unsigned int)command,
                   boost::asio::const_buffer(payload.data(), payload.size()));
  return package.second;
}

void ZnpSreqHandler::OnFrame(ZnpCommandType type, ZnpSubsystem subsys,
                             uint8_t command,
                             boost::asio::const_buffer payload) {
  if (type != ZnpCommandType::SRSP) {
    return;
  }
  std::queue<SreqCallback>& callback_queue(
      srsp_queue_[std::make_pair(subsys, command)]);
  if (callback_queue.empty()) {
    std::cout << "SRSP received for an empty queue" << std::endl;
    return;
  }
  SreqCallback callback = std::move(callback_queue.front());
  callback_queue.pop();
  std::vector<uint8_t> payload_vector(boost::asio::buffer_size(payload));
  std::memcpy(&payload_vector[0],
              boost::asio::buffer_cast<const uint8_t*>(payload),
              boost::asio::buffer_size(payload));
  callback(nullptr, std::move(payload_vector));
}
}  // namespace znp

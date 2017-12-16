#include "znp/znp_sreq_handler.h"
#include <stlab/concurrency/immediate_executor.hpp>
#include "asio_executor.h"
#include "logging.h"

namespace znp {
ZnpSreqHandler::ZnpSreqHandler(std::shared_ptr<ZnpPort> port)
    : port_(port),
      on_frame_connection_(port_->on_frame_.connect(
          std::bind(&ZnpSreqHandler::OnFrame, this, std::placeholders::_1,
                    std::placeholders::_2, std::placeholders::_3))) {}

stlab::future<std::vector<uint8_t>> ZnpSreqHandler::SReq(
    ZnpCommand command, const std::vector<uint8_t>& payload) {
  auto package = stlab::package<std::vector<uint8_t>(std::exception_ptr,
                                                     std::vector<uint8_t>)>(
      stlab::immediate_executor,
      [](std::exception_ptr exc, std::vector<uint8_t> retval) {
        if (exc != nullptr) {
          std::rethrow_exception(exc);
        }
        return retval;
      });
  srsp_queue_[command].push(package.first);
  port_->SendFrame(ZnpCommandType::SREQ, command, payload);
  return package.second;
}

stlab::future<std::vector<uint8_t>> ZnpSreqHandler::SReqStatus(
    ZnpCommand command, const std::vector<uint8_t>& payload) {
  return SReq(command, payload).then([](const std::vector<uint8_t>& v) {
    if (v.size() < 1) {
      throw std::runtime_error("Status byte missing");
    }
    if ((ZnpStatus)v[0] != ZnpStatus::Success) {
      LOG("Main", warning) << "Status was not success :(";
      throw std::runtime_error("Status was not success");
    }

    return std::vector<uint8_t>(v.begin() + 1, v.end());
  });
}

stlab::future<std::vector<uint8_t>> ZnpSreqHandler::WaitForAReq(
    ZnpCommand command) {
  auto package = stlab::package<std::vector<uint8_t>(std::exception_ptr,
                                                     std::vector<uint8_t>)>(
      stlab::immediate_executor,
      [](std::exception_ptr exc, std::vector<uint8_t> retval) {
        if (exc != nullptr) {
          std::rethrow_exception(exc);
        }
        return retval;
      });
  areq_queue_[command].push(package.first);
  return package.second;
}

void ZnpSreqHandler::OnFrame(ZnpCommandType type, ZnpCommand command,
                             const std::vector<uint8_t>& payload) {
  if (type == ZnpCommandType::SRSP) {
    std::queue<SreqCallback>& callback_queue(srsp_queue_[command]);
    if (callback_queue.empty()) {
      LOG("ZnpSreqHandler", warning)
          << "SRSP received for an empty queue" << std::endl;
      return;
    }
    SreqCallback callback = std::move(callback_queue.front());
    callback_queue.pop();
    callback(nullptr, payload);
  } else if (type == ZnpCommandType::AREQ) {
    std::queue<AreqCallback>& callback_queue(areq_queue_[command]);
    if (callback_queue.empty()) {
      return;
    }
    AreqCallback callback = std::move(callback_queue.front());
    callback_queue.pop();
    callback(nullptr, payload);
  }
}
}  // namespace znp

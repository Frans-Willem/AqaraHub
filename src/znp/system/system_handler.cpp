#include "znp/system/system_handler.h"
#include "asio_executor.h"
#include "znp/encoding.h"

namespace znp {
namespace system {
SystemHandler::SystemHandler(boost::asio::io_service& io_service,
                             std::shared_ptr<ZnpPort> port,
                             std::shared_ptr<ZnpSreqHandler> sreq_handler)
    : io_service_(io_service),
      port_(port),
      sreq_handler_(sreq_handler),
      on_frame_connection_(port->on_frame_.connect(
          std::bind(&SystemHandler::OnFrame, this, std::placeholders::_1,
                    std::placeholders::_2, std::placeholders::_3,
                    std::placeholders::_4))) {}

stlab::future<ResetInfo> SystemHandler::Reset(bool soft_reset) {
  auto package = stlab::package<ResetInfo(ResetInfo)>(
      AsioExecutor(io_service_), [](ResetInfo info) { return info; });

  auto callback = package.first;
  on_reset_.connect_extended(
      [callback](const boost::signals2::connection connection, ResetInfo info) {
        connection.disconnect();
        callback(info);
      });

  port_->SendFrame(ZnpCommandType::AREQ, ZnpSubsystem::SYS, (uint8_t)SystemCommand::RESET, znp::Encode<uint8_t>(soft_reset ? 1 : 0));

  return package.second;
}

stlab::future<Capability> SystemHandler::Ping() {
  std::vector<uint8_t> empty;
  auto future = sreq_handler_->SReq(ZnpSubsystem::SYS,
                                    (uint8_t)SystemCommand::PING, empty);
  return future.then([](const std::vector<uint8_t>& payload) {
    if (payload.size() != 2) {
      throw std::runtime_error("Response to PING was not of expected size");
    }
    uint16_t capability = (payload[1] << 8) | payload[0];
    return (Capability)capability;
  });
}

void SystemHandler::OnFrame(ZnpCommandType type, ZnpSubsystem subsys,
                            uint8_t command,
                            const std::vector<uint8_t>& payload) {
  if (subsys != ZnpSubsystem::SYS) {
    return;
  }
  SystemCommand sys_cmd = (SystemCommand)command;
  if (sys_cmd == SystemCommand::RESET_IND) {
    if (payload.size() != 6) {
      std::cerr << "RESET_IND was not of expected size" << std::endl;
      return;
    }
    ResetInfo info;
    info.reason = (ResetReason)payload[0];
    info.TransportRev = payload[1];
    info.ProductId = payload[2];
    info.MajorRel = payload[3];
    info.MinorRel = payload[4];
    info.HwRev = payload[5];
    on_reset_(info);
  }
}
}  // namespace system
}  // namespace znp

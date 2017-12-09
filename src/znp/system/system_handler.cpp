#include "znp/system/system_handler.h"
#include "asio_executor.h"

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

  uint8_t reset_type = soft_reset ? 1 : 0;

  port_->SendFrame(ZnpCommandType::AREQ, ZnpSubsystem::SYS, (uint8_t)SystemCommand::RESET, boost::asio::const_buffer(&reset_type, 1));

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
                            boost::asio::const_buffer payload) {
  if (subsys != ZnpSubsystem::SYS) {
    return;
  }
  SystemCommand sys_cmd = (SystemCommand)command;
  if (sys_cmd == SystemCommand::RESET_IND) {
    if (boost::asio::buffer_size(payload) != 6) {
      std::cerr << "RESET_IND was not of expected size" << std::endl;
      return;
    }
    const uint8_t* payload_data =
        boost::asio::buffer_cast<const uint8_t*>(payload);
    ResetInfo info;
    info.reason = (ResetReason)payload_data[0];
    info.TransportRev = payload_data[1];
    info.ProductId = payload_data[2];
    info.MajorRel = payload_data[3];
    info.MinorRel = payload_data[4];
    info.HwRev = payload_data[5];
    on_reset_(info);
  }
}
}  // namespace system
}  // namespace znp

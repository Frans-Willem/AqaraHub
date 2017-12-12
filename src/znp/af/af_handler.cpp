#include "znp/af/af_handler.h"
#include <boost/fusion/sequence/io.hpp>
#include "logging.h"
#include "znp/encoding.h"

namespace znp {
namespace af {
AfHandler::AfHandler(std::shared_ptr<ZnpPort> port,
                     std::shared_ptr<ZnpSreqHandler> sreq_handler)
    : port_(port),
      sreq_handler_(sreq_handler),
      on_frame_connection_(port_->on_frame_.connect(
          std::bind(&AfHandler::OnFrame, this, std::placeholders::_1,
                    std::placeholders::_2, std::placeholders::_3,
                    std::placeholders::_4))) {}

stlab::future<void> AfHandler::Register(uint8_t endpoint, uint16_t profile_id,
                                        uint16_t device_id, uint8_t version,
                                        Latency latency,
                                        std::vector<uint16_t> input_clusters,
                                        std::vector<uint16_t> output_clusters) {
  return sreq_handler_
      ->SReqStatus(ZnpSubsystem::AF, (uint8_t)AfCommand::REGISTER,
                   znp::EncodeT(endpoint, profile_id, device_id, version,
                                latency, input_clusters, output_clusters))
      .then(znp::Decode<void>);
}

void AfHandler::OnFrame(ZnpCommandType cmdtype, ZnpSubsystem subsys,
                        uint8_t command, boost::asio::const_buffer payload) {
  if (cmdtype != ZnpCommandType::AREQ || subsys != ZnpSubsystem::AF) {
    return;
  }
  const uint8_t* buffer_start =
      boost::asio::buffer_cast<const uint8_t*>(payload);
  const uint8_t* buffer_end = &buffer_start[boost::asio::buffer_size(payload)];
  switch ((AfCommand)command) {
    case AfCommand::INCOMING_MSG:
      OnIncomingMsg(std::vector<uint8_t>(buffer_start, buffer_end));
      break;
    default:
      break;
  }
}

void AfHandler::OnIncomingMsg(const std::vector<uint8_t>& payload) {
  LOG("AfHandler", debug) << "OnIncomingMsg!";
  // There are always 3 more bytes following the AF_INCOMING_MSG command.
  // Examples found online also contain the extra 3 bytes.
  // I have no idea what they mean, unfortunately...
  auto msg = DecodePartial<IncomingMsg>(payload);
  on_incoming_message_(msg);
}

}  // namespace af
}  // namespace znp

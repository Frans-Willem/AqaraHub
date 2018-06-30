#include "zcl/zcl_endpoint.h"
#include <boost/log/utility/manipulators/dump.hpp>
#include "logging.h"
#include "xiaomi/ff01_attribute.h"
#include "zcl/encoding.h"

namespace zcl {
ZclEndpoint::ZclEndpoint(std::shared_ptr<znp::ZnpApi> znp_api, uint8_t endpoint)
    : znp_api_(znp_api), endpoint_(endpoint) {}

stlab::future<std::shared_ptr<ZclEndpoint>> ZclEndpoint::Create(
    std::shared_ptr<znp::ZnpApi> znp_api, uint8_t endpoint, uint16_t profile_id,
    uint16_t device_id, uint8_t version, znp::Latency latency,
    std::vector<uint16_t> input_clusters,
    std::vector<uint16_t> output_clusters) {
  return znp_api
      ->AfRegister(endpoint, profile_id, device_id, version, latency,
                   std::move(input_clusters), std::move(output_clusters))
      .then([znp_api, endpoint]() {
        std::shared_ptr<ZclEndpoint> _this(new ZclEndpoint(znp_api, endpoint));
        _this->AttachListeners();
        return _this;
      });
}

void ZclEndpoint::AttachListeners() {
  std::weak_ptr<ZclEndpoint> weak_this(shared_from_this());
  listeners_.push_back(znp_api_->af_on_incoming_msg_.connect(
      [weak_this](const znp::IncomingMsg& message) {
        if (auto _this = weak_this.lock()) {
          try {
            _this->OnIncomingMsg(message);
          } catch (const std::exception& ex) {
            LOG("ZclEndpoint", warning)
                << "Exception on incoming message: " << ex.what();
          }
        }
      }));
}

void ZclEndpoint::OnIncomingMsg(const znp::IncomingMsg& message) {
  if (message.DstEndpoint != endpoint_) {
    return;
  }
  if (last_msg_[message.SrcAddr] == message.Data) {
    LOG("ZclEndpoint", debug)
        << "Ignoring duplicate message from " << (unsigned int)message.SrcAddr;
    return;
  }
  last_msg_[message.SrcAddr] = message.Data;
  auto frame = znp::Decode<ZclFrame>(message.Data);
  if (frame.frame_type == ZclFrameType::Global) {
    on_command_(message.SrcAddr, message.SrcEndpoint,
                (ZclClusterId)message.ClusterId, true, frame.command_identifier,
                frame.payload);
  } else if (frame.frame_type == ZclFrameType::Local) {
    on_command_(message.SrcAddr, message.SrcEndpoint,
                (ZclClusterId)message.ClusterId, false,
                frame.command_identifier, frame.payload);
  } else {
    LOG("ZclEndpoint", debug) << "Unknown command type";
  }
}

stlab::future<void> ZclEndpoint::SendCommand(znp::ShortAddress address,
                                             uint8_t endpoint,
                                             ZclClusterId cluster_id,
                                             bool is_global_command,
                                             ZclCommandId command_id,
                                             std::vector<uint8_t> payload) {
  ZclFrame frame;
  frame.frame_type =
      is_global_command ? ZclFrameType::Global : ZclFrameType::Local;
  frame.direction = ZclDirection::ClientToServer;
  frame.disable_default_response = false;
  frame.reserved = 0;
  frame.transaction_sequence_number = NextTransSeqNumFor(address);
  frame.command_identifier = command_id;
  frame.payload = std::move(payload);
  return znp_api_->AfDataRequest(address, endpoint, endpoint_,
                                 (uint16_t)cluster_id, 0, 0, 30,
                                 znp::Encode(frame));
}

uint8_t ZclEndpoint::NextTransSeqNumFor(znp::ShortAddress address) {
  return send_trans_seq_nums_[address]++;
}
}  // namespace zcl

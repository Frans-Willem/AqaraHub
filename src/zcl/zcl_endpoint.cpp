#include "zcl/zcl_endpoint.h"
#include <boost/log/utility/manipulators/dump.hpp>
#include "logging.h"
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
  if (frame.frame_type == ZclFrameType::Global &&
      (ZclGlobalCommandId)frame.command_identifier ==
          ZclGlobalCommandId::ReportAttributes) {
    OnIncomingReportAttributes(message, frame);
    return;
  }
  if (frame.frame_type == ZclFrameType::Global &&
      (ZclGlobalCommandId)frame.command_identifier ==
          ZclGlobalCommandId::ReadAttributes) {
    LOG("ZclEndpoint", debug)
        << "Attempt to read coordinator attributes. ClusterID "
        << message.ClusterId << " Attributes "
        << boost::log::dump(frame.payload.data(), frame.payload.size());
    return;
  }
  LOG("ZclEndpoint", debug) << "ZCL Frame not handled " << frame.frame_type
                            << " " << (unsigned int)frame.command_identifier;
}

void ZclEndpoint::OnIncomingReportAttributes(const znp::IncomingMsg& message,
                                             const ZclFrame& frame) {
  AttributeReport report;
  report.group_id = message.GroupId;
  report.cluster_id = (ZclClusterId)message.ClusterId;
  report.source_address = message.SrcAddr;
  report.source_endpoint = message.SrcEndpoint;
  report.was_broadcast = message.WasBroadcast;
  report.link_quality = message.LinkQuality;
  report.security_use = message.SecurityUse;
  report.timestamp = message.TimeStamp;
  report.trans_seq_number = message.TransSeqNumber;

  std::vector<uint8_t>::const_iterator current_data = frame.payload.begin();
  while (current_data != frame.payload.end()) {
    std::tuple<ZclAttributeId, ZclVariant> attribute;
    znp::EncodeHelper<std::tuple<ZclAttributeId, ZclVariant>>::Decode(
        attribute, current_data, frame.payload.end());
    report.attributes.push_back(std::move(attribute));
  }
  on_report_attributes_(report);
}

stlab::future<void> ZclEndpoint::WriteAttributes(
    znp::ShortAddress address, uint8_t endpoint, ZclClusterId cluster_id,
    std::vector<std::tuple<ZclAttributeId, ZclVariant>> attributes) {
  ZclFrame frame;
  frame.frame_type = ZclFrameType::Global;
  frame.direction = ZclDirection::ClientToServer;
  frame.disable_default_response = false;
  frame.reserved = 0;
  frame.transaction_sequence_number = NextTransSeqNumFor(address);
  frame.command_identifier = (uint8_t)ZclGlobalCommandId::WriteAttributes;
  for (const auto& attribute : attributes) {
    auto encoded =
        znp::Encode<std::tuple<ZclAttributeId, ZclVariant>>(attribute);
    frame.payload.insert(frame.payload.end(), encoded.begin(), encoded.end());
  }
  return znp_api_->AfDataRequest(address, endpoint, endpoint_,
                                 (uint16_t)cluster_id, 0, 0, 30,
                                 znp::Encode(frame));
}

stlab::future<void> ZclEndpoint::SendCommand(znp::ShortAddress address,
                                             uint8_t endpoint,
                                             ZclClusterId cluster_id,
                                             uint8_t command_id,
                                             std::vector<uint8_t> payload) {
  ZclFrame frame;
  frame.frame_type = ZclFrameType::Local;
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

#ifndef _ZCL_ZCL_ENDPOINT_H_
#define _ZCL_ZCL_ENDPOINT_H_
#include "zcl/zcl.h"
#include "znp/znp_api.h"

namespace zcl {
class ZclEndpoint : public std::enable_shared_from_this<ZclEndpoint> {
 public:
  struct AttributeReport {
    uint16_t group_id;
    ZclClusterId cluster_id;
    uint16_t source_address;
    uint8_t source_endpoint;
    uint8_t was_broadcast;
    uint8_t link_quality;
    uint8_t security_use;
    uint32_t timestamp;
    uint8_t trans_seq_number;
    std::vector<std::tuple<zcl::ZclAttributeId, ZclVariant>> attributes;
  };

  static stlab::future<std::shared_ptr<ZclEndpoint>> Create(
      std::shared_ptr<znp::ZnpApi> znp_api, uint8_t endpoint,
      uint16_t profile_id, uint16_t device_id, uint8_t version,
      znp::Latency latency, std::vector<uint16_t> input_clusters,
      std::vector<uint16_t> output_clusters);

  stlab::future<void> SendCommand(znp::ShortAddress address, uint8_t endpoint,
                                  ZclClusterId cluster_id,
                                  bool is_global_command,
                                  ZclCommandId command_id,
                                  std::vector<uint8_t> payload);

  boost::signals2::signal<void(znp::ShortAddress source_address,
                               uint8_t source_endpoint, ZclClusterId cluster_id,
                               bool is_global_command, ZclCommandId command_id,
                               std::vector<uint8_t> payload)>
      on_command_;

 private:
  ZclEndpoint(std::shared_ptr<znp::ZnpApi> znp_api, uint8_t endpoint);
  void AttachListeners();
  void OnIncomingMsg(const znp::IncomingMsg& message);
  void OnIncomingReportAttributes(const znp::IncomingMsg& message,
                                  const ZclFrame& frame);
  uint8_t NextTransSeqNumFor(znp::ShortAddress address);

  std::shared_ptr<znp::ZnpApi> znp_api_;
  const uint8_t endpoint_;
  std::vector<boost::signals2::connection> listeners_;
  std::map<znp::ShortAddress, uint8_t> send_trans_seq_nums_;
  std::map<znp::ShortAddress, std::vector<uint8_t>> last_msg_;
};
}  // namespace zcl
#endif  // _ZCL_ZCL_ENDPOINT_H_

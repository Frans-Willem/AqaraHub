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
  stlab::future<ZclVariant> ReadAttribute();
  stlab::future<void> WriteAttribute();

  boost::signals2::signal<void(AttributeReport)> on_report_attributes_;

 private:
  ZclEndpoint(std::shared_ptr<znp::ZnpApi> znp_api, uint8_t endpoint);
  void AttachListeners();
  void OnIncomingMsg(const znp::IncomingMsg& message);
  void OnIncomingReportAttributes(const znp::IncomingMsg& message,
                                  const ZclFrame& frame);

  std::shared_ptr<znp::ZnpApi> znp_api_;
  const uint8_t endpoint_;
  std::vector<boost::signals2::connection> listeners_;
};
}  // namespace zcl
#endif  // _ZCL_ZCL_ENDPOINT_H_

#ifndef _ZCL_NAME_REGISTRY_H_
#define _ZCL_NAME_REGISTRY_H_
#include <boost/bimap.hpp>
#include <map>
#include <string>
#include "zcl/zcl.h"

namespace zcl {
class NameRegistry {
 public:
  NameRegistry() = default;
  ~NameRegistry() = default;
  bool ReadFromInfo(const std::string& filename);
  std::string ClusterToString(zcl::ZclClusterId cluster_id);
  boost::optional<zcl::ZclClusterId> ClusterFromString(
      const std::string& cluster_name);
  std::string AttributeToString(zcl::ZclClusterId cluster_id,
                                zcl::ZclAttributeId attribute_id);
  boost::optional<zcl::ZclAttributeId> AttributeFromString(
      zcl::ZclClusterId cluster_id, const std::string& attribute_name);

 private:
  boost::bimap<zcl::ZclClusterId, std::string> cluster_names_;
  std::map<zcl::ZclClusterId, boost::bimap<zcl::ZclAttributeId, std::string>>
      attribute_names_;
};
}  // namespace zcl
#endif  //_ZCL_NAME_REGISTRY_H_

#ifndef _ZCL_NAME_REGISTRY_H_
#define _ZCL_NAME_REGISTRY_H_
#include <boost/bimap.hpp>
#include <map>
#include <string>
#include "clusterdb/cluster_db.h"
#include "zcl/zcl.h"

namespace zcl {
class NameRegistry {
 public:
  NameRegistry() = default;
  ~NameRegistry() = default;
  bool ReadFromInfo(const std::string& filename,
                    std::function<std::string(std::string)> name_mangler);
  std::string ClusterToString(zcl::ZclClusterId cluster_id);
  boost::optional<zcl::ZclClusterId> ClusterFromString(
      const std::string& cluster_name);
  std::string AttributeToString(zcl::ZclClusterId cluster_id,
                                zcl::ZclAttributeId attribute_id);
  boost::optional<zcl::ZclAttributeId> AttributeFromString(
      zcl::ZclClusterId cluster_id, const std::string& attribute_name);
  std::string CommandToString(zcl::ZclClusterId cluster_id,
                              ZclCommandId command);
  boost::optional<zcl::ZclCommandId> CommandFromString(
      zcl::ZclClusterId cluster_id, const std::string& command_name);

 private:
  clusterdb::ClusterDb db_;
};
}  // namespace zcl
#endif  //_ZCL_NAME_REGISTRY_H_

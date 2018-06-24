#ifndef _CLUSTERDB_CLUSTER_DB_H_
#define _CLUSTERDB_CLUSTER_DB_H_
#include "clusterdb/cluster_info.h"
#include "clusterdb/command_info.h"

namespace clusterdb {
class ClusterDb {
 public:
  ClusterDb();
  ~ClusterDb();
  boost::optional<const ClusterInfo&> ClusterByName(
      const std::string& name) const;
  boost::optional<const ClusterInfo&> ClusterById(
      const zcl::ZclClusterId& id) const;
  boost::optional<const CommandInfo&> GlobalCommandByName(
      const std::string& name) const;
  boost::optional<const CommandInfo&> GlobalCommandById(
      const zcl::ZclCommandId& id) const;
  boost::optional<std::pair<bool, const CommandInfo&>> CommandOutByName(
      const zcl::ZclClusterId& cluster_id, const std::string& name);

  bool ParseFromFile(const std::string& filename,
                     std::function<std::string(std::string)> name_mangler);

  struct Context;

 private:
  std::unique_ptr<Context> ctx_;
};
}  // namespace clusterdb
#endif  // _CLUSTERDB_CLUSTER_DB_H_

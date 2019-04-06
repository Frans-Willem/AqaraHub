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
  boost::optional<const CommandInfo&> CommandByName(
      const zcl::ZclClusterId& cluster_id, zcl::ZclDirection direction,
      const std::string& name) const;
  boost::optional<const CommandInfo&> CommandById(
      const zcl::ZclClusterId& cluster_id, const zcl::ZclCommandId& id,
      bool is_global, zcl::ZclDirection direction) const;

  bool ParseFromFile(const std::string& filename,
                     std::function<std::string(std::string)> name_mangler);
  bool ParseFromStream(std::istream& stream,
                       std::function<std::string(std::string)> name_mangler);

  struct Context;

 private:
  std::unique_ptr<Context> ctx_;
};
}  // namespace clusterdb
#endif  // _CLUSTERDB_CLUSTER_DB_H_

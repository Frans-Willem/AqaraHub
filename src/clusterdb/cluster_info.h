#ifndef _CLUSTERDB_CLUSTER_INFO_H_
#define _CLUSTERDB_CLUSTER_INFO_H_
#include "clusterdb/attribute_info.h"
#include "clusterdb/command_info.h"
#include "clusterdb/searchable_list.h"
#include "zcl/zcl.h"

namespace clusterdb {
struct ClusterInfo {
  zcl::ZclClusterId id;
  std::string name;
  SearchableList<AttributeInfo> attributes;
  SearchableList<CommandInfo> commands_in;
  SearchableList<CommandInfo> commands_out;
};
}  // namespace clusterdb
#endif  // _CLUSTERDB_CLUSTER_INFO_H_

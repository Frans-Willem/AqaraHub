#ifndef _CLUSTERDB_COMMAND_INFO_H_
#define _CLUSTERDB_COMMAND_INFO_H_
#include "clusterdb/argument_info.h"
#include "zcl/zcl.h"

namespace clusterdb {
struct CommandInfo {
  zcl::ZclCommandId id;
  std::string name;
  std::vector<ArgumentInfo> arguments;
};
}  // namespace clusterdb
#endif  //_CLUSTERDB_COMMAND_INFO_H_

#ifndef _CLUSTERDB_COMMAND_INFO_H_
#define _CLUSTERDB_COMMAND_INFO_H_
#include "dynamic_encoding/common.h"
#include "zcl/zcl.h"

namespace clusterdb {
struct CommandInfo {
  zcl::ZclCommandId id;
  std::string name;
  dynamic_encoding::ObjectType data;
};
}  // namespace clusterdb
#endif  //_CLUSTERDB_COMMAND_INFO_H_

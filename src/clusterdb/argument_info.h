#ifndef _CLUSTERDB_ARGUMENT_INFO_H_
#define _CLUSTERDB_ARGUMENT_INFO_H_
#include <boost/variant.hpp>
#include <string>
#include "zcl/zcl.h"

namespace clusterdb {
struct ArgumentInfo;

enum class SimpleArgumentType { Variant, Attribute };
struct DataTypeArgumentType {
  zcl::DataType datatype;
};
struct RepeatedArgumentType {
  std::vector<ArgumentInfo> contents;
};
struct ArgumentInfo {
  std::string name;
  boost::variant<SimpleArgumentType, DataTypeArgumentType, RepeatedArgumentType>
      type;
};
}  // namespace clusterdb
#endif  // _CLUSTERDB_ARGUMENT_INFO_H_

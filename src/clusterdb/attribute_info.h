#ifndef _CLUSTERDB_ATTRIBUTE_INFO_H_
#define _CLUSTERDB_ATTRIBUTE_INFO_H_
#include <boost/optional.hpp>
#include "zcl/zcl.h"

namespace clusterdb {
struct AttributeInfo {
  zcl::ZclAttributeId id;
  std::string name;
  boost::optional<zcl::DataType> datatype;
};
}  // namespace clusterdb
#endif  // _CLUSTERDB_ATTRIBUTE_INFO_H_

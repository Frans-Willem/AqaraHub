#ifndef _DYNAMIC_ENCODING_COMMON_H_
#define _DYNAMIC_ENCODING_COMMON_H_
#include <boost/variant.hpp>
#include <memory>
#include "zcl/zcl.h"

namespace clusterdb {
struct ClusterInfo;
}
namespace dynamic_encoding {
struct Context {
  boost::optional<const clusterdb::ClusterInfo&> cluster;
};
struct VariantType {};
struct ObjectType;
struct GreedyRepeatedType;

typedef boost::variant<VariantType, zcl::DataType,
                       boost::recursive_wrapper<ObjectType>,
                       boost::recursive_wrapper<GreedyRepeatedType>>
    AnyType;

struct ObjectEntry {
  std::string name;
  AnyType type;
};
struct ObjectType {
  std::vector<ObjectEntry> properties;
};
struct GreedyRepeatedType {
  AnyType element_type;
};

bool operator==(const VariantType& a, const VariantType& b);
bool operator==(const ObjectEntry& a, const ObjectEntry& b);
bool operator==(const ObjectType& a, const ObjectType& b);
bool operator==(const GreedyRepeatedType& a, const GreedyRepeatedType& b);
}  // namespace dynamic_encoding
#endif  // _DYNAMIC_ENCODING_COMMON_H_

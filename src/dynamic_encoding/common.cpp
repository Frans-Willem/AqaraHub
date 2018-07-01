#include "dynamic_encoding/common.h"

namespace dynamic_encoding {
bool operator==(const VariantType& a, const VariantType& b) { return true; }
bool operator==(const ObjectEntry& a, const ObjectEntry& b) {
  return a.name == b.name && a.type == b.type;
}
bool operator==(const ObjectType& a, const ObjectType& b) {
  return a.properties == b.properties;
}
bool operator==(const GreedyRepeatedType& a, const GreedyRepeatedType& b) {
  return a.element_type == b.element_type;
}
bool operator==(const XiaomiFF01Type& a, const XiaomiFF01Type& b) {
  return true;
}
bool operator==(const ErrorOrType& a, const ErrorOrType& b) {
  return a.success_type == b.success_type;
}
}  // namespace dynamic_encoding

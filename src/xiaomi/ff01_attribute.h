#ifndef _XIAOMI_FF01_ATTRIBUTE_H_
#define _XIAOMI_FF01_ATTRIBUTE_H_
#include "zcl/zcl.h"
#include <boost/optional.hpp>
#include <map>
#include <tao/json.hpp>

namespace xiaomi {
boost::optional<std::map<uint8_t, zcl::ZclVariant>> DecodeFF01Attribute(zcl::ZclClusterId cluster_id, zcl::ZclAttributeId attribute_id, const zcl::ZclVariant& current_value);
tao::json::value FF01AttributeToJson(const std::map<uint8_t, zcl::ZclVariant>& value);
}
#endif//_XIAOMI_FF01_ATTRIBUTE_H_

#include "xiaomi/ff01_attribute.h"
#include <boost/log/utility/manipulators/dump.hpp>
#include "logging.h"
#include "zcl/encoding.h"
#include "zcl/to_json.h"

namespace xiaomi {
void FixupFF01ReportLength(zcl::ZclClusterId cluster_id,
                           std::vector<uint8_t>& payload) {
  if (cluster_id == (zcl::ZclClusterId)0x0000 /* Basic */ &&
      payload.size() > 4) {
    std::vector<uint8_t>::const_iterator data_ptr = payload.begin();
    // Read the first attribute header to check if it's 0xFF01 & string
    std::tuple<zcl::ZclAttributeId, zcl::DataType> attr_header;
    znp::EncodeHelper<decltype(attr_header)>::Decode(attr_header, data_ptr,
                                                     payload.end());
    if (std::get<0>(attr_header) == (zcl::ZclAttributeId)0xFF01 &&
        std::get<1>(attr_header) == zcl::DataType::string) {
      // Read the string length
      uint8_t length;
      znp::EncodeHelper<uint8_t>::Decode(length, data_ptr, payload.end());
      std::size_t length_remaining = std::distance(data_ptr, payload.cend());
      // Is the reported length less than the remaining number of bytes?
      if (((std::size_t)length) > length_remaining) {
        // Then fix it up
        LOG("FF01", debug) << "Xiaomi FF01 attribute was longer than remaining "
                              "data, patching it up. ("
                           << (std::size_t)length << " > " << length_remaining
                           << ")";
        auto new_header =
            std::make_tuple(attr_header, (uint8_t)length_remaining);
        std::vector<uint8_t>::iterator write_ptr = payload.begin();
        znp::EncodeHelper<decltype(new_header)>::Encode(new_header, write_ptr,
                                                        payload.end());
      }
    }
  }
}
boost::optional<std::map<uint8_t, zcl::ZclVariant>> DecodeFF01Attribute(
    zcl::ZclClusterId cluster_id, zcl::ZclAttributeId attribute_id,
    const zcl::ZclVariant& current_value) {
  if (cluster_id != (zcl::ZclClusterId)0x0000 /* Basic */ ||
      attribute_id != (zcl::ZclAttributeId)0xFF01) {
    return boost::none;
  }
  boost::optional<std::string> str_data(
      current_value.Get<zcl::DataType::string>());
  if (!str_data) {
    return boost::none;
  }
  std::vector<uint8_t> vec_data(str_data->begin(), str_data->end());
  std::map<uint8_t, zcl::ZclVariant> retval;
  try {
    std::vector<uint8_t>::const_iterator current_data = vec_data.begin();
    while (current_data != vec_data.end()) {
      std::tuple<uint8_t, zcl::ZclVariant> item;
      znp::EncodeHelper<std::tuple<uint8_t, zcl::ZclVariant>>::Decode(
          item, current_data, vec_data.end());
      retval[std::get<0>(item)] = std::get<1>(item);
    }

  } catch (const std::exception& ex) {
    LOG("FF01", warning) << "Exception while decoding FF01: " << ex.what();
  }

  return retval;
}

tao::json::value FF01AttributeToJson(
    const std::map<uint8_t, zcl::ZclVariant>& value) {
  tao::json::value retval(tao::json::empty_object);
  for (const auto& item : value) {
    retval += {{boost::str(boost::format("%d") % (unsigned int)item.first),
                zcl::to_json(item.second)}};
  }

  return {{"type", "xiaomi_ff01"}, {"value", retval}};
}

}  // namespace xiaomi

#include <string_enum.h>
#include <zcl/from_json.h>
#include <zcl/zcl_string_enum.h>

namespace zcl {
static DataType datatype_from_json(const tao::json::value& value) {
  if (value.is_string()) {
    boost::optional<DataType> datatype =
        string_to_enum<DataType>(value.get_string());
    if (!datatype) {
      throw std::runtime_error("Datatype not recognized");
    }
    return *datatype;
  }
  throw std::runtime_error("Unable to convert value to datatype");
}
ZclVariant from_json(const tao::json::value& value) {
  auto json_obj = value.get_object();
  DataType dt = datatype_from_json(json_obj["type"]);
  tao::json::value raw_value = json_obj["value"];
  switch (dt) {
    case DataType::uint8:
      return ZclVariant::Create<DataType::uint8>(
          (uint8_t)raw_value.get_unsigned());
    default:
      throw std::runtime_error("Unsupported datatype");
  }
}
}  // namespace zcl

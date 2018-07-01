#include "dynamic_encoding/encoding.h"
#include "clusterdb/cluster_info.h"
#include "zcl/zcl_string_enum.h"
#include "znp/encoding.h"

namespace dynamic_encoding {
template <typename T>
void NormalEncodeAppend(const T& value, std::vector<uint8_t>& target) {
  std::size_t size = znp::EncodeHelper<T>::GetSize(value);
  std::size_t target_size = target.size();
  target.resize(target_size + size, 0);
  auto parsed_until = target.begin() + target_size;
  znp::EncodeHelper<T>::Encode(value, parsed_until, target.end());
  if (parsed_until != target.end()) {
    throw std::runtime_error("EncodeHelper allocated too much data");
  }
}

template <typename FT, typename IT, std::size_t MAN, std::size_t EXP>
void EncodeFloat(const FT& value, std::vector<uint8_t>& target) {
  typedef typename znp::FloatEncodeHelper<FT, IT, MAN, EXP> EncodeHelper;
  std::size_t size = EncodeHelper::GetSize(value);
  std::size_t target_size = target.size();
  target.resize(target_size + size, 0);
  auto parsed_until = target.begin() + target_size;
  EncodeHelper::Encode(value, parsed_until, target.end());
  if (parsed_until != target.end()) {
    throw std::runtime_error("EncodeHelper allocated too much data");
  }
}

template <typename IT>
void EncodeInteger(const IT& value, std::size_t bytes,
                   std::vector<uint8_t>& target) {
  typedef typename std::make_unsigned<IT>::type UT;
  UT unsigned_value = (UT)value;
  UT rest = unsigned_value >> (bytes * 8);
  if (std::is_signed<IT>::value) {
    const UT rest_negative = ((UT)~0) >> (bytes * 8);
    if (rest != 0 && rest != rest_negative) {
      throw std::runtime_error(
          boost::str(boost::format("Value %d does not fit in %d bits") %
                     (int64_t)value % (bytes * 8)));
    }
  } else {
    if (rest != 0) {
      throw std::runtime_error(
          boost::str(boost::format("Value %d does not fit in %d bits") %
                     (uint64_t)value % (bytes * 8)));
    }
  }
  for (std::size_t byte = 0; byte < bytes; byte++) {
    target.push_back((std::uint8_t)((unsigned_value >> (byte * 8)) & 0xFF));
  }
}

void EncodeTyped(const Context& ctx, const VariantType& type,
                 const tao::json::value::object_t& value,
                 std::vector<uint8_t>& target);
void EncodeTyped(const Context& ctx, const VariantType& type,
                 const tao::json::value& value, std::vector<uint8_t>& target);
void EncodeTyped(const Context& ctx, const ObjectType& object,
                 const tao::json::value& value, std::vector<uint8_t>& target);
void EncodeTyped(const Context& ctx, const zcl::DataType& datatype,
                 const tao::json::value& value, std::vector<uint8_t>& target);
void EncodeTyped(const Context& ctx, const GreedyRepeatedType& type,
                 const tao::json::value& value, std::vector<uint8_t>& target);

void EncodeTyped(const Context& ctx, const VariantType& type,
                 const tao::json::value::object_t& value,
                 std::vector<uint8_t>& target) {
  auto found_type = value.find("type");
  auto found_value = value.find("value");
  if (found_type == value.end()) {
    throw std::runtime_error(
        "JSON object for variant did not have 'type' property");
  }
  zcl::DataType datatype = zcl::DataType::nodata;
  if (auto x =
          string_to_enum<zcl::DataType>(found_type->second.as<std::string>())) {
    datatype = *x;
  } else {
    throw std::runtime_error("Invalid type '" +
                             tao::json::to_string(found_type->second) + "'");
  }
  NormalEncodeAppend(datatype, target);
  EncodeTyped(
      ctx, datatype,
      (found_value == value.end()) ? tao::json::null : found_value->second,
      target);
}
void EncodeTyped(const Context& ctx, const VariantType& type,
                 const tao::json::value& value, std::vector<uint8_t>& target) {
  if (value == tao::json::null) {
    NormalEncodeAppend(zcl::DataType::nodata, target);
  } else {
    EncodeTyped(ctx, type, value.get_object(), target);
  }
}

void EncodeTyped(const Context& ctx, const ObjectType& object_type,
                 const tao::json::value& value, std::vector<uint8_t>& target) {
  const tao::json::value::object_t& object = value.get_object();
  for (const auto& property : object_type.properties) {
    auto found = object.find(property.name);
    Encode(ctx, property.type,
           (found == object.end()) ? tao::json::null : found->second, target);
  }
}

void EncodeTyped(const Context& ctx, const XiaomiFF01Type& type,
                 const tao::json::value& value, std::vector<uint8_t>& target) {
  throw std::runtime_error("Xiaomi FF01 encoding not implemented");
}

void EncodeTyped(const Context& ctx, const zcl::DataType& datatype,
                 const tao::json::value& value, std::vector<uint8_t>& target) {
  switch (datatype) {
    case zcl::DataType::nodata: {
      if (value != tao::json::null) {
        throw std::runtime_error("No data expected for type nodata");
      }
      return;
      return;
    }
    case zcl::DataType::data8:
    case zcl::DataType::data16:
    case zcl::DataType::data24:
    case zcl::DataType::data32:
    case zcl::DataType::data40:
    case zcl::DataType::data48:
    case zcl::DataType::data56:
    case zcl::DataType::data64: {
      std::size_t length =
          1 + ((std::size_t)datatype - (std::size_t)zcl::DataType::data8);
      const tao::json::value::array_t& array_value = value.get_array();
      if (array_value.size() != length) {
        throw std::runtime_error(
            boost::str(boost::format("Type %s expects array of length %d") %
                       enum_to_string<zcl::DataType>(datatype) % length));
      }
      for (const auto& item : array_value) {
        EncodeInteger(item.as<unsigned int>(), 1, target);
      }
      return;
    }
    case zcl::DataType::_bool: {
      if (value == tao::json::null) {
        EncodeInteger<uint8_t>(0xFF, 1, target);
      } else {
        EncodeInteger<uint8_t>(value.as<bool>() ? 1 : 0, 1, target);
      }
      return;
    }
    case zcl::DataType::map8:
    case zcl::DataType::map16:
    case zcl::DataType::map24:
    case zcl::DataType::map32:
    case zcl::DataType::map40:
    case zcl::DataType::map48:
    case zcl::DataType::map56:
    case zcl::DataType::map64: {
      std::size_t length =
          1 + ((std::size_t)datatype - (std::size_t)zcl::DataType::map8);
      const tao::json::value::array_t& array_value = value.get_array();
      if (array_value.size() != length * 8) {
        throw std::runtime_error(boost::str(
            boost::format("Type %s expects array of length %d, got %d") %
            enum_to_string<zcl::DataType>(datatype) % (length * 8) %
            array_value.size()));
      }
      std::size_t current_bit = (length * 8) - 1;
      std::uint64_t bitmask = 0;
      for (const auto& item : array_value) {
        if (item.as<bool>()) {
          bitmask |= (1 << current_bit);
        }
        current_bit--;
      }
      EncodeInteger(bitmask, length, target);
      return;
    }
    case zcl::DataType::uint8:
    case zcl::DataType::uint16:
    case zcl::DataType::uint24:
    case zcl::DataType::uint32:
    case zcl::DataType::uint40:
    case zcl::DataType::uint48:
    case zcl::DataType::uint56:
    case zcl::DataType::uint64: {
      std::size_t length =
          1 + ((std::size_t)datatype - (std::size_t)zcl::DataType::uint8);
      EncodeInteger<std::uint64_t>(value.as<std::uint64_t>(), length, target);
      return;
    }
    case zcl::DataType::int8:
    case zcl::DataType::int16:
    case zcl::DataType::int24:
    case zcl::DataType::int32:
    case zcl::DataType::int40:
    case zcl::DataType::int48:
    case zcl::DataType::int56:
    case zcl::DataType::int64: {
      std::size_t length =
          1 + ((std::size_t)datatype - (std::size_t)zcl::DataType::int8);
      EncodeInteger<std::int64_t>(value.as<std::int64_t>(), length, target);
      return;
    }
    case zcl::DataType::enum8:
    case zcl::DataType::enum16: {
      std::size_t length =
          1 + ((std::size_t)datatype - (std::size_t)zcl::DataType::enum8);
      EncodeInteger(value.as<unsigned int>(), length, target);
      return;
    }
    case zcl::DataType::semi: {
      EncodeFloat<float, std::uint16_t, 10, 5>(value.as<float>(), target);
      return;
    }
    case zcl::DataType::single: {
      EncodeFloat<float, std::uint32_t, 23, 8>(value.as<float>(), target);
      return;
    }
    case zcl::DataType::_double: {
      EncodeFloat<double, std::uint64_t, 52, 11>(value.as<double>(), target);
      return;
    }
    case zcl::DataType::octstr:
    case zcl::DataType::octstr16: {
      std::size_t size_bytes = (datatype == zcl::DataType::octstr16 ? 2 : 1);
      std::size_t invalid_size = (1 << (size_bytes * 8)) - 1;
      if (value == tao::json::null) {
        EncodeInteger(invalid_size, size_bytes, target);
      } else {
        const tao::json::value::array_t& array_value = value.get_array();
        EncodeInteger(array_value.size(), size_bytes, target);
        for (const auto& item : array_value) {
          EncodeInteger(item.as<unsigned int>(), 1, target);
        }
      }
      return;
    }
    case zcl::DataType::string:
    case zcl::DataType::string16: {
      std::size_t size_bytes = (datatype == zcl::DataType::string16 ? 2 : 1);
      std::size_t invalid_size = (1 << (size_bytes * 8)) - 1;
      if (value == tao::json::null) {
        EncodeInteger(invalid_size, size_bytes, target);
      } else {
        std::string string_value(value.as<std::string>());
        EncodeInteger(string_value.size(), size_bytes, target);
        std::copy(string_value.cbegin(), string_value.cend(),
                  std::back_inserter(target));
      }
      return;
    }
    case zcl::DataType::_struct: {
      std::size_t invalid_size = 0xFFFF;
      if (value == tao::json::null) {
        EncodeInteger(invalid_size, 2, target);
      } else {
        const tao::json::value::array_t& array_value = value.get_array();
        EncodeInteger(array_value.size(), 2, target);
        for (const auto& item : array_value) {
          EncodeTyped(ctx, VariantType{}, item, target);
        }
      }
      return;
    }
    case zcl::DataType::array:
    case zcl::DataType::set:
    case zcl::DataType::bag: {
      const tao::json::value::object_t& object_value = value.get_object();
      auto found_element_type = object_value.find("element_type");
      auto found_elements = object_value.find("elements");
      const tao::json::value& element_type_value =
          (found_element_type != object_value.end())
              ? found_element_type->second
              : tao::json::null;
      const tao::json::value& elements_value =
          (found_elements != object_value.end()) ? found_elements->second
                                                 : tao::json::null;
      zcl::DataType element_datatype = zcl::DataType::nodata;
      if (auto x =
              string_to_enum<zcl::DataType>(element_type_value.get_string())) {
        element_datatype = *x;
      } else {
        throw std::runtime_error(
            boost::str(boost::format("Invalid element_type '%s' for '%s'") %
                       enum_to_string<zcl::DataType>(element_datatype) %
                       enum_to_string<zcl::DataType>(datatype)));
      }
      NormalEncodeAppend(element_datatype, target);
      if (elements_value == tao::json::null) {
        EncodeInteger<std::size_t>(0xFFFF, 2, target);
      } else {
        tao::json::value::array_t elements_arr = elements_value.get_array();
        EncodeInteger<std::size_t>(elements_arr.size(), 2, target);
        for (const auto& item : elements_arr) {
          EncodeTyped(ctx, element_datatype, item, target);
        }
      }
      return;
    }
      // Missing: ToD, date, UTC
    case zcl::DataType::attribId: {
      if (value == tao::json::null) {
        NormalEncodeAppend((zcl::ZclAttributeId)0xFFFF, target);
      } else {
        if (value.is_string()) {
          if (ctx.cluster) {
            if (auto attribute_info =
                    ctx.cluster->attributes.FindByName(value.get_string())) {
              NormalEncodeAppend(attribute_info->id, target);
            } else {
              throw std::runtime_error("Unknown attribute id '" +
                                       value.get_string() + "'");
            }
          } else {
            throw std::runtime_error(
                "Unable to decode string attribute identifier in this context");
          }
        } else if (value.is_integer()) {
          EncodeInteger(value.as<unsigned int>(), 2, target);
        } else {
          throw std::runtime_error(
              "Expected either string or integer for attribute ID");
        }
      }
      return;
    }
    case zcl::DataType::unk: {
      if (value != tao::json::null) {
        throw std::runtime_error("No data expected for type unk");
      }
      return;
    }
    default:
      throw std::runtime_error(
          boost::str(boost::format("Encoding of type %s not implemented") %
                     enum_to_string<zcl::DataType>(datatype)));
  }
}

void EncodeTyped(const Context& ctx, const GreedyRepeatedType& type,
                 const tao::json::value& value, std::vector<uint8_t>& target) {
  const tao::json::value::array_t& array_value = value.get_array();
  for (const auto& item : array_value) {
    Encode(ctx, type.element_type, item, target);
  }
}

void Encode(const Context& ctx, const AnyType& type,
            const tao::json::value& value, std::vector<uint8_t>& target) {
  boost::apply_visitor(
      [&ctx, &value, &target](const auto& x) {
        EncodeTyped(ctx, x, value, target);
      },
      type);
}
void Encode(const Context& ctx, const ObjectType& object,
            const tao::json::value& value, std::vector<uint8_t>& target) {
  EncodeTyped(ctx, object, value, target);
}

}  // namespace dynamic_encoding

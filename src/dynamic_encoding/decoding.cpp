#include "dynamic_encoding/decoding.h"
#include <type_traits>
#include "clusterdb/cluster_info.h"
#include "string_enum.h"
#include "zcl/zcl_string_enum.h"
#include "znp/encoding.h"

namespace dynamic_encoding {
template <typename IT>
IT DecodeInteger(std::size_t bytes, std::vector<uint8_t>::const_iterator& begin,
                 const std::vector<uint8_t>::const_iterator& end) {
  typedef typename std::make_unsigned<IT>::type UT;
  UT unsigned_value = 0;
  for (std::size_t byte = 0; byte < bytes; byte++) {
    if (begin == end) {
      throw std::runtime_error("Not enough data to decode integer");
    }
    unsigned_value |= ((IT) * (begin++)) << (byte * 8);
  }
  if (std::is_signed<IT>::value) {
    if (unsigned_value >> ((bytes * 8) - 1) == 1) {
      unsigned_value |= ((UT)~0) << (bytes * 8);
    }
  }
  return (IT)unsigned_value;
}

struct Decoder {
  typedef tao::json::value result_type;

  std::vector<uint8_t>::const_iterator& begin;
  const std::vector<uint8_t>::const_iterator& end;
  const Context& ctx;

  tao::json::value operator()(const VariantType& variant) {
    tao::json::value::object_t ret;
    zcl::DataType type;
    znp::EncodeHelper<zcl::DataType>::Decode(type, begin, end);
    ret["type"] = enum_to_string<zcl::DataType>(type);
    ret["value"] = (*this)(type);
    return ret;
  }

  tao::json::value operator()(const zcl::DataType& datatype) {
    switch (datatype) {
      case zcl::DataType::nodata:
        return tao::json::null;
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
        tao::json::value::array_t ret;
        for (std::size_t i = 0; i < length; i++) {
          ret.push_back(DecodeInteger<unsigned int>(1, begin, end));
        }
        return ret;
      }
      case zcl::DataType::_bool: {
        uint8_t x = DecodeInteger<uint8_t>(1, begin, end);
        if (x == 0xFF) {
          return tao::json::null;
        } else {
          return (x != 0);
        }
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
        std::uint64_t value = DecodeInteger<std::uint64_t>(length, begin, end);
        tao::json::value::array_t ret;
        for (std::size_t bit = length * 8; bit > 0; bit--) {
          ret.push_back(((value >> (bit - 1)) & 0x1) != 0);
        }
        return ret;
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
        return DecodeInteger<std::uint64_t>(length, begin, end);
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
        return DecodeInteger<std::int64_t>(length, begin, end);
      }
      case zcl::DataType::enum8:
      case zcl::DataType::enum16: {
        std::size_t length =
            1 + ((std::size_t)datatype - (std::size_t)zcl::DataType::enum8);
        return DecodeInteger<std::uint64_t>(length, begin, end);
      }
      case zcl::DataType::semi: {
        float value;
        znp::FloatEncodeHelper<float, std::uint16_t, 10, 5>::Decode(value,
                                                                    begin, end);
        return value;
      }
      case zcl::DataType::single: {
        float value;
        znp::FloatEncodeHelper<float, std::uint32_t, 23, 8>::Decode(value,
                                                                    begin, end);
        return value;
      }
      case zcl::DataType::_double: {
        double value;
        znp::FloatEncodeHelper<double, std::uint64_t, 52, 11>::Decode(
            value, begin, end);
        return value;
      }
      case zcl::DataType::octstr:
      case zcl::DataType::octstr16: {
        std::size_t size_bytes = (datatype == zcl::DataType::octstr16 ? 2 : 1);
        std::size_t invalid_size = (1 << (size_bytes * 8)) - 1;
        std::size_t size = DecodeInteger<std::size_t>(size_bytes, begin, end);
        if (size == invalid_size) {
          return tao::json::null;
        }
        if ((std::size_t)std::distance(begin, end) < size) {
          throw std::runtime_error("Not enough data to decode octet string");
        }
        tao::json::value::array_t ret;
        while (size--) {
          ret.push_back((unsigned int)*(begin++));
        }
        return ret;
      }
      case zcl::DataType::string:
      case zcl::DataType::string16: {
        std::size_t size_bytes = (datatype == zcl::DataType::string16 ? 2 : 1);
        std::size_t invalid_size = (1 << (size_bytes * 8)) - 1;
        std::size_t size = DecodeInteger<std::size_t>(size_bytes, begin, end);
        if (size == invalid_size) {
          return tao::json::null;
        }
        if ((std::size_t)std::distance(begin, end) < size) {
          throw std::runtime_error(
              "Not enough data to decode character string");
        }
        std::string ret(begin, begin + size);
        begin += size;
        return ret;
      }
      case zcl::DataType::_struct: {
        std::size_t invalid_size = 0xFFFF;
        std::size_t size = DecodeInteger<std::size_t>(2, begin, end);
        if (size == invalid_size) {
          return tao::json::null;
        }
        tao::json::value::array_t ret;
        while (size--) {
          ret.push_back((*this)(VariantType{}));
        }
        return ret;
      }

      case zcl::DataType::array:
      case zcl::DataType::set:
      case zcl::DataType::bag: {
        zcl::DataType datatype;
        znp::EncodeHelper<zcl::DataType>::Decode(datatype, begin, end);
        std::size_t invalid_size = 0xFFFF;
        std::size_t size = DecodeInteger<std::size_t>(2, begin, end);
        if (size == invalid_size) {
          return tao::json::value::object_t{
              {"element_type", enum_to_string<zcl::DataType>(datatype)},
              {"elements", tao::json::null},
          };
        }
        tao::json::value::array_t ret;
        while (size--) {
          ret.push_back((*this)(datatype));
        }
        return tao::json::value::object_t{
            {"element_type", enum_to_string<zcl::DataType>(datatype)},
            {"elements", ret},
        };
      }
        // Missing: ToD, date, UTC
      case zcl::DataType::attribId: {
        zcl::ZclAttributeId id;
        znp::EncodeHelper<zcl::ZclAttributeId>::Decode(id, begin, end);
        if (ctx.cluster) {
          if (auto attribute_info = ctx.cluster->attributes.FindById(id)) {
            return attribute_info->name;
          }
        }
        return (unsigned int)id;
      }
      case zcl::DataType::unk: {
        return tao::json::null;
      }
      default:
        throw std::runtime_error(
            boost::str(boost::format("Decoding of type %s not implemented") %
                       enum_to_string<zcl::DataType>(datatype)));
    }
  }

  tao::json::value operator()(const ObjectType& object) {
    tao::json::value::object_t ret;
    for (const auto& property : object.properties) {
      ret[property.name] = property.type.apply_visitor(*this);
      ;
    }
    return ret;
  }

  tao::json::value operator()(const GreedyRepeatedType& repeated) {
    tao::json::value::array_t ret;
    while (begin != end) {
      ret.push_back(repeated.element_type.apply_visitor(*this));
    }
    return ret;
  }
};
tao::json::value Decode(const Context& ctx, const AnyType& type,
                        std::vector<uint8_t>::const_iterator& begin,
                        const std::vector<uint8_t>::const_iterator& end) {
  Decoder dec{begin, end, ctx};
  return type.apply_visitor(dec);
}
tao::json::value Decode(const Context& ctx, const ObjectType& object,
                        std::vector<uint8_t>::const_iterator& begin,
                        const std::vector<uint8_t>::const_iterator& end) {
  Decoder dec{begin, end, ctx};
  return dec(object);
}
}  // namespace dynamic_encoding

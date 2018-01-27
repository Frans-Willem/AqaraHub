#include "zcl/zcl.h"
#include <boost/format.hpp>
#include <boost/log/utility/manipulators/dump.hpp>
#include "string_enum.h"
#include "zcl/zcl_string_enum.h"

namespace zcl {
std::ostream& operator<<(std::ostream& stream, const ZclFrameType& frame_type) {
  return stream << enum_to_string(frame_type);
}

std::ostream& operator<<(std::ostream& stream, const ZclDirection& direction) {
  return stream << enum_to_string(direction);
}

std::ostream& operator<<(std::ostream& stream, const ZclFrame& header) {
  stream << "{frame_type: " << header.frame_type;
  if (header.manufacturer_code) {
    stream << ", manufacturer_code: " << *header.manufacturer_code;
  } else {
    stream << ", manufacturer_code: "
           << "none";
  }
  stream << ", direction: " << header.direction
         << ", disable_default_response: "
         << (header.disable_default_response ? "true" : "false")
         << ", reserved: " << (unsigned int)header.reserved
         << ", transaction_sequence_number: "
         << (unsigned int)header.transaction_sequence_number
         << ", command_identifier: " << (unsigned int)header.command_identifier
         << "}";
  return stream;
}

std::ostream& operator<<(std::ostream& stream, const ZclClusterId& cluster_id) {
  return stream << enum_to_string(cluster_id);
}

std::ostream& operator<<(std::ostream& stream, const ZclVariant& variant) {
  switch (variant.type_) {
    case DataType::_bool:
      return stream << (*variant.Get<DataType::_bool>() ? "true" : "false");
    case DataType::map8:
      return stream << "(map8) " << variant.Get<DataType::map8>()->to_string();
    case DataType::uint8:
      return stream << "(uint8) " << std::dec
                    << (unsigned int)*variant.Get<DataType::uint8>();
    case DataType::uint16:
      return stream << "(uint16) " << std::dec
                    << (unsigned int)*variant.Get<DataType::uint16>();
    case DataType::uint24:
      return stream << "(uint24) " << std::dec
                    << (unsigned int)*variant.Get<DataType::uint24>();
    case DataType::uint32:
      return stream << "(uint32) " << std::dec
                    << *variant.Get<DataType::uint32>();
    case DataType::uint40:
      return stream << "(uint40) " << std::dec
                    << *variant.Get<DataType::uint40>();
    case DataType::uint48:
      return stream << "(uint48) " << std::dec
                    << *variant.Get<DataType::uint48>();
    case DataType::uint56:
      return stream << "(uint56) " << std::dec
                    << *variant.Get<DataType::uint56>();
    case DataType::uint64:
      return stream << "(uint64) " << std::dec
                    << *variant.Get<DataType::uint64>();
    case DataType::int8:
      return stream << "(int8) " << std::dec
                    << (int)*variant.Get<DataType::int8>();
    case DataType::int16:
      return stream << "(int16) " << std::dec
                    << (int)*variant.Get<DataType::int16>();
    case DataType::int24:
      return stream << "(int24) " << std::dec
                    << (int)*variant.Get<DataType::int24>();
    case DataType::int32:
      return stream << "(int32) " << std::dec
                    << *variant.Get<DataType::int32>();
    case DataType::int40:
      return stream << "(int40) " << std::dec
                    << *variant.Get<DataType::int40>();
    case DataType::int48:
      return stream << "(int48) " << std::dec
                    << *variant.Get<DataType::int48>();
    case DataType::int56:
      return stream << "(int56) " << std::dec
                    << *variant.Get<DataType::int56>();
    case DataType::int64:
      return stream << "(int64) " << std::dec
                    << *variant.Get<DataType::int64>();
    case DataType::string: {
      std::string data(*variant.Get<DataType::string>());
      return stream << "(string) "
                    << boost::log::dump(data.data(), data.size());
    }
    case DataType::_struct: {
      auto data = *variant.Get<DataType::_struct>();
      stream << "(struct " << data.size() << ") [";
      bool first = true;
      for (auto& item : data) {
        if (!first) {
          stream << ", ";
        } else {
          first = false;
        }
        stream << item;
      }
      return stream << "]";
    }
    default:
      return stream << "(type " << std::hex << (unsigned int)variant.type_
                    << ")";
  }
}
}  // namespace zcl

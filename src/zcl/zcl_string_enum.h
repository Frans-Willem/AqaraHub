#ifndef _ZCL_ZCL_STRING_ENUM_H_
#define _ZCL_ZCL_STRING_ENUM_H_
#include "string_enum.h"
#include "zcl/zcl.h"

template <>
struct StringEnumHelper<zcl::ZclFrameType> {
  static std::map<zcl::ZclFrameType, std::string> lookup() {
    return {{zcl::ZclFrameType::Global, "Global"},
            {zcl::ZclFrameType::Local, "Local"},
            {zcl::ZclFrameType::Reserved1, "Reserved1"},
            {zcl::ZclFrameType::Reserved2, "Reserved2"}};
  }
};

template <>
struct StringEnumHelper<zcl::ZclDirection> {
  static std::map<zcl::ZclDirection, std::string> lookup() {
    return {{zcl::ZclDirection::ClientToServer, "ClientToServer"},
            {zcl::ZclDirection::ServerToClient, "ServerToClient"}};
  }
};

template <>
struct StringEnumHelper<zcl::DataType> {
  static std::map<zcl::DataType, std::string> lookup() {
    return {{zcl::DataType::nodata, "nodata"},
            {zcl::DataType::data8, "data8"},
            {zcl::DataType::data16, "data16"},
            {zcl::DataType::data24, "data24"},
            {zcl::DataType::data32, "data32"},
            {zcl::DataType::data40, "data40"},
            {zcl::DataType::data56, "data56"},
            {zcl::DataType::data64, "data64"},
            {zcl::DataType::_bool, "bool"},
            {zcl::DataType::map8, "map8"},
            {zcl::DataType::map16, "map16"},
            {zcl::DataType::map24, "map24"},
            {zcl::DataType::map32, "map32"},
            {zcl::DataType::map40, "map40"},
            {zcl::DataType::map48, "map48"},
            {zcl::DataType::map56, "map56"},
            {zcl::DataType::map64, "map64"},
            {zcl::DataType::uint8, "uint8"},
            {zcl::DataType::uint16, "uint16"},
            {zcl::DataType::uint24, "uint24"},
            {zcl::DataType::uint32, "uint32"},
            {zcl::DataType::uint40, "uint40"},
            {zcl::DataType::uint48, "uint48"},
            {zcl::DataType::uint56, "uint56"},
            {zcl::DataType::uint64, "uint64"},
            {zcl::DataType::int8, "int8"},
            {zcl::DataType::int16, "int16"},
            {zcl::DataType::int24, "int24"},
            {zcl::DataType::int32, "int32"},
            {zcl::DataType::int40, "int40"},
            {zcl::DataType::int48, "int48"},
            {zcl::DataType::int56, "int56"},
            {zcl::DataType::int64, "int64"},
            {zcl::DataType::enum8, "enum8"},
            {zcl::DataType::enum16, "enum16"},
            {zcl::DataType::semi, "semi"},
            {zcl::DataType::single, "single"},
            {zcl::DataType::_double, "double"},
            {zcl::DataType::octstr, "octstr"},
            {zcl::DataType::string, "string"},
            {zcl::DataType::octstr16, "octstr16"},
            {zcl::DataType::string16, "string16"},
            {zcl::DataType::array, "array"},
            {zcl::DataType::_struct, "struct"},
            {zcl::DataType::set, "set"},
            {zcl::DataType::bag, "bag"},
            {zcl::DataType::ToD, "ToD"},
            {zcl::DataType::date, "date"},
            {zcl::DataType::UTC, "UTC"},
            {zcl::DataType::clusterId, "clusterId"},
            {zcl::DataType::attribId, "attribId"},
            {zcl::DataType::bacOID, "bacOID"},
            {zcl::DataType::EUI64, "EUI64"},
            {zcl::DataType::key128, "key128"},
            {zcl::DataType::unk, "unk"}};
  }
};
#endif  // _ZCL_ZCL_STRING_ENUM_H_

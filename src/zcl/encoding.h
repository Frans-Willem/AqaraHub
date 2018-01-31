#ifndef _ZCL_ENCODING_H_
#define _ZCL_ENCODING_H_
#include <boost/format.hpp>
#include "zcl/zcl.h"
#include "zcl/zcl_string_enum.h"
#include "znp/encoding.h"

namespace zcl {
// Template magic to define how to encode/decode a value of a specific datatype
template <DataType DT>
struct DataTypeEncodeHelper {
  template <typename... T>
  static inline std::size_t GetSize(const boost::variant<T...>& value) {
    throw std::runtime_error(boost::str(
        boost::format("Encoding/decoding for datatype %s not yet implemented") %
        enum_to_string(DT)));
  }
  template <typename... T>
  static inline void Encode(const boost::variant<T...>& value,
                            znp::EncodeTarget::iterator& begin,
                            znp::EncodeTarget::iterator end) {
    throw std::runtime_error(boost::str(
        boost::format("Encoding/decoding for datatype %s not yet implemented") %
        enum_to_string(DT)));
  }
  template <typename... T>
  static inline void Decode(boost::variant<T...>& value,
                            znp::EncodeTarget::const_iterator& begin,
                            znp::EncodeTarget::const_iterator& end) {
    throw std::runtime_error(boost::str(
        boost::format("Encoding/decoding for datatype %s not yet implemented") %
        enum_to_string(DT)));
  }
};
// Normal encode helper for a datatype
template <DataType DT>
struct NormalDataTypeEncodeHelper {
  typedef typename DataTypeHelper<DT>::Type Type;
  template <typename... T>
  static inline std::size_t GetSize(const boost::variant<T...>& value) {
    return znp::EncodeHelper<Type>::GetSize(boost::get<Type>(value));
  }
  template <typename... T>
  static inline void Encode(const boost::variant<T...>& value,
                            znp::EncodeTarget::iterator& begin,
                            znp::EncodeTarget::iterator end) {
    znp::EncodeHelper<Type>::Encode(boost::get<Type>(value), begin, end);
  }
  template <typename... T>
  static inline void Decode(boost::variant<T...>& value,
                            znp::EncodeTarget::const_iterator& begin,
                            znp::EncodeTarget::const_iterator& end) {
    Type typed_value;
    znp::EncodeHelper<Type>::Decode(typed_value, begin, end);
    value = typed_value;
  }
};
// Encode helper for (u)intXX
template <DataType DT, std::size_t N>
struct IntDataTypeEncodeHelper {
  typedef typename DataTypeHelper<DT>::Type Type;
  typedef typename std::make_unsigned<Type>::type UType;
  template <typename... T>
  static inline std::size_t GetSize(const boost::variant<T...>& value) {
    return N / 8;
  }
  template <typename... T>
  static inline void Encode(const boost::variant<T...>& value,
                            znp::EncodeTarget::iterator& begin,
                            znp::EncodeTarget::iterator end) {
    UType unsigned_value = (UType)boost::get<Type>(value);
    for (std::size_t shift = 0; shift < N; shift += 8) {
      if (begin == end) {
        throw std::runtime_error("Not enough space to encode ZCL integer");
      }
      *(begin++) = (uint8_t)((unsigned_value >> shift) & 0xFF);
    }
  }
  template <typename... T>
  static inline void Decode(boost::variant<T...>& value,
                            znp::EncodeTarget::const_iterator& begin,
                            znp::EncodeTarget::const_iterator end) {
    UType unsigned_value = 0;
    for (std::size_t shift = 0; shift < N; shift += 8) {
      if (begin == end) {
        throw std::runtime_error("Not enough space to decode ZCL integer");
      }
      unsigned_value |= ((UType) * (begin++)) << shift;
    }
    value = (Type)unsigned_value;
  }
};
// Encode helper for octstr, string, octstr16, string16
// LT = Length type (e.g. uint8_t or uint16_t)
template <DataType DT, typename LT>
struct VectorDataTypeEncodeHelper {
  typedef typename DataTypeHelper<DT>::Type Type;
  typedef typename Type::value_type ElType;

  template <typename... T>
  static inline std::size_t GetSize(const boost::variant<T...>& value) {
    const auto& typed_value = boost::get<Type>(value);
    auto length = typed_value.size();
    std::size_t size = znp::EncodeHelper<LT>::GetSize((LT)length);
    for (const auto& item : typed_value) {
      size += znp::EncodeHelper<ElType>::GetSize(item);
    }
    return size;
  }
  template <typename... T>
  static inline void Encode(const boost::variant<T...>& value,
                            znp::EncodeTarget::iterator& begin,
                            znp::EncodeTarget::iterator end) {
    const auto& typed_value = boost::get<Type>(value);
    auto length = typed_value.size();
    znp::EncodeHelper<LT>::Encode((LT)length, begin, end);
    for (const auto& item : typed_value) {
      znp::EncodeHelper<ElType>::Encode(item, begin, end);
    }
  }
  template <typename... T>
  static inline void Decode(boost::variant<T...>& value,
                            znp::EncodeTarget::const_iterator& begin,
                            znp::EncodeTarget::const_iterator end) {
    Type typed_value;
    LT length = 0;
    znp::EncodeHelper<LT>::Decode(length, begin, end);
    typed_value.resize(length);
    for (auto& item : typed_value) {
      znp::EncodeHelper<ElType>::Decode(item, begin, end);
    }
    value = typed_value;
  }
};

template <>
struct DataTypeEncodeHelper<DataType::_bool>
    : NormalDataTypeEncodeHelper<DataType::_bool> {};
template <>
struct DataTypeEncodeHelper<DataType::map8>
    : NormalDataTypeEncodeHelper<DataType::map8> {};
template <>
struct DataTypeEncodeHelper<DataType::map16>
    : NormalDataTypeEncodeHelper<DataType::map16> {};
template <>
struct DataTypeEncodeHelper<DataType::map24>
    : NormalDataTypeEncodeHelper<DataType::map24> {};
template <>
struct DataTypeEncodeHelper<DataType::map32>
    : NormalDataTypeEncodeHelper<DataType::map32> {};
template <>
struct DataTypeEncodeHelper<DataType::map40>
    : NormalDataTypeEncodeHelper<DataType::map40> {};
template <>
struct DataTypeEncodeHelper<DataType::map56>
    : NormalDataTypeEncodeHelper<DataType::map56> {};
template <>
struct DataTypeEncodeHelper<DataType::map64>
    : NormalDataTypeEncodeHelper<DataType::map64> {};
template <>
struct DataTypeEncodeHelper<DataType::uint8>
    : IntDataTypeEncodeHelper<DataType::uint8, 8> {};
template <>
struct DataTypeEncodeHelper<DataType::uint16>
    : IntDataTypeEncodeHelper<DataType::uint16, 16> {};
template <>
struct DataTypeEncodeHelper<DataType::uint24>
    : IntDataTypeEncodeHelper<DataType::uint24, 24> {};
template <>
struct DataTypeEncodeHelper<DataType::uint32>
    : IntDataTypeEncodeHelper<DataType::uint32, 32> {};
template <>
struct DataTypeEncodeHelper<DataType::uint40>
    : IntDataTypeEncodeHelper<DataType::uint40, 40> {};
template <>
struct DataTypeEncodeHelper<DataType::uint48>
    : IntDataTypeEncodeHelper<DataType::uint48, 48> {};
template <>
struct DataTypeEncodeHelper<DataType::uint56>
    : IntDataTypeEncodeHelper<DataType::uint56, 56> {};
template <>
struct DataTypeEncodeHelper<DataType::uint64>
    : IntDataTypeEncodeHelper<DataType::uint64, 64> {};
template <>
struct DataTypeEncodeHelper<DataType::int8>
    : IntDataTypeEncodeHelper<DataType::int8, 8> {};
template <>
struct DataTypeEncodeHelper<DataType::int16>
    : IntDataTypeEncodeHelper<DataType::int16, 16> {};
template <>
struct DataTypeEncodeHelper<DataType::int24>
    : IntDataTypeEncodeHelper<DataType::int24, 24> {};
template <>
struct DataTypeEncodeHelper<DataType::int32>
    : IntDataTypeEncodeHelper<DataType::int32, 32> {};
template <>
struct DataTypeEncodeHelper<DataType::int40>
    : IntDataTypeEncodeHelper<DataType::int40, 40> {};
template <>
struct DataTypeEncodeHelper<DataType::int48>
    : IntDataTypeEncodeHelper<DataType::int48, 48> {};
template <>
struct DataTypeEncodeHelper<DataType::int56>
    : IntDataTypeEncodeHelper<DataType::int56, 56> {};
template <>
struct DataTypeEncodeHelper<DataType::int64>
    : IntDataTypeEncodeHelper<DataType::int64, 64> {};
template <>
struct DataTypeEncodeHelper<DataType::octstr>
    : VectorDataTypeEncodeHelper<DataType::octstr, std::uint8_t> {};
template <>
struct DataTypeEncodeHelper<DataType::string>
    : VectorDataTypeEncodeHelper<DataType::string, std::uint8_t> {};
template <>
struct DataTypeEncodeHelper<DataType::octstr16>
    : VectorDataTypeEncodeHelper<DataType::octstr16, std::uint16_t> {};
template <>
struct DataTypeEncodeHelper<DataType::string16>
    : VectorDataTypeEncodeHelper<DataType::string16, std::uint16_t> {};
template <>
struct DataTypeEncodeHelper<DataType::_struct>
    : VectorDataTypeEncodeHelper<DataType::_struct, std::uint16_t> {};

// Template magic to select the proper DataTypeEncodeHelper from a run-time
// type.
template <DataType MIN, DataType MAX>
struct VariantEncodeHelper;
template <DataType DT>
struct VariantEncodeHelper<DT, DT> {
  template <typename... T>
  static inline std::size_t GetSize(DataType type,
                                    const boost::variant<T...>& value) {
    if (type != DT) {
      throw std::runtime_error("DataType did not match!");
    }
    return DataTypeEncodeHelper<DT>::GetSize(value);
  }
  template <typename... T>
  static inline void Encode(DataType type, const boost::variant<T...>& value,
                            znp::EncodeTarget::iterator& begin,
                            znp::EncodeTarget::iterator end) {
    if (type != DT) {
      throw std::runtime_error("DataType did not match!");
    }
    DataTypeEncodeHelper<DT>::Encode(value, begin, end);
  }
  template <typename... T>
  static inline void Decode(DataType type, boost::variant<T...>& value,
                            znp::EncodeTarget::const_iterator& begin,
                            znp::EncodeTarget::const_iterator& end) {
    if (type != DT) {
      throw std::runtime_error("DataType did not match!");
    }
    DataTypeEncodeHelper<DT>::Decode(value, begin, end);
  }
};
template <DataType MIN, DataType MAX>
struct VariantEncodeHelper {
  template <typename... T>
  static inline std::size_t GetSize(DataType type,
                                    const boost::variant<T...>& value) {
    constexpr uint8_t mid = (((uint8_t)MIN) + ((uint8_t)MAX)) / 2;
    if ((uint8_t)type > mid) {
      return VariantEncodeHelper<(DataType)(mid + 1), MAX>::GetSize(type,
                                                                    value);
    } else {
      return VariantEncodeHelper<MIN, (DataType)mid>::GetSize(type, value);
    }
  }
  template <typename... T>
  static inline void Encode(DataType type, const boost::variant<T...>& value,
                            znp::EncodeTarget::iterator& begin,
                            znp::EncodeTarget::iterator end) {
    constexpr uint8_t mid = (((uint8_t)MIN) + ((uint8_t)MAX)) / 2;
    if ((uint8_t)type > mid) {
      VariantEncodeHelper<(DataType)(mid + 1), MAX>::Encode(type, value, begin,
                                                            end);
    } else {
      VariantEncodeHelper<MIN, (DataType)mid>::Encode(type, value, begin, end);
    }
  }
  template <typename... T>
  static inline void Decode(DataType type, boost::variant<T...>& value,
                            znp::EncodeTarget::const_iterator& begin,
                            znp::EncodeTarget::const_iterator& end) {
    constexpr uint8_t mid = (((uint8_t)MIN) + ((uint8_t)MAX)) / 2;
    if ((uint8_t)type > mid) {
      VariantEncodeHelper<(DataType)(mid + 1), MAX>::Decode(type, value, begin,
                                                            end);
    } else {
      VariantEncodeHelper<MIN, (DataType)mid>::Decode(type, value, begin, end);
    }
  }
};
}  // namespace zcl
namespace znp {
template <>
class EncodeHelper<zcl::ZclFrame> {
 public:
  static inline std::size_t GetSize(const zcl::ZclFrame& value) {
    if (value.manufacturer_code) {
      return 5 + value.payload.size();
    } else {
      return 3 + value.payload.size();
    }
  }
  static inline void Encode(const zcl::ZclFrame& value,
                            EncodeTarget::iterator& begin,
                            EncodeTarget::iterator end) {
    uint8_t frame_control = 0;
    frame_control |= (uint8_t)value.frame_type;
    frame_control |= (uint8_t)(value.manufacturer_code ? 1 : 0) << 2;
    frame_control |= (uint8_t)value.direction << 3;
    frame_control |= (uint8_t)(value.disable_default_response ? 1 : 0) << 4;
    frame_control |= value.reserved << 5;
    EncodeHelper<uint8_t>::Encode(frame_control, begin, end);
    if (value.manufacturer_code) {
      EncodeHelper<uint16_t>::Encode(*value.manufacturer_code, begin, end);
    }
    EncodeHelper<uint8_t>::Encode(value.transaction_sequence_number, begin,
                                  end);
    EncodeHelper<uint8_t>::Encode(value.command_identifier, begin, end);
    if (end - begin != value.payload.end() - value.payload.begin()) {
      throw std::runtime_error("Encoding buffer was of wrong size");
    }
    std::copy(value.payload.begin(), value.payload.end(), begin);
    begin = end;
  }
  static inline void Decode(zcl::ZclFrame& value,
                            EncodeTarget::const_iterator& begin,
                            EncodeTarget::const_iterator end) {
    uint8_t frame_control;
    EncodeHelper<uint8_t>::Decode(frame_control, begin, end);
    value.frame_type =
        (zcl::ZclFrameType)(uint8_t)((frame_control >> 0) & 0b11);
    bool has_manufacturer_code = ((frame_control >> 2) & 0b1) != 0;
    value.direction = (zcl::ZclDirection)(uint8_t)((frame_control >> 3) & 0b1);
    value.disable_default_response = ((frame_control >> 4) & 0b1) != 0;
    value.reserved = frame_control >> 5;
    if (has_manufacturer_code) {
      uint16_t manufacturer_code;
      EncodeHelper<uint16_t>::Decode(manufacturer_code, begin, end);
      value.manufacturer_code = manufacturer_code;
    } else {
      value.manufacturer_code = boost::none;
    }
    EncodeHelper<uint8_t>::Decode(value.transaction_sequence_number, begin,
                                  end);
    EncodeHelper<uint8_t>::Decode(value.command_identifier, begin, end);
    value.payload = std::vector<uint8_t>(begin, end);
    begin = end;
  }
};
template <>
class EncodeHelper<zcl::ZclVariant> {
 public:
  static inline std::size_t GetSize(const zcl::ZclVariant& variant) {
    return EncodeHelper<zcl::DataType>::GetSize(variant.type_) +
           zcl::VariantEncodeHelper<zcl::DataType::nodata,
                                    zcl::DataType::unk>::GetSize(variant.type_,
                                                                 variant.data_);
  }
  static inline void Encode(const zcl::ZclVariant& variant,
                            EncodeTarget::iterator& begin,
                            EncodeTarget::iterator end) {
    EncodeHelper<zcl::DataType>::Encode(variant.type_, begin, end);
    zcl::VariantEncodeHelper<zcl::DataType::nodata, zcl::DataType::unk>::Encode(
        variant.type_, variant.data_, begin, end);
  }

  static inline void Decode(zcl::ZclVariant& variant,
                            EncodeTarget::const_iterator& begin,
                            EncodeTarget::const_iterator end) {
    EncodeHelper<zcl::DataType>::Decode(variant.type_, begin, end);
    zcl::VariantEncodeHelper<zcl::DataType::nodata, zcl::DataType::unk>::Decode(
        variant.type_, variant.data_, begin, end);
  }
};
}  // namespace znp
#endif  // _ZCL_ENCODING_H_

#ifndef _ZCL_ENCODING_H_
#define _ZCL_ENCODING_H_
#include "zcl/zcl.h"
#include "znp/encoding.h"

namespace znp {
template <>
class EncodeHelper<zcl::ZclFrame> {
 public:
  static inline std::size_t GetSize(const zcl::ZclFrame& value) {
    if (value.manufacturer_code) {
      return 3 + value.payload.size();
    } else {
      return 5 + value.payload.size();
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
 private:
  template <zcl::DataType T>
  static inline void DefaultDecode(zcl::ZclVariant& variant,
                                   EncodeTarget::const_iterator& begin,
                                   EncodeTarget::const_iterator end) {
    typename zcl::DataTypeHelper<T>::Type value;
    EncodeHelper<typename zcl::DataTypeHelper<T>::Type>::Decode(value, begin,
                                                                end);
    variant.data_ = value;
  }
  template <zcl::DataType T, std::size_t N>
  static inline std::enable_if_t<
      std::is_integral<typename zcl::DataTypeHelper<T>::Type>::value &&
      std::is_unsigned<typename zcl::DataTypeHelper<T>::Type>::value>
  IntDecode(zcl::ZclVariant& variant, EncodeTarget::const_iterator& begin,
            EncodeTarget::const_iterator end) {
    typedef typename zcl::DataTypeHelper<T>::Type VT;
    VT value = 0;
    for (std::size_t shift = 0; shift < N; shift += 8) {
      if (begin == end) {
        throw std::runtime_error(
            "Not enough data to decode ZCL unsigned integer");
      }
      value |= ((VT) * (begin++)) << shift;
    }
    variant.data_ = value;
  }
  template <zcl::DataType T, std::size_t N>
  static inline std::enable_if_t<
      std::is_integral<typename zcl::DataTypeHelper<T>::Type>::value &&
      std::is_signed<typename zcl::DataTypeHelper<T>::Type>::value>
  IntDecode(zcl::ZclVariant& variant, EncodeTarget::const_iterator& begin,
            EncodeTarget::const_iterator end) {
    typedef typename zcl::DataTypeHelper<T>::Type VT;
    typedef typename std::make_unsigned<VT>::type UVT;
    UVT value = 0;
    for (std::size_t shift = 0; shift < N; shift += 8) {
      if (begin == end) {
        throw std::runtime_error("Not enough data to decode ZCL integer");
      }
      value |= ((UVT) * (begin++)) << shift;
    }
    variant.data_ = (VT)value;
  }

 public:
  static inline std::size_t GetSize(const zcl::ZclVariant& variant) {
    switch (variant.type_) {
      default:
        LOG("Encoding", critical) << "(GetSize) Unsupported variant type: 0x"
                                  << std::hex << (unsigned int)variant.type_;
        throw std::runtime_error("Unsupported variant type");
        return 0;
    }
  }
  static inline void Encode(const zcl::ZclVariant& variant,
                            EncodeTarget::iterator& begin,
                            EncodeTarget::iterator end) {
    EncodeHelper<zcl::DataType>::Encode(variant.type_, begin, end);
    switch (variant.type_) {
      default:
        LOG("Encoding", critical) << "(Encode) Unsupported variant type: 0x"
                                  << std::hex << (unsigned int)variant.type_;
        throw std::runtime_error("Unsupported variant type");
    }
  }

  static inline void Decode(zcl::ZclVariant& variant,
                            EncodeTarget::const_iterator& begin,
                            EncodeTarget::const_iterator end) {
    EncodeHelper<zcl::DataType>::Decode(variant.type_, begin, end);
    switch (variant.type_) {
      case zcl::DataType::nodata:
        break;
      case zcl::DataType::map8:
        DefaultDecode<zcl::DataType::map8>(variant, begin, end);
        break;
      case zcl::DataType::_bool:
        DefaultDecode<zcl::DataType::_bool>(variant, begin, end);
        break;
      case zcl::DataType::uint8:
        IntDecode<zcl::DataType::uint8, 8>(variant, begin, end);
        break;
      case zcl::DataType::uint16:
        IntDecode<zcl::DataType::uint16, 16>(variant, begin, end);
        break;
      case zcl::DataType::uint24:
        IntDecode<zcl::DataType::uint24, 24>(variant, begin, end);
        break;
      case zcl::DataType::uint32:
        IntDecode<zcl::DataType::uint32, 32>(variant, begin, end);
        break;
      case zcl::DataType::uint40:
        IntDecode<zcl::DataType::uint40, 40>(variant, begin, end);
        break;
      case zcl::DataType::uint48:
        IntDecode<zcl::DataType::uint48, 48>(variant, begin, end);
        break;
      case zcl::DataType::uint56:
        IntDecode<zcl::DataType::uint56, 56>(variant, begin, end);
        break;
      case zcl::DataType::uint64:
        IntDecode<zcl::DataType::uint64, 64>(variant, begin, end);
        break;
      case zcl::DataType::int8:
        IntDecode<zcl::DataType::int8, 8>(variant, begin, end);
        break;
      case zcl::DataType::int16:
        IntDecode<zcl::DataType::int16, 16>(variant, begin, end);
        break;
      case zcl::DataType::int24:
        IntDecode<zcl::DataType::int24, 24>(variant, begin, end);
        break;
      case zcl::DataType::int32:
        IntDecode<zcl::DataType::int32, 32>(variant, begin, end);
        break;
      case zcl::DataType::int40:
        IntDecode<zcl::DataType::int40, 40>(variant, begin, end);
        break;
      case zcl::DataType::int48:
        IntDecode<zcl::DataType::int48, 48>(variant, begin, end);
        break;
      case zcl::DataType::int56:
        IntDecode<zcl::DataType::int56, 56>(variant, begin, end);
        break;
      case zcl::DataType::int64:
        IntDecode<zcl::DataType::int64, 64>(variant, begin, end);
        break;
      case zcl::DataType::string: {
        uint8_t length;
        EncodeHelper<uint8_t>::Decode(length, begin, end);
        if (end - begin < length) {
          throw std::runtime_error("Not enough data for ZCL string");
        }
        variant.data_ = std::string(begin, begin + length);
        begin = begin + length;
        break;
      }
      case zcl::DataType::_struct: {
        uint16_t length;
        EncodeHelper<uint16_t>::Decode(length, begin, end);
        std::vector<zcl::ZclVariant> value((std::size_t)length);
        for (auto& item : value) {
          EncodeHelper<zcl::ZclVariant>::Decode(item, begin, end);
        }
        LOG("Encoding", debug)
            << "Length: " << length << ", Size: " << value.size();
        variant.data_ = value;
        break;
      }
      default:
        LOG("Encoding", critical) << "(Decode) Unsupported variant type: 0x"
                                  << std::hex << (unsigned int)variant.type_;
        throw std::runtime_error("Unsupported variant type");
    }
  }
};
}  // namespace znp
#endif  // _ZCL_ENCODING_H_

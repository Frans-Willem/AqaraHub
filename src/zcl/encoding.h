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
    if (end - begin != value.payload.size()) {
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
        DefaultDecode<zcl::DataType::uint8>(variant, begin, end);
        break;
      case zcl::DataType::uint16:
        DefaultDecode<zcl::DataType::uint16>(variant, begin, end);
        break;
      case zcl::DataType::int16:
        DefaultDecode<zcl::DataType::int16>(variant, begin, end);
        break;
      default:
        LOG("Encoding", critical) << "(Decode) Unsupported variant type: 0x"
                                  << std::hex << (unsigned int)variant.type_;
        throw std::runtime_error("Unsupported variant type");
    }
  }
};
}  // namespace znp
#endif  // _ZCL_ENCODING_H_

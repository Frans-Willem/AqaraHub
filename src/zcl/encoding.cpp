#include <template_lookup.h>
#include <zcl/encoding.h>
#include <znp/encoding.h>
#include <map>
#include <memory>
#include <type_traits>

namespace zcl {
namespace {
// Interface all encoding helpers should implement
struct VariantEncodeHelper {
  virtual ~VariantEncodeHelper() = default;
  virtual std::size_t GetSize(const zcl::ZclVariant& variant) = 0;
  virtual void Encode(const zcl::ZclVariant& variant,
                      znp::EncodeTarget::iterator& begin,
                      znp::EncodeTarget::iterator end) = 0;
  virtual void Decode(zcl::ZclVariant& variant,
                      znp::EncodeTarget::const_iterator& begin,
                      znp::EncodeTarget::const_iterator end) = 0;
};

// Implementation, templated, with some default error messages.
template <DataType DT>
struct VariantEncodeHelperImpl : VariantEncodeHelper {
  std::size_t GetSize(const zcl::ZclVariant& variant) override {
    throw std::runtime_error(boost::str(
        boost::format("Encoding/decoding for datatype %s not yet implemented") %
        enum_to_string(DT)));
  }
  void Encode(const zcl::ZclVariant& variant,
              znp::EncodeTarget::iterator& begin,
              znp::EncodeTarget::iterator end) override {
    throw std::runtime_error(boost::str(
        boost::format("Encoding/decoding for datatype %s not yet implemented") %
        enum_to_string(DT)));
  }
  void Decode(zcl::ZclVariant& variant,
              znp::EncodeTarget::const_iterator& begin,
              znp::EncodeTarget::const_iterator end) override {
    throw std::runtime_error(boost::str(
        boost::format("Encoding/decoding for datatype %s not yet implemented") %
        enum_to_string(DT)));
  }
};

// nodata type
template <>
struct VariantEncodeHelperImpl<DataType::nodata> : VariantEncodeHelper {
  std::size_t GetSize(const zcl::ZclVariant& variant) override { return 0; }
  void Encode(const zcl::ZclVariant& variant,
              znp::EncodeTarget::iterator& begin,
              znp::EncodeTarget::iterator end) override {}
  void Decode(zcl::ZclVariant& variant,
              znp::EncodeTarget::const_iterator& begin,
              znp::EncodeTarget::const_iterator end) override {
    variant = ZclVariant::Create<DataType::nodata>();
  }
};

// Bool type, with support for invalid values.
template <>
struct VariantEncodeHelperImpl<DataType::_bool> : VariantEncodeHelper {
  std::size_t GetSize(const zcl::ZclVariant& variant) override {
    boost::optional<bool> value = variant.Get<DataType::_bool>();
    if (value) return znp::EncodeHelper<bool>::GetSize(*value);
    return znp::EncodeHelper<uint8_t>::GetSize(0xFF);
  }
  void Encode(const zcl::ZclVariant& variant,
              znp::EncodeTarget::iterator& begin,
              znp::EncodeTarget::iterator end) override {
    boost::optional<bool> value = variant.Get<DataType::_bool>();
    if (value) {
      znp::EncodeHelper<bool>::Encode(*value, begin, end);
    } else {
      znp::EncodeHelper<uint8_t>::Encode(0xFF, begin, end);
    }
  }
  void Decode(zcl::ZclVariant& variant,
              znp::EncodeTarget::const_iterator& begin,
              znp::EncodeTarget::const_iterator end) override {
    uint8_t value;
    znp::EncodeHelper<uint8_t>::Decode(value, begin, end);
    if (value == 0xFF) {
      variant = zcl::ZclVariant::Create<DataType::_bool>();
    } else if (value <= 1) {
      variant = zcl::ZclVariant::Create<DataType::_bool>(value > 0);
    } else {
      throw std::runtime_error("Unsupported boolean value");
    }
  }
};

// Implementation to be used by datatypes that can be serialized as-is.
template <DataType DT>
struct DefaultVariantEncodeHelperImpl : VariantEncodeHelper {
  typedef typename DataTypeHelper<DT>::Type ValueType;

  std::size_t GetSize(const zcl::ZclVariant& variant) override {
    boost::optional<ValueType> value = variant.Get<DT>();
    if (!value) {
      throw std::runtime_error("Invalid not supported for this datatype");
    }
    return znp::EncodeHelper<ValueType>::GetSize(*value);
  }

  void Encode(const zcl::ZclVariant& variant,
              znp::EncodeTarget::iterator& begin,
              znp::EncodeTarget::iterator end) override {
    boost::optional<ValueType> value = variant.Get<DT>();
    if (!value) {
      throw std::runtime_error("Invalid not supported for this datatype");
    }
    return znp::EncodeHelper<ValueType>::Encode(*value, begin, end);
  }
  void Decode(zcl::ZclVariant& variant,
              znp::EncodeTarget::const_iterator& begin,
              znp::EncodeTarget::const_iterator end) override {
    ValueType value;
    znp::EncodeHelper<ValueType>::Decode(value, begin, end);
    variant = ZclVariant::Create<DT>(value);
  }
};

template <>
struct VariantEncodeHelperImpl<DataType::data8>
    : DefaultVariantEncodeHelperImpl<DataType::data8> {};
template <>
struct VariantEncodeHelperImpl<DataType::data16>
    : DefaultVariantEncodeHelperImpl<DataType::data16> {};
template <>
struct VariantEncodeHelperImpl<DataType::data24>
    : DefaultVariantEncodeHelperImpl<DataType::data24> {};
template <>
struct VariantEncodeHelperImpl<DataType::data32>
    : DefaultVariantEncodeHelperImpl<DataType::data32> {};
template <>
struct VariantEncodeHelperImpl<DataType::data40>
    : DefaultVariantEncodeHelperImpl<DataType::data40> {};
template <>
struct VariantEncodeHelperImpl<DataType::data48>
    : DefaultVariantEncodeHelperImpl<DataType::data48> {};
template <>
struct VariantEncodeHelperImpl<DataType::data56>
    : DefaultVariantEncodeHelperImpl<DataType::data56> {};
template <>
struct VariantEncodeHelperImpl<DataType::data64>
    : DefaultVariantEncodeHelperImpl<DataType::data64> {};
template <>
struct VariantEncodeHelperImpl<DataType::map8>
    : DefaultVariantEncodeHelperImpl<DataType::map8> {};
template <>
struct VariantEncodeHelperImpl<DataType::map16>
    : DefaultVariantEncodeHelperImpl<DataType::map16> {};
template <>
struct VariantEncodeHelperImpl<DataType::map24>
    : DefaultVariantEncodeHelperImpl<DataType::map24> {};
template <>
struct VariantEncodeHelperImpl<DataType::map32>
    : DefaultVariantEncodeHelperImpl<DataType::map32> {};
template <>
struct VariantEncodeHelperImpl<DataType::map40>
    : DefaultVariantEncodeHelperImpl<DataType::map40> {};
template <>
struct VariantEncodeHelperImpl<DataType::map48>
    : DefaultVariantEncodeHelperImpl<DataType::map48> {};
template <>
struct VariantEncodeHelperImpl<DataType::map56>
    : DefaultVariantEncodeHelperImpl<DataType::map56> {};
template <>
struct VariantEncodeHelperImpl<DataType::map64>
    : DefaultVariantEncodeHelperImpl<DataType::map64> {};

// Implementation for lists
template <DataType DT, typename LT>
struct VectorVariantEncodeHelperImpl : VariantEncodeHelper {
  typedef typename DataTypeHelper<DT>::Type ValueType;
  typedef typename ValueType::value_type ElementType;

  std::size_t GetSize(const zcl::ZclVariant& variant) override {
    boost::optional<ValueType> value = variant.Get<DT>();
    if (!value) {
      return znp::EncodeHelper<LT>::GetSize((LT)-1);
    } else {
      std::size_t retval = znp::EncodeHelper<LT>::GetSize((LT)value->size());
      for (const auto& item : *value) {
        retval += znp::EncodeHelper<ElementType>::GetSize(item);
      }
      return retval;
    }
  }

  void Encode(const zcl::ZclVariant& variant,
              znp::EncodeTarget::iterator& begin,
              znp::EncodeTarget::iterator end) override {
    boost::optional<ValueType> value = variant.Get<DT>();
    if (!value) {
      znp::EncodeHelper<LT>::Encode((LT)-1, begin, end);
    } else {
      znp::EncodeHelper<LT>::Encode((LT)value->size(), begin, end);
      for (const auto& item : *value) {
        znp::EncodeHelper<ElementType>::Encode(item, begin, end);
      }
    }
  }
  void Decode(zcl::ZclVariant& variant,
              znp::EncodeTarget::const_iterator& begin,
              znp::EncodeTarget::const_iterator end) override {
    LT length;
    znp::EncodeHelper<LT>::Decode(length, begin, end);
    if (length == (LT)-1) {
      variant = zcl::ZclVariant::Create<DT>();
      return;
    }
    ValueType value;
    value.resize(length);
    for (auto& item : value) {
      znp::EncodeHelper<ElementType>::Decode(item, begin, end);
    }
    variant = ZclVariant::Create<DT>(value);
  }
};

template <>
struct VariantEncodeHelperImpl<DataType::octstr>
    : VectorVariantEncodeHelperImpl<DataType::octstr, std::uint8_t> {};
template <>
struct VariantEncodeHelperImpl<DataType::string>
    : VectorVariantEncodeHelperImpl<DataType::string, std::uint8_t> {};
template <>
struct VariantEncodeHelperImpl<DataType::octstr16>
    : VectorVariantEncodeHelperImpl<DataType::octstr16, std::uint16_t> {};
template <>
struct VariantEncodeHelperImpl<DataType::string16>
    : VectorVariantEncodeHelperImpl<DataType::string, std::uint16_t> {};
template <>
struct VariantEncodeHelperImpl<DataType::_struct>
    : VectorVariantEncodeHelperImpl<DataType::_struct, std::uint16_t> {};

template <DataType DT, std::size_t N>
struct IntVariantEncodeHelperImpl : VariantEncodeHelper {
  typedef typename DataTypeHelper<DT>::Type ValueType;
  typedef typename std::make_unsigned<ValueType>::type UnsignedType;

  std::size_t GetSize(const ZclVariant& variant) override { return N / 8; }
  void Encode(const ZclVariant& variant, znp::EncodeTarget::iterator& begin,
              znp::EncodeTarget::iterator end) override {
    boost::optional<ValueType> opt_value = variant.Get<DT>();
    ValueType value;
    if (opt_value) {
      value = *opt_value;
    } else if (std::is_unsigned<ValueType>()) {
      value = std::numeric_limits<ValueType>::max();
    } else {
      value = std::numeric_limits<ValueType>::min();
    }

    UnsignedType unsigned_value = (UnsignedType)value;
    for (std::size_t shift = 0; shift < N; shift += 8) {
      if (begin == end) {
        throw std::runtime_error("Not enough space to encode ZCL integer");
      }
      *(begin++) = (uint8_t)((unsigned_value >> shift) & 0xFF);
    }
  }
  void Decode(ZclVariant& variant, znp::EncodeTarget::const_iterator& begin,
              znp::EncodeTarget::const_iterator end) override {
    UnsignedType unsigned_value = 0;
    for (std::size_t shift = 0; shift < N; shift += 8) {
      if (begin == end) {
        throw std::runtime_error("Not enough space to decode ZCL integer");
      }
      unsigned_value |= ((UnsignedType) * (begin++)) << shift;
    }
    ValueType value = (ValueType)unsigned_value;
    ValueType invalid_value = std::is_unsigned<ValueType>()
                                  ? std::numeric_limits<ValueType>::max()
                                  : std::numeric_limits<ValueType>::min();
    if (value == invalid_value) {
      variant = ZclVariant::Create<DT>();
    } else {
      variant = ZclVariant::Create<DT>(value);
    }
  }
};

template <>
struct VariantEncodeHelperImpl<DataType::int8>
    : IntVariantEncodeHelperImpl<DataType::int8, 8> {};
template <>
struct VariantEncodeHelperImpl<DataType::int16>
    : IntVariantEncodeHelperImpl<DataType::int16, 16> {};
template <>
struct VariantEncodeHelperImpl<DataType::int24>
    : IntVariantEncodeHelperImpl<DataType::int24, 24> {};
template <>
struct VariantEncodeHelperImpl<DataType::int32>
    : IntVariantEncodeHelperImpl<DataType::int32, 32> {};
template <>
struct VariantEncodeHelperImpl<DataType::int40>
    : IntVariantEncodeHelperImpl<DataType::int40, 40> {};
template <>
struct VariantEncodeHelperImpl<DataType::int48>
    : IntVariantEncodeHelperImpl<DataType::int48, 48> {};
template <>
struct VariantEncodeHelperImpl<DataType::int56>
    : IntVariantEncodeHelperImpl<DataType::int56, 56> {};
template <>
struct VariantEncodeHelperImpl<DataType::int64>
    : IntVariantEncodeHelperImpl<DataType::int64, 64> {};
template <>
struct VariantEncodeHelperImpl<DataType::uint8>
    : IntVariantEncodeHelperImpl<DataType::uint8, 8> {};
template <>
struct VariantEncodeHelperImpl<DataType::uint16>
    : IntVariantEncodeHelperImpl<DataType::uint16, 16> {};
template <>
struct VariantEncodeHelperImpl<DataType::uint24>
    : IntVariantEncodeHelperImpl<DataType::uint24, 24> {};
template <>
struct VariantEncodeHelperImpl<DataType::uint32>
    : IntVariantEncodeHelperImpl<DataType::uint32, 32> {};
template <>
struct VariantEncodeHelperImpl<DataType::uint40>
    : IntVariantEncodeHelperImpl<DataType::uint40, 40> {};
template <>
struct VariantEncodeHelperImpl<DataType::uint48>
    : IntVariantEncodeHelperImpl<DataType::uint48, 48> {};
template <>
struct VariantEncodeHelperImpl<DataType::uint56>
    : IntVariantEncodeHelperImpl<DataType::uint56, 56> {};
template <>
struct VariantEncodeHelperImpl<DataType::uint64>
    : IntVariantEncodeHelperImpl<DataType::uint64, 64> {};

template <DataType DT, typename IT, std::size_t MAN, std::size_t EXP>
struct FloatVariantEncodeHelperImpl : VariantEncodeHelper {
  typedef typename DataTypeHelper<DT>::Type ValueType;
  typedef typename znp::FloatEncodeHelper<ValueType, IT, MAN, EXP>
      ContainedEncoder;

  std::size_t GetSize(const ZclVariant& variant) override {
    if (auto value = variant.Get<DT>()) {
      return ContainedEncoder::GetSize(*value);
    }
    throw std::runtime_error("Variant did not contain expected value");
    return 0;
  }
  void Encode(const ZclVariant& variant, znp::EncodeTarget::iterator& begin,
              znp::EncodeTarget::iterator end) override {
    if (auto value = variant.Get<DT>()) {
      return ContainedEncoder::Encode(*value, begin, end);
    }
    throw std::runtime_error("Variant did not contain expected value");
  }
  void Decode(ZclVariant& variant, znp::EncodeTarget::const_iterator& begin,
              znp::EncodeTarget::const_iterator end) override {
    ValueType value;
    ContainedEncoder::Decode(value, begin, end);
    variant = ZclVariant::Create<DT>(value);
  }
};

template <>
struct VariantEncodeHelperImpl<DataType::semi>
    : FloatVariantEncodeHelperImpl<DataType::semi, uint16_t, 10, 5> {};
template <>
struct VariantEncodeHelperImpl<DataType::single>
    : FloatVariantEncodeHelperImpl<DataType::single, uint32_t, 23, 8> {};
template <>
struct VariantEncodeHelperImpl<DataType::_double>
    : FloatVariantEncodeHelperImpl<DataType::_double, uint64_t, 52, 11> {};

const std::map<DataType, std::unique_ptr<VariantEncodeHelper>>& EncoderMap() {
  static std::map<DataType, std::unique_ptr<VariantEncodeHelper>> map =
          template_lookup::CreateEnumLookup<DataType, DataType::nodata,
                                            DataType::unk, VariantEncodeHelper,
                                            VariantEncodeHelperImpl>();
  return map;
}
}  // namespace
}  // namespace zcl

namespace znp {
std::size_t EncodeHelper<zcl::ZclVariant>::GetSize(
    const zcl::ZclVariant& variant) {
  auto& map = zcl::EncoderMap();
  auto found = map.find(variant.GetType());
  if (found == map.end()) {
    throw std::runtime_error("Invalid datatype!");
  }
  return EncodeHelper<zcl::DataType>::GetSize(variant.GetType()) +
         found->second->GetSize(variant);
}

void EncodeHelper<zcl::ZclVariant>::Encode(const zcl::ZclVariant& variant,
                                           EncodeTarget::iterator& begin,
                                           EncodeTarget::iterator end) {
  auto& map = zcl::EncoderMap();
  auto found = map.find(variant.GetType());
  if (found == map.end()) {
    throw std::runtime_error("Invalid datatype!");
  }
  EncodeHelper<zcl::DataType>::Encode(variant.GetType(), begin, end);
  found->second->Encode(variant, begin, end);
}

void EncodeHelper<zcl::ZclVariant>::Decode(zcl::ZclVariant& variant,
                                           EncodeTarget::const_iterator& begin,
                                           EncodeTarget::const_iterator end) {
  zcl::DataType datatype;
  EncodeHelper<zcl::DataType>::Decode(datatype, begin, end);
  auto& map = zcl::EncoderMap();
  auto found = map.find(datatype);
  if (found == map.end()) {
    throw std::runtime_error("Invalid datatype!");
  }
  found->second->Decode(variant, begin, end);
}
}  // namespace znp

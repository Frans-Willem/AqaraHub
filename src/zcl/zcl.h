#ifndef _ZCL_ZCL_H_
#define _ZCL_ZCL_H_
#include <bitset>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <cstdint>
#include <iostream>
#include <vector>
#include "znp/encoding.h"

namespace zcl {
enum class ZclFrameType : uint8_t {
  Global = 0,
  Local = 1,
  Reserved1 = 2,
  Reserved2 = 3
};
std::ostream& operator<<(std::ostream& stream, const ZclFrameType& frame_type);

enum class ZclDirection : uint8_t { ClientToServer = 0, ServerToClient = 1 };
std::ostream& operator<<(std::ostream& stream, const ZclDirection& direction);

struct ZclFrame {
  ZclFrameType frame_type;
  boost::optional<uint16_t> manufacturer_code;
  ZclDirection direction;
  bool disable_default_response;
  uint8_t reserved;
  uint8_t transaction_sequence_number;
  uint8_t command_identifier;
  std::vector<uint8_t> payload;
};
std::ostream& operator<<(std::ostream& stream, const ZclFrame& header);

enum class ZclGlobalCommandId : uint8_t {
  ReadAttributes = 0x00,
  ReadAttributesResponse = 0x01,
  WriteAttributes = 0x02,
  WriteAttributesUndivided = 0x03,
  WriteAttributesResponse = 0x04,
  WriteAttributesNoResponse = 0x05,
  ConfigureReporting = 0x06,
  ConfigureReportingResponse = 0x07,
  ReadReportingConfiguration = 0x08,
  ReadReportingConfigurationResponse = 0x09,
  ReportAttributes = 0x0a,
  DefaultResponse = 0x0b,
  DiscoverAttributes = 0x0c,
  DiscoverAttributesResponse = 0x0d
};

enum class ZclClusterId : uint16_t {
  // Deliberately left empty
};
enum class ZclAttributeId : uint16_t {
  // Deliberately left empty
};

enum class ZclGeneralCommand : uint8_t {
  ReadAttributes = 0x00,
  ReadAttributesResponse = 0x01,
  WriteAttributes = 0x02,
  WriteAttributesUndivided = 0x03,
  WriteAttributesResponse = 0x04,
  WriteAttributesNoResponse = 0x05,
  ConfigureReporting = 0x06,
  ConfigureReportingResponse = 0x07,
  ReadReportingConfiguration = 0x08,
  ReadReportingConfigurationResponse = 0x09,
  ReportAttributes = 0x0A,
  DefaultResponse = 0x0B,
  DiscoverAttributes = 0x0C,
  DiscoverAttributesResponse = 0x0D,
  ReadAttributesStructured = 0x0E,
  WriteAttributesStructured = 0x0F,
  WriteAttributesStructuredResponse = 0x10,
  DiscoverCommandsReceived = 0x11,
  DiscoverCommandsReceivedResponse = 0x12,
  DiscoverCommandsGenerated = 0x13,
  DiscoverCommandsGeneratedResponse = 0x14,
  DiscoverAttributesExtended = 0x15,
  DiscoverAttributesExtendedResponse = 0x16
};

enum class DataType : uint8_t {
  nodata = 0x00,
  data8 = 0x08,
  data16 = 0x09,
  data24 = 0x0a,
  data32 = 0x0b,
  data40 = 0x0d,
  data56 = 0x0e,
  data64 = 0x0f,
  _bool = 0x10,  // Prefixed with _ to not clash with C++ keyword.
  map8 = 0x18,
  map16 = 0x19,
  map24 = 0x1a,
  map32 = 0x1b,
  map40 = 0x1c,
  map48 = 0x1d,
  map56 = 0x1e,
  map64 = 0x1f,
  uint8 = 0x20,
  uint16 = 0x21,
  uint24 = 0x22,
  uint32 = 0x23,
  uint40 = 0x24,
  uint48 = 0x25,
  uint56 = 0x26,
  uint64 = 0x27,
  int8 = 0x28,
  int16 = 0x29,
  int24 = 0x2a,
  int32 = 0x2b,
  int40 = 0x2c,
  int48 = 0x2d,
  int56 = 0x2e,
  int64 = 0x2f,
  enum8 = 0x30,
  enum16 = 0x31,
  semi = 0x38,
  single = 0x39,
  _double = 0x3a,
  octstr = 0x41,
  string = 0x42,
  octstr16 = 0x43,
  string16 = 0x44,
  array = 0x48,
  _struct = 0x4c,
  set = 0x50,
  bag = 0x51,
  ToD = 0xe0,
  date = 0xe1,
  UTC = 0xe2,
  clusterId = 0xe8,
  attribId = 0xe9,
  bacOID = 0xea,
  EUI64 = 0xf0,
  key128 = 0xf1,
  unk = 0xff
};

template <DataType DT>
struct DataTypeHelper {
  typedef void Type;
};
template <>
struct DataTypeHelper<DataType::nodata> {
  typedef void Type;
};
template <>
struct DataTypeHelper<DataType::_bool> {
  typedef bool Type;
};
template <>
struct DataTypeHelper<DataType::map8> {
  typedef std::bitset<8> Type;
};
template <>
struct DataTypeHelper<DataType::map16> {
  typedef std::bitset<16> Type;
};
template <>
struct DataTypeHelper<DataType::map24> {
  typedef std::bitset<24> Type;
};
template <>
struct DataTypeHelper<DataType::map32> {
  typedef std::bitset<32> Type;
};
template <>
struct DataTypeHelper<DataType::map40> {
  typedef std::bitset<40> Type;
};
template <>
struct DataTypeHelper<DataType::map48> {
  typedef std::bitset<48> Type;
};
template <>
struct DataTypeHelper<DataType::map56> {
  typedef std::bitset<56> Type;
};
template <>
struct DataTypeHelper<DataType::map64> {
  typedef std::bitset<64> Type;
};
template <>
struct DataTypeHelper<DataType::uint8> {
  typedef uint8_t Type;
};
template <>
struct DataTypeHelper<DataType::uint16> {
  typedef uint16_t Type;
};
template <>
struct DataTypeHelper<DataType::uint24> {
  typedef uint32_t Type;
};
template <>
struct DataTypeHelper<DataType::uint32> {
  typedef uint32_t Type;
};
template <>
struct DataTypeHelper<DataType::uint40> {
  typedef uint64_t Type;
};
template <>
struct DataTypeHelper<DataType::uint48> {
  typedef uint64_t Type;
};
template <>
struct DataTypeHelper<DataType::uint56> {
  typedef uint64_t Type;
};
template <>
struct DataTypeHelper<DataType::uint64> {
  typedef uint64_t Type;
};
template <>
struct DataTypeHelper<DataType::int8> {
  typedef int8_t Type;
};
template <>
struct DataTypeHelper<DataType::int16> {
  typedef int16_t Type;
};
template <>
struct DataTypeHelper<DataType::int24> {
  typedef int32_t Type;
};
template <>
struct DataTypeHelper<DataType::int32> {
  typedef int32_t Type;
};
template <>
struct DataTypeHelper<DataType::int40> {
  typedef int64_t Type;
};
template <>
struct DataTypeHelper<DataType::int48> {
  typedef int64_t Type;
};
template <>
struct DataTypeHelper<DataType::int56> {
  typedef int64_t Type;
};
template <>
struct DataTypeHelper<DataType::int64> {
  typedef int64_t Type;
};
template <>
struct DataTypeHelper<DataType::semi> {
  typedef double Type;  // Double instead of float, as boost::variant only
                        // allows 20 types :(
};
template <>
struct DataTypeHelper<DataType::single> {
  typedef double Type;  // Double instead of float, as boost::variant only
                        // allows 20 types :(
};
template <>
struct DataTypeHelper<DataType::_double> {
  typedef double Type;
};
template <>
struct DataTypeHelper<DataType::octstr> {
  typedef std::string Type;
};
template <>
struct DataTypeHelper<DataType::string> {
  typedef std::string Type;
};
template <>
struct DataTypeHelper<DataType::octstr16> {
  typedef std::string Type;
};
template <>
struct DataTypeHelper<DataType::string16> {
  typedef std::string Type;
};
class ZclVariant;
template <>
struct DataTypeHelper<DataType::_struct> {
  typedef std::vector<ZclVariant> Type;
};

// Helper template-magic to check if type T is contained in types R...
template <typename T, typename... R>
struct TypeContained;
template <typename T>
struct TypeContained<T> : std::false_type {};
template <typename T, typename... R>
struct TypeContained<T, T, R...> : std::true_type {};
template <typename T, typename T2, typename... R>
struct TypeContained<T, T2, R...> : TypeContained<T, R...> {};

// Combines a type T, with a boost::variant<...>, to a combined boost::variant
// voids are ignored, and passing void instead of a boost::variant as the second
// parameter attempts to create a new boost::variant.
template <typename T, typename V>
struct AddTypeToVariantHelper;
template <typename T, typename... R>
struct AddTypeToVariantHelper<T, boost::variant<R...>> {
  typedef typename std::conditional<
      TypeContained<T, R...>::value || std::is_void<T>::value,
      boost::variant<R...>, boost::variant<T, R...>>::type Type;
};
template <typename T>
struct AddTypeToVariantHelper<T, void> {
  typedef boost::variant<T> Type;
};
template <>
struct AddTypeToVariantHelper<void, void> {
  typedef void Type;
};

// Create a boost::variant able to hold all types corresponding to to DataTypes
// from begin to end (inclusive)
template <DataType begin, DataType end>
struct DataTypeVariantHelper;
template <DataType DT>
struct DataTypeVariantHelper<DT, DT> {
  typedef typename AddTypeToVariantHelper<typename DataTypeHelper<DT>::Type,
                                          void>::Type Type;
};
template <DataType B, DataType E>
struct DataTypeVariantHelper {
  typedef typename AddTypeToVariantHelper<
      typename DataTypeHelper<B>::Type,
      typename DataTypeVariantHelper<(DataType)((uint8_t)B + 1), E>::Type>::Type
      Type;
};

template <typename T>
struct to_json_helper;
class ZclVariant {
 public:
  ZclVariant() { type_ = DataType::nodata; }
  template <DataType T>
  static ZclVariant Create(typename DataTypeHelper<T>::Type value) {
    ZclVariant retval;
    retval.type_ = T;
    retval.data_ = value;
    return retval;
  }
  inline DataType GetType() const { return type_; }
  template <DataType T>
  boost::optional<typename DataTypeHelper<T>::Type> Get() const {
    if (type_ != T) {
      return boost::none;
    }
    return boost::get<typename DataTypeHelper<T>::Type>(data_);
  }

 private:
  typename DataTypeVariantHelper<DataType::nodata, DataType::unk>::Type data_;
  DataType type_;

  friend class znp::EncodeHelper<zcl::ZclVariant>;
  friend class zcl::to_json_helper<zcl::ZclVariant>;
  friend std::ostream& operator<<(std::ostream& stream,
                                  const ZclVariant& variant);
};
}  // namespace zcl
#endif  // _ZCL_ZCL_H_

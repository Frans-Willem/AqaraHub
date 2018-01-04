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
template <DataType T>
class DataTypeHelper;

template <>
class DataTypeHelper<DataType::nodata> {
 public:
  typedef void Type;
};
template <>
class DataTypeHelper<DataType::_bool> {
 public:
  typedef bool Type;
};
template <>
class DataTypeHelper<DataType::map8> {
 public:
  typedef std::bitset<8> Type;
};
template <>
class DataTypeHelper<DataType::map16> {
 public:
  typedef std::bitset<16> Type;
};
template <>
class DataTypeHelper<DataType::map24> {
 public:
  typedef std::bitset<24> Type;
};
template <>
class DataTypeHelper<DataType::map32> {
 public:
  typedef std::bitset<32> Type;
};
template <>
class DataTypeHelper<DataType::map40> {
 public:
  typedef std::bitset<40> Type;
};
template <>
class DataTypeHelper<DataType::map48> {
 public:
  typedef std::bitset<48> Type;
};
template <>
class DataTypeHelper<DataType::map56> {
 public:
  typedef std::bitset<56> Type;
};
template <>
class DataTypeHelper<DataType::map64> {
 public:
  typedef std::bitset<64> Type;
};
template <>
class DataTypeHelper<DataType::uint8> {
 public:
  typedef uint8_t Type;
};
template <>
class DataTypeHelper<DataType::uint16> {
 public:
  typedef uint16_t Type;
};
template <>
class DataTypeHelper<DataType::uint24> {
 public:
  typedef uint32_t Type;
};
template <>
class DataTypeHelper<DataType::uint32> {
 public:
  typedef uint32_t Type;
};
template <>
class DataTypeHelper<DataType::uint40> {
 public:
  typedef uint64_t Type;
};
template <>
class DataTypeHelper<DataType::uint48> {
 public:
  typedef uint64_t Type;
};
template <>
class DataTypeHelper<DataType::uint56> {
 public:
  typedef uint64_t Type;
};
template <>
class DataTypeHelper<DataType::uint64> {
 public:
  typedef uint64_t Type;
};
template <>
class DataTypeHelper<DataType::int8> {
 public:
  typedef int8_t Type;
};
template <>
class DataTypeHelper<DataType::int16> {
 public:
  typedef int16_t Type;
};
template <>
class DataTypeHelper<DataType::int24> {
 public:
  typedef int32_t Type;
};
template <>
class DataTypeHelper<DataType::int32> {
 public:
  typedef int32_t Type;
};
template <>
class DataTypeHelper<DataType::int40> {
 public:
  typedef int64_t Type;
};
template <>
class DataTypeHelper<DataType::int48> {
 public:
  typedef int64_t Type;
};
template <>
class DataTypeHelper<DataType::int56> {
 public:
  typedef int64_t Type;
};
template <>
class DataTypeHelper<DataType::int64> {
 public:
  typedef int64_t Type;
};
template <>
class DataTypeHelper<DataType::string> {
 public:
  typedef std::string Type;
};
class ZclVariant;
template <>
class DataTypeHelper<DataType::_struct> {
 public:
  typedef std::vector<ZclVariant> Type;
};

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
  };
  /*
  template <>
  boost::optional<void> Get<DataType::nodata>() {
    return type_ == DataType::none;
  }
  */

 private:
  boost::variant<bool, uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t,
                 int32_t, int64_t, std::bitset<8>, std::string,
                 std::vector<ZclVariant>>
      data_;
  DataType type_;

  friend class znp::EncodeHelper<zcl::ZclVariant>;
  friend std::ostream& operator<<(std::ostream& stream,
                                  const ZclVariant& variant);
};
}  // namespace zcl
#endif  // _ZCL_ZCL_H_

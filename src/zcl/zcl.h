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

enum class ZclClusterId : uint16_t {
  // Deliberately left empty
};
enum class ZclAttributeId : uint16_t {
  // Deliberately left empty
};
enum class ZclCommandId : uint8_t {
  // Deliberately left empty
};

struct ZclFrame {
  ZclFrameType frame_type;
  boost::optional<uint16_t> manufacturer_code;
  ZclDirection direction;
  bool disable_default_response;
  uint8_t reserved;
  uint8_t transaction_sequence_number;
  ZclCommandId command_identifier;
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
  data40 = 0x0c,
  data48 = 0x0d,
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
struct DataTypeHelper<DataType::data8> {
  typedef std::array<std::uint8_t, 1> Type;
};
template <>
struct DataTypeHelper<DataType::data16> {
  typedef std::array<std::uint8_t, 2> Type;
};
template <>
struct DataTypeHelper<DataType::data24> {
  typedef std::array<std::uint8_t, 3> Type;
};
template <>
struct DataTypeHelper<DataType::data32> {
  typedef std::array<std::uint8_t, 4> Type;
};
template <>
struct DataTypeHelper<DataType::data40> {
  typedef std::array<std::uint8_t, 5> Type;
};
template <>
struct DataTypeHelper<DataType::data48> {
  typedef std::array<std::uint8_t, 6> Type;
};
template <>
struct DataTypeHelper<DataType::data56> {
  typedef std::array<std::uint8_t, 7> Type;
};
template <>
struct DataTypeHelper<DataType::data64> {
  typedef std::array<std::uint8_t, 8> Type;
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
  typedef float Type;
};
template <>
struct DataTypeHelper<DataType::single> {
  typedef double Type;
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

template <typename T, typename Enable = void>
struct VariantStorageHelper {
  typedef T StorageType;
  static StorageType ToStorage(T input) { return input; }
  static T FromStorage(StorageType storage) { return storage; }
};
template <typename T>
struct VariantStorageHelper<
    T,
    std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value &&
                     std::numeric_limits<T>::digits <= 64>> {
  typedef uint64_t StorageType;
  static StorageType ToStorage(T input) { return (StorageType)input; }
  static T FromStorage(StorageType storage) { return (T)storage; }
};
template <typename T>
struct VariantStorageHelper<
    T,
    std::enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value &&
                     std::numeric_limits<T>::digits <= 64>> {
  typedef int64_t StorageType;
  static StorageType ToStorage(T input) { return (StorageType)input; }
  static T FromStorage(StorageType storage) { return (T)storage; }
};
template <std::size_t N>
struct VariantStorageHelper<std::bitset<N>, std::enable_if_t<N <= 64>> {
  typedef uint64_t StorageType;
  static StorageType ToStorage(std::bitset<N> input) {
    return (StorageType)input.to_ullong();
  }
  static std::bitset<N> FromStorage(StorageType storage) {
    return std::bitset<N>(storage);
  }
};

template <typename T>
struct VariantStorageHelper<
    T, std::enable_if_t<std::is_floating_point<T>::value &&
                        std::numeric_limits<T>::digits <=
                            std::numeric_limits<double>::digits>> {
  typedef double StorageType;
  static StorageType ToStorage(T input) { return (StorageType)input; }
  static T FromStorage(StorageType storage) { return (T)storage; }
};
template <>
struct VariantStorageHelper<bool> {
  typedef uint64_t StorageType;
  static StorageType ToStorage(bool input) { return input ? 1 : 0; }
  static bool FromStorage(StorageType storage) { return storage > 0; }
};
template <std::size_t N>
struct VariantStorageHelper<std::array<std::uint8_t, N>> {
  typedef std::string StorageType;
  static StorageType ToStorage(const std::array<std::uint8_t, N>& input) {
    return StorageType(input.begin(), input.end());
  }
  static std::array<std::uint8_t, N> FromStorage(const StorageType& storage) {
    std::array<std::uint8_t, N> ret{{0}};
    if (storage.size() >= N) {
      std::copy(storage.begin(), storage.begin() + N, ret.begin());
    } else {
      std::copy(storage.begin(), storage.end(), ret.begin());
    }
    return ret;
  };
};

template <DataType DT, typename... I>
void SetVariant(boost::optional<boost::variant<I...>>& storage,
                typename DataTypeHelper<DT>::Type value) {
  typedef typename DataTypeHelper<DT>::Type ValueType;
  typedef VariantStorageHelper<ValueType> StorageHelper;
  storage = StorageHelper::ToStorage(value);
}

template <typename T>
struct to_json_helper;
class ZclVariant {
 public:
  ZclVariant() { type_ = DataType::nodata; }
  template <DataType T>
  static ZclVariant Create(typename DataTypeHelper<T>::Type value) {
    ZclVariant retval;
    retval.type_ = T;
    SetVariant<T>(retval.data_, value);
    return retval;
  }
  template <DataType T>
  static ZclVariant Create() {
    ZclVariant retval;
    retval.type_ = T;
    retval.data_ = boost::none;
    return retval;
  }
  inline DataType GetType() const { return type_; }
  template <DataType T>
  boost::optional<typename DataTypeHelper<T>::Type> Get() const {
    typedef typename DataTypeHelper<T>::Type ValueType;
    typedef typename VariantStorageHelper<ValueType>::StorageType StorageType;
    if (!data_) {
      return boost::none;
    }
    if (type_ != T) {
      return boost::none;
    }
    const StorageType* storage_ptr = boost::relaxed_get<StorageType>(&*data_);
    if (!storage_ptr) {
      return boost::none;
    }
    return VariantStorageHelper<ValueType>::FromStorage(*storage_ptr);
  }

  bool operator==(const ZclVariant& other) const;

 private:
  boost::optional<boost::variant<uint64_t, int64_t, double, std::string,
                                 std::vector<ZclVariant>>>
      data_;
  DataType type_;

  friend class znp::EncodeHelper<zcl::ZclVariant>;
  friend struct zcl::to_json_helper<zcl::ZclVariant>;
  friend std::ostream& operator<<(std::ostream& stream,
                                  const ZclVariant& variant);
};
}  // namespace zcl
#endif  // _ZCL_ZCL_H_

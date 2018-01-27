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

enum class ZclClusterId : uint16_t {
  Basic = 0x0000,
  PowerConfiguration = 0x0001,
  DeviceTemperatureConfiguration = 0x0002,
  Identify = 0x0003,
  Groups = 0x0004,
  Scenes = 0x0005,
  OnOff = 0x0006,
  OnOffSwitchConfiguration = 0x0007,
  LevelControl = 0x0008,
  Alarms = 0x0009,
  Time = 0x000a,
  RSSILocation = 0x000b,  // or just RSSI ?
  AnalogInput = 0x000c,
  AnalogOutput = 0x000d,
  AnalogValue = 0x000e,
  BinaryInput = 0x000f,
  BinaryOutput = 0x0010,
  BinaryValue = 0x0011,
  MultistateInput = 0x0012,
  MultistateOutput = 0x0013,
  MultistateValue = 0x0014,
  Commissioning = 0x0015,
  Partition = 0x0016,
  OTA = 0x0019,
  PowerProfile = 0x001a,
  EN50523ApplianceControl = 0x001b,
  PollControl = 0x0020,
  MobileDeviceConfiguration = 0x0022,
  NeighborCleaning = 0x0023,
  NearestGateway = 0x0024,
  ShadeConfiguration = 0x0100,
  DoorLock = 0x0101,
  WindowCovering = 0x0102,
  PumpConfigurationAndControl = 0x0200,
  Thermostat = 0x0201,
  FanControl = 0x0202,
  DehumidificationControl = 0x0203,
  ThermostatUserInterfaceConfiguration = 0x0204,
  ColorControl = 0x0300,
  BallastConfiguration = 0x0301,
  IlluminanceMeasurement = 0x0400,
  IlluminanceLevelSensing = 0x0401,
  TemperatureMeasurement = 0x0402,
  PressureMeasurement = 0x0403,
  FlowMeasurement = 0x0404,
  RelativeHumidityMeasurement = 0x0405,
  OccupancySensing = 0x0406,
  IASZone = 0x0500,
  IASACE = 0x0501,
  IASWD = 0x0502,
  GenericTunnel = 0x0600,
  BACnetProtocolTunnel = 0x0601,
  AnalogInputBACnetRegular = 0x0602,
  AnalogInputBACnetExtended = 0x0603,
  AnalogOutputBACnetRegular = 0x0604,
  AnalogOutputBACnetExtended = 0x0605,
  AnalogValueBACnetRegular = 0x0606,
  AnalogValueBACnetExtended = 0x0607,
  BinaryInputBACnetRegular = 0x0608,
  BinaryInputBACnetExtended = 0x0609,
  BinaryOutputBACnetRegular = 0x060a,
  BinaryOutputBACnetExtended = 0x060b,
  BinaryValueBACnetRegular = 0x060c,
  BinaryValyeBACnetExtended = 0x060d,
  MultistateInputBACnetRegular = 0x060e,
  MultistateInputBACnetExtended = 0x060f,
  MultistateOutputBACnetRegular = 0x0610,
  MultistateOutputBACnetExtended = 0x0611,
  MultistateValueBACnetRegular = 0x0612,
  MultistateValueBACnetExtended = 0x0613,
  _11073ProtocolTunnel = 0x0614,
  ISO7818ProtocolTunnel = 0x0615,
  RetailTunnel = 0x0617,
  Price = 0x0700,
  DemandResponseAndLoadControl = 0x0701,
  Metering = 0x0702,
  Messaging = 0x0703,
  Tunneling = 0x0704,
  Information = 0x0900,
  Chatting = 0x0905,
  VoiceOverZigbee = 0x0904,
  EN50523ApplianceIdentification = 0x0b00,
  MeterIdentification = 0x0b01,
  EN50523ApplianceEventsAndAlerts = 0x0b02,
  EN50523ApplianceStatistics = 0x0b03,
  ElectricalMeasurement = 0x0b04,
  Diagnostics = 0x0b05,
  TouchlinkCommissioning = 0x1000,
};

std::ostream& operator<<(std::ostream& stream, const ZclClusterId& cluster_id);

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

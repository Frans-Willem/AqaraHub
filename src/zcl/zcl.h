#ifndef _ZCL_ZCL_H_
#define _ZCL_ZCL_H_
#include <boost/optional.hpp>
#include <cstdint>
#include <iostream>
#include <vector>

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
}  // namespace zcl
#endif  // _ZCL_ZCL_H_

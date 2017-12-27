#include "zcl/zcl.h"

namespace zcl {
std::ostream& operator<<(std::ostream& stream, const ZclFrameType& frame_type) {
  switch (frame_type) {
    case ZclFrameType::Global:
      return stream << "Global";
    case ZclFrameType::Local:
      return stream << "Local";
    default:
      return stream << "ZclFrameType(" << (unsigned int)frame_type << ")";
  }
}

std::ostream& operator<<(std::ostream& stream, const ZclDirection& direction) {
  switch (direction) {
    case ZclDirection::ClientToServer:
      return stream << "ClientToServer";
    case ZclDirection::ServerToClient:
      return stream << "ServerToClient";
  }
}

std::ostream& operator<<(std::ostream& stream, const ZclFrame& header) {
  stream << "{frame_type: " << header.frame_type;
  if (header.manufacturer_code) {
    stream << ", manufacturer_code: " << *header.manufacturer_code;
  } else {
    stream << ", manufacturer_code: "
           << "none";
  }
  stream << ", direction: " << header.direction
         << ", disable_default_response: "
         << (header.disable_default_response ? "true" : "false")
         << ", reserved: " << header.reserved
         << ", transaction_sequence_number: "
         << (unsigned int)header.transaction_sequence_number
         << ", command_identifier: " << (unsigned int)header.command_identifier
         << "}";
  return stream;
}
}  // namespace zcl

#include "znp/simpleapi/simpleapi.h"

namespace znp {
namespace simpleapi {
std::ostream& operator<<(std::ostream& stream, SimpleAPICommand command) {
  switch (command) {
    case SimpleAPICommand::START_REQUEST:
      return stream << "START_REQUEST";
    case SimpleAPICommand::BIND_DEVICE:
      return stream << "BIND_DEVICE";
    case SimpleAPICommand::ALLOW_BIND:
      return stream << "ALLOW_BIND";
    case SimpleAPICommand::SEND_DATA_REQUEST:
      return stream << "SEND_DATA_REQUEST";
    case SimpleAPICommand::READ_CONFIGURATION:
      return stream << "READ_CONFIGURATION";
    case SimpleAPICommand::WRITE_CONFIGURATION:
      return stream << "WRITE_CONFIGURATION";
    case SimpleAPICommand::GET_DEVICE_INFO:
      return stream << "GET_DEVICE_INFO";
    case SimpleAPICommand::FIND_DEVICE_REQUEST:
      return stream << "FIND_DEVICE_REQUEST";
    case SimpleAPICommand::PERMIT_JOINING_REQUEST:
      return stream << "PERMIT_JOINING_REQUEST";
    case SimpleAPICommand::SYSTEM_RESET:
      return stream << "SYSTEM_RESET";
    case SimpleAPICommand::START_CONFIRM:
      return stream << "START_CONFIRM";
    case SimpleAPICommand::BIND_CONFIRM:
      return stream << "BIND_CONFIRM";
    case SimpleAPICommand::ALLOW_BIND_CONFIRM:
      return stream << "ALLOW_BIND_CONFIRM";
    case SimpleAPICommand::SEND_DATA_CONFIRM:
      return stream << "SEND_DATA_CONFIRM";
    case SimpleAPICommand::FIND_DEVICE_CONFIRM:
      return stream << "FIND_DEVICE_CONFIRM";
    case SimpleAPICommand::RECEIVE_DATA_INDICATION:
      return stream << "RECEIVE_DATA_INDICATION";
    default:
      return stream << "SimpleAPICommand(" << (unsigned int)command << ")";
  }
}
}  // namespace simpleapi
}  // namespace znp

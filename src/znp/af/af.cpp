#include "znp/af/af.h"

namespace znp {
namespace af {
std::ostream& operator<<(std::ostream& stream, AfCommand command) {
  switch (command) {
    case AfCommand::REGISTER:
      return stream << "REGISTER";
    case AfCommand::DATA_REQUEST:
      return stream << "DATA_REQUEST";
    case AfCommand::DATA_REQUEST_EXT:
      return stream << "DATA_REQUEST_EXT";
    case AfCommand::DATA_REQUEST_SRC_RTG:
      return stream << "DATA_REQUEST_SRC_RTG";
    case AfCommand::INTER_PAN_CTL:
      return stream << "INTER_PAN_CTL";
    case AfCommand::DATA_STORE:
      return stream << "DATA_STORE";
    case AfCommand::DATA_RETRIEVE:
      return stream << "DATA_RETRIEVE";
    case AfCommand::APSF_CONFIG_SET:
      return stream << "APSF_CONFIG_SET";
    case AfCommand::DATA_CONFIRM:
      return stream << "DATA_CONFIRM";
    case AfCommand::REFLECT_ERROR:
      return stream << "REFLECT_ERROR";
    case AfCommand::INCOMING_MSG:
      return stream << "INCOMING_MSG";
    case AfCommand::INCOMING_MSG_EXT:
      return stream << "INCOMING_MSG_EXT";
    default:
      return stream << "AfCommand(" << (unsigned int)command << ")";
  }
}
}  // namespace af
}  // namespace znp

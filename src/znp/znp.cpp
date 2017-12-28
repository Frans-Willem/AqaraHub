#include "znp/znp.h"
#include <boost/variant/get.hpp>
#include <map>

namespace znp {
std::ostream& operator<<(std::ostream& stream, const ZnpSubsystem& subsys) {
  switch (subsys) {
    case ZnpSubsystem::RPC_Error:
      return stream << "RPC_Error";
    case ZnpSubsystem::SYS:
      return stream << "SYS";
    case ZnpSubsystem::MAC:
      return stream << "MAC";
    case ZnpSubsystem::NWK:
      return stream << "NWK";
    case ZnpSubsystem::AF:
      return stream << "AF";
    case ZnpSubsystem::ZDO:
      return stream << "ZDO";
    case ZnpSubsystem::SAPI:
      return stream << "SAPI";
    case ZnpSubsystem::UTIL:
      return stream << "UTIL";
    case ZnpSubsystem::DEBUG:
      return stream << "DEBUG";
    case ZnpSubsystem::APP:
      return stream << "APP";
    default:
      return stream << "UNK(" << (int)subsys << ")";
  }
}

std::ostream& operator<<(std::ostream& stream, const ZnpCommandType& type) {
  switch (type) {
    case ZnpCommandType::POLL:
      return stream << "POLL";
    case ZnpCommandType::SREQ:
      return stream << "SREQ";
    case ZnpCommandType::AREQ:
      return stream << "AREQ";
    case ZnpCommandType::SRSP:
      return stream << "SRSP";
    default:
      return stream << "UNK(" << (int)type << ")";
  }
}

std::ostream& operator<<(std::ostream& stream, SysCommand command) {
  switch (command) {
    case SysCommand::RESET:
      return stream << "RESET";
    case SysCommand::PING:
      return stream << "PING";
    case SysCommand::VERSION:
      return stream << "VERSION";
    case SysCommand::SET_EXTADDR:
      return stream << "SET_EXTADDR";
    case SysCommand::GET_EXTADDR:
      return stream << "GET_EXTADDR";
    case SysCommand::RAM_READ:
      return stream << "RAM_READ";
    case SysCommand::RAM_WRITE:
      return stream << "RAM_WRITE";
    case SysCommand::OSAL_NV_ITEM_INIT:
      return stream << "OSAL_NV_ITEM_INIT";
    case SysCommand::OSAL_NV_READ:
      return stream << "OSAL_NV_READ";
    case SysCommand::OSAL_NV_WRITE:
      return stream << "OSAL_NV_WRITE";
    case SysCommand::OSAL_START_TIMER:
      return stream << "OSAL_START_TIMER";
    case SysCommand::OSAL_STOP_TIMER:
      return stream << "OSAL_STOP_TIMER";
    case SysCommand::RANDOM:
      return stream << "RANDOM";
    case SysCommand::ADC_READ:
      return stream << "ADC_READ";
    case SysCommand::GPIO:
      return stream << "GPIO";
    case SysCommand::STACK_TUNE:
      return stream << "STACK_TUNE";
    case SysCommand::SET_TIME:
      return stream << "SET_TIME";
    case SysCommand::GET_TIME:
      return stream << "GET_TIME";
    case SysCommand::OSAL_NV_DELETE:
      return stream << "OSAL_NV_DELETE";
    case SysCommand::OSAL_NV_LENGTH:
      return stream << "OSAL_NV_LENGTH";
    case SysCommand::TEST_RF:
      return stream << "TEST_RF";
    case SysCommand::TEST_LOOPBACK:
      return stream << "TEST_LOOPBACK";
    case SysCommand::RESET_IND:
      return stream << "RESET_IND";
    case SysCommand::OSAL_TIMER_EXPIRED:
      return stream << "OSAL_TIMER_EXPIRED";
    default:
      return stream << std::hex << (unsigned int)command;
  }
}

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

std::ostream& operator<<(std::ostream& stream, ZdoCommand command) {
  switch (command) {
    case ZdoCommand::NWK_ADDR_REQ:
      return stream << "NWK_ADDR_REQ";
    case ZdoCommand::IEEE_ADDR_REQ:
      return stream << "IEEE_ADDR_REQ";
    case ZdoCommand::NODE_DESC_REQ:
      return stream << "NODE_DESC_REQ";
    case ZdoCommand::POWER_DESC_REQ:
      return stream << "POWER_DESC_REQ";
    case ZdoCommand::SIMPLE_DESC_REQ:
      return stream << "SIMPLE_DESC_REQ";
    case ZdoCommand::ACTIVE_EP_REQ:
      return stream << "ACTIVE_EP_REQ";
    case ZdoCommand::MATCH_DESC_REQ:
      return stream << "MATCH_DESC_REQ";
    case ZdoCommand::COMPLEX_DESC_REQ:
      return stream << "COMPLEX_DESC_REQ";
    case ZdoCommand::USER_DESC_REQ:
      return stream << "USER_DESC_REQ";
    case ZdoCommand::DEVICE_ANNCE:
      return stream << "DEVICE_ANNCE";
    case ZdoCommand::USER_DESC_SET:
      return stream << "USER_DESC_SET";
    case ZdoCommand::SERVER_DISC_REQ:
      return stream << "SERVER_DISC_REQ";
    case ZdoCommand::END_DEVICE_BIND_REQ:
      return stream << "END_DEVICE_BIND_REQ";
    case ZdoCommand::BIND_REQ:
      return stream << "BIND_REQ";
    case ZdoCommand::UNBIND_REQ:
      return stream << "UNBIND_REQ";
    case ZdoCommand::SET_LINK_KEY:
      return stream << "SET_LINK_KEY";
    case ZdoCommand::REMOVE_LINK_KEY:
      return stream << "REMOVE_LINK_KEY";
    case ZdoCommand::GET_LINK_KEY:
      return stream << "GET_LINK_KEY";
    case ZdoCommand::MGMT_NWK_DISC_REQ:
      return stream << "MGMT_NWK_DISC_REQ";
    case ZdoCommand::MGMT_LQI_REQ:
      return stream << "MGMT_LQI_REQ";
    case ZdoCommand::MGMT_RTQ_REQ:
      return stream << "MGMT_RTQ_REQ";
    case ZdoCommand::MGMT_BIND_REQ:
      return stream << "MGMT_BIND_REQ";
    case ZdoCommand::MGMT_LEAVE_REQ:
      return stream << "MGMT_LEAVE_REQ";
    case ZdoCommand::MGMT_DIRECT_JOIN_REQ:
      return stream << "MGMT_DIRECT_JOIN_REQ";
    case ZdoCommand::MGMT_PERMIT_JOIN_REQ:
      return stream << "MGMT_PERMIT_JOIN_REQ";
    case ZdoCommand::MGMT_NWK_UPDATE_REQ:
      return stream << "MGMT_NWK_UPDATE_REQ";
    case ZdoCommand::MSG_CB_REGISTER:
      return stream << "MSG_CB_REGISTER";
    case ZdoCommand::MGS_CB_REMOVE:
      return stream << "MGS_CB_REMOVE";
    case ZdoCommand::STARTUP_FROM_APP:
      return stream << "STARTUP_FROM_APP";
    case ZdoCommand::AUTO_FIND_DESTINATION:
      return stream << "AUTO_FIND_DESTINATION";
    case ZdoCommand::NWK_ADDR_RSP:
      return stream << "NWK_ADDR_RSP";
    case ZdoCommand::IEEE_ADDR_RSP:
      return stream << "IEEE_ADDR_RSP";
    case ZdoCommand::NODE_DESC_RSP:
      return stream << "NODE_DESC_RSP";
    case ZdoCommand::POWER_DESC_RSP:
      return stream << "POWER_DESC_RSP";
    case ZdoCommand::SIMPLE_DESC_RSP:
      return stream << "SIMPLE_DESC_RSP";
    case ZdoCommand::ACTIVE_EP_RSP:
      return stream << "ACTIVE_EP_RSP";
    case ZdoCommand::MATCH_DESC_RSP:
      return stream << "MATCH_DESC_RSP";
    case ZdoCommand::COMPLEX_DESC_RSP:
      return stream << "COMPLEX_DESC_RSP";
    case ZdoCommand::USER_DESC_RSP:
      return stream << "USER_DESC_RSP";
    case ZdoCommand::USER_DESC_CONF:
      return stream << "USER_DESC_CONF";
    case ZdoCommand::SERVER_DISC_RSP:
      return stream << "SERVER_DISC_RSP";
    case ZdoCommand::END_DEVICE_BIND_RSP:
      return stream << "END_DEVICE_BIND_RSP";
    case ZdoCommand::BIND_RSP:
      return stream << "BIND_RSP";
    case ZdoCommand::UNBIND_RSP:
      return stream << "UNBIND_RSP";
    case ZdoCommand::MGMT_NWK_DISC_RSP:
      return stream << "MGMT_NWK_DISC_RSP";
    case ZdoCommand::MGMT_LQI_RSP:
      return stream << "MGMT_LQI_RSP";
    case ZdoCommand::MGMT_RTG_RSP:
      return stream << "MGMT_RTG_RSP";
    case ZdoCommand::MGMT_BIND_RSP:
      return stream << "MGMT_BIND_RSP";
    case ZdoCommand::MGMT_LEAVE_RSP:
      return stream << "MGMT_LEAVE_RSP";
    case ZdoCommand::MGMT_DIRECT_JOIN_RSP:
      return stream << "MGMT_DIRECT_JOIN_RSP";
    case ZdoCommand::MGMT_PERMIT_JOIN_RSP:
      return stream << "MGMT_PERMIT_JOIN_RSP";
    case ZdoCommand::STATE_CHANGE_IND:
      return stream << "STATE_CHANGE_IND";
    case ZdoCommand::END_DEVICE_ANNCE_IND:
      return stream << "END_DEVICE_ANNCE_IND";
    case ZdoCommand::MATCH_DESC_RSP_SENT:
      return stream << "MATCH_DESC_RSP_SENT";
    case ZdoCommand::STATUS_ERROR_RSP:
      return stream << "STATUS_ERROR_RSP";
    case ZdoCommand::SRC_RTG_IND:
      return stream << "SRC_RTG_IND";
    case ZdoCommand::LEAVE_IND:
      return stream << "LEAVE_IND";
    case ZdoCommand::TC_DEV_IND:
      return stream << "TC_DEV_IND";
    case ZdoCommand::PERMIT_JOIN_IND:
      return stream << "PERMIT_JOIN_IND";
    case ZdoCommand::MSG_CB_INCOMING:
      return stream << "MSG_CB_INCOMING";
    default:
      return stream << "ZdoCommand(" << (unsigned int)command << ")";
  }
}

std::ostream& operator<<(std::ostream& stream, SapiCommand command) {
  switch (command) {
    case SapiCommand::START_REQUEST:
      return stream << "START_REQUEST";
    case SapiCommand::BIND_DEVICE:
      return stream << "BIND_DEVICE";
    case SapiCommand::ALLOW_BIND:
      return stream << "ALLOW_BIND";
    case SapiCommand::SEND_DATA_REQUEST:
      return stream << "SEND_DATA_REQUEST";
    case SapiCommand::READ_CONFIGURATION:
      return stream << "READ_CONFIGURATION";
    case SapiCommand::WRITE_CONFIGURATION:
      return stream << "WRITE_CONFIGURATION";
    case SapiCommand::GET_DEVICE_INFO:
      return stream << "GET_DEVICE_INFO";
    case SapiCommand::FIND_DEVICE_REQUEST:
      return stream << "FIND_DEVICE_REQUEST";
    case SapiCommand::PERMIT_JOINING_REQUEST:
      return stream << "PERMIT_JOINING_REQUEST";
    case SapiCommand::SYSTEM_RESET:
      return stream << "SYSTEM_RESET";
    case SapiCommand::START_CONFIRM:
      return stream << "START_CONFIRM";
    case SapiCommand::BIND_CONFIRM:
      return stream << "BIND_CONFIRM";
    case SapiCommand::ALLOW_BIND_CONFIRM:
      return stream << "ALLOW_BIND_CONFIRM";
    case SapiCommand::SEND_DATA_CONFIRM:
      return stream << "SEND_DATA_CONFIRM";
    case SapiCommand::FIND_DEVICE_CONFIRM:
      return stream << "FIND_DEVICE_CONFIRM";
    case SapiCommand::RECEIVE_DATA_INDICATION:
      return stream << "RECEIVE_DATA_INDICATION";
    default:
      return stream << "SapiCommand(" << (unsigned int)command << ")";
  }
}

std::ostream& operator<<(std::ostream& stream, UtilCommand command) {
  switch (command) {
    case UtilCommand::GET_DEVICE_INFO:
      return stream << "GET_DEVICE_INFO";
    case UtilCommand::GET_NV_INFO:
      return stream << "GET_NV_INFO";
    case UtilCommand::SET_PANID:
      return stream << "SET_PANID";
    case UtilCommand::SET_CHANNELS:
      return stream << "SET_CHANNELS";
    case UtilCommand::SET_SECLEVEL:
      return stream << "SET_SECLEVEL";
    case UtilCommand::SET_PRECFGKEY:
      return stream << "SET_PRECFGKEY";
    case UtilCommand::CALLBACK_SUB_CMD:
      return stream << "CALLBACK_SUB_CMD";
    case UtilCommand::KEY_EVENT:
      return stream << "KEY_EVENT";
    case UtilCommand::TIME_ALIVE:
      return stream << "TIME_ALIVE";
    case UtilCommand::LED_CONTROL:
      return stream << "LED_CONTROL";
    case UtilCommand::TEST_LOOPBACK:
      return stream << "TEST_LOOPBACK";
    case UtilCommand::DATA_REQ:
      return stream << "DATA_REQ";
    case UtilCommand::SRC_MATCH_ENABLE:
      return stream << "SRC_MATCH_ENABLE";
    case UtilCommand::SRC_MATCH_ADD_ENTRY:
      return stream << "SRC_MATCH_ADD_ENTRY";
    case UtilCommand::SRC_MATCH_DEL_ENTRY:
      return stream << "SRC_MATCH_DEL_ENTRY";
    case UtilCommand::SRC_MATCH_CHECK_SRC_ADDR:
      return stream << "SRC_MATCH_CHECK_SRC_ADDR";
    case UtilCommand::SRC_MATCH_ACK_ALL_PENDING:
      return stream << "SRC_MATCH_ACK_ALL_PENDING";
    case UtilCommand::SRC_MATCH_CHECK_ALL_PENDING:
      return stream << "SRC_MATCH_CHECK_ALL_PENDING";
    case UtilCommand::ADDRMGR_EXT_ADDR_LOOKUP:
      return stream << "ADDRMGR_EXT_ADDR_LOOKUP";
    case UtilCommand::ADDRMGR_NWK_ADDR_LOOKUP:
      return stream << "ADDRMGR_NWK_ADDR_LOOKUP";
    case UtilCommand::APSME_LINK_KEY_DATA_GET:
      return stream << "APSME_LINK_KEY_DATA_GET";
    case UtilCommand::APSME_LINK_KEY_NV_ID_GET:
      return stream << "APSME_LINK_KEY_NV_ID_GET";
    case UtilCommand::ASSOC_COUNT:
      return stream << "ASSOC_COUNT";
    case UtilCommand::ASSOC_FIND_DEVICE:
      return stream << "ASSOC_FIND_DEVICE";
    case UtilCommand::ASSOC_GET_WITH_ADDRESS:
      return stream << "ASSOC_GET_WITH_ADDRESS";
    case UtilCommand::APSME_REQUEST_KEY_CMD:
      return stream << "APSME_REQUEST_KEY_CMD";
    case UtilCommand::ZCL_KEY_EST_INIT_EST:
      return stream << "ZCL_KEY_EST_INIT_EST";
    case UtilCommand::ZCL_KEY_EST_SIGN:
      return stream << "ZCL_KEY_EST_SIGN";
    case UtilCommand::UTIL_SYNC_REQ:
      return stream << "UTIL_SYNC_REQ";
    case UtilCommand::ZCL_KEY_ESTABLISH_IND:
      return stream << "ZCL_KEY_ESTABLISH_IND";
    default:
      return stream << "UtilCommand(" << (unsigned int)command << ")";
  }
}

ZnpCommand::ZnpCommand(ZnpSubsystem subsystem, uint8_t command)
    : value_(subsystem, command) {}
ZnpCommand::ZnpCommand(SysCommand command)
    : value_(ZnpSubsystem::SYS, (uint8_t)command) {}
ZnpCommand::ZnpCommand(AfCommand command)
    : value_(ZnpSubsystem::AF, (uint8_t)command) {}
ZnpCommand::ZnpCommand(ZdoCommand command)
    : value_(ZnpSubsystem::ZDO, (uint8_t)command) {}
ZnpCommand::ZnpCommand(SapiCommand command)
    : value_(ZnpSubsystem::SAPI, (uint8_t)command) {}
ZnpCommand::ZnpCommand(UtilCommand command)
    : value_(ZnpSubsystem::UTIL, (uint8_t)command) {}

ZnpSubsystem ZnpCommand::Subsystem() { return value_.first; }
uint8_t ZnpCommand::RawCommand() { return value_.second; }
bool operator==(const ZnpCommand& a, const ZnpCommand& b) {
  return a.value_ == b.value_;
}
bool operator!=(const ZnpCommand& a, const ZnpCommand& b) {
  return a.value_ != b.value_;
}
bool operator<=(const ZnpCommand& a, const ZnpCommand& b) {
  return a.value_ <= b.value_;
}
bool operator>=(const ZnpCommand& a, const ZnpCommand& b) {
  return a.value_ >= b.value_;
}
bool operator<(const ZnpCommand& a, const ZnpCommand& b) {
  return a.value_ < b.value_;
}
bool operator>(const ZnpCommand& a, const ZnpCommand& b) {
  return a.value_ > b.value_;
}
std::ostream& operator<<(std::ostream& stream, ZnpCommand command) {
  stream << command.value_.first << "_";
  switch (command.value_.first) {
    case ZnpSubsystem::SYS:
      return stream << (SysCommand)command.value_.second;
    case ZnpSubsystem::AF:
      return stream << (AfCommand)command.value_.second;
    case ZnpSubsystem::ZDO:
      return stream << (ZdoCommand)command.value_.second;
    case ZnpSubsystem::SAPI:
      return stream << (SapiCommand)command.value_.second;
    case ZnpSubsystem::UTIL:
      return stream << (UtilCommand)command.value_.second;
    default:
      return stream << std::hex << (unsigned int)command.value_.second;
  }
}

std::ostream& operator<<(std::ostream& stream, const Capability& cap) {
  uint16_t unhandled = (uint16_t)cap;
  bool first = true;
  std::map<uint16_t, std::string> names{{(uint16_t)Capability::SYS, "SYS"},
                                        {(uint16_t)Capability::MAC, "MAC"},
                                        {(uint16_t)Capability::NWK, "NWK"},
                                        {(uint16_t)Capability::AF, "AF"},
                                        {(uint16_t)Capability::ZDO, "ZDO"},
                                        {(uint16_t)Capability::SAPI, "SAPI"},
                                        {(uint16_t)Capability::UTIL, "UTIL"},
                                        {(uint16_t)Capability::DEBUG, "DEBUG"},
                                        {(uint16_t)Capability::APP, "APP"},
                                        {(uint16_t)Capability::ZOAD, "ZOAD"}};
  for (auto name : names) {
    if (unhandled & name.first) {
      if (!first) stream << "|";
      first = false;
      stream << name.second;
      unhandled &= ~name.first;
    }
  }
  if (unhandled != 0) {
    if (!first) stream << "|";
    first = false;
    stream << std::hex << (unsigned int)unhandled;
  }
  return stream;
}

std::ostream& operator<<(std::ostream& stream, const ResetReason& reason) {
  switch (reason) {
    case ResetReason::PowerUp:
      return stream << "Power-up";
    case ResetReason::External:
      return stream << "External";
    case ResetReason::Watchdog:
      return stream << "Watch-dog";
    default:
      return stream << "UNK(" << (unsigned int)reason << ")";
  }
}

std::ostream& operator<<(std::ostream& stream, const ResetInfo& info) {
  return stream << "[" << info.reason << " " << (unsigned int)info.TransportRev
                << " " << (unsigned int)info.ProductId << " "
                << (unsigned int)info.MajorRel << "."
                << (unsigned int)info.MinorRel << "."
                << (unsigned int)info.HwRev << "]";
}

}  // namespace znp

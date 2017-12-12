#include "znp/simpleapi/simpleapi_handler.h"
#include "znp/encoding.h"

namespace znp {
namespace simpleapi {
SimpleAPIHandler::SimpleAPIHandler(std::shared_ptr<ZnpSreqHandler> sreq_handler)
    : sreq_handler_(sreq_handler) {}

stlab::future<std::vector<uint8_t>> SimpleAPIHandler::ReadRawConfiguration(
    ConfigurationOption option) {
  std::vector<uint8_t> payload;
  payload.push_back((uint8_t)option);
  return sreq_handler_
      ->SReqStatus(ZnpSubsystem::SAPI,
                   (uint8_t)SimpleAPICommand::READ_CONFIGURATION, payload)
      .then([option](const std::vector<uint8_t>& response) {
        if (response.size() < 2 ||
            response.size() != 2 + (std::size_t)response[1]) {
          throw std::runtime_error("READ_CONFIGURATION invalid response size");
        }
        if (response[0] != (uint8_t)option) {
          throw std::runtime_error(
              "READ_CONFIGURATION responded with wrong configuration option");
        }
        return std::vector<uint8_t>(response.begin() + 2, response.end());
      });
}

stlab::future<void> SimpleAPIHandler::WriteRawConfiguration(
    ConfigurationOption option, std::vector<uint8_t> data) {
  std::vector<uint8_t> payload;
  payload.push_back((uint8_t)option);
  if (data.size() > 255) {
    throw std::runtime_error("Data to write exceeded 255 bytes");
  }
  payload.push_back((uint8_t)data.size());
  payload.insert(payload.end(), data.begin(), data.end());
  return sreq_handler_
      ->SReqStatus(ZnpSubsystem::SAPI,
                   (uint8_t)SimpleAPICommand::WRITE_CONFIGURATION, payload)
      .then([](const std::vector<uint8_t>& data) {
        if (data.size() != 0) {
          throw std::runtime_error(
              "Response to WRITE_CONFIGURATION was not zero");
        }
        return;
      });
}

stlab::future<void> SimpleAPIHandler::WriteStartupOption(
    StartupOption startup_option) {
  return WriteConfiguration(ConfigurationOption::STARTUP_OPTION,
                            startup_option);
}

stlab::future<void> SimpleAPIHandler::WriteLogicalType(
    LogicalType logical_type) {
  return WriteConfiguration(ConfigurationOption::LOGICAL_TYPE, logical_type);
}

stlab::future<void> SimpleAPIHandler::PermitJoiningRequest(uint16_t destination,
                                                           uint8_t timeout) {
  return sreq_handler_
      ->SReqStatus(ZnpSubsystem::SAPI,
                   (uint8_t)SimpleAPICommand::PERMIT_JOINING_REQUEST,
                   znp::EncodeT(destination, timeout))
      .then(znp::Decode<void>);
}

stlab::future<std::vector<uint8_t>> SimpleAPIHandler::GetRawDeviceInfo(
    DeviceInfo info) {
  return sreq_handler_
      ->SReq(ZnpSubsystem::SAPI, (uint8_t)SimpleAPICommand::GET_DEVICE_INFO,
             znp::Encode((uint8_t)info))
      .then([info](const std::vector<uint8_t>& retdata) {
        if (retdata.size() < 1) {
          throw std::runtime_error(
              "SAPI::GET_DEVICE_INFO returned insufficient data.");
        }
        if (retdata[0] != (uint8_t)info) {
          throw std::runtime_error("SAPI::GET_DEVICE_INFO returned wrong info");
        }
        return std::vector<uint8_t>(retdata.begin() + 1, retdata.end());
      });
}

stlab::future<zdo::DeviceState> SimpleAPIHandler::GetDeviceState() {
  return GetDeviceInfo<zdo::DeviceState>(DeviceInfo::DeviceState);
}
}  // namespace simpleapi
}  // namespace znp

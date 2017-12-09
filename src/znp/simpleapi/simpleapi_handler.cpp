#include "znp/simpleapi/simpleapi_handler.h"

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
          throw std::runtime_error("READ_CONFIGURATION response size mismatch");
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

stlab::future<void> SimpleAPIHandler::WriteStartupOption(StartupOption option) {
  std::vector<uint8_t> data;
  data.push_back((uint8_t)option);
  return WriteRawConfiguration(ConfigurationOption::STARTUP_OPTION, data);
}

stlab::future<void> SimpleAPIHandler::PermitJoiningRequest(uint16_t destination,
                                                           uint8_t timeout) {
  std::vector<uint8_t> data(3);
  data[0] = (uint8_t)(destination >> 8);
  data[1] = (uint8_t)(destination & 0xFF);
  data[2] = timeout;
  return sreq_handler_
      ->SReqStatus(ZnpSubsystem::SAPI,
                  (uint8_t)SimpleAPICommand::PERMIT_JOINING_REQUEST, data)
      .then([](const std::vector<uint8_t>& data) {
        if (data.size() != 0) {
          throw std::runtime_error(
              "Response to PERMIT_JOINING_REQUEST was not empty");
        }
      });
}
}  // namespace simpleapi
}  // namespace znp

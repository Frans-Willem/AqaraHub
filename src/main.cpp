#include <boost/asio.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/manipulators/dump.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <iostream>
#include <sstream>
#include <stlab/concurrency/future.hpp>
#include <stlab/concurrency/immediate_executor.hpp>
#include <stlab/concurrency/utility.hpp>
#include "asio_executor.h"
#include "coroutines.h"
#include "logging.h"
#include "znp/af/af.h"
#include "znp/encoding.h"
#include "znp/simpleapi/simpleapi_handler.h"
#include "znp/system/system.h"
#include "znp/system/system_handler.h"
#include "znp/zdo/zdo_handler.h"
#include "znp/znp_port.h"
#include "znp/znp_sreq_handler.h"
#include "znp/af/af_handler.h"

stlab::future<void> Initialize(
    std::shared_ptr<znp::system::SystemHandler> system_handler,
    std::shared_ptr<znp::simpleapi::SimpleAPIHandler> simpleapi_handler,
    std::shared_ptr<znp::zdo::ZdoHandler> zdo_handler,
	std::shared_ptr<znp::af::AfHandler> af_handler) {
  LOG("Initialize", debug)
      << "Doing initial reset, and instructing to clear config on next reset";
  std::ignore = co_await system_handler->Reset(true);
  co_await simpleapi_handler->WriteStartupOption(
      znp::simpleapi::StartupOption::ClearConfig |
      znp::simpleapi::StartupOption::ClearState);
  LOG("Initialize", debug) << "Doing final reset";
  std::ignore = co_await system_handler->Reset(true);
  LOG("Initialize", debug) << "Writing all configuration";
  co_await simpleapi_handler->WriteConfiguration<uint16_t>(
      znp::simpleapi::ConfigurationOption::PANID, 0x1A62);
  co_await simpleapi_handler->WriteConfiguration<uint64_t>(
      znp::simpleapi::ConfigurationOption::EXTENDED_PAN_ID, 0xDDDDDDDDDDDDDDDD);
  co_await simpleapi_handler->WriteConfiguration<uint32_t>(
      znp::simpleapi::ConfigurationOption::CHANLIST, 0x00000800);
  co_await simpleapi_handler->WriteLogicalType(
      znp::simpleapi::LogicalType::Coordinator);
  std::array<uint8_t, 16> presharedkey = {
      {1, 3, 5, 7, 9, 11, 13, 15, 0, 2, 4, 6, 8, 10, 12, 13}};
  co_await simpleapi_handler->WriteConfiguration(
      znp::simpleapi::ConfigurationOption::PRECFGKEY, presharedkey);
  co_await simpleapi_handler->WriteConfiguration<uint8_t>(
      znp::simpleapi::ConfigurationOption::PRECFGKEYS_ENABLE, 0);
  co_await simpleapi_handler->WriteConfiguration<uint8_t>(
      znp::simpleapi::ConfigurationOption::ZDO_DIRECT_CB, 1);
  LOG("Initialize", debug) << "Starting ZDO";
  auto future_state = zdo_handler->WaitForState(
      {znp::zdo::DeviceState::ZB_COORD},
      {znp::zdo::DeviceState::COORD_STARTING, znp::zdo::DeviceState::HOLD,
       znp::zdo::DeviceState::INIT});
  uint8_t ret = co_await zdo_handler->StartupFromApp(100);
  LOG("Initialize", debug) << "ZDO Start return value: " << (unsigned int)ret;
  uint8_t device_state = co_await future_state;
  LOG("Initialize", debug) << "Final device state "
                           << (unsigned int)device_state;
  auto first_join =
      co_await zdo_handler->PermitJoin(znp::AddrMode::ShortAddress, 0, 0, 0);
  LOG("Initialize", debug) << "First PermitJoin OK " << first_join;
  auto second_join =
      co_await zdo_handler->PermitJoin((znp::AddrMode)15, 0xFFFC, 60, 0);
  LOG("Initialize", debug) << "Second PermitJoin OK " << second_join;

  co_await af_handler->Register(1, 0x0104, 5, 0, znp::af::Latency::NoLatency, std::vector<uint16_t>(), std::vector<uint16_t>());
  co_return;
}

void OnFrameDebug(std::string prefix, znp::ZnpCommandType cmdtype,
                  znp::ZnpSubsystem subsys, uint8_t command,
                  boost::asio::const_buffer payload) {
  std::stringstream ss_command;
  switch (subsys) {
    case znp::ZnpSubsystem::SAPI:
      ss_command << (znp::simpleapi::SimpleAPICommand)command;
      break;
    case znp::ZnpSubsystem::ZDO:
      ss_command << (znp::zdo::ZdoCommand)command;
      break;
    case znp::ZnpSubsystem::AF:
      ss_command << (znp::af::AfCommand)command;
	  break;
    default:
      ss_command << std::hex << (unsigned int)command;
      break;
  }
  LOG("FRAME", debug) << prefix << " " << cmdtype << " " << subsys << " "
                      << ss_command.str() << " "
                      << boost::log::dump(
                             boost::asio::buffer_cast<const uint8_t*>(payload),
                             boost::asio::buffer_size(payload));
}

int main() {
  boost::asio::io_service io_service;
  boost::asio::io_service::work work(io_service);

  auto console_log = boost::log::add_console_log(std::cerr);
  boost::log::formatter formatter =
      boost::log::expressions::stream
      << "<" << boost::log::expressions::attr<severity_level>("Severity") << ">"
      << " "
      << "[" << boost::log::expressions::attr<std::string>("Channel") << "] "
      << boost::log::expressions::message;
  console_log->set_formatter(formatter);
  LOG("Main", info) << "Starting";
  auto port = std::make_shared<znp::ZnpPort>(io_service, "/dev/ttyACM0");
  port->on_frame_.connect(std::bind(
      OnFrameDebug, "<<", std::placeholders::_1, std::placeholders::_2,
      std::placeholders::_3, std::placeholders::_4));
  port->on_sent_.connect(std::bind(OnFrameDebug, ">>", std::placeholders::_1,
                                   std::placeholders::_2, std::placeholders::_3,
                                   std::placeholders::_4));
  auto sreq_handler = std::make_shared<znp::ZnpSreqHandler>(port);
  auto system_handler = std::make_shared<znp::system::SystemHandler>(
      io_service, port, sreq_handler);
  auto simpleapi_handler =
      std::make_shared<znp::simpleapi::SimpleAPIHandler>(sreq_handler);
  auto zdo_handler = std::make_shared<znp::zdo::ZdoHandler>(port, sreq_handler,
                                                            simpleapi_handler);
  auto af_handler = std::make_shared<znp::af::AfHandler>(port, sreq_handler);

  // Reset device
  LOG("Main", info) << "Initializing";

  Initialize(system_handler, simpleapi_handler, zdo_handler, af_handler)
      .then([]() { LOG("Main", info) << "Initialization complete?"; })
      .recover([](stlab::future<void> v) {
        LOG("Main", info) << "In final handler";
        try {
          v.get_try();
        } catch (const std::exception& exc) {
          LOG("Main", critical) << "Exception: " << exc.what();
        }
      })
      .detach();

  std::cout << "IO Service starting" << std::endl;
  io_service.run();
  std::cout << "IO Service done" << std::endl;
  return 0;
}

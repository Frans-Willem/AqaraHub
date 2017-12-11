#include <boost/asio.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/manipulators/dump.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <iostream>
#include <stlab/concurrency/future.hpp>
#include <stlab/concurrency/immediate_executor.hpp>
#include <stlab/concurrency/utility.hpp>
#include "asio_executor.h"
#include "logging.h"
#include "znp/encoding.h"
#include "znp/simpleapi/simpleapi_handler.h"
#include "znp/system/system.h"
#include "znp/system/system_handler.h"
#include "znp/znp_port.h"
#include "znp/znp_sreq_handler.h"

stlab::future<void> chain(
    boost::asio::io_service& io_service,
    std::vector<std::function<stlab::future<void>()>> functions) {
  auto current = stlab::make_ready_future(AsioExecutor(io_service));
  for (auto function : functions) {
    current = current.then(function);
  }
  return current;
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
  auto sreq_handler = std::make_shared<znp::ZnpSreqHandler>(io_service, port);
  auto system_handler = std::make_shared<znp::system::SystemHandler>(
      io_service, port, sreq_handler);
  auto simpleapi_handler =
      std::make_shared<znp::simpleapi::SimpleAPIHandler>(sreq_handler);

  // Reset device
  LOG("Main", info) << "Initializing";

  chain(io_service,
        {[system_handler]() {
           return system_handler->Reset(true).then([](auto f) {});
         },
         [simpleapi_handler]() {
           return simpleapi_handler->WriteStartupOption(
               znp::simpleapi::StartupOption::ClearConfig);
         },
         [system_handler]() {
           return system_handler->Reset(true).then([](auto f) {});
         },
         [simpleapi_handler]() {
           return simpleapi_handler->WriteConfiguration<uint16_t>(
               znp::simpleapi::ConfigurationOption::PANID, 0x1A62);
         },
         [simpleapi_handler]() {
           return simpleapi_handler->WriteConfiguration<uint64_t>(
               znp::simpleapi::ConfigurationOption::EXTENDED_PAN_ID,
               0xDDDDDDDDDDDDDDDD);
         },
         [simpleapi_handler]() {
           return simpleapi_handler->WriteConfiguration<uint32_t>(
               znp::simpleapi::ConfigurationOption::CHANLIST, 0x00000800);
         },
         [simpleapi_handler]() {
           return simpleapi_handler->WriteLogicalType(
               znp::simpleapi::LogicalType::Coordinator);
         },
         [simpleapi_handler]() {
           std::array<uint8_t, 16> presharedkey = {1, 3, 5, 7, 9, 11, 13, 15,
                                                  0, 2, 4, 6, 8, 10, 12, 13};
           return simpleapi_handler->WriteConfiguration(
               znp::simpleapi::ConfigurationOption::PRECFGKEY, presharedkey);
         },
         [simpleapi_handler]() {
           return simpleapi_handler->WriteConfiguration<uint8_t>(
               znp::simpleapi::ConfigurationOption::PRECFGKEYS_ENABLE, 0);
         },
         [simpleapi_handler]() {
           return simpleapi_handler->WriteConfiguration<uint8_t>(
               znp::simpleapi::ConfigurationOption::ZDO_DIRECT_CB, 1);
         },
         [simpleapi_handler]() {
           return simpleapi_handler
               ->GetDeviceInfo<uint8_t>(znp::simpleapi::DeviceInfo::DeviceState)
               .then([](const uint8_t data) {
                 LOG("Main", debug)
                     << "DeviceInfo(0): "
                     << (unsigned int)data;
               });
         }})
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

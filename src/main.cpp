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
  LOG("Main", info) << "Sending reset";
  std::vector<uint8_t> epan_id;
  for (uint8_t i = 0; i < 8; i++) {
    epan_id.push_back(i);
  }

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
         [simpleapi_handler, epan_id]() {
           return simpleapi_handler->WriteRawConfiguration(
               znp::simpleapi::ConfigurationOption::EXTENDED_PAN_ID, epan_id);
         },
         [simpleapi_handler]() {
           return simpleapi_handler->PermitJoiningRequest(0xFFFC, 60);
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

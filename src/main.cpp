#include <boost/asio.hpp>
#include "logging.h"
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/expressions.hpp>
#include <iostream>
#include <stlab/concurrency/future.hpp>
#include "znp/system/system.h"
#include "znp/system/system_handler.h"
#include "znp/znp_port.h"
#include "znp/znp_sreq_handler.h"

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

  // Reset device
  LOG("Main", info) << "Sending reset";
  system_handler->Reset(true)
      .then([system_handler](znp::system::ResetInfo info) {
        std::cout << "Reset done: " << info << std::endl;
        return system_handler->Ping();
      })
      .then([](znp::system::Capability cap) {
        std::cout << "Ping response: " << cap << std::endl;
      })
      .detach();

  std::cout << "IO Service starting" << std::endl;
  io_service.run();
  std::cout << "IO Service done" << std::endl;
  return 0;
}

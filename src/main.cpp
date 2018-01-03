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
#include "zcl/encoding.h"
#include "zcl/zcl.h"
#include "znp/encoding.h"
#include "znp/znp_api.h"
#include "znp/znp_port.h"

struct ZnpConfiguration {
	uint16_t pan_id;
	uint64_t extended_pan_id;
	uint32_t chan_list;
	znp::LogicalType logical_type;
	std::array<uint8_t, 16> presharedkey;
	bool precfgkeys_enable;
	bool zdo_direct_cb;
};

stlab::future<ZnpConfiguration> ReadZnpConfiguration(
    std::shared_ptr<znp::ZnpApi> api) {
  return stlab::when_all(
      stlab::immediate_executor,
      [](uint16_t pan_id, uint64_t extended_pan_id, uint32_t chan_list,
         znp::LogicalType logical_type, std::array<uint8_t, 16> presharedkey,
         bool precfgkeys_enable, bool zdo_direct_cb) {
        ZnpConfiguration retval;
        retval.pan_id = pan_id;
        retval.extended_pan_id = extended_pan_id;
        retval.chan_list = chan_list;
        retval.logical_type = logical_type;
        retval.presharedkey = presharedkey;
        retval.precfgkeys_enable = precfgkeys_enable;
        retval.zdo_direct_cb = zdo_direct_cb;
        return retval;
      },
      api->SapiReadConfiguration<znp::ConfigurationOption::PANID>(),
      api->SapiReadConfiguration<znp::ConfigurationOption::EXTENDED_PAN_ID>(),
      api->SapiReadConfiguration<znp::ConfigurationOption::CHANLIST>(),
      api->SapiReadConfiguration<znp::ConfigurationOption::LOGICAL_TYPE>(),
      api->SapiReadConfiguration<znp::ConfigurationOption::PRECFGKEY>(),
      api->SapiReadConfiguration<znp::ConfigurationOption::PRECFGKEYS_ENABLE>(),
      api->SapiReadConfiguration<znp::ConfigurationOption::ZDO_DIRECT_CB>());
}

stlab::future<void> Initialize(std::shared_ptr<znp::ZnpApi> api) {
  LOG("Initialize", debug)
      << "Doing initial reset, and instructing to clear config on next reset";
  co_await api->SapiWriteConfiguration<
      znp::ConfigurationOption::STARTUP_OPTION>(
      znp::StartupOption::ClearConfig);  // | znp::StartupOption::ClearState);
  std::ignore = co_await api->SysReset(true);
  co_await api->SapiWriteConfiguration<
      znp::ConfigurationOption::STARTUP_OPTION>(
      znp::StartupOption::ClearConfig | znp::StartupOption::ClearState);
  LOG("Initialize", debug) << "Doing final reset";
  std::ignore = co_await api->SysReset(true);
  auto caps = co_await api->SysPing();
  LOG("Initialize", debug) << "Capabilities: " << caps;
  LOG("Initialize", debug) << "Writing all configuration";
  co_await api->SapiWriteConfiguration<znp::ConfigurationOption::PANID>(0x1A62);
  co_await api
      ->SapiWriteConfiguration<znp::ConfigurationOption::EXTENDED_PAN_ID>(
          0xDDDDDDDDDDDDDDDD);
  co_await api->SapiWriteConfiguration<znp::ConfigurationOption::CHANLIST>(
      0x00000800);
  co_await api->SapiWriteConfiguration<znp::ConfigurationOption::LOGICAL_TYPE>(
      znp::LogicalType::Coordinator);
  std::array<uint8_t, 16> presharedkey = {
      {1, 3, 5, 7, 9, 11, 13, 15, 0, 2, 4, 6, 8, 10, 12, 13}};
  co_await api->SapiWriteConfiguration<znp::ConfigurationOption::PRECFGKEY>(
      presharedkey);
  co_await api
      ->SapiWriteConfiguration<znp::ConfigurationOption::PRECFGKEYS_ENABLE>(
          false);
  co_await api->SapiWriteConfiguration<znp::ConfigurationOption::ZDO_DIRECT_CB>(
      true);

  LOG("Initialize", debug) << "Starting ZDO";
  auto future_state =
      api->WaitForState({znp::DeviceState::ZB_COORD},
                        {znp::DeviceState::COORD_STARTING,
                         znp::DeviceState::HOLD, znp::DeviceState::INIT});
  uint8_t ret = co_await api->ZdoStartupFromApp(100);
  LOG("Initialize", debug) << "ZDO Start return value: " << (unsigned int)ret;
  uint8_t device_state = co_await future_state;
  LOG("Initialize", debug) << "Final device state "
                           << (unsigned int)device_state;

  auto first_join =
      co_await api->ZdoMgmtPermitJoin(znp::AddrMode::ShortAddress, 0, 0, 0);
  LOG("Initialize", debug) << "First PermitJoin OK " << first_join;
  auto second_join =
      co_await api->ZdoMgmtPermitJoin((znp::AddrMode)15, 0xFFFC, 60, 0);
  LOG("Initialize", debug) << "Second PermitJoin OK " << second_join;

  co_await api->AfRegister(1, 0x0104, 5, 0, znp::Latency::NoLatency,
                           std::vector<uint16_t>(), std::vector<uint16_t>());

  /*
      for (unsigned int id = 0; id < 0x1000; id++) {
        try {
          auto result = co_await api->SysOsalNvReadRaw((znp::NvItemId)id, 0);
          LOG("Initialize", debug)
              << std::hex << id << ": "
              << boost::log::dump(result.data(), result.size());
        } catch (const std::exception& exc) {
        }
      }
      LOG("Initialize", debug) << "--- Done ---";
  */
  co_return;
}

void OnFrameDebug(std::string prefix, znp::ZnpCommandType cmdtype,
                  znp::ZnpCommand command,
                  const std::vector<uint8_t>& payload) {
  LOG("FRAME", debug) << prefix << " " << cmdtype << " " << command << " "
                      << boost::log::dump(payload.data(), payload.size());
}

void AfIncomingMsg(std::shared_ptr<znp::ZnpApi> api,
                   const znp::IncomingMsg& message) {
  LOG("MSG", debug) << "GroupId: " << message.GroupId
                    << ", ClusterId: " << message.ClusterId
                    << ", SrcAddr: " << message.SrcAddr
                    << ", SrcEndpoint: " << (unsigned int)message.SrcEndpoint
                    << ", DstEndpoint: " << (unsigned int)message.DstEndpoint
                    << ", WasBroadcast: " << (unsigned int)message.WasBroadcast
                    << ", LinkQuality: " << (unsigned int)message.LinkQuality
                    << ", SecurityUse: " << (unsigned int)message.SecurityUse
                    << ", TimeStamp: " << message.TimeStamp
                    << ", TransSeqNumber: "
                    << (unsigned int)message.TransSeqNumber;
  try {
    auto frame = znp::Decode<zcl::ZclFrame>(message.Data);
    LOG("MSG", debug) << frame;
  } catch (const std::exception& exc) {
    LOG("MSG", debug) << "Exception: " << exc.what();
  }

    api->UtilAddrmgrNwkAddrLookup(message.SrcAddr)
        .recover([](auto f) {
          try {
            auto response = *f.get_try();
            LOG("MSG", debug) << "IEEE Address: " << std::hex << response;

          } catch (const std::exception& exc) {
            LOG("MSG", debug)
                << "Exception while getting IEEE Address: " << exc.what();
          }
        })
        .detach();
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
  port->on_frame_.connect(std::bind(OnFrameDebug, "<<", std::placeholders::_1,
                                    std::placeholders::_2,
                                    std::placeholders::_3));
  port->on_sent_.connect(std::bind(OnFrameDebug, ">>", std::placeholders::_1,
                                   std::placeholders::_2,
                                   std::placeholders::_3));
  auto api = std::make_shared<znp::ZnpApi>(port);

  api->af_on_incoming_msg_.connect(
      std::bind(&AfIncomingMsg, api, std::placeholders::_1));

  // Reset device
  LOG("Main", info) << "Initializing";

  Initialize(api)
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

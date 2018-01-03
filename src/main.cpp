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

struct FullConfiguration {
  znp::StartupOption startup_option;
  uint16_t pan_id;
  uint64_t extended_pan_id;
  uint32_t chan_list;
  znp::LogicalType logical_type;
  std::array<uint8_t, 16> presharedkey;
  bool precfgkeys_enable;
  bool zdo_direct_cb;

  bool operator==(const FullConfiguration& other) const {
    return this->startup_option == other.startup_option &&
           this->pan_id == other.pan_id &&
           this->extended_pan_id == other.extended_pan_id &&
           this->chan_list == other.chan_list &&
           this->logical_type == other.logical_type &&
           this->presharedkey == other.presharedkey &&
           this->precfgkeys_enable == other.precfgkeys_enable &&
           this->zdo_direct_cb == other.zdo_direct_cb;
  }
  bool operator!=(const FullConfiguration& other) const {
    return !(*this == other);
  }
};

stlab::future<FullConfiguration> ReadFullConfiguration(
    std::shared_ptr<znp::ZnpApi> api) {
  return stlab::when_all(
      stlab::immediate_executor,
      [](znp::StartupOption startup_option, uint16_t pan_id,
         uint64_t extended_pan_id, uint32_t chan_list,
         znp::LogicalType logical_type, std::array<uint8_t, 16> presharedkey,
         bool precfgkeys_enable, bool zdo_direct_cb) {
        FullConfiguration retval;
        retval.startup_option = startup_option;
        retval.pan_id = pan_id;
        retval.extended_pan_id = extended_pan_id;
        retval.chan_list = chan_list;
        retval.logical_type = logical_type;
        retval.presharedkey = presharedkey;
        retval.precfgkeys_enable = precfgkeys_enable;
        retval.zdo_direct_cb = zdo_direct_cb;
        return retval;
      },
      api->SapiReadConfiguration<znp::ConfigurationOption::STARTUP_OPTION>(),
      api->SapiReadConfiguration<znp::ConfigurationOption::PANID>(),
      api->SapiReadConfiguration<znp::ConfigurationOption::EXTENDED_PAN_ID>(),
      api->SapiReadConfiguration<znp::ConfigurationOption::CHANLIST>(),
      api->SapiReadConfiguration<znp::ConfigurationOption::LOGICAL_TYPE>(),
      api->SapiReadConfiguration<znp::ConfigurationOption::PRECFGKEY>(),
      api->SapiReadConfiguration<znp::ConfigurationOption::PRECFGKEYS_ENABLE>(),
      api->SapiReadConfiguration<znp::ConfigurationOption::ZDO_DIRECT_CB>());
}

stlab::future<void> WriteFullConfiguration(std::shared_ptr<znp::ZnpApi> api,
                                           const FullConfiguration& config) {
  return stlab::when_all(
      stlab::immediate_executor, []() { return; },
      api->SapiWriteConfiguration<znp::ConfigurationOption::STARTUP_OPTION>(
          config.startup_option),
      api->SapiWriteConfiguration<znp::ConfigurationOption::PANID>(
          config.pan_id),
      api->SapiWriteConfiguration<znp::ConfigurationOption::EXTENDED_PAN_ID>(
          config.extended_pan_id),
      api->SapiWriteConfiguration<znp::ConfigurationOption::CHANLIST>(
          config.chan_list),
      api->SapiWriteConfiguration<znp::ConfigurationOption::LOGICAL_TYPE>(
          config.logical_type),
      api->SapiWriteConfiguration<znp::ConfigurationOption::PRECFGKEY>(
          config.presharedkey),
      api->SapiWriteConfiguration<znp::ConfigurationOption::PRECFGKEYS_ENABLE>(
          config.precfgkeys_enable),
      api->SapiWriteConfiguration<znp::ConfigurationOption::ZDO_DIRECT_CB>(
          config.zdo_direct_cb));
}

stlab::future<void> Initialize(std::shared_ptr<znp::ZnpApi> api) {
  LOG("Initialize", debug)
      << "Doing initial reset, without clearing config or state";
  co_await api
      ->SapiWriteConfiguration<znp::ConfigurationOption::STARTUP_OPTION>(
          znp::StartupOption::None);
  std::ignore = co_await api->SysReset(true);
  LOG("Initialize", debug) << "Building desired configuration";
  FullConfiguration desired_config;
  desired_config.startup_option = znp::StartupOption::None;
  desired_config.pan_id = 0x1A62;
  desired_config.extended_pan_id = 0xDDDDDDDDDDDDDDDD;
  desired_config.chan_list = 0x800;
  desired_config.logical_type = znp::LogicalType::Coordinator;
  desired_config.presharedkey = std::array<uint8_t, 16>(
      {{1, 3, 5, 7, 9, 11, 13, 15, 0, 2, 4, 6, 8, 10, 12, 13}});
  desired_config.precfgkeys_enable = false;
  desired_config.zdo_direct_cb = true;
  LOG("Initialize", debug) << "Verifying full configuration";
  auto current_config = co_await ReadFullConfiguration(api);
  if (current_config != desired_config) {
    LOG("Initialize", debug) << "Desired configuration does not match current "
                                "configuration. Full reset is needed...";
    co_await api
        ->SapiWriteConfiguration<znp::ConfigurationOption::STARTUP_OPTION>(
            znp::StartupOption::ClearConfig | znp::StartupOption::ClearState);
    std::ignore = co_await api->SysReset(true);
  } else {
    LOG("Initialize", debug) << "Desired configuration matches current "
                                "configuration, ready to start!";
  }
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

  /*
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
          */
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

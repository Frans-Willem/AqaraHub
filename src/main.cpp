// vim: set shiftwidth=2 tabstop=2 expandtab:
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/manipulators/dump.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <iostream>
#include <sstream>
#include <stlab/concurrency/future.hpp>
#include <stlab/concurrency/immediate_executor.hpp>
#include <stlab/concurrency/utility.hpp>
#include "asio_executor.h"
#include "coro.h"
#include "logging.h"
#include "mqtt_wrapper.h"
#include "string_enum.h"
#include "zcl/encoding.h"
#include "zcl/name_registry.h"
#include "zcl/to_json.h"
#include "zcl/zcl.h"
#include "zcl/zcl_string_enum.h"
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

void Initialize(coro::Await await, std::shared_ptr<znp::ZnpApi> api,
                std::array<uint8_t, 16> presharedkey) {
  LOG("Initialize", debug)
      << "Doing initial reset, without clearing config or state";
  await(api->SapiWriteConfiguration<znp::ConfigurationOption::STARTUP_OPTION>(
      znp::StartupOption::None));
  std::ignore = await(api->SysReset(true));
  LOG("Initialize", debug) << "Building desired configuration";
  auto coord_ieee_addr =
      await(api->SapiGetDeviceInfo<znp::DeviceInfo::DeviceIEEEAddress>());
  LOG("Initialize", debug) << "Device IEEE Address: " << std::hex
                           << coord_ieee_addr;
  FullConfiguration desired_config;
  desired_config.startup_option = znp::StartupOption::None;
  desired_config.pan_id = coord_ieee_addr & 0xFFFF;
  desired_config.extended_pan_id = coord_ieee_addr;
  // TODO: Not entirely sure how to pick a right value for this.
  desired_config.chan_list = 0x0800;
  desired_config.logical_type = znp::LogicalType::Coordinator;
  desired_config.presharedkey = presharedkey;
  desired_config.precfgkeys_enable = false;
  desired_config.zdo_direct_cb = true;
  LOG("Initialize", debug) << "Verifying full configuration";
  auto current_config = await(ReadFullConfiguration(api));
  if (current_config != desired_config) {
    LOG("Initialize", debug) << "Desired configuration does not match current "
                                "configuration. Full reset is needed...";
    await(api->SapiWriteConfiguration<znp::ConfigurationOption::STARTUP_OPTION>(
        znp::StartupOption::ClearConfig | znp::StartupOption::ClearState));
    std::ignore = await(api->SysReset(true));
    await(WriteFullConfiguration(api, desired_config));
  } else {
    LOG("Initialize", debug) << "Desired configuration matches current "
                                "configuration, ready to start!";
  }
  LOG("Initialize", debug) << "Starting ZDO";
  auto future_state =
      api->WaitForState({znp::DeviceState::ZB_COORD},
                        {znp::DeviceState::COORD_STARTING,
                         znp::DeviceState::HOLD, znp::DeviceState::INIT});
  uint8_t ret = await(api->ZdoStartupFromApp(100));
  LOG("Initialize", debug) << "ZDO Start return value: " << (unsigned int)ret;
  uint8_t device_state = await(future_state);
  LOG("Initialize", debug) << "Final device state "
                           << (unsigned int)device_state;

  auto first_join =
      await(api->ZdoMgmtPermitJoin(znp::AddrMode::ShortAddress, 0, 0, 0));
  LOG("Initialize", debug) << "First PermitJoin OK " << first_join;
  auto second_join =
      await(api->ZdoMgmtPermitJoin((znp::AddrMode)15, 0xFFFC, 60, 0));
  LOG("Initialize", debug) << "Second PermitJoin OK " << second_join;

  await(api->AfRegister(1, 0x0104, 5, 0, znp::Latency::NoLatency,
                        std::vector<uint16_t>(), std::vector<uint16_t>()));
  return;
}

void OnFrameDebug(std::string prefix, znp::ZnpCommandType cmdtype,
                  znp::ZnpCommand command,
                  const std::vector<uint8_t>& payload) {
  LOG("FRAME", debug) << prefix << " " << cmdtype << " " << command << " "
                      << boost::log::dump(payload.data(), payload.size());
}

void AfIncomingMsg(std::shared_ptr<znp::ZnpApi> api,
                   std::shared_ptr<MqttWrapper> mqtt_wrapper,
                   std::string mqtt_prefix,
                   std::shared_ptr<zcl::NameRegistry> name_registry,
                   const znp::IncomingMsg& message) {
  try {
    auto frame = znp::Decode<zcl::ZclFrame>(message.Data);
    LOG("MSG", debug) << "SrcAddr: " << message.SrcAddr
                      << ", ClusterId: " << message.ClusterId
                      << ", SrcEndpoint: " << (unsigned int)message.SrcEndpoint;
    LOG("MSG", debug) << "Payload: "
                      << boost::log::dump(frame.payload.data(),
                                          frame.payload.size());
    if (frame.frame_type == zcl::ZclFrameType::Global &&
        frame.command_identifier == 0x0a) {  // Report

      unsigned int SrcEndpoint = (unsigned int)message.SrcEndpoint;
      uint16_t ClusterId = message.ClusterId;
      api->UtilAddrmgrNwkAddrLookup(message.SrcAddr)
          .then([mqtt_wrapper, mqtt_prefix, name_registry, SrcEndpoint,
                 ClusterId, frame](auto ieee_addr) {
            std::vector<stlab::future<void>> publishes;
            std::vector<uint8_t>::const_iterator current =
                frame.payload.begin();
            while (current != frame.payload.end()) {
              std::tuple<uint16_t, zcl::ZclVariant> attribute;
              znp::EncodeHelper<std::tuple<uint16_t, zcl::ZclVariant>>::Decode(
                  attribute, current, frame.payload.end());
              LOG("MSG", debug)
                  << "Source: " << ieee_addr
                  << ", SrcEndpoint: " << (unsigned int)SrcEndpoint
                  << ", ClusterId 0x" << std::hex << ClusterId
                  << ", Attribute 0x" << std::get<0>(attribute) << ": "
                  << std::get<1>(attribute);

              std::string topic_name = boost::str(
                  boost::format("%s%08X/%d/%s/%04X") % mqtt_prefix % ieee_addr %
                  (unsigned int)SrcEndpoint %
                  name_registry->ClusterToString((zcl::ZclClusterId)ClusterId) %
                  name_registry->AttributeToString(
                      (zcl::ZclClusterId)ClusterId,
                      (zcl::ZclAttributeId)std::get<0>(attribute)));
              tao::json::value json_value(
                  zcl::to_json(std::get<1>(attribute), true));
              std::string message_content(tao::json::to_string(json_value));
              LOG("MSG", debug)
                  << "Publishing to " << topic_name << ": " << message_content;
              publishes.push_back(mqtt_wrapper->Publish(
                  topic_name, message_content, mqtt::qos::at_least_once, true));
            }
            return stlab::when_all(
                stlab::immediate_executor,
                []() {

                },
                std::make_pair(publishes.begin(), publishes.end()));
          })
          .recover([](auto f) {
            try {
              f.get_try();
            } catch (const std::exception& ex) {
              LOG("MSG", debug)
                  << "Exception while handling message:" << ex.what();
              return;
            }
            LOG("MSG", debug) << "Attributes reported to MQTT";
          })
          .detach();
    }
  } catch (const std::exception& exc) {
    LOG("MSG", debug) << "Exception: " << exc.what();
  }
}

std::shared_ptr<MqttWrapper> MqttWrapperFromUrl(
    boost::asio::io_service& io_service, std::string url) {
  // TODO: Find some external URL parsing library to handle this for us
  // properly, including username and password and such.
  boost::regex re("([^:]*)://(.*?)(:([^:/]+))?/?");
  boost::smatch match;
  if (!boost::regex_match(url, match, re)) {
    throw std::runtime_error("Malformed MQTT URL");
  }
  std::string protocol(match[1].first, match[1].second);
  std::string host(match[2].first, match[2].second);
  std::string port(match[4].first, match[4].second);
  if (protocol == "mqtt") {
    return MqttWrapper::Create(
        [](boost::asio::io_service& io_service, std::string host,
           std::string port) {
          LOG("MqttWrapper", debug)
              << "Creating connection to " << host << " : " << port;
          return mqtt::make_client(io_service, host, port);
        },
        io_service, host, (port == "" ? "1883" : port));
  }
  if (protocol == "mqtts") {
    return MqttWrapper::Create(
        [](boost::asio::io_service& io_service, std::string host,
           std::string port) {
          return mqtt::make_tls_client(io_service, host, port);
        },
        io_service, host, (port == "" ? "8883" : port));
  }
#if defined(MQTT_USE_WS)
  if (protocol == "ws") {
    return MqttWrapper::Create(
        [](boost::asio::io_service& io_service, std::string host,
           std::string port) {
          return mqtt::make_client_ws(io_service, host, port);
        },
        io_service, host, port);
  }
  if (protocol == "wss") {
    return MqttWrapper::Create(
        [](boost::asio::io_service& io_service, std::string host,
           std::string port) {
          return mqtt::make_tls_client_ws(io_service, host, port);
        },
        io_service, host, port);
  }
#endif
  throw std::runtime_error("Unsupported MQTT protocol");
  return std::shared_ptr<MqttWrapper>();
}

int main(int argc, const char** argv) {
  // Set up logging to console (stderr)
  auto console_log = boost::log::add_console_log(std::cerr);
  boost::log::formatter formatter =
      boost::log::expressions::stream
      << "<" << boost::log::expressions::attr<severity_level>("Severity") << ">"
      << " "
      << "[" << boost::log::expressions::attr<std::string>("Channel") << "] "
      << boost::log::expressions::message;
  console_log->set_formatter(formatter);

  // Parse command line
  boost::program_options::options_description description(
      "Open-source Xiaomi Aqara Zigbee Hub");

  // clang-format off
  description.add_options()
    ("help,h",
     "Produce this help message.")
    ("port,p",
     boost::program_options::value<std::string>(),
     "Serial port where the ZNP dongle is attached")
    ("mqtt,m",
     boost::program_options::value<std::string>()->default_value("mqtt://127.0.0.1:1883/"),
     "MQTT Server, e.g. mqtt://127.0.0.1:1883/")
    ("topic,t",
     boost::program_options::value<std::string>()->default_value("AqaraHub"),
     "MQTT Root topic, e.g. AqaraHub")
    ("psk",
     boost::program_options::value<std::string>()->default_value("AqaraHub"),
     "Zigbee Network pre-shared key. Maximum 16 characters, will be truncated when longer")
    ("name-registry",
     boost::program_options::value<std::string>()->default_value("../attributes.info"),
     "Boost property-tree info file containing cluster and attribute names")
    ;
  // clang-format on
  boost::program_options::variables_map variables;
  try {
    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, description),
        variables);
  } catch (const boost::program_options::error& ex) {
    std::cerr << ex.what() << std::endl;
    return EXIT_FAILURE;
  }
  boost::program_options::notify(variables);

  if (variables.count("help") || variables.count("port") == 0 ||
      variables.count("mqtt") == 0 || variables.count("topic") == 0) {
    std::cerr << description << std::endl;
    return EXIT_SUCCESS;
  }

  std::string serial_port = variables["port"].as<std::string>();
  LOG("Main", info) << "Serial port: " << serial_port;

  // Read cluster & attribute names
  auto name_registry = std::make_shared<zcl::NameRegistry>();
  if (!name_registry->ReadFromInfo(
          variables["name-registry"].as<std::string>())) {
    LOG("Main", warning) << "Unable to read '"
                         << variables["name-registry"].as<std::string>()
                         << "' name registry";
  }

  // Start working
  boost::asio::io_service io_service;
  boost::asio::io_service::work work(io_service);

  LOG("Main", info) << "Setting up ZNP connection";
  auto port = std::make_shared<znp::ZnpPort>(io_service, serial_port);
  port->on_frame_.connect(std::bind(OnFrameDebug, "<<", std::placeholders::_1,
                                    std::placeholders::_2,
                                    std::placeholders::_3));
  port->on_sent_.connect(std::bind(OnFrameDebug, ">>", std::placeholders::_1,
                                   std::placeholders::_2,
                                   std::placeholders::_3));
  auto api = std::make_shared<znp::ZnpApi>(port);

  LOG("Main", info) << "Setting up MQTT connection";

  std::shared_ptr<MqttWrapper> mqtt_wrapper;
  try {
    mqtt_wrapper =
        MqttWrapperFromUrl(io_service, variables["mqtt"].as<std::string>());
  } catch (const std::exception& ex) {
    std::cerr << ex.what() << std::endl;
    return EXIT_FAILURE;
  }

  std::string mqtt_prefix = variables["topic"].as<std::string>();
  if (mqtt_prefix.size() > 0 && mqtt_prefix[mqtt_prefix.size() - 1] != '/') {
    mqtt_prefix += "/";
  }
  LOG("Main", info) << "Using MQTT prefix '" << mqtt_prefix << "'";

  api->af_on_incoming_msg_.connect(std::bind(&AfIncomingMsg, api, mqtt_wrapper,
                                             mqtt_prefix, name_registry,
                                             std::placeholders::_1));

  // Reset device
  LOG("Main", info) << "Initializing ZNP device";
  std::string presharedkey_str(variables["psk"].as<std::string>());
  std::array<uint8_t, 16> presharedkey;
  presharedkey.fill(0);
  std::copy_n(presharedkey_str.begin(),
              std::min(presharedkey.size(), presharedkey_str.size()),
              presharedkey.begin());

  LOG("Main", info) << "Using PSK "
                    << boost::log::dump(presharedkey.data(),
                                        presharedkey.size());

  coro::Run(AsioExecutor(io_service), Initialize, api, presharedkey)
      .then([]() { LOG("Main", info) << "Initialization complete!"; })
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

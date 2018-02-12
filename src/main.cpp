// vim: set shiftwidth=2 tabstop=2 expandtab:
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/manipulators/dump.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/program_options.hpp>
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
#include "xiaomi/ff01_attribute.h"
#include "zcl/name_registry.h"
#include "zcl/to_json.h"
#include "zcl/zcl.h"
#include "zcl/zcl_endpoint.h"
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

void OnReportAttributes(std::shared_ptr<znp::ZnpApi> api,
                        std::shared_ptr<zcl::ZclEndpoint> endpoint,
                        std::shared_ptr<MqttWrapper> mqtt_wrapper,
                        std::string mqtt_prefix,
                        std::shared_ptr<zcl::NameRegistry> name_registry,
                        const zcl::ZclEndpoint::AttributeReport& report) {
  LOG("Report", debug) << "Attempting to look up full source address";
  auto source_endpoint = (unsigned int)report.source_endpoint;
  auto cluster_id = report.cluster_id;
  auto attributes = report.attributes;
  api->UtilAddrmgrNwkAddrLookup(report.source_address)
      .then([mqtt_wrapper, mqtt_prefix, name_registry, source_endpoint,
             cluster_id, attributes](auto ieee_addr) {
        std::vector<stlab::future<void>> publishes;
        auto cluster_name = name_registry->ClusterToString(cluster_id);
        for (const auto& attribute : attributes) {
          auto attribute_name = name_registry->AttributeToString(
              cluster_id, std::get<0>(attribute));
          boost::optional<std::map<uint8_t, zcl::ZclVariant>> opt_xiaomi_ff01(
              xiaomi::DecodeFF01Attribute(cluster_id, std::get<0>(attribute),
                                          std::get<1>(attribute)));
          tao::json::value json_value(
              opt_xiaomi_ff01 ? xiaomi::FF01AttributeToJson(*opt_xiaomi_ff01)
                              : zcl::to_json(std::get<1>(attribute), true));
          std::string topic_name = boost::str(
              boost::format("%sreport/%08X/%d/%s/%04X") % mqtt_prefix %
              ieee_addr % (unsigned int)source_endpoint % cluster_name %
              attribute_name);
          std::string message_content(tao::json::to_string(json_value));
          LOG("Report", debug)
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
          LOG("Report", debug)
              << "Exception while handling report: " << ex.what();
          return;
        }
        LOG("Report", debug) << "All done";
      })
      .detach();
}

void OnPublishWrite(std::shared_ptr<znp::ZnpApi> api,
                    std::shared_ptr<zcl::ZclEndpoint> endpoint,
                    std::shared_ptr<zcl::NameRegistry> name_registry,
                    std::string topic, std::string message) {
  if (topic == "permitjoin") {
    std::size_t endpos;
    unsigned long seconds = stoul(message, &endpos, 0);
    if (endpos != message.size()) {
      LOG("OnPublishWrite", warning)
          << "Unable to parse permitjoin contents '" << message << "'";
      return;
    }
    if (seconds >= 0xFF) {
      seconds = 0xFE;
    }
    api->ZdoMgmtPermitJoin((znp::AddrMode)15, 0xFFFC, seconds, 0)
        .recover([](auto f) {
          try {
            f.get_try();
            LOG("PermitJoin", debug) << "Permit join OK";
          } catch (const std::exception& ex) {
            LOG("PermitJoin", debug) << "Permit join failed: " << ex.what();
          }

        })
        .detach();
    return;
  }
  LOG("OnPublishWrite", debug) << "Topic '" << topic << "' not handled";
}

void OnPublish(std::shared_ptr<znp::ZnpApi> api,
               std::shared_ptr<zcl::ZclEndpoint> endpoint,
               std::string mqtt_prefix,
               std::shared_ptr<zcl::NameRegistry> name_registry,
               std::string topic, std::string message, std::uint8_t qos,
               bool retain) {
  std::string write_prefix(mqtt_prefix + "write/");
  if (boost::starts_with(topic, write_prefix)) {
    OnPublishWrite(api, endpoint, name_registry,
                   topic.substr(write_prefix.size()), message);
    return;
  }

  LOG("OnPublish", debug) << "Unhandled MQTT publish to " << topic;
}

void OnPermitJoin(std::shared_ptr<MqttWrapper> mqtt_wrapper,
                  std::string mqtt_prefix, uint8_t duration) {
  mqtt_wrapper
      ->Publish(mqtt_prefix + "report/permitjoin",
                boost::str(boost::format("%d") % (unsigned int)duration),
                mqtt::qos::at_least_once, false)
      .recover([](auto f) {
        try {
          f.get_try();
          LOG("OnPermitJoin", debug) << "Published OK";
        } catch (const std::exception& ex) {
          LOG("OnPermitJoin", debug) << "Publish failure: " << ex.what();
        }
      })
      .detach();
}

std::shared_ptr<zcl::ZclEndpoint> Initialize(
    coro::Await await, std::shared_ptr<znp::ZnpApi> api,
    std::array<uint8_t, 16> presharedkey,
    std::shared_ptr<MqttWrapper> mqtt_wrapper, std::string mqtt_prefix,
    std::shared_ptr<zcl::NameRegistry> name_registry) {
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

  std::ignore =
      await(api->ZdoMgmtPermitJoin(znp::AddrMode::ShortAddress, 0, 0, 0));

  auto endpoint = await(zcl::ZclEndpoint::Create(
      api, 1, 0x0104, 5, 0, znp::Latency::NoLatency, {}, {}));
  std::weak_ptr<zcl::ZclEndpoint> weak_endpoint(endpoint);
  std::weak_ptr<znp::ZnpApi> weak_api(api);

  endpoint->on_report_attributes_.connect(
      [weak_api, weak_endpoint, mqtt_wrapper, mqtt_prefix,
       name_registry](const zcl::ZclEndpoint::AttributeReport& report) {
        if (auto api = weak_api.lock()) {
          if (auto endpoint = weak_endpoint.lock()) {
            OnReportAttributes(api, endpoint, mqtt_wrapper, mqtt_prefix,
                               name_registry, report);
          }
        }
      });

  api->zdo_on_permit_join_.connect(std::bind(
      &OnPermitJoin, mqtt_wrapper, mqtt_prefix, std::placeholders::_1));

  mqtt_wrapper->on_publish_.connect(
      std::bind(&OnPublish, api, endpoint, mqtt_prefix, name_registry,
                std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3, std::placeholders::_4));
  await(mqtt_wrapper->Subscribe(
      {{mqtt_prefix + "write/#", mqtt::qos::at_least_once}}));

  /*
  auto bool_true=zcl::ZclVariant::Create<zcl::DataType::_bool>(false);
  await(endpoint->WriteAttributes(0x1b0b, 1, (zcl::ZclClusterId)0x0006,
  {{(zcl::ZclAttributeId)0x0000, bool_true}}));
  */
  // await(endpoint->SendCommand(0x1b0b, 1, (zcl::ZclClusterId)0x0006, 0x02,
  // {}));
  return endpoint;
}

void OnFrameDebug(std::string prefix, znp::ZnpCommandType cmdtype,
                  znp::ZnpCommand command,
                  const std::vector<uint8_t>& payload) {
  LOG("FRAME", debug) << prefix << " " << cmdtype << " " << command << " "
                      << boost::log::dump(payload.data(), payload.size());
}

std::string MakeNameSafeForMqtt(std::string name) {
  auto new_end = std::remove(name.begin(), name.end(), '/');
  return std::string(name.begin(), new_end);
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
  if (!name_registry->ReadFromInfo(variables["name-registry"].as<std::string>(),
                                   MakeNameSafeForMqtt)) {
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
        MqttWrapper::FromUrl(io_service, variables["mqtt"].as<std::string>());
  } catch (const std::exception& ex) {
    std::cerr << ex.what() << std::endl;
    return EXIT_FAILURE;
  }

  std::string mqtt_prefix = variables["topic"].as<std::string>();
  if (mqtt_prefix.size() > 0 && mqtt_prefix[mqtt_prefix.size() - 1] != '/') {
    mqtt_prefix += "/";
  }
  LOG("Main", info) << "Using MQTT prefix '" << mqtt_prefix << "'";

  // Creating pre-shared-key
  std::string presharedkey_str(variables["psk"].as<std::string>());
  std::array<uint8_t, 16> presharedkey;
  presharedkey.fill(0);
  std::copy_n(presharedkey_str.begin(),
              std::min(presharedkey.size(), presharedkey_str.size()),
              presharedkey.begin());

  LOG("Main", info) << "Using PSK "
                    << boost::log::dump(presharedkey.data(),
                                        presharedkey.size());

  // Initializing
  auto endpoint =
      coro::Run(AsioExecutor(io_service), Initialize, api, presharedkey,
                mqtt_wrapper, mqtt_prefix, name_registry)
          .then([](auto r) {
            LOG("Main", info) << "Initialization complete!";
            return r;
          })
          .recover([](auto f) {
            LOG("Main", info) << "In final handler";
            try {
              return f.get_try();
            } catch (const std::exception& exc) {
              LOG("Main", critical) << "Exception: " << exc.what();
              return (boost::optional<std::shared_ptr<zcl::ZclEndpoint>>)
                  boost::none;
            }
          });

  std::cout << "IO Service starting" << std::endl;
  io_service.run();
  std::cout << "IO Service done" << std::endl;
  return 0;
}

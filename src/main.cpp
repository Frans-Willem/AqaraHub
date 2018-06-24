// vim: set shiftwidth=2 tabstop=2 expandtab:
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/manipulators/dump.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <regex>
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
#include "zcl/encoding.h"
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
  LOG("Report", debug) << "Attempting to look up full source address for "
                       << (unsigned int)report.source_address;
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
                              : zcl::to_json(std::get<1>(attribute)));
          std::string topic_name = boost::str(
              boost::format("%sreport/%016X/%d/%s/%04X") % mqtt_prefix %
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

void OnPublishPermitJoin(std::shared_ptr<znp::ZnpApi> api,
                         std::string message) {
  std::size_t endpos;
  unsigned long seconds = stoul(message, &endpos, 0);
  if (endpos != message.size()) {
    LOG("OnPublishPermitJoin", warning)
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

/** Sends a Zigbee cluster library command. Expects cluster_id & command already
 * resolved, and the arguments already turned to a JSON array. */
void SendCommand(std::shared_ptr<znp::ZnpApi> api,
                 std::shared_ptr<zcl::ZclEndpoint> endpoint,
                 znp::IEEEAddress destination_address,
                 std::uint8_t destination_endpoint,
                 zcl::ZclClusterId cluster_id, zcl::ZclCommandId command,
                 const std::vector<tao::json::value>& arguments) {
  std::vector<uint8_t> payload;
  try {
    for (const auto& json_argument : arguments) {
      zcl::ZclVariant variant_argument = zcl::from_json(json_argument);
      std::vector<uint8_t> encoded_argument = znp::Encode(variant_argument);
      payload.insert(payload.end(),
                     encoded_argument.begin() +
                         znp::EncodedSize(variant_argument.GetType()),
                     encoded_argument.end());
    }
  } catch (const std::exception& ex) {
    LOG("SendCommand", error)
        << "Unable to convert JSON to Zigbee Cluster Library datatype: "
        << ex.what();
    return;
  }

  LOG("SendCommand", info) << "Encoded payload: "
                           << boost::log::dump(payload.data(), payload.size());

  LOG("SendCommand", info) << "Looking up Short Address from IEEE address";
  api->UtilAddrmgrExtAddrLookup(destination_address)
      .then([endpoint, destination_endpoint, cluster_id, command,
             payload](znp::ShortAddress short_address) {
        LOG("SendCommand", info) << "Response received, short address: "
                                 << (unsigned int)short_address;
        return endpoint->SendCommand(short_address, destination_endpoint,
                                     cluster_id, command, payload);
      })
      .recover([](auto f) {
        try {
          f.get_try();
          LOG("SendCommand", info) << "Command sent";
        } catch (const std::exception& ex) {
          LOG("SendCommand", warning)
              << "Exception while sending command: " << ex.what();
        }
      })
      .detach();
}

/** Called on MQTT publish of a long-form command, e.g. the command name is part
 * of the MQTT topic. */
void OnPublishCommandLong(std::shared_ptr<znp::ZnpApi> api,
                          std::shared_ptr<zcl::ZclEndpoint> endpoint,
                          std::shared_ptr<zcl::NameRegistry> name_registry,
                          znp::IEEEAddress destination_address,
                          std::uint8_t destination_endpoint,
                          std::string cluster_name, std::string command_name,
                          std::string message) {
  LOG("OnPublishCommandLong", debug)
      << "Destination " << destination_address << ", endpoint "
      << (unsigned int)destination_endpoint << ", cluster name '"
      << cluster_name << "', command name '" << command_name << "'";
  boost::optional<zcl::ZclClusterId> cluster_id =
      name_registry->ClusterFromString(cluster_name);
  if (!cluster_id) {
    LOG("OnPublishCommandLong", warning)
        << "Unable to look up cluster id '" << cluster_name << "'";
    return;
  }
  boost::optional<zcl::ZclCommandId> command =
      name_registry->CommandFromString(*cluster_id, command_name);
  if (!command) {
    LOG("OnPublishCommandLong", warning)
        << "Unable to look up command '" << command_name << "' in cluster '"
        << cluster_name << "'";
    return;
  }

  tao::json::value json_arguments = tao::json::value::array({});
  if (message.size() > 0) {
    try {
      json_arguments = tao::json::from_string(message);
    } catch (const std::exception& ex) {
      LOG("OnPublishCommandLong", error)
          << "Unable to decode message payload: " << ex.what();
      return;
    }
  }
  if (!json_arguments.is_array()) {
    json_arguments = tao::json::value::array({json_arguments});
  }
  SendCommand(api, endpoint, destination_address, destination_endpoint,
              *cluster_id, *command, json_arguments.get_array());
}

/** Called on MQTT publish of a short-form command, e.g. command name part of
 * the JSON payload. */
void OnPublishCommandShort(std::shared_ptr<znp::ZnpApi> api,
                           std::shared_ptr<zcl::ZclEndpoint> endpoint,
                           std::shared_ptr<zcl::NameRegistry> name_registry,
                           znp::IEEEAddress destination_address,
                           std::uint8_t destination_endpoint,
                           std::string cluster_name, std::string message) {
  LOG("OnPublishCommandShort", debug)
      << "Destination " << destination_address << ", endpoint "
      << (unsigned int)destination_endpoint << ", cluster name '"
      << cluster_name << "'";
  boost::optional<zcl::ZclClusterId> cluster_id =
      name_registry->ClusterFromString(cluster_name);
  if (!cluster_id) {
    LOG("OnPublishCommandShort", warning)
        << "Unable to look up cluster id '" << cluster_name << "'";
    return;
  }
  std::map<std::string, tao::json::value> obj_message;
  try {
    obj_message = tao::json::from_string(message).get_object();
  } catch (const std::exception& ex) {
    LOG("OnPublishCommandShort", error) << "Unable to decode message payload, "
                                           "or message was not a JSON object. "
                                        << ex.what();
    return;
  }
  auto found_command = obj_message.find("command");
  if (found_command == obj_message.end()) {
    LOG("OnPublishCommandShort", error)
        << "JSON object did not contain a 'command' property";
    return;
  }
  zcl::ZclCommandId command;
  if (found_command->second.is_string()) {
    auto opt_command = name_registry->CommandFromString(
        *cluster_id, found_command->second.get_string());
    if (!opt_command) {
      LOG("OnPublishCommandShort", error)
          << "Command with name '" << found_command->second.get_string()
          << "' could not be decoded";
      return;
    }
    command = *opt_command;
  } else if (found_command->second.is_number()) {
    if (!found_command->second.is_unsigned() ||
        found_command->second.get_unsigned() >= 256) {
      LOG("OnPublishCommandShort", error)
          << "Expected 'command' property to be an unsigned integer, lower "
             "than 256";
      return;
    }
    command =
        (zcl::ZclCommandId)(std::uint8_t)found_command->second.get_unsigned();
  } else {
    LOG("OnPublishCommandShort", error)
        << "Expected 'command' property to either be integer or string";
    return;
  }
  tao::json::value arguments = tao::json::null;
  auto found_arguments = obj_message.find("arguments");
  if (found_arguments != obj_message.end()) {
    arguments = found_arguments->second;
  }
  std::vector<tao::json::value> arguments_array;
  if (arguments.is_array()) {
    arguments_array = arguments.get_array();
  } else if (arguments.is_null()) {
    arguments_array.clear();
  } else if (arguments.is_object()) {
    arguments_array.push_back(arguments);
  } else {
    LOG("OnPublishCommandShort", error)
        << "Expected either no arguments (null), a single object, or an array "
           "as arguments";
    return;
  }
  SendCommand(api, endpoint, destination_address, destination_endpoint,
              *cluster_id, command, arguments_array);
}

void OnPublish(std::shared_ptr<znp::ZnpApi> api,
               std::shared_ptr<zcl::ZclEndpoint> endpoint,
               std::string mqtt_prefix,
               std::shared_ptr<zcl::NameRegistry> name_registry,
               std::string topic, std::string message, std::uint8_t qos,
               bool retain) {
  try {
    if (!boost::starts_with(topic, mqtt_prefix)) {
      LOG("OnPublish", debug)
          << "Ignoring publish not starting with our prefix";
      return;
    }
    topic = topic.substr(mqtt_prefix.size());

    std::smatch match;
    static std::regex re_write_permitjoin("write/permitjoin");
    if (std::regex_match(topic, re_write_permitjoin)) {
      OnPublishPermitJoin(api, message);
      return;
    }

    static std::regex re_command_short(
        "command/([0-9a-fA-F]+)/([0-9]+)/([^/]+)");
    if (std::regex_match(topic, match, re_command_short)) {
      OnPublishCommandShort(api, endpoint, name_registry,
                            std::stoull(match[1], 0, 16),
                            std::stoul(match[2], 0, 10), match[3], message);
      return;
    }

    static std::regex re_command_long(
        "command/([0-9a-fA-F]+)/([0-9]+)/([^/]+)/([^/]+)");
    if (std::regex_match(topic, match, re_command_long)) {
      OnPublishCommandLong(
          api, endpoint, name_registry, std::stoull(match[1], 0, 16),
          std::stoul(match[2], 0, 10), match[3], match[4], message);
      return;
    }

    LOG("OnPublish", debug) << "Unhandled MQTT publish to " << topic;
  } catch (const std::exception& ex) {
    LOG("OnPublish", debug) << "Exception: " << ex.what();
  }
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

void OnIncomingMsg(std::shared_ptr<znp::ZnpApi> api,
                   std::shared_ptr<MqttWrapper> mqtt_wrapper,
                   std::string mqtt_prefix, const znp::IncomingMsg& message) {
  api->UtilAddrmgrNwkAddrLookup(message.SrcAddr)
      .then([message, mqtt_wrapper, mqtt_prefix](znp::IEEEAddress ieee_addr) {
        return mqtt_wrapper->Publish(
            boost::str(boost::format("%sreport/%016X/linkquality") %
                       mqtt_prefix % ieee_addr),
            boost::str(boost::format("%d") % (unsigned int)message.LinkQuality),
            mqtt::qos::at_least_once, false);
      })
      .recover([](auto f) {
        try {
          f.get_try();
        } catch (const std::exception& ex) {
          LOG("OnIncomingMsg", warning)
              << "Unable to publish link quality: " << ex.what();
        }
      })
      .detach();
}

std::shared_ptr<zcl::ZclEndpoint> Initialize(
    coro::Await await, std::shared_ptr<znp::ZnpApi> api,
    std::array<uint8_t, 16> presharedkey,
    std::shared_ptr<MqttWrapper> mqtt_wrapper, std::string mqtt_prefix,
    std::shared_ptr<zcl::NameRegistry> name_registry) {
  LOG("Initialize", debug) << "Doing initial reset (this may take up to a full "
                              "minute after a dongle power-cycle)";
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
  api->af_on_incoming_msg_.connect(std::bind(
      &OnIncomingMsg, api, mqtt_wrapper, mqtt_prefix, std::placeholders::_1));

  mqtt_wrapper->on_publish_.connect(
      std::bind(&OnPublish, api, endpoint, mqtt_prefix, name_registry,
                std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3, std::placeholders::_4));
  await(mqtt_wrapper->Subscribe(
      {{mqtt_prefix + "write/#", mqtt::qos::at_least_once},
       {mqtt_prefix + "command/#", mqtt::qos::at_least_once}}));
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

  // Read cluster, command, & attribute names
  auto name_registry = std::make_shared<zcl::NameRegistry>();
  if (!name_registry->ReadFromInfo(variables["name-registry"].as<std::string>(),
                                   MakeNameSafeForMqtt)) {
    LOG("Main", critical) << "Unable to read '"
                          << variables["name-registry"].as<std::string>()
                          << "' name registry";
    return EXIT_FAILURE;
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
  int exit_code = EXIT_SUCCESS;
  auto endpoint =
      coro::Run(AsioExecutor(io_service), Initialize, api, presharedkey,
                mqtt_wrapper, mqtt_prefix, name_registry)
          .then([](auto r) {
            LOG("Main", info) << "Initialization complete!";
            return r;
          })
          .recover([&io_service, &exit_code](auto f) {
            LOG("Main", info) << "In final handler";
            try {
              return f.get_try();
            } catch (const std::exception& exc) {
              LOG("Main", critical) << "Exception: " << exc.what();
              exit_code = EXIT_FAILURE;
              io_service.stop();
              return (boost::optional<std::shared_ptr<zcl::ZclEndpoint>>)
                  boost::none;
            }
          });

  port->on_error_.connect([&io_service,
                           &exit_code](const boost::system::error_code& error) {
    LOG("Main", critical) << "Exiting because of IO error: " << error.message();
    exit_code = EXIT_FAILURE;
    io_service.stop();
  });

  std::cout << "IO Service starting" << std::endl;
  io_service.run();
  std::cout << "IO Service done" << std::endl;
  return exit_code;
}

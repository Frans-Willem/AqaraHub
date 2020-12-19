// vim: set shiftwidth=2 tabstop=2 expandtab:
#include <boost/algorithm/hex.hpp>
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
#include "clusterdb/cluster_db.h"
#include "coro.h"
#include "dynamic_encoding/decoding.h"
#include "dynamic_encoding/encoding.h"
#include "logging.h"
#include "mqtt_wrapper.h"
#include "string_enum.h"
#include "zcl/encoding.h"
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
           (this->pan_id == other.pan_id || this->pan_id == 0xFFFF ||
            other.pan_id == 0xFFFF) &&
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

void OnPublishDirectJoin(std::shared_ptr<znp::ZnpApi> api,
                         znp::IEEEAddress device_address) {
  api->ZdoMgmtDirectJoin(0x0000, device_address)
      .recover([](auto f) {
        try {
          f.get_try();
          LOG("DirectJoin", debug) << "Direct join OK";
        } catch (const std::exception& ex) {
          LOG("DirectJoin", debug) << "Direct join failed: " << ex.what();
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
                 std::shared_ptr<const clusterdb::ClusterInfo> cluster_info,
                 std::shared_ptr<const clusterdb::CommandInfo> command_info,
                 const tao::json::value& json_data) {
  std::vector<uint8_t> payload;
  try {
    dynamic_encoding::Context ctx;
    ctx.cluster = *cluster_info;
    dynamic_encoding::Encode(ctx, command_info->data, json_data, payload);
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
      .then([endpoint, destination_endpoint, cluster_info, command_info,
             payload](znp::ShortAddress short_address) {
        LOG("SendCommand", info) << "Response received, short address: "
                                 << (unsigned int)short_address;
        return endpoint->SendCommand(short_address, destination_endpoint,
                                     cluster_info->id, command_info->is_global,
                                     zcl::ZclDirection::ClientToServer,
                                     command_info->id, payload);
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
                          std::shared_ptr<clusterdb::ClusterDb> cluster_db,
                          znp::IEEEAddress destination_address,
                          std::uint8_t destination_endpoint,
                          std::string cluster_name, std::string command_name,
                          std::string message) {
  LOG("OnPublishCommandLong", debug)
      << "Destination " << destination_address << ", endpoint "
      << (unsigned int)destination_endpoint << ", cluster name '"
      << cluster_name << "', command name '" << command_name << "'";
  auto cluster_info = cluster_db->ClusterByName(cluster_name);
  if (!cluster_info) {
    LOG("OnPublishCommandLong", warning)
        << "Unable to look up cluster info for '" << cluster_name << "'";
    return;
  }
  auto command_info = cluster_db->CommandByName(
      cluster_info->id, zcl::ZclDirection::ClientToServer, command_name);
  if (!command_info) {
    LOG("OnPublishCommandLong", warning)
        << "Unable to look up command info for '" << command_name
        << "' in cluster '" << cluster_name << "'";
    return;
  }
  tao::json::value json_data = tao::json::null;
  if (message.size() > 0) {
    try {
      json_data = tao::json::from_string(message);
    } catch (const std::exception& ex) {
      LOG("OnPublishCommandLong", error)
          << "Unable to decode message payload as JSON: " << ex.what();
      return;
    }
  }

  SendCommand(api, endpoint, destination_address, destination_endpoint,
              std::shared_ptr<const clusterdb::ClusterInfo>(
                  cluster_db, cluster_info.get_ptr()),
              std::shared_ptr<const clusterdb::CommandInfo>(
                  cluster_db, command_info.get_ptr()),
              json_data);
}

/** Called on MQTT publish of a short-form command, e.g. command name part of
 * the JSON payload. */
void OnPublishCommandShort(std::shared_ptr<znp::ZnpApi> api,
                           std::shared_ptr<zcl::ZclEndpoint> endpoint,
                           std::shared_ptr<clusterdb::ClusterDb> cluster_db,
                           znp::IEEEAddress destination_address,
                           std::uint8_t destination_endpoint,
                           std::string cluster_name, std::string message) {
  LOG("OnPublishCommandShort", debug)
      << "Destination " << destination_address << ", endpoint "
      << (unsigned int)destination_endpoint << ", cluster name '"
      << cluster_name << "'";
  auto cluster_info = cluster_db->ClusterByName(cluster_name);
  if (!cluster_info) {
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
  auto command_info = cluster_db->CommandByName(
      cluster_info->id, zcl::ZclDirection::ClientToServer,
      found_command->second.get_string());
  if (!command_info) {
    LOG("OnPublishCommandShort", error)
        << "Command with name '" << found_command->second.get_string()
        << "' could not be decoded";
    return;
  }
  tao::json::value arguments = tao::json::null;
  auto found_arguments = obj_message.find("arguments");
  if (found_arguments != obj_message.end()) {
    arguments = found_arguments->second;
  }
  SendCommand(api, endpoint, destination_address, destination_endpoint,
              std::shared_ptr<const clusterdb::ClusterInfo>(
                  cluster_db, cluster_info.get_ptr()),
              std::shared_ptr<const clusterdb::CommandInfo>(
                  cluster_db, command_info.get_ptr()),
              arguments);
}

void OnPublish(std::shared_ptr<znp::ZnpApi> api,
               std::shared_ptr<zcl::ZclEndpoint> endpoint,
               std::string mqtt_prefix, std::string mqtt_prefix_write,
               std::shared_ptr<clusterdb::ClusterDb> cluster_db,
               std::string atopic, std::string message, std::uint8_t qos,
               bool retain) {
  try {
    std::smatch match;
    if (boost::starts_with(atopic, mqtt_prefix_write)) {
      std::string topic = atopic.substr(mqtt_prefix_write.size());
   
      static std::regex re_write_permitjoin("write/permitjoin");
      if (std::regex_match(topic, re_write_permitjoin)) {
        OnPublishPermitJoin(api, message);
        return;
      }
      static std::regex re_write_directjoin("write/directjoin/([0-9a-fA-F]+)");
      if (std::regex_match(topic, match, re_write_directjoin)) {
        OnPublishDirectJoin(api, std::stoull(match[1], 0, 16));
        return;
      }
      LOG("OnPublish", debug) << "Unhandled MQTT publish to " << topic << " in prefix " << mqtt_prefix_write;
    } else if (boost::starts_with(atopic, mqtt_prefix)) {
      std::string topic = atopic.substr(mqtt_prefix.size());
      static std::regex re_command_short("([0-9a-fA-F]+)/([0-9]+)/out/([^/]+)");
      if (std::regex_match(topic, match, re_command_short)) {
        OnPublishCommandShort(api, endpoint, cluster_db,
                              std::stoull(match[1], 0, 16),
                              std::stoul(match[2], 0, 10), match[3], message);
        return;
      }

      static std::regex re_command_long(
          "([0-9a-fA-F]+)/([0-9]+)/out/([^/]+)/([^/]+)");
      if (std::regex_match(topic, match, re_command_long)) {
        OnPublishCommandLong(
            api, endpoint, cluster_db, std::stoull(match[1], 0, 16),
            std::stoul(match[2], 0, 10), match[3], match[4], message);
        return;
      }
      LOG("OnPublish", debug) << "Unhandled MQTT publish to " << topic << " in prefix " << mqtt_prefix;
    } else {
      LOG("OnPublish", debug)
          << "Ignoring publish not starting with our prefix";
    }
  } catch (const std::exception& ex) {
    LOG("OnPublish", debug) << "Exception: " << ex.what();
  }
}

void OnPermitJoinHelper(std::shared_ptr<MqttWrapper> mqtt_wrapper,
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

void OnPermitJoin(std::shared_ptr<MqttWrapper> mqtt_wrapper,
                  std::string mqtt_prefix, std::string mqtt_prefix_write, uint8_t duration) {
  OnPermitJoinHelper(mqtt_wrapper, mqtt_prefix_write, duration);
  if (mqtt_prefix != mqtt_prefix_write) {
    OnPermitJoinHelper(mqtt_wrapper, mqtt_prefix, duration);
  }
}

void OnTcDevice(std::shared_ptr<MqttWrapper> mqtt_wrapper,
                std::string mqtt_prefix, znp::ShortAddress network_address,
                znp::IEEEAddress ieee_address,
                znp::ShortAddress parent_address) {
  const tao::json::value information = {
      {"network_address", network_address},
      {"ieee_address", boost::str(boost::format("%016X") % ieee_address)},
      {"parent_address", parent_address}};

  LOG("OnTcDevice", info) << "Device added to trustcenter: "
                          << boost::str(boost::format("%016X") % ieee_address);

  mqtt_wrapper
      ->Publish(mqtt_prefix + "report/trustcenter_device",
                tao::json::to_string(information), mqtt::qos::at_least_once,
                false)
      .recover([](auto f) {
        try {
          f.get_try();
          LOG("OnTcDevice", debug) << "Published OK";
        } catch (const std::exception& ex) {
          LOG("OnTcDevice", debug) << "Publish failure: " << ex.what();
        }
      })
      .detach();
}

void OnEndDeviceAnnounce(std::shared_ptr<MqttWrapper> mqtt_wrapper,
                         std::string mqtt_prefix,
                         znp::ShortAddress source_address,
                         znp::ShortAddress network_address,
                         znp::IEEEAddress ieee_address, uint8_t capabilities) {
  const tao::json::value information = {
      {"source", source_address},
      {"network_address", network_address},
      {"ieee_address", boost::str(boost::format("%016X") % ieee_address)},
      {"capabilities", (int)capabilities}};

  LOG("OnEndDeviceAnnounce", info)
      << "End device announced: "
      << boost::str(boost::format("%016X") % ieee_address);

  mqtt_wrapper
      ->Publish(mqtt_prefix + "report/end_device_announce",
                tao::json::to_string(information), mqtt::qos::at_least_once,
                false)
      .recover([](auto f) {
        try {
          f.get_try();
          LOG("OnEndDeviceAnnounce", debug) << "Published OK";
        } catch (const std::exception& ex) {
          LOG("OnEndDeviceAnnounce", debug) << "Publish failure: " << ex.what();
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
            boost::str(boost::format("%s%016X/linkquality") % mqtt_prefix %
                       ieee_addr),
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

stlab::future<void> PublishValue(std::shared_ptr<MqttWrapper> mqtt_wrapper,
                                 const std::string& topic, bool recursive,
                                 const tao::json::value& value) {
  std::string value_as_string(tao::json::to_string(value));
  LOG("PublishValue", info)
      << "Publishing to '" << topic << "': " << value_as_string;
  std::vector<stlab::future<void>> futures;
  futures.push_back(
      mqtt_wrapper
          ->Publish(topic, value_as_string, mqtt::qos::at_least_once, false)
          .recover([](auto f) {
            try {
              f.get_try();
            } catch (const std::exception& ex) {
              LOG("PublishValue", warning)
                  << "Unable to publish to MQTT: " << ex.what();
            }
          }));
  if (recursive) {
    if (value.is_object()) {
      const tao::json::value::object_t& object_value = value.get_object();
      for (const auto& item : object_value) {
        futures.push_back(PublishValue(
            mqtt_wrapper,
            boost::str(boost::format("%s/%s") % topic % item.first), recursive,
            item.second));
      }
    } else if (value.is_array()) {
      const tao::json::value::array_t& array_value = value.get_array();
      for (std::size_t index = 0; index < array_value.size(); index++) {
        futures.push_back(PublishValue(
            mqtt_wrapper, boost::str(boost::format("%s/%d") % topic % index),
            recursive, array_value[index]));
      }
    }
  }
  if (futures.size() == 1) {
    return futures[0];
  } else {
    return stlab::when_all(
        stlab::immediate_executor, []() {},
        std::make_pair(futures.begin(), futures.end()));
  }
}

const tao::json::value& JsonGetProperty(const tao::json::value& object,
                                        const std::string& property) {
  static tao::json::value not_found = tao::json::null;
  if (!object.is_object()) {
    return not_found;
  }
  const tao::json::value::object_t& object_map = object.get_object();
  auto found = object_map.find(property);
  if (found == object_map.end()) {
    return not_found;
  }
  return found->second;
}

const tao::json::value::array_t& JsonAsArray(const tao::json::value& array) {
  static tao::json::value::array_t empty{};
  if (!array.is_array()) {
    return empty;
  }
  return array.get_array();
}

void OnZclCommand(std::shared_ptr<MqttWrapper> mqtt_wrapper,
                  std::string mqtt_prefix, bool mqtt_recursive_publish,
                  znp::IEEEAddress source_address, uint8_t source_endpoint,
                  std::shared_ptr<const clusterdb::ClusterInfo> cluster_info,
                  std::shared_ptr<const clusterdb::CommandInfo> command_info,
                  std::vector<uint8_t> payload) {
  std::string topic(boost::str(
      boost::format("%s%016X/%d/in/%s/%s") % mqtt_prefix % source_address %
      (unsigned int)source_endpoint % cluster_info->name % command_info->name));
  tao::json::value json_payload;
  try {
    dynamic_encoding::Context ctx;
    ctx.cluster = *cluster_info;
    auto parsed_until = payload.cbegin();
    json_payload = dynamic_encoding::Decode(ctx, command_info->data,
                                            parsed_until, payload.cend());
    if (parsed_until != payload.cend()) {
      LOG("OnZclCommand", warning) << "Not all data properly parsed";
    }
  } catch (const std::exception& ex) {
    LOG("OnZclCommand", warning)
        << "Unable to decode command payload: " << ex.what();
    return;
  }
  std::vector<stlab::future<void>> futures;
  futures.push_back(
      PublishValue(mqtt_wrapper, topic, mqtt_recursive_publish, json_payload));

  // Is this a repeated object type, with attribId as first property?
  if (command_info->data.properties.size() > 0) {
    if (const auto* repeated_type =
            boost::relaxed_get<dynamic_encoding::ArrayType>(
                &command_info->data.properties[0].type)) {
      if (const auto* repeated_object_type =
              boost::relaxed_get<dynamic_encoding::ObjectType>(
                  &repeated_type->element_type)) {
        if (repeated_object_type->properties.size() >= 2 &&
            repeated_object_type->properties[0].type ==
                dynamic_encoding::AnyType(zcl::DataType::attribId)) {
          LOG("OnZclCommand", info) << "Looks like something per-attribute. "
                                       "Publishing per-attribute too";
          // All checks succeeded, let's attempt to unpack the JSON!
          const tao::json::value::array_t& reports =
              JsonAsArray(JsonGetProperty(
                  json_payload, command_info->data.properties[0].name));
          for (const auto& report : reports) {
            const tao::json::value& attribute_id = JsonGetProperty(
                report, repeated_object_type->properties[0].name);
            const tao::json::value& attribute_value = JsonGetProperty(
                report, repeated_object_type->properties[1].name);
            std::string subtopic;
            if (attribute_id.is_string()) {
              subtopic = attribute_id.get_string();
            } else if (attribute_id.is_unsigned()) {
              subtopic = boost::str(boost::format("0x%04X") %
                                    attribute_id.get_unsigned());
            } else {
              subtopic = tao::json::to_string(attribute_id);
            }
            futures.push_back(PublishValue(mqtt_wrapper, topic + "/" + subtopic,
                                           mqtt_recursive_publish,
                                           attribute_value));
          }
        }
      }
    }
  }

  if (futures.size() == 1) {
    futures[0].detach();
  } else {
    stlab::when_all(
        stlab::immediate_executor, []() {},
        std::make_pair(futures.begin(), futures.end()))
        .detach();
  }
}

void OnZclCommand(std::shared_ptr<clusterdb::ClusterDb> cluster_db,
                  std::shared_ptr<znp::ZnpApi> api,
                  std::shared_ptr<MqttWrapper> mqtt_wrapper,
                  std::string mqtt_prefix, bool mqtt_recursive_publish,
                  znp::ShortAddress source_address, uint8_t source_endpoint,
                  zcl::ZclClusterId cluster_id, bool is_global_command,
                  zcl::ZclDirection direction, zcl::ZclCommandId command_id,
                  std::vector<uint8_t> payload) {
  auto cluster_info = cluster_db->ClusterById(cluster_id);
  if (!cluster_info) {
    LOG("OnZclCommand", warning)
        << boost::str(boost::format("Unknown cluster ID 0x%02X, ignoring") %
                      (unsigned int)cluster_id);
    return;
  }
  boost::optional<const clusterdb::CommandInfo&> command_info =
      cluster_db->CommandById(cluster_id, command_id, is_global_command,
                              direction);
  if (!command_info) {
    LOG("OnZclCommand", warning) << boost::str(
        boost::format("Unknown command ID 0x%02X in cluster '%s', ignoring") %
        (unsigned int)command_id % cluster_info->name);
    return;
  }
  std::shared_ptr<const clusterdb::CommandInfo> ptr_command_info(
      cluster_db, command_info.get_ptr());
  std::shared_ptr<const clusterdb::ClusterInfo> ptr_cluster_info(
      cluster_db, cluster_info.get_ptr());

  api->UtilAddrmgrNwkAddrLookup(source_address)
      .then([mqtt_wrapper, mqtt_prefix, mqtt_recursive_publish, source_endpoint,
             ptr_cluster_info, ptr_command_info,
             payload](znp::IEEEAddress source_address) {
        OnZclCommand(mqtt_wrapper, mqtt_prefix, mqtt_recursive_publish,
                     source_address, source_endpoint, ptr_cluster_info,
                     ptr_command_info, payload);
      })
      .recover([](auto f) {
        try {
          f.get_try();
        } catch (const std::exception& ex) {
          LOG("OnZclCommand", warning)
              << "Exception while looking up long address of device: "
              << ex.what();
        }
      })
      .detach();
}

std::shared_ptr<zcl::ZclEndpoint> Initialize(
    coro::Await await, std::shared_ptr<znp::ZnpApi> api, uint16_t pan_id,
    uint32_t chan_list, std::array<uint8_t, 16> presharedkey,
    std::shared_ptr<MqttWrapper> mqtt_wrapper,
    std::string mqtt_prefix, std::string mqtt_prefix_write, 
    bool mqtt_recursive_publish,
    std::shared_ptr<clusterdb::ClusterDb> cluster_db) {
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
  desired_config.pan_id = pan_id;
  desired_config.extended_pan_id = coord_ieee_addr;
  desired_config.chan_list = chan_list;
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

  endpoint->on_command_.connect(
      [cluster_db, weak_api, mqtt_wrapper, mqtt_prefix, mqtt_recursive_publish](
          znp::ShortAddress source_address, uint8_t source_endpoint,
          zcl::ZclClusterId cluster_id, bool is_global_command,
          zcl::ZclDirection direction, zcl::ZclCommandId command_id,
          std::vector<uint8_t> payload) {
        if (auto api = weak_api.lock()) {
          OnZclCommand(cluster_db, api, mqtt_wrapper, mqtt_prefix,
                       mqtt_recursive_publish, source_address, source_endpoint,
                       cluster_id, is_global_command, direction, command_id,
                       std::move(payload));
        }
      });

  api->zdo_on_permit_join_.connect(std::bind(
      &OnPermitJoin, mqtt_wrapper, mqtt_prefix, mqtt_prefix_write, std::placeholders::_1));
  api->af_on_incoming_msg_.connect(std::bind(
      &OnIncomingMsg, api, mqtt_wrapper, mqtt_prefix, std::placeholders::_1));
  api->zdo_on_trustcenter_device_.connect(
      std::bind(&OnTcDevice, mqtt_wrapper, mqtt_prefix, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3));
  api->zdo_on_end_device_announce_.connect(std::bind(
      &OnEndDeviceAnnounce, mqtt_wrapper, mqtt_prefix, std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

  mqtt_wrapper->on_publish_.connect(std::bind(
      &OnPublish, api, endpoint, mqtt_prefix, mqtt_prefix_write, cluster_db, std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
  await(mqtt_wrapper->Subscribe({
      {mqtt_prefix_write + "write/#", mqtt::qos::at_least_once},
      {mqtt_prefix + "+/+/out/#", mqtt::qos::at_least_once},
  }));
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

void MakePrefixEndWithSlash(std::string &mqtt_prefix) {
  if (mqtt_prefix.size() > 0 && mqtt_prefix[mqtt_prefix.size() - 1] != '/') {
    mqtt_prefix += "/";
  }
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

  const uint32_t CHANNEL_ALL_MASK = 0x07FFF800;

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
    ("write-topic,w",
     boost::program_options::value<std::string>(),
     "Put write topic under different MQTT Root. This is useful for machines with multiple sticks, but wanting to publish as one.")
    ("panid",
     boost::program_options::value<uint16_t>()->default_value(0xFFFF),
     "Zigbee PAN ID")
    ("psk",
     boost::program_options::value<std::string>(),
     "Zigbee Network pre-shared key. Maximum 16 characters, will be truncated when longer")
    ("pskhex",
     boost::program_options::value<std::string>(),
     "Zigbee Network pre-shared key in hexadecimal notation, maximum of 16 bytes (32 hex characters), will be padded with 0-bytes.")
    ("cluster-info",
     boost::program_options::value<std::string>()->default_value("../clusters.info"),
     "Boost property-tree info file containing cluster, attribute, and command information")
    ("recursive-publish",
     "Recursively publish object properties and array elements to sub-topics")
    ("channelmask,c",
     boost::program_options::value<std::string>()->default_value("0x0800"),
     "Allowed channel mask. Bit 0 channel 1 to bit 31 channel 32, i.e. channel 11 - 0x0800, channel 26 = 0x04000000")
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
  auto cluster_db = std::make_shared<clusterdb::ClusterDb>();
  if (!cluster_db->ParseFromFile(variables["cluster-info"].as<std::string>(),
                                 MakeNameSafeForMqtt)) {
    LOG("Main", critical) << "Unable to read '"
                          << variables["cluster-info"].as<std::string>()
                          << "' for cluster information";
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
  auto api = std::make_shared<znp::ZnpApi>(io_service, port);

  std::string mqtt_prefix = variables["topic"].as<std::string>();
  MakePrefixEndWithSlash(mqtt_prefix);
  LOG("Main", info) << "Using MQTT prefix '" << mqtt_prefix << "'";
  std::string mqtt_prefix_write = mqtt_prefix;

  bool mqtt_recursive_publish = (variables.count("recursive-publish") > 0);
  LOG("Main", info) << "Recursively publishing object and array properties";

  if (variables.count("write-topic") > 0) {
     mqtt_prefix_write = variables["write-topic"].as<std::string>();
  }
  MakePrefixEndWithSlash(mqtt_prefix_write);
  LOG("Main", info) << "Using MQTT prefix write '" << mqtt_prefix_write << "'";

  LOG("Main", info) << "Setting up MQTT connection";
  std::shared_ptr<MqttWrapper> mqtt_wrapper;
  try {
    mqtt_wrapper =
        MqttWrapper::FromUrl(io_service, variables["mqtt"].as<std::string>(), mqtt_prefix_write);
  } catch (const std::exception& ex) {
    std::cerr << ex.what() << std::endl;
    return EXIT_FAILURE;
  }

  // Creating pre-shared-key
  std::array<uint8_t, 16> presharedkey;
  presharedkey.fill(0);
  if (variables.count("psk") > 0 && variables.count("pskhex") > 0) {
    LOG("Main", critical)
        << "Error: pass either --psk or --pskhex, but not both.";
    return EXIT_FAILURE;
  }
  if (variables.count("psk")) {
    std::string presharedkey_str(variables["psk"].as<std::string>());
    if (presharedkey_str.size() > presharedkey.size()) {
      LOG("Main", warning) << "WARNING: Presharedkey will be truncated to "
                           << presharedkey.size() << " bytes";
    }
    std::copy_n(presharedkey_str.begin(),
                std::min(presharedkey.size(), presharedkey_str.size()),
                presharedkey.begin());
  } else if (variables.count("pskhex")) {
    std::vector<uint8_t> presharedkey_input;
    try {
      boost::algorithm::unhex(variables["pskhex"].as<std::string>(),
                              std::back_inserter(presharedkey_input));
    } catch (...) {
      LOG("Main", critical) << "Unable to parse hexadecimal PSK";
      return EXIT_FAILURE;
    }
    if (presharedkey_input.size() > presharedkey.size()) {
      LOG("Main", critical)
          << "PSK length " << presharedkey_input.size()
          << " is larger than the allowed " << presharedkey.size() << " bytes";
      return EXIT_FAILURE;
    }
    std::copy(presharedkey_input.begin(), presharedkey_input.end(),
              presharedkey.begin());
  } else {
    LOG("Main", info)
        << "No PSK specified on command-line, using default ('AqaraHub')";
    std::string presharedkey_str("AqaraHub");
    std::copy_n(presharedkey_str.begin(),
                std::min(presharedkey.size(), presharedkey_str.size()),
                presharedkey.begin());
  }

  LOG("Main", info) << "Using PSK "
                    << boost::log::dump(presharedkey.data(),
                                        presharedkey.size());

  // Initializing
  int exit_code = EXIT_SUCCESS;
  auto endpoint =
      coro::Run(
          AsioExecutor(io_service), Initialize, api,
          variables["panid"].as<uint16_t>(),
          std::stoul(variables["channelmask"].as<std::string>(), nullptr, 0) &
              CHANNEL_ALL_MASK,
          presharedkey, mqtt_wrapper,
          mqtt_prefix, mqtt_prefix_write,
          mqtt_recursive_publish,
          cluster_db)
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

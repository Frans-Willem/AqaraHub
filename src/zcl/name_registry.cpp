#include "zcl/name_registry.h"
#include <boost/format.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>
#include "logging.h"

namespace zcl {

bool NameRegistry::ReadFromInfo(
    const std::string& filename,
    std::function<std::string(std::string)> name_mangler) {
  boost::property_tree::ptree tree;
  boost::property_tree::info_parser::read_info(filename, tree);
  // Parse clusters
  for (const auto& entry : tree) {
    std::string name = name_mangler(entry.second.data());
    std::string s_id = entry.first;
    std::size_t end_parsed;
    unsigned long id = std::stoul(s_id, &end_parsed, 0);
    if (end_parsed != s_id.size()) {
    }
    if (id > (unsigned long)std::numeric_limits<std::uint16_t>::max()) {
      LOG("NameRegistry", info) << "ClusterID " << s_id << " too big";
      return false;
    }
    cluster_names_.insert({(zcl::ZclClusterId)(std::uint16_t)id, name});

    auto attributes = entry.second.get_child_optional("attributes");

    // Parse attributes
    if (attributes) {
      for (const auto& attr_entry : *attributes) {
        std::string a_name = name_mangler(attr_entry.second.data());
        std::string s_a_id = attr_entry.first;
        unsigned long a_id = std::stoul(s_a_id, &end_parsed, 0);
        if (end_parsed != s_a_id.size()) {
          LOG("NameRegistry", warning)
              << "Unable to fully parse Attribute ID " << s_a_id;
          return false;
        }
        if (a_id >
            (unsigned long)std::numeric_limits<
                std::underlying_type<zcl::ZclAttributeId>::type>::max()) {
          LOG("NameRegistry", warning)
              << "Attribute ID " << s_a_id << " too big";
          return false;
        }
        attribute_names_[(zcl::ZclClusterId)id].insert(
            {(zcl::ZclAttributeId)(
                 std::underlying_type<zcl::ZclAttributeId>::type)a_id,
             a_name});
      }
    }

    auto commands = entry.second.get_child_optional("commands");
    // Parse commands
    if (commands) {
      for (const auto& cmd_entry : *commands) {
        std::string c_name = name_mangler(cmd_entry.second.data());
        std::string s_c_id = cmd_entry.first;
        unsigned long c_id = std::stoul(s_c_id, &end_parsed, 0);
        if (end_parsed != s_c_id.size()) {
          LOG("NameRegistry", warning)
              << "Unable to fully parse Command ID " << s_c_id;
          return false;
        }
        if (c_id > (unsigned long)std::numeric_limits<
                       std::underlying_type<zcl::ZclCommandId>::type>::max()) {
          LOG("NameRegistry", warning)
              << "Attribute ID " << s_c_id << " too big";
          return false;
        }
        command_names_[(zcl::ZclClusterId)id].insert(
            {(zcl::ZclCommandId)(
                 std::underlying_type<zcl::ZclCommandId>::type)c_id,
             c_name});
      }
    }
  }
  return true;
}

std::string NameRegistry::ClusterToString(zcl::ZclClusterId cluster_id) {
  auto found = cluster_names_.left.find(cluster_id);
  if (found != cluster_names_.left.end()) {
    return found->second;
  }
  return boost::str(boost::format("0x%04X") % ((unsigned int)cluster_id));
}

boost::optional<zcl::ZclClusterId> NameRegistry::ClusterFromString(
    const std::string& cluster_name) {
  auto found = cluster_names_.right.find(cluster_name);
  if (found != cluster_names_.right.end()) {
    return found->second;
  }
  std::size_t end_pos;
  unsigned long id = std::stoul(cluster_name, &end_pos, 16);
  if (end_pos != cluster_name.size()) {
    return boost::none;
  }
  if (id > std::numeric_limits<std::uint16_t>::max()) {
    return boost::none;
  }
  return (zcl::ZclClusterId)id;
}

std::string NameRegistry::AttributeToString(zcl::ZclClusterId cluster_id,
                                            zcl::ZclAttributeId attribute_id) {
  auto cluster_found = attribute_names_.find(cluster_id);
  if (cluster_found != attribute_names_.end()) {
    auto attribute_found = cluster_found->second.left.find(attribute_id);
    if (attribute_found != cluster_found->second.left.end()) {
      return attribute_found->second;
    }
  }
  return boost::str(boost::format("0x%04X") % ((unsigned int)attribute_id));
}

boost::optional<zcl::ZclAttributeId> NameRegistry::AttributeFromString(
    zcl::ZclClusterId cluster_id, const std::string& attribute_name) {
  auto cluster_found = attribute_names_.find(cluster_id);
  if (cluster_found != attribute_names_.end()) {
    auto attribute_found = cluster_found->second.right.find(attribute_name);
    if (attribute_found != cluster_found->second.right.end()) {
      return attribute_found->second;
    }
  }
  std::size_t end_pos;
  unsigned long id = std::stoul(attribute_name, &end_pos, 0);
  if (end_pos != attribute_name.size()) {
    return boost::none;
  }
  if (id > std::numeric_limits<
               std::underlying_type<zcl::ZclAttributeId>::type>::max()) {
    return boost::none;
  }
  return (zcl::ZclAttributeId)(
      std::underlying_type<zcl::ZclAttributeId>::type)id;
}

std::string NameRegistry::CommandToString(zcl::ZclClusterId cluster_id,
                                          zcl::ZclCommandId command) {
  auto cluster_found = command_names_.find(cluster_id);
  if (cluster_found != command_names_.end()) {
    auto command_found = cluster_found->second.left.find(command);
    if (command_found != cluster_found->second.left.end()) {
      return command_found->second;
    }
  }
  return boost::str(boost::format("0x%02X") % ((unsigned int)command));
}

boost::optional<zcl::ZclCommandId> NameRegistry::CommandFromString(
    zcl::ZclClusterId cluster_id, const std::string& command_name) {
  auto cluster_found = command_names_.find(cluster_id);
  if (cluster_found != command_names_.end()) {
    auto command_found = cluster_found->second.right.find(command_name);
    if (command_found != cluster_found->second.right.end()) {
      return command_found->second;
    }
  }
  std::size_t end_pos;
  unsigned long id = std::stoul(command_name, &end_pos, 0);
  if (end_pos != command_name.size()) {
    return boost::none;
  }
  if (id > std::numeric_limits<
               std::underlying_type<zcl::ZclCommandId>::type>::max()) {
    return boost::none;
  }
  return (zcl::ZclCommandId)(std::underlying_type<zcl::ZclCommandId>::type)id;
}
}  // namespace zcl

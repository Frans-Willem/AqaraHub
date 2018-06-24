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
  return db_.ParseFromFile(filename, name_mangler);
}

std::string NameRegistry::ClusterToString(zcl::ZclClusterId cluster_id) {
  auto found = db_.ClusterById(cluster_id);
  if (found) {
    return found->name;
  }
  return boost::str(boost::format("0x%04X") % ((unsigned int)cluster_id));
}

boost::optional<zcl::ZclClusterId> NameRegistry::ClusterFromString(
    const std::string& cluster_name) {
  auto found = db_.ClusterByName(cluster_name);
  if (found) {
    return found->id;
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
  auto cluster_found = db_.ClusterById(cluster_id);
  if (cluster_found) {
    auto attribute_found = cluster_found->attributes.FindById(attribute_id);
    if (attribute_found) {
      return attribute_found->name;
    }
  }
  return boost::str(boost::format("0x%04X") % ((unsigned int)attribute_id));
}

boost::optional<zcl::ZclAttributeId> NameRegistry::AttributeFromString(
    zcl::ZclClusterId cluster_id, const std::string& attribute_name) {
  auto cluster_found = db_.ClusterById(cluster_id);
  if (cluster_found) {
    auto attribute_found = cluster_found->attributes.FindByName(attribute_name);
    if (attribute_found) {
      return attribute_found->id;
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
  auto cluster_found = db_.ClusterById(cluster_id);
  if (cluster_found) {
    auto command_found = cluster_found->commands_out.FindById(command);
    if (command_found) {
      return command_found->name;
    }
  }
  return boost::str(boost::format("0x%02X") % ((unsigned int)command));
}

boost::optional<zcl::ZclCommandId> NameRegistry::CommandFromString(
    zcl::ZclClusterId cluster_id, const std::string& command_name) {
  auto cluster_found = db_.ClusterById(cluster_id);
  if (cluster_found) {
    auto command_found = cluster_found->commands_out.FindByName(command_name);
    if (command_found) {
      return command_found->id;
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

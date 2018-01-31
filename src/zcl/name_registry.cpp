#include "zcl/name_registry.h"
#include <boost/format.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>
#include "logging.h"

namespace zcl {
bool NameRegistry::ReadFromInfo(const std::string& filename) {
  boost::property_tree::ptree tree;
  boost::property_tree::info_parser::read_info(filename, tree);
  for (const auto& entry : tree) {
    std::string name = entry.second.data();
    std::string s_id = entry.first;
    std::size_t end_parsed;
    unsigned long id = std::stoul(s_id, &end_parsed, 0);
    if (end_parsed != s_id.size()) {
      LOG("NameRegistry", info) << "Unable to fully parse ID " << s_id;
      return false;
    }
    if (id > (unsigned long)std::numeric_limits<std::uint16_t>::max()) {
      LOG("NameRegistry", info) << "ClusterID " << s_id << " too big";
      return false;
    }
    cluster_names_.insert({(zcl::ZclClusterId)(std::uint16_t)id, name});
    for (const auto& attr_entry : entry.second) {
      std::string a_name = attr_entry.second.data();
      std::string s_a_id = attr_entry.first;
      unsigned long a_id = std::stoul(s_a_id, &end_parsed, 0);
      if (end_parsed != s_a_id.size()) {
        LOG("NameRegistry", warning)
            << "Unable to fully parse Attribute ID " << s_a_id;
        return false;
      }
      if (a_id > (unsigned long)std::numeric_limits<std::uint16_t>::max()) {
        LOG("NameRegistry", warning) << "Attribute ID " << s_a_id << " too big";
        return false;
      }
      attribute_names_[(zcl::ZclClusterId)id].insert(
          {(zcl::ZclAttributeId)(std::uint16_t)a_id, a_name});
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
  unsigned long id = std::stoul(attribute_name, &end_pos, 16);
  if (end_pos != attribute_name.size()) {
    return boost::none;
  }
  if (id > std::numeric_limits<std::uint16_t>::max()) {
    return boost::none;
  }
  return (zcl::ZclAttributeId)id;
}
}  // namespace zcl

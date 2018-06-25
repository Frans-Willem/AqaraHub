#include "clusterdb/cluster_db.h"
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <functional>
#include "clusterdb/searchable_list.h"
#include "string_enum.h"
#include "zcl/zcl_string_enum.h"

namespace clusterdb {
struct ClusterDb::Context {
  SearchableList<ClusterInfo> clusters;
  SearchableList<CommandInfo> global_commands;
};

namespace {
bool ParseArgumentListFromPTree(
    std::vector<ArgumentInfo>& arguments,
    const boost::property_tree::ptree& tree,
    std::function<std::string(std::string)> name_mangler) {
  for (const auto& entry : tree) {
    ArgumentInfo argument;
    argument.name = name_mangler(entry.second.data());
    std::string argument_type = entry.first;
    boost::optional<zcl::DataType> opt_datatype =
        string_to_enum<zcl::DataType>(argument_type);
    if (opt_datatype) {
      argument.type = DataTypeArgumentType{*opt_datatype};
    } else if (argument_type == "variant") {
      argument.type = SimpleArgumentType::Variant;
    } else if (argument_type == "attribute") {
      argument.type = SimpleArgumentType::Attribute;
    } else if (argument_type == "repeated") {
      RepeatedArgumentType repeated_type;
      if (!ParseArgumentListFromPTree(repeated_type.contents, entry.second,
                                      name_mangler)) {
        return false;
      }
      argument.type = std::move(repeated_type);
    } else {
      LOG("ClusterDb", critical) << "Unknown datatype " << argument_type;
      return false;
    }
    arguments.emplace_back(std::move(argument));
  }
  return true;
}

bool ParseCommandListFromPTree(
    SearchableList<CommandInfo>& commands,
    const boost::property_tree::ptree& tree,
    std::function<std::string(std::string)> name_mangler) {
  for (const auto& entry : tree) {
    std::string s_command_id = entry.first;
    std::size_t end_command_id;
    unsigned long command_id = std::stoul(s_command_id, &end_command_id, 0);
    if (end_command_id != s_command_id.size()) {
      LOG("ClusterDb", critical)
          << "Unable to fully parse command id " << s_command_id;
      return false;
    }
    if (command_id > (unsigned long)std::numeric_limits<std::uint8_t>::max()) {
      LOG("ClusterDb", critical)
          << "Command ID " << s_command_id << " does not fit in 8-bit";
      return false;
    }
    CommandInfo command_info;
    command_info.id = (zcl::ZclCommandId)(std::uint8_t)command_id;
    command_info.name = name_mangler(entry.second.data());
    if (!ParseArgumentListFromPTree(command_info.arguments, entry.second,
                                    name_mangler)) {
      return false;
    }
    if (!commands.Add(std::move(command_info))) {
      return false;
    }
  }
  return true;
}

bool ParseArgumentListFromPTree(
    SearchableList<AttributeInfo>& attributes,
    const boost::property_tree::ptree& tree,
    std::function<std::string(std::string)> name_mangler) {
  for (const auto& entry : tree) {
    std::string s_attribute_id = entry.first;
    std::size_t end_attribute_id;
    unsigned long attribute_id =
        std::stoul(s_attribute_id, &end_attribute_id, 0);
    if (end_attribute_id != s_attribute_id.size()) {
      LOG("ClusterDb", critical)
          << "Unable to fully parse attribute id " << s_attribute_id;
      return false;
    }
    if (attribute_id >
        (unsigned long)std::numeric_limits<std::uint16_t>::max()) {
      LOG("ClusterDb", critical)
          << "Attribute ID " << s_attribute_id << " does not fit in 16-bit";
      return false;
    }
    AttributeInfo attribute_info;
    attribute_info.id = (zcl::ZclAttributeId)attribute_id;
    attribute_info.name = name_mangler(entry.second.data());
    boost::optional<std::string> type_name =
        entry.second.get_optional<std::string>("type");
    if (type_name) {
      boost::optional<zcl::DataType> opt_type =
          string_to_enum<zcl::DataType>(*type_name);
      if (!opt_type) {
        LOG("ClusterDb", critical) << "Unknown type '" << *type_name << "'";
        return false;
      }
      attribute_info.datatype = opt_type;
    } else {
      attribute_info.datatype = boost::none;
    }
    if (!attributes.Add(std::move(attribute_info))) {
      return false;
    }
  }
  return true;
}

bool ParseClusterInfoFromPTree(
    ClusterInfo& cluster, const boost::property_tree::ptree& tree,
    std::function<std::string(std::string)> name_mangler) {
  cluster.name = name_mangler(tree.data());
  for (const auto& entry : tree) {
    if (entry.first == "attributes") {
      if (!ParseArgumentListFromPTree(cluster.attributes, entry.second,
                                      name_mangler)) {
        return false;
      }
    } else if (entry.first == "commands") {
      if (entry.second.data() == "in") {
        if (!ParseCommandListFromPTree(cluster.commands_in, entry.second,
                                       name_mangler)) {
          return false;
        }
      } else if (entry.second.data() == "out") {
        if (!ParseCommandListFromPTree(cluster.commands_out, entry.second,
                                       name_mangler)) {
          return false;
        }
      } else {
        LOG("ClusterDb", critical)
            << "Unknown commands section '" << entry.second.data()
            << "' in cluster '" << cluster.name << "'";
        return false;
      }
    } else {
      LOG("ClusterDb", critical) << "Unknown section '" << entry.first
                                 << "' in cluster '" << cluster.name << "'";
      return false;
    }
  }
  return true;
}

bool ParseFromPTree(const std::unique_ptr<ClusterDb::Context>& ctx,
                    const boost::property_tree::ptree& tree,
                    std::function<std::string(std::string)> name_mangler) {
  for (const auto& cluster_entry : tree) {
    if (cluster_entry.first == "global") {
      if (!ParseCommandListFromPTree(ctx->global_commands, cluster_entry.second,
                                     name_mangler)) {
        return false;
      }
    } else {
      std::string s_cluster_id = cluster_entry.first;
      std::size_t end_cluster_id;
      unsigned long cluster_id = std::stoul(s_cluster_id, &end_cluster_id, 0);
      if (end_cluster_id != s_cluster_id.size()) {
        LOG("ClusterDb", critical)
            << "Unable to fully parse cluster id " << s_cluster_id;
        return false;
      }
      if (cluster_id >
          (unsigned long)std::numeric_limits<std::uint16_t>::max()) {
        LOG("ClusterDb", critical)
            << "ClusterID " << s_cluster_id << " too big to fit in 16 bits";
        return false;
      }
      ClusterInfo cluster_info;
      cluster_info.id = (zcl::ZclClusterId)cluster_id;
      if (!ParseClusterInfoFromPTree(cluster_info, cluster_entry.second,
                                     name_mangler)) {
        LOG("ClusterDb", critical) << "Unable to parse cluster info for "
                                   << cluster_entry.second.data();
        return false;
      }
      if (!ctx->clusters.Add(std::move(cluster_info))) {
        LOG("ClusterDb", critical)
            << "Unable to add cluster info for " << cluster_entry.second.data();
        return false;
      }
    }
  }
  return true;
}
}  // namespace

ClusterDb::ClusterDb() : ctx_(std::make_unique<ClusterDb::Context>()) {}
ClusterDb::~ClusterDb() {}

boost::optional<const ClusterInfo&> ClusterDb::ClusterByName(
    const std::string& name) const {
  return ctx_->clusters.FindByName(name);
}
boost::optional<const ClusterInfo&> ClusterDb::ClusterById(
    const zcl::ZclClusterId& id) const {
  return ctx_->clusters.FindById(id);
}

boost::optional<const CommandInfo&> ClusterDb::GlobalCommandByName(
    const std::string& name) const {
  return ctx_->global_commands.FindByName(name);
}

boost::optional<const CommandInfo&> ClusterDb::GlobalCommandById(
    const zcl::ZclCommandId& id) const {
  return ctx_->global_commands.FindById(id);
}

boost::optional<std::pair<bool, const CommandInfo&>>
ClusterDb::CommandOutByName(const zcl::ZclClusterId& cluster_id,
                            const std::string& name) {
  auto global_found = GlobalCommandByName(name);
  if (global_found) {
    return std::make_pair(true, std::ref(*global_found));
  }
  auto cluster_found = ClusterById(cluster_id);
  if (!cluster_found) {
    return boost::none;
  }
  auto local_found = cluster_found->commands_out.FindByName(name);
  if (local_found) {
    return std::make_pair(false, std::ref(*local_found));
  }
  return boost::none;
}

bool ClusterDb::ParseFromFile(
    const std::string& filename,
    std::function<std::string(std::string)> name_mangler) {
  boost::property_tree::ptree tree;
  boost::property_tree::info_parser::read_info(filename, tree);
  return ParseFromPTree(this->ctx_, tree, name_mangler);
}

bool ClusterDb::ParseFromStream(
    std::istream& stream,
    std::function<std::string(std::string)> name_mangler) {
  boost::property_tree::ptree tree;
  boost::property_tree::info_parser::read_info(stream, tree);
  return ParseFromPTree(this->ctx_, tree, name_mangler);
}
}  // namespace clusterdb

#include "mqtt_wrapper.h"
#include <boost/optional/optional_io.hpp>
#include <boost/lexical_cast.hpp>
#include <regex>
#include <limits>
#include <cstdlib>
#include "mqtt_wrapper_impl.h"
#include "uri_parser.h"

bool MqttWrapper::Parameters::operator==(
    const MqttWrapper::Parameters& other) const {
  return use_tls == other.use_tls && use_ws == other.use_ws &&
         hostname == other.hostname && port == other.port &&
         username == other.username && password == other.password &&
         client_id == other.client_id;
}

std::ostream& operator<<(std::ostream& s,
                         const MqttWrapper::Parameters& params) {
  return s << "{ " << (params.use_tls ? "TLS" : "Unencrypted") << ", "
           << (params.use_ws ? "WebSocket" : "TCP") << ", " << params.hostname
           << ", " << params.port << ", " << params.username << ", "
           << (params.password ? "(password)" : "(no password)") << ", "
           << params.client_id << " }";
}

boost::optional<MqttWrapper::Parameters> MqttWrapper::ParseUrl(
    const std::string& url) {
  boost::optional<URI> parsed_uri = ParseURI(url);
  if (!parsed_uri) {
    LOG("MqttWrapper", warning)
        << "Unable to parse MQTT URL '" << url << "', not a valid URI";
    return boost::none;
  }
  MqttWrapper::Parameters params;
  if (parsed_uri->scheme == "mqtt") {
    params.use_tls = false;
    params.use_ws = false;
  } else if (parsed_uri->scheme == "mqtts") {
    params.use_tls = true;
    params.use_ws = false;
  } else if (parsed_uri->scheme == "ws") {
    params.use_tls = false;
    params.use_ws = true;
  } else if (parsed_uri->scheme == "wss") {
    params.use_tls = true;
    params.use_ws = true;
  } else {
    LOG("MqttWrapper", warning)
        << "Invalid MQTT connection scheme '" << parsed_uri->scheme << "'";
    return boost::none;
  }
  if (parsed_uri->authority.userinfo) {
    std::regex re("^([^:]*)(:(.*))?$");
    std::smatch match;
    if (!std::regex_match(*parsed_uri->authority.userinfo, match, re)) {
      LOG("MqttWrapper", warning)
          << "Invalid MQTT connection URI, unable to parse userinfo";
      return boost::none;
    }
    params.username = URIUnescape(std::string(match[1].first, match[1].second));
    if (match[3].matched) {
      params.password =
          URIUnescape(std::string(match[3].first, match[3].second));
    }
  }
  params.hostname = parsed_uri->authority.host;
  params.port = parsed_uri->authority.port;

  if (parsed_uri->path != "" && parsed_uri->path != "/") {
    LOG("MqttWrapper", warning) << "Invalid MQTT connection URI, path "
                                   "(anything after /) not supported.";
    return boost::none;
  }
  if (parsed_uri->query) {
    std::regex re("^clientid=([^=]*)$");
    std::smatch match;
    if (!std::regex_match(*parsed_uri->query, match, re)) {
      LOG("MqttWrapper", warning) << "Invalid MQTT connection URI, only query "
                                     "in the form of ?clientid=... supported.";
      return boost::none;
    }
    params.client_id =
        URIUnescape(std::string(match[1].first, match[1].second));
  }
  if (parsed_uri->fragment) {
    LOG("MqttWrapper", warning)
        << "Invalid MQTT connection URI, fragment (#) not supported.";
    return boost::none;
  }
  return std::move(params);
}

std::shared_ptr<MqttWrapper> MqttWrapper::FromParameters(
    boost::asio::io_service& io_service,
    MqttWrapper::Parameters params,
    std::string mqtt_prefix_write) {
  params.port = params.port ? *params.port : "1883";
  params.client_id = params.client_id ? *params.client_id
         : "AqaraHub-"+mqtt_prefix_write+"-"+boost::lexical_cast<std::string>(rand());

  LOG("MqttWrapper", info)
        << "Params: " << params;

  if (params.use_ws) {
#if defined(MQTT_USE_WS)
    if (params.use_tls) {
      return CreateMqttWrapperImpl(
          [](boost::asio::io_service& io_service, const std::string& host,
             const std::string& port, const std::string& client_id,
             const boost::optional<std::string>& username,
             const boost::optional<std::string>& password) {
            auto client = mqtt::make_tls_client_ws(io_service, host, port);
            client->set_client_id(client_id);
            if (username) client->set_user_name(*username);
            if (password) client->set_password(*password);
            return client;
          },
          io_service, params.hostname, *params.port,
          *params.client_id, params.username,
          params.password);
    } else {
      return CreateMqttWrapperImpl(
          [](boost::asio::io_service& io_service, const std::string& host,
             const std::string& port, const std::string& client_id,
             const boost::optional<std::string>& username,
             const boost::optional<std::string>& password) {
            auto client = mqtt::make_client_ws(io_service, host, port);
            client->set_client_id(client_id);
            if (username) client->set_user_name(*username);
            if (password) client->set_password(*password);
            return client;
          },
          io_service, params.hostname, *params.port,
          *params.client_id, params.username,
          params.password);
    }
#else
    throw std::runtime_error("Websocket support was not built-in");
    return nullptr;
#endif
  } else {
    if (params.use_tls) {
      return CreateMqttWrapperImpl(
          [](boost::asio::io_service& io_service, const std::string& host,
             const std::string& port, const std::string& client_id,
             const boost::optional<std::string>& username,
             const boost::optional<std::string>& password) {
            auto client = mqtt::make_tls_client(io_service, host, port);
            client->set_client_id(client_id);
            if (username) client->set_user_name(*username);
            if (password) client->set_password(*password);
            return client;
          },
          io_service, params.hostname, *params.port,
          *params.client_id, params.username,
          params.password);
    } else {
      return CreateMqttWrapperImpl(
          [](boost::asio::io_service& io_service, const std::string& host,
             const std::string& port, const std::string& client_id,
             const boost::optional<std::string>& username,
             const boost::optional<std::string>& password) {
            auto client = mqtt::make_client(io_service, host, port);
            client->set_client_id(client_id);
            if (username) client->set_user_name(*username);
            if (password) client->set_password(*password);
            return client;
          },
          io_service, params.hostname, *params.port,
          *params.client_id, params.username,
          params.password);
    }
  }
}

std::shared_ptr<MqttWrapper> MqttWrapper::FromUrl(
    boost::asio::io_service& io_service, std::string url, std::string mqtt_prefix_write) {
  auto params = ParseUrl(url);
  if (!params) {
    throw std::runtime_error("MQTT URI Parse error");
  }
  return FromParameters(io_service, *params, mqtt_prefix_write);
}

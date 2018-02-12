#include "mqtt_wrapper.h"
#include <regex>
#include "mqtt_wrapper_impl.h"

std::shared_ptr<MqttWrapper> MqttWrapper::FromUrl(
    boost::asio::io_service& io_service, std::string url) {
  // TODO: Find some external URL parsing library to handle this for us
  // properly, including username and password and such.
  std::regex re("([^:]*)://(.*?)(:([^:/]+))?/?");
  std::smatch match;
  if (!std::regex_match(url, match, re)) {
    throw std::runtime_error("Malformed MQTT URL");
  }
  std::string protocol(match[1].first, match[1].second);
  std::string host(match[2].first, match[2].second);
  std::string port(match[4].first, match[4].second);
  if (protocol == "mqtt") {
    return CreateMqttWrapperImpl(
        [](boost::asio::io_service& io_service, std::string host,
           std::string port) {
          LOG("MqttWrapper", debug)
              << "Creating connection to " << host << " : " << port;
          return mqtt::make_client(io_service, host, port);
        },
        io_service, host, (port == "" ? "1883" : port));
  }
  if (protocol == "mqtts") {
    return CreateMqttWrapperImpl(
        [](boost::asio::io_service& io_service, std::string host,
           std::string port) {
          return mqtt::make_tls_client(io_service, host, port);
        },
        io_service, host, (port == "" ? "8883" : port));
  }
#if defined(MQTT_USE_WS)
  if (protocol == "ws") {
    return CreateMqttWrapperImpl(
        [](boost::asio::io_service& io_service, std::string host,
           std::string port) {
          return mqtt::make_client_ws(io_service, host, port);
        },
        io_service, host, port);
  }
  if (protocol == "wss") {
    return CreateMqttWrapperImpl(
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

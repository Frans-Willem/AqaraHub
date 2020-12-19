#ifndef _MQTT_WRAPPER_H_
#define _MQTT_WRAPPER_H_
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <mqtt/qos.hpp>
#include <set>
#include <stlab/concurrency/future.hpp>
#include <string>

class MqttWrapper {
 public:
  virtual ~MqttWrapper() = default;
  virtual stlab::future<void> Publish(
      std::string topic_name, std::string message,
      std::uint8_t qos = mqtt::qos::at_most_once, bool retain = false) = 0;
  virtual stlab::future<void> Subscribe(
      std::set<std::tuple<std::string, std::uint8_t>> topics) = 0;
  boost::signals2::signal<void(std::string topic, std::string message,
                               std::uint8_t qos, bool retain)>
      on_publish_;

  struct Parameters {
    bool use_tls;
    bool use_ws;
    std::string hostname;
    boost::optional<std::string> port;
    boost::optional<std::string> username;
    boost::optional<std::string> password;
    boost::optional<std::string> client_id;
    bool operator==(const Parameters& other) const;
  };

  static boost::optional<Parameters> ParseUrl(const std::string& url);
  static std::shared_ptr<MqttWrapper> FromUrl(
      boost::asio::io_service& io_service, std::string url, std::string mqtt_prefix_write);
  static std::shared_ptr<MqttWrapper> FromParameters(
      boost::asio::io_service& io_service, Parameters params, std::string mqtt_prefix_write);
};

std::ostream& operator<<(std::ostream& s,
                         const MqttWrapper::Parameters& params);

#endif  // _MQTT_WRAPPER_H_

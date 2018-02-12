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

  static std::shared_ptr<MqttWrapper> FromUrl(
      boost::asio::io_service& io_service, std::string url);
};

#endif  // _MQTT_WRAPPER_H_

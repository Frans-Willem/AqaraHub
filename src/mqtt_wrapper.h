#ifndef _MQTT_WRAPPER_H_
#define _MQTT_WRAPPER_H_
#include <boost/asio.hpp>
#include <mqtt_client_cpp.hpp>
#include <queue>
#include "logging.h"

class MqttWrapper {
 public:
  virtual ~MqttWrapper() = default;
  virtual stlab::future<void> Publish(
      std::string topic_name, std::string message,
      std::uint8_t qos = mqtt::qos::at_most_once, bool retain = false) = 0;
  template <typename F, typename... Args>
  static std::shared_ptr<MqttWrapper> Create(
      F f, boost::asio::io_service& io_service, Args... args);
};

template <typename C>
class MqttWrapperImpl : public MqttWrapper {
 public:
  MqttWrapperImpl(boost::asio::io_service& io_service,
                  std::shared_ptr<C> client)
      : io_service_(io_service), client_(client) {
    client_->set_client_id("AqaraHub");
    client_->set_clean_session(true);
    client_->set_connack_handler(std::bind(&MqttWrapperImpl::ConnAckHandler,
                                           this, std::placeholders::_1,
                                           std::placeholders::_2));
	client_->set_pub_res_sent_handler([](std::uint16_t packet_id) {
			LOG("MqttWrapper", debug) << "pub res sent: " << packet_id;
			});
	client_->set_puback_handler(std::bind(&MqttWrapperImpl::PublishAcknowledgedHandler, this, std::placeholders::_1));
	client_->set_pubcomp_handler(std::bind(&MqttWrapperImpl::PublishCompletedHandler, this, std::placeholders::_1));
    client_->connect(std::bind(&MqttWrapperImpl::FinishHandler, this,
                               std::placeholders::_1));
  }
  stlab::future<void> Publish(std::string topic_name, std::string message,
                              std::uint8_t qos, bool retain) override {
    auto package = stlab::package<void(std::exception_ptr)>(
        stlab::immediate_executor, [](std::exception_ptr ex) {
          if (ex) {
            std::rethrow_exception(ex);
          }
        });
    publish_queue_.push({topic_name, message, qos, retain, package.first});
    io_service_.post([this]() { this->TrySend(); });
    return package.second;
  }

 private:
  boost::asio::io_service& io_service_;
  std::shared_ptr<C> client_;
  bool connected_;
  struct PublishQueueItem {
    std::string topic_name;
    std::string message;
    std::uint8_t qos;
    bool retain;
    std::function<void(std::exception_ptr)> callback;
  };
  std::queue<PublishQueueItem> publish_queue_;
  std::map<std::uint16_t, PublishQueueItem> publish_inprogress_;

  void TrySend() {
    if (!connected_) {
      return;
    }
	while (!publish_queue_.empty()) {
		auto item = publish_queue_.front();
		publish_queue_.pop();
		auto packet_id = client_->publish(item.topic_name, item.message, item.qos, item.retain);
		if (item.qos == mqtt::qos::at_most_once) {
			// No more acknowledgements follow, so resolve the promise.
			item.callback(nullptr);
		} else {
			// Acknowledgements should follow, keep a copy of the item.
			// TODO: What should we do in case of duplicate packet_id ?
			publish_inprogress_[packet_id] = item;
		}
	}
  }

  bool ConnAckHandler(bool sp, std::uint8_t connack_return_code) {
    if (connack_return_code != mqtt::connect_return_code::accepted) {
      LOG("MqttWrapper", warning)
          << "Connection not accepted: "
          << mqtt::connect_return_code_to_str(connack_return_code);
      return false;
    }
    LOG("MqttWrapper", debug)
        << "ConnAckHandler: " << (sp ? "clean" : "not clean");
    connected_ = true;
    io_service_.post([this]() { this->TrySend(); });
    return true;
  };

  void FinishHandler(const boost::system::error_code& error) {
    LOG("MqttWrapper", debug) << "FinishHandler: " << error;
    connected_ = false;
  }

  bool PublishAcknowledgedHandler(std::uint16_t packet_id) {
	  // Indicates a packet with QoS at least once was published succesfully.
	  auto found = publish_inprogress_.find(packet_id);
	  if (found == publish_inprogress_.end()) {
		  LOG("MqttWrapper", debug) << "PublishAcknowledge: Packet ID " << packet_id << " not found";
		  return true;
	  }
	  auto item = found->second;
	  if (item.qos != mqtt::qos::at_least_once) {
		  LOG("MqttWrapper", debug) << "PublishAcknowledge: Packet ID " << packet_id << " was supposed to receive PublishComplete...";
		  return true;
	  }
	  publish_inprogress_.erase(found);
	  item.callback(nullptr);
	  return true;
  }

  bool PublishCompletedHandler(std::uint16_t packet_id) {
	  // Indicates a packet with QoS exactly once was published succesfully.
	  auto found = publish_inprogress_.find(packet_id);
	  if (found == publish_inprogress_.end()) {
		  LOG("MqttWrapper", debug) << "PublishComplete: Packet ID " << packet_id << " not found";
		  return true;
	  }
	  auto item = found->second;
	  if (item.qos != mqtt::qos::exactly_once) {
		  LOG("MqttWrapper", debug) << "PublishComplete: Packet ID " << packet_id << " was supposed to receive PublishAcknowledge...";
		  return true;
	  }
	  publish_inprogress_.erase(found);
	  item.callback(nullptr);
	  return true;
  }
};

template <typename F, typename... Args>
std::shared_ptr<MqttWrapper> MqttWrapper::Create(
    F f, boost::asio::io_service& io_service, Args... args) {
  typedef typename std::result_of<F(boost::asio::io_service&, Args...)>::type
      ReturnType;
  typedef typename ReturnType::element_type ClientType;
  return std::make_shared<MqttWrapperImpl<ClientType>>(io_service,
                                                       f(io_service, args...));
}
#endif  // _MQTT_WRAPPER_H_

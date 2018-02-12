#ifndef _MQTT_WRAPPER_IMPL_H_
#define _MQTT_WRAPPER_IMPL_H_
#include <mqtt_client_cpp.hpp>
#include <queue>
#include <set>
#include <stlab/concurrency/serial_queue.hpp>
#include <stlab/concurrency/utility.hpp>
#include "asio_executor.h"
#include "logging.h"
#include "mqtt_wrapper.h"

template <typename C>
class MqttWrapperImpl
    : public MqttWrapper,
      public std::enable_shared_from_this<MqttWrapperImpl<C>> {
 public:
  MqttWrapperImpl(boost::asio::io_service& io_service,
                  std::shared_ptr<C> client)
      : mutex_queue_(AsioExecutor(io_service)),
        io_service_(io_service),
        client_(client) {
    client_->set_client_id("AqaraHub");
    client_->set_clean_session(true);
  }
  void PostConstructor() {
    std::weak_ptr<MqttWrapperImpl<C>> weak_this(this->shared_from_this());
    client_->set_connack_handler([weak_this](auto a, auto b) {
      if (auto _this = weak_this.lock()) {
        _this
            ->mutex_queue_(
                [weak_this](auto a, auto b) {
                  if (auto _this = weak_this.lock()) {
                    _this->ConnAckHandler(a, b);
                  }
                },
                a, b)
            .detach();
        return true;
      }
      return false;
    });
    client_->set_puback_handler([weak_this](auto a) {
      if (auto _this = weak_this.lock()) {
        _this
            ->mutex_queue_(
                [weak_this](auto a) {
                  if (auto _this = weak_this.lock()) {
                    _this->PublishAcknowledgedHandler(a);
                  }
                },
                a)
            .detach();
        return true;
      }
      return false;
    });
    client_->set_pubcomp_handler([weak_this](auto a) {
      if (auto _this = weak_this.lock()) {
        _this
            ->mutex_queue_(
                [weak_this](auto a) {
                  if (auto _this = weak_this.lock()) {
                    _this->PublishCompletedHandler(a);
                  }
                },
                a)
            .detach();
        return true;
      }
      return false;
    });
    client_->connect([weak_this](auto a) {
      if (auto _this = weak_this.lock()) {
        _this
            ->mutex_queue_(
                [weak_this](auto a) {
                  if (auto _this = weak_this.lock()) {
                    _this->FinishHandler(a);
                  }
                },
                a)
            .detach();
        return true;
      }
      return false;
    });
    client_->set_publish_handler([weak_this](auto a, auto b, auto c, auto d) {
      if (auto _this = weak_this.lock()) {
        _this
            ->mutex_queue_(
                [weak_this](auto a, auto b, auto c, auto d) {
                  if (auto _this = weak_this.lock()) {
                    _this->PublishHandler(a, b, c, d);
                  }
                },
                a, b, c, d)
            .detach();
        return true;
      }
      return false;
    });
  }
  stlab::future<void> Publish(std::string topic_name, std::string message,
                              std::uint8_t qos, bool retain) override {
    auto _this = this->shared_from_this();
    auto package = stlab::package<void(std::exception_ptr)>(
        AsioExecutor(io_service_), [](std::exception_ptr ex) {
          if (ex) {
            std::rethrow_exception(ex);
          }
        });
    PublishQueueItem item{topic_name, message, qos, retain, package.first};
    return mutex_queue_(
        [_this](PublishQueueItem item) { _this->SafePublish(item); },
        std::move(item));
    return package.second;
  }
  stlab::future<void> Subscribe(
      std::set<std::tuple<std::string, std::uint8_t>> topics) override {
    if (topics.empty()) {
      return stlab::make_ready_future(AsioExecutor(io_service_));
    }
    auto _this = this->shared_from_this();
    return mutex_queue_([_this](auto x) { _this->SafeSubscribe(x); },
                        std::move(topics));
  }

 private:
  stlab::serial_queue_t mutex_queue_;
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
  std::set<std::tuple<std::string, std::uint8_t>> subscriptions_;

  void SafePublish(PublishQueueItem item) {
    if (!connected_) {
      publish_queue_.push(std::move(item));
      return;
    }
    if (item.qos == mqtt::qos::at_most_once) {
      auto callback = item.callback;
      client_->acquired_async_publish(
          0, item.topic_name, item.message, item.qos, item.retain,
          [callback](const boost::system::error_code& error) {
            if (error) {
              callback(std::make_exception_ptr(error));
            } else {
              callback(nullptr);
            }
          });
      return;
    }
    auto packet_id = client_->acquire_unique_packet_id();
    auto _this = this->shared_from_this();
    publish_inprogress_[packet_id] = item;
    client_->acquired_async_publish(
        packet_id, item.topic_name, item.message, item.qos, item.retain,
        [packet_id, _this](const boost::system::error_code& error) {
          _this
              ->mutex_queue_(
                  [_this, packet_id](const boost::system::error_code& error) {
                    _this->AsyncPublishCallback(packet_id, error);
                  },
                  error)
              .detach();
        });
  }

  void SafeSubscribe(
      std::set<std::tuple<std::string, std::uint8_t>> subscriptions) {
    subscriptions_.insert(subscriptions.begin(), subscriptions.end());
    if (connected_) {
      LOG("MqttWrapper", debug) << "Sending async subscribe directly";
      client_->async_subscribe(
          std::vector<std::tuple<std::string, std::uint8_t>>(
              subscriptions.begin(), subscriptions.end()));
    }
  }

  void ConnAckHandler(bool sp, std::uint8_t connack_return_code) {
    if (connack_return_code != mqtt::connect_return_code::accepted) {
      LOG("MqttWrapper", warning)
          << "Connection not accepted: "
          << mqtt::connect_return_code_to_str(connack_return_code);
      return;
    }
    LOG("MqttWrapper", debug) << "ConnAckHandler: clean=" << sp;
    connected_ = true;
    if (!subscriptions_.empty()) {
      LOG("MqttWrapper", debug) << "Sending async subscribe after connect";
      client_->async_subscribe(
          std::vector<std::tuple<std::string, std::uint8_t>>(
              subscriptions_.begin(), subscriptions_.end()));
    }
    std::queue<PublishQueueItem> queue_copy;
    std::swap(queue_copy, publish_queue_);
    while (!queue_copy.empty()) {
      auto item = queue_copy.front();
      queue_copy.pop();
      SafePublish(item);
    }
  }

  void FinishHandler(const boost::system::error_code& error) {
    LOG("MqttWrapper", debug) << "FinishHandler: " << error;
    connected_ = false;
    // TODO: Probably trigger reconnect here? Maybe with a small timer ?
  }

  void AsyncPublishCallback(std::uint16_t packet_id,
                            const boost::system::error_code& error) {
    if (error) {
      auto found = publish_inprogress_.find(packet_id);
      if (found == publish_inprogress_.end()) {
        LOG("MqttWrapper", debug)
            << "AsyncPublishCallback: Packet ID " << packet_id << " not found";
        return;
      }
      auto item = found->second;
      publish_inprogress_.erase(found);
      item.callback(std::make_exception_ptr(error));
      return;
    }
  }

  void PublishAcknowledgedHandler(std::uint16_t packet_id) {
    // Indicates a packet with QoS at least once was published succesfully.
    auto found = publish_inprogress_.find(packet_id);
    if (found == publish_inprogress_.end()) {
      LOG("MqttWrapper", debug)
          << "PublishAcknowledge: Packet ID " << packet_id << " not found";
      return;
    }
    auto item = found->second;
    if (item.qos != mqtt::qos::at_least_once) {
      LOG("MqttWrapper", debug)
          << "PublishAcknowledge: Packet ID " << packet_id
          << " was supposed to receive PublishComplete...";
      return;
    }
    publish_inprogress_.erase(found);
    item.callback(nullptr);
    return;
  }

  void PublishCompletedHandler(std::uint16_t packet_id) {
    // Indicates a packet with QoS exactly once was published succesfully.
    auto found = publish_inprogress_.find(packet_id);
    if (found == publish_inprogress_.end()) {
      LOG("MqttWrapper", debug)
          << "PublishComplete: Packet ID " << packet_id << " not found";
      return;
    }
    auto item = found->second;
    if (item.qos != mqtt::qos::exactly_once) {
      LOG("MqttWrapper", debug)
          << "PublishComplete: Packet ID " << packet_id
          << " was supposed to receive PublishAcknowledge...";
      return;
    }
    publish_inprogress_.erase(found);
    item.callback(nullptr);
    return;
  }

  void PublishHandler(std::uint8_t fixed_header,
                      boost::optional<std::uint16_t> packet_id,
                      std::string topic_name, std::string message) {
    this->on_publish_(topic_name, message, mqtt::publish::get_qos(fixed_header),
                      mqtt::publish::is_retain(fixed_header));
  }
};
template <typename F, typename... Args>
static std::shared_ptr<MqttWrapper> CreateMqttWrapperImpl(
    F f, boost::asio::io_service& io_service, Args... args) {
  typedef typename std::result_of<F(boost::asio::io_service&, Args...)>::type
      ReturnType;
  typedef typename ReturnType::element_type ClientType;
  auto wrapper = std::make_shared<MqttWrapperImpl<ClientType>>(
      io_service, f(io_service, args...));
  wrapper->PostConstructor();
  return wrapper;
}

#endif  // _MQTT_WRAPPER_IMPL_H_

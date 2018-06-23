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
#include "weak_bind.h"

template <typename C>
class MqttWrapperImpl
    : public MqttWrapper,
      public std::enable_shared_from_this<MqttWrapperImpl<C>> {
 public:
  MqttWrapperImpl(boost::asio::io_service& io_service,
                  std::shared_ptr<C> client)
      : mutex_queue_(AsioExecutor(io_service)),
        mutex_queue_executor_(mutex_queue_.executor()),
        io_service_(io_service),
        client_(client),
        reconnect_timer_(io_service) {
    client_->set_client_id("AqaraHub");
    client_->set_clean_session(true);
  }
  void PostConstructor() {
    std::shared_ptr<MqttWrapperImpl<C>> self_ptr(this->shared_from_this());
    std::weak_ptr<MqttWrapperImpl<C>> weak_this(self_ptr);
    std::shared_ptr<stlab::executor_t> executor_ptr(self_ptr,
                                                    &mutex_queue_executor_);
    client_->set_connack_handler(WeakExecutorBind(
        executor_ptr, &MqttWrapperImpl<C>::ConnAckHandler, self_ptr,
        std::placeholders::_1, std::placeholders::_2));
    client_->set_puback_handler(WeakExecutorBind(
        executor_ptr, &MqttWrapperImpl<C>::PublishAcknowledgedHandler, self_ptr,
        std::placeholders::_1));
    client_->set_pubcomp_handler(WeakExecutorBind(
        executor_ptr, &MqttWrapperImpl<C>::PublishCompletedHandler, self_ptr,
        std::placeholders::_1));
    client_->set_publish_handler(
        WeakExecutorBind(executor_ptr, &MqttWrapperImpl<C>::PublishHandler,
                         self_ptr, std::placeholders::_1, std::placeholders::_2,
                         std::placeholders::_3, std::placeholders::_4));
    client_->set_error_handler(
        WeakExecutorBind(executor_ptr, &MqttWrapperImpl<C>::ErrorHandler,
                         self_ptr, std::placeholders::_1));
    LOG("MqttWrapper", info) << "Connecting...";
    client_->connect(WeakExecutorBind(executor_ptr,
                                      &MqttWrapperImpl<C>::FinishHandler,
                                      self_ptr, std::placeholders::_1));
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
  stlab::executor_t mutex_queue_executor_;
  boost::asio::io_service& io_service_;
  std::shared_ptr<C> client_;
  enum class ConnectionState { Disconnected, Connecting, Connected };
  ConnectionState state_;
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
  boost::asio::deadline_timer reconnect_timer_;

  void SafePublish(PublishQueueItem item) {
    if (state_ != ConnectionState::Connected) {
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
    if (state_ == ConnectionState::Connected) {
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
    LOG("MqttWrapper", debug) << "Connected, clean=" << sp;
    state_ = ConnectionState::Connected;
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

  void ErrorHandler(const boost::system::error_code& error) {
    std::weak_ptr<MqttWrapperImpl<C>> weak_this(this->shared_from_this());
    LOG("MqttWrapper", debug) << "ErrorHandler: " << error;
    state_ = ConnectionState::Disconnected;
    StartReconnectTimer();
  }

  void FinishHandler(const boost::system::error_code& error) {
    LOG("MqttWrapper", debug) << "FinishHandler: " << error;
    state_ = ConnectionState::Disconnected;
    StartReconnectTimer();
  }

  void StartReconnectTimer() {
    boost::system::error_code ignore;
    reconnect_timer_.cancel(ignore);
    LOG("MqttWrapper", debug) << "Starting reconnect timer...";
    reconnect_timer_.expires_from_now(boost::posix_time::seconds(5));
    std::shared_ptr<MqttWrapperImpl<C>> self_ptr(this->shared_from_this());
    std::shared_ptr<stlab::executor_t> executor_ptr(self_ptr,
                                                    &mutex_queue_executor_);
    reconnect_timer_.async_wait(
        WeakExecutorBind(executor_ptr, &MqttWrapperImpl<C>::OnReconnectTimer,
                         self_ptr, std::placeholders::_1));
  }

  void OnReconnectTimer(const boost::system::error_code& ec) {
    if (ec == boost::asio::error::operation_aborted) {
      // Assume cancelled
      return;
    }
    if (ec) {
      LOG("MqttWrapper", error) << "OnReconnectTimer had error: " << ec;
      return;
    }
    if (state_ != ConnectionState::Disconnected) {
      LOG("MqttWrapper", error) << "OnReconnectTimer: already connected :/";
      return;
    }
    LOG("MqttWrapper", info) << "Reconnecting to MQTT server...";
    std::shared_ptr<MqttWrapperImpl<C>> self_ptr(this->shared_from_this());
    std::shared_ptr<stlab::executor_t> executor_ptr(self_ptr,
                                                    &mutex_queue_executor_);
    client_->connect(WeakExecutorBind(executor_ptr,
                                      &MqttWrapperImpl<C>::FinishHandler,
                                      self_ptr, std::placeholders::_1));
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

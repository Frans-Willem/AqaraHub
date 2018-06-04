#ifndef _ZNP_PORT_H_
#define _ZNP_PORT_H_
#include <boost/asio.hpp>
#include <boost/signals2/signal.hpp>
#include <queue>
#include <stlab/concurrency/future.hpp>
#include <vector>
#include "znp/znp_raw_interface.h"

namespace znp {
class ZnpPort : public ZnpRawInterface {
 public:
  ZnpPort(boost::asio::io_service& io_service, const std::string& port);
  ~ZnpPort() = default;
  void SendFrame(ZnpCommandType type, ZnpCommand command,
                 const std::vector<uint8_t>& payload) override;

  boost::signals2::signal<void(ZnpCommandType, ZnpCommand,
                               const std::vector<uint8_t>&)>
      on_sent_;

  boost::signals2::signal<void(const boost::system::error_code&)> on_error_;

 private:
  boost::asio::serial_port port_;
  bool send_in_progress_;
  std::queue<std::vector<uint8_t>> send_queue_;

  void TrySend();
  void SendHandler(const boost::system::error_code& error,
                   std::size_t bytes_transferred);
  void StartReceive();
  void StartOfFrameHandler(std::shared_ptr<uint8_t> marker,
                           const boost::system::error_code& error,
                           std::size_t bytes_transferred);
  void FrameLengthHandler(std::shared_ptr<uint8_t> length,
                          const boost::system::error_code& error,
                          std::size_t bytes_transferred);
  void FrameHandler(std::shared_ptr<std::vector<uint8_t>> frame,
                    const boost::system::error_code& error,
                    std::size_t bytes_transferred);
};
}  // namespace znp
#endif  // _ZNP_PORT_H_

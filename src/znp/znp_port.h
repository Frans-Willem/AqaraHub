#ifndef _ZNP_PORT_H_
#define _ZNP_PORT_H_
#include <boost/asio.hpp>
#include <boost/signals2/signal.hpp>
#include <queue>
#include <stlab/concurrency/future.hpp>
#include <vector>
#include "znp.h"

namespace znp {
class ZnpPort {
 public:
  ZnpPort(boost::asio::io_service& io_service, const std::string& port);
  ~ZnpPort() = default;
  void SendFrame(ZnpCommandType type, ZnpSubsystem subsystem,
                 uint8_t command, boost::asio::const_buffer payload);

  boost::signals2::signal<void(ZnpCommandType, ZnpSubsystem, uint8_t,
                               boost::asio::const_buffer)>
      on_frame_;

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

#include "znp_port.h"
#include <boost/log/utility/manipulators/dump.hpp>
#include <iomanip>
#include <iostream>
#include "logging.h"

namespace znp {
ZnpPort::ZnpPort(boost::asio::io_service& io_service, const std::string& port)
    : port_(io_service, port), send_in_progress_(false), send_queue_() {
  port_.set_option(boost::asio::serial_port_base::baud_rate(115200));
  port_.set_option(boost::asio::serial_port_base::character_size(8));
  port_.set_option(boost::asio::serial_port_base::stop_bits(
      boost::asio::serial_port_base::stop_bits::one));
  port_.set_option(boost::asio::serial_port_base::parity(
      boost::asio::serial_port_base::parity::none));
  port_.set_option(boost::asio::serial_port_base::flow_control(
      boost::asio::serial_port_base::flow_control::none));
  StartReceive();
}

void ZnpPort::SendFrame(ZnpCommandType type, ZnpSubsystem subsystem,
                        uint8_t command, const std::vector<uint8_t>& payload) {
  if (payload.size() > 255) {
    throw std::runtime_error(
        "ZNP Command Payload size should not exceed 255 bytes");
  }
  // SOF + Length (1 byte) + Command (2 byte) + payload + Checksum
  std::vector<uint8_t> buffer(1 + 1 + 2 + payload.size() + 1);
  buffer[0] = 0xFE;
  buffer[1] = payload.size();
  buffer[2] = (((unsigned int)type) << 4) | (((unsigned int)subsystem) & 0xF);
  buffer[3] = command;
  std::copy(payload.begin(), payload.end(), buffer.begin() + 4);

  uint8_t crc = 0;
  for (std::size_t i = 1; i < buffer.size() - 1; i++) {
    crc ^= buffer[i];
  }
  buffer[buffer.size() - 1] = crc;
  send_queue_.emplace(std::move(buffer));
  TrySend();
  on_sent_(type, subsystem, command, payload);
}

void ZnpPort::TrySend() {
  if (send_in_progress_) {
    return;
  }
  if (send_queue_.empty()) {
    return;
  }
  send_in_progress_ = true;
  std::vector<uint8_t>& first_in_queue(send_queue_.front());
  std::array<boost::asio::const_buffer, 1> buffers;
  buffers[0] =
      boost::asio::const_buffer(first_in_queue.data(), first_in_queue.size());
  boost::asio::async_write(
      port_, buffers,
      std::bind(&ZnpPort::SendHandler, this, std::placeholders::_1,
                std::placeholders::_2));
}

void ZnpPort::SendHandler(const boost::system::error_code& error,
                          std::size_t bytes_transferred) {
  if (error) {
    LOG("ZnpPort", critical) << "async_write failed";
    return;
  }
  if (!send_queue_.empty()) {
    send_queue_.pop();
  }
  send_in_progress_ = false;
  TrySend();
}

void ZnpPort::StartReceive() {
  auto target = std::make_shared<uint8_t>(0);
  boost::asio::async_read(
      port_, boost::asio::buffer(target.get(), sizeof(uint8_t)),
      std::bind(&ZnpPort::StartOfFrameHandler, this, target,
                std::placeholders::_1, std::placeholders::_2));
}

void ZnpPort::StartOfFrameHandler(std::shared_ptr<uint8_t> marker,
                                  const boost::system::error_code& error,
                                  std::size_t bytes_transferred) {
  if (error) {
    LOG("ZnpPort", critical) << "Error while reading SOF";
    return;
  }
  if (*marker != 0xFE) {
    LOG("ZnpPort", trace) << "No SOF marker, dropping data";
    return;
  }
  boost::asio::async_read(
      port_, boost::asio::buffer(marker.get(), sizeof(uint8_t)),
      std::bind(&ZnpPort::FrameLengthHandler, this, marker,
                std::placeholders::_1, std::placeholders::_2));
}

void ZnpPort::FrameLengthHandler(std::shared_ptr<uint8_t> length,
                                 const boost::system::error_code& error,
                                 std::size_t bytes_transferred) {
  if (error) {
    LOG("ZnpPort", critical) << "Error while reading length";
    return;
  }
  auto buffer =
      std::make_shared<std::vector<uint8_t>>(2 + ((std::size_t)*length) + 1);
  boost::asio::async_read(
      port_, boost::asio::buffer(buffer->data(), buffer->size()),
      std::bind(&ZnpPort::FrameHandler, this, buffer, std::placeholders::_1,
                std::placeholders::_2));
}

void ZnpPort::FrameHandler(std::shared_ptr<std::vector<uint8_t>> frame,
                           const boost::system::error_code& error,
                           std::size_t bytes_transferred) {
  if (error) {
    LOG("ZnpPort", critical) << "Error while reading frame";
    return;
  }
  StartReceive();
  uint8_t crc = 0;
  crc ^= frame->size() - 3;  // Length byte
  for (std::size_t i = 0; i < frame->size() - 1; i++) {
    crc ^= (*frame)[i];
  }
  if (crc != (*frame)[frame->size() - 1]) {
    LOG("ZnpPort", warning) << "CRC does not match, dropping frame";
    return;
  }
  ZnpCommandType type = (ZnpCommandType)((*frame)[0] >> 4);
  ZnpSubsystem subsystem = (ZnpSubsystem)((*frame)[0] & 0xF);
  unsigned int command = (*frame)[1];
  on_frame_(type, subsystem, command,
            std::vector<uint8_t>(frame->begin() + 2, frame->end() - 1));
}
}  // namespace znp

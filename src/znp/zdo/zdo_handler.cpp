#include "znp/zdo/zdo_handler.h"
#include <stlab/concurrency/immediate_executor.hpp>
#include <stlab/concurrency/utility.hpp>
#include "logging.h"
#include "znp/encoding.h"

namespace znp {
namespace zdo {
ZdoHandler::ZdoHandler(
    std::shared_ptr<ZnpPort> port, std::shared_ptr<ZnpSreqHandler> sreq_handler,
    std::shared_ptr<simpleapi::SimpleAPIHandler> simpleapi_handler)
    : port_(port),
      sreq_handler_(sreq_handler),
      simpleapi_handler_(simpleapi_handler) {}

stlab::future<StartupFromAppResponse> ZdoHandler::StartupFromApp(
    uint16_t start_delay) {
  return sreq_handler_
      ->SReq(ZdoCommand::STARTUP_FROM_APP, znp::Encode(start_delay))
      .then(znp::Decode<StartupFromAppResponse>);
}

stlab::future<DeviceState> ZdoHandler::WaitForState(
    std::set<DeviceState> end_states, std::set<DeviceState> allowed_states) {
  auto port = port_;
  return simpleapi_handler_->GetDeviceState().then(
      [end_states, allowed_states, port](DeviceState state) {
        if (end_states.count(state) != 0) {
          LOG("WaitForState", debug) << "Immediately reached end state";
          return stlab::make_ready_future(state, stlab::immediate_executor);
        }
        if (allowed_states.count(state) == 0) {
          LOG("WaitForState", debug) << "Immediately reached non-allowed state";
          throw std::runtime_error("Invalid state reached");
        }
        LOG("WaitForState", debug) << "Subscribing to messages";
        stlab::future<DeviceState> future;
        stlab::packaged_task<std::exception_ptr, DeviceState> promise;
        std::tie(promise, future) =
            stlab::package<DeviceState(std::exception_ptr, DeviceState)>(
                stlab::immediate_executor,
                [](std::exception_ptr exc, DeviceState state) {
                  if (exc) {
                    std::rethrow_exception(exc);
                  }
                  return state;
                });
        port->on_frame_.connect_extended(
            [promise, end_states, allowed_states](
                const boost::signals2::connection& connection,
                ZnpCommandType cmdtype, ZnpCommand command,
                const std::vector<uint8_t>& payload) {
              if (cmdtype != ZnpCommandType::AREQ ||
                  command != ZdoCommand::STATE_CHANGE_IND) {
                return;
              }
              LOG("WaitForState", debug) << "Got state message";
              DeviceState state = DeviceState::HOLD;
              try {
                state = znp::Decode<DeviceState>(payload);
                if (end_states.count(state) != 0) {
                  connection.disconnect();
                  promise(nullptr, state);
                  return;
                }
                if (allowed_states.count(state) == 0) {
                  throw std::runtime_error("Invalid state reached");
                }
              } catch (...) {
                connection.disconnect();
                promise(std::current_exception(), state);
                return;
              }
            });

        return future;
      });
}

stlab::future<uint16_t> ZdoHandler::PermitJoin(AddrMode addr_mode,
                                               uint16_t dst_address,
                                               uint8_t duration,
                                               uint8_t tc_significance) {
  auto sreq_handler = sreq_handler_;
  return sreq_handler
      ->SReqStatus(ZdoCommand::MGMT_PERMIT_JOIN_REQ,
                   znp::EncodeT<AddrMode, uint16_t, uint8_t, uint8_t>(
                       addr_mode, dst_address, duration, tc_significance))
      .then(znp::Decode<void>)
      .then([sreq_handler]() {
        return sreq_handler->WaitForAReq(ZdoCommand::MGMT_PERMIT_JOIN_RSP);
      })
      .then(znp::DecodeT<uint16_t, ZnpStatus>)
      .then([](std::tuple<uint16_t, ZnpStatus> retval) {
        if (std::get<1>(retval) != ZnpStatus::Success) {
          throw std::runtime_error("PermitJoin returned non-success status");
        }
        return std::get<0>(retval);
      });
}
}  // namespace zdo
}  // namespace znp

#include "znp/znp_api.h"
#include <stlab/concurrency/immediate_executor.hpp>
#include <stlab/concurrency/utility.hpp>
#include "logging.h"
#include "znp/encoding.h"

namespace znp {
ZnpApi::ZnpApi(std::shared_ptr<ZnpRawInterface> interface)
    : raw_(std::move(interface)),
      on_frame_connection_(raw_->on_frame_.connect(
          std::bind(&ZnpApi::OnFrame, this, std::placeholders::_1,
                    std::placeholders::_2, std::placeholders::_3))) {
  AddSimpleEventHandler(ZnpCommandType::AREQ, SysCommand::RESET_IND,
                        sys_on_reset_, false);
  AddSimpleEventHandler(ZnpCommandType::AREQ, ZdoCommand::STATE_CHANGE_IND,
                        zdo_on_state_change_, false);
  // NOTE: INCOMING_MSG sometimes has 3 extra trailing bytes, so allow a partial
  // decoding.
  AddSimpleEventHandler(ZnpCommandType::AREQ, AfCommand::INCOMING_MSG,
                        af_on_incoming_msg_, true);
}

stlab::future<ResetInfo> ZnpApi::SysReset(bool soft_reset) {
  auto retval = WaitFor(ZnpCommandType::AREQ, SysCommand::RESET_IND)
                    .then(znp::Decode<ResetInfo>);
  raw_->SendFrame(ZnpCommandType::AREQ, SysCommand::RESET,
                  Encode<bool>(soft_reset));
  return retval;
}

stlab::future<Capability> ZnpApi::SysPing() {
  return RawSReq(SysCommand::PING, znp::Encode()).then(znp::Decode<Capability>);
}

stlab::future<void> ZnpApi::AfRegister(uint8_t endpoint, uint16_t profile_id,
                                       uint16_t device_id, uint8_t version,
                                       Latency latency,
                                       std::vector<uint16_t> input_clusters,
                                       std::vector<uint16_t> output_clusters) {
  return RawSReq(AfCommand::REGISTER,
                 znp::EncodeT(endpoint, profile_id, device_id, version, latency,
                              input_clusters, output_clusters))
      .then(CheckOnlyStatus);
}

stlab::future<StartupFromAppResponse> ZnpApi::ZdoStartupFromApp(
    uint16_t start_delay_ms) {
  return RawSReq(ZdoCommand::STARTUP_FROM_APP, znp::Encode(start_delay_ms))
      .then(&znp::Decode<StartupFromAppResponse>);
}
stlab::future<uint16_t> ZnpApi::ZdoMgmtPermitJoin(AddrMode addr_mode,
                                                  uint16_t dst_address,
                                                  uint8_t duration,
                                                  uint8_t tc_significance) {
  return WaitAfter(RawSReq(ZdoCommand::MGMT_PERMIT_JOIN_REQ,
                           znp::EncodeT(addr_mode, dst_address, duration,
                                        tc_significance))
                       .then(CheckOnlyStatus),
                   ZnpCommandType::AREQ, ZdoCommand::MGMT_PERMIT_JOIN_RSP)
      .then(znp::DecodeT<uint16_t, ZnpStatus>)
      .then([](std::tuple<uint16_t, ZnpStatus> retval) {
        if (std::get<1>(retval) != ZnpStatus::Success) {
          throw std::runtime_error("PermitJoin returned non-success status");
        }
        return std::get<0>(retval);
      });
}

stlab::future<std::vector<uint8_t>> ZnpApi::SapiReadConfigurationRaw(
    ConfigurationOption option) {
  return RawSReq(SapiCommand::READ_CONFIGURATION, znp::Encode(option))
      .then(&CheckStatus)
      .then(&znp::Decode<std::tuple<ConfigurationOption, std::vector<uint8_t>>>)
      .then([option](const std::tuple<ConfigurationOption,
                                      std::vector<uint8_t>>& retval) {
        if (option != std::get<0>(retval)) {
          throw std::runtime_error("Read configuration returned wrong option");
        }
        return std::get<1>(retval);
      });
}

stlab::future<void> ZnpApi::SapiWriteConfigurationRaw(
    ConfigurationOption option, const std::vector<uint8_t>& value) {
  return RawSReq(SapiCommand::WRITE_CONFIGURATION, znp::EncodeT(option, value))
      .then(&CheckOnlyStatus);
}

stlab::future<std::vector<uint8_t>> ZnpApi::SapiGetDeviceInfoRaw(
    DeviceInfo info) {
  return RawSReq(SapiCommand::GET_DEVICE_INFO, znp::EncodeT(info))
      .then([info](const std::vector<uint8_t>& retval) {
        if (retval.size() < 1) {
          throw std::runtime_error(
              "Expected more data from GetDeviceInfo response");
        }
        if ((DeviceInfo)retval[0] != info) {
          throw std::runtime_error("Wrong DeviceInfo returned");
        }
        return std::vector<uint8_t>(retval.begin() + 1, retval.end());
      });
}

void ZnpApi::OnFrame(ZnpCommandType type, ZnpCommand command,
                     const std::vector<uint8_t>& payload) {
  // Handle WaitFor queue
  std::queue<QueueCallback>& callback_queue(
      queue_[std::make_pair(type, command)]);
  if (!callback_queue.empty()) {
    QueueCallback callback = std::move(callback_queue.front());
    callback_queue.pop();
    callback(nullptr, payload);
  }
  // Handle normal registered handlers
  std::list<DefaultHandler>& handler_list(
      handlers_[std::make_pair(type, command)]);
  for (auto& handler : handler_list) {
    handler(payload);
  }
}

stlab::future<DeviceState> ZnpApi::WaitForState(
    std::set<DeviceState> end_states, std::set<DeviceState> allowed_states) {
  // TODO: Fix lifetime issues here with passing 'this'. Maybe shared_from_this
  // ?
  return SapiGetDeviceInfo<DeviceInfo::DeviceState>().then(
      [end_states, allowed_states, this](DeviceState state) {
        if (end_states.count(state) != 0) {
          LOG("WaitForState", debug) << "Immediately reached end state";
          return stlab::make_ready_future(state, stlab::immediate_executor);
        }
        if (allowed_states.count(state) == 0) {
          LOG("WaitForState", debug) << "Immediately reached non-allowed state";
          throw std::runtime_error("Invalid state reached");
        }
        LOG("WaitForState", debug) << "Subscribing to on_state_change_ event";
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
        this->zdo_on_state_change_.connect_extended(
            [promise, end_states, allowed_states](
                const boost::signals2::connection& connection,
                DeviceState state) {
              LOG("WaitForState", debug) << "Got on_state_change_";
              if (end_states.count(state) != 0) {
                connection.disconnect();
                promise(nullptr, state);
                return;
              }
              if (allowed_states.count(state) == 0) {
                connection.disconnect();
                promise(std::make_exception_ptr(
                            std::runtime_error("Non-allowed state reached")),
                        DeviceState());
                return;
              }
            });

        return future;
      });
}

stlab::future<std::vector<uint8_t>> ZnpApi::WaitFor(ZnpCommandType type,
                                                    ZnpCommand command) {
  auto package = stlab::package<std::vector<uint8_t>(std::exception_ptr,
                                                     std::vector<uint8_t>)>(
      stlab::immediate_executor,
      [](std::exception_ptr exc, std::vector<uint8_t> retval) {
        if (exc != nullptr) {
          std::rethrow_exception(exc);
        }
        return retval;
      });
  queue_[std::make_pair(type, command)].push(std::move(package.first));
  return package.second;
}

stlab::future<std::vector<uint8_t>> ZnpApi::WaitAfter(
    stlab::future<void> first_request, ZnpCommandType type,
    ZnpCommand command) {
  // TODO: Fix lifetime issues here!
  auto f = first_request.then(
      [this, type, command]() { return this->WaitFor(type, command); });
  f.detach();
  return f;
}

stlab::future<std::vector<uint8_t>> ZnpApi::RawSReq(
    ZnpCommand command, const std::vector<uint8_t>& payload) {
  auto retval = WaitFor(ZnpCommandType::SRSP, command);
  raw_->SendFrame(ZnpCommandType::SREQ, command, payload);
  return retval;
}

std::vector<uint8_t> ZnpApi::CheckStatus(const std::vector<uint8_t>& response) {
  if (response.size() < 1) {
    throw std::runtime_error("Empty response received");
  }
  if (response[0] != (uint8_t)ZnpStatus::Success) {
    // TODO: Parse and throw proper error!
    throw std::runtime_error("ZNP Status was not success");
  }
  return std::vector<uint8_t>(response.begin() + 1, response.end());
}

void ZnpApi::CheckOnlyStatus(const std::vector<uint8_t>& response) {
  if (CheckStatus(response).size() != 0) {
    throw std::runtime_error("Empty response after status expected");
  }
}

}  // namespace znp

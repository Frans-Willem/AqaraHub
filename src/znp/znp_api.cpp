#include "znp/znp_api.h"
#include <sstream>
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
  auto package = stlab::package<ResetInfo(ResetInfo)>(
      stlab::immediate_executor, [](ResetInfo info) { return info; });
  sys_on_reset_.connect_extended(
      [package](const boost::signals2::connection& connection, ResetInfo info) {
        connection.disconnect();
        package.first(info);
      });
  raw_->SendFrame(ZnpCommandType::AREQ, SysCommand::RESET,
                  Encode<bool>(soft_reset));
  return package.second;
}

stlab::future<Capability> ZnpApi::SysPing() {
  return RawSReq(SysCommand::PING, znp::Encode()).then(znp::Decode<Capability>);
}

stlab::future<void> ZnpApi::SysOsalNvItemInitRaw(
    NvItemId Id, uint16_t ItemLen, std::vector<uint8_t> InitData) {
  return RawSReq(SysCommand::OSAL_NV_ITEM_INIT,
                 znp::EncodeT(Id, ItemLen, InitData))
      .then(&ZnpApi::CheckOnlyStatus);
}

stlab::future<std::vector<uint8_t>> ZnpApi::SysOsalNvReadRaw(NvItemId Id,
                                                             uint8_t Offset) {
  return RawSReq(SysCommand::OSAL_NV_READ, znp::EncodeT(Id, Offset))
      .then(&ZnpApi::CheckStatus)
      .then(&znp::Decode<std::vector<uint8_t>>);
}

stlab::future<void> ZnpApi::SysOsalNvWriteRaw(NvItemId Id, uint8_t Offset,
                                              std::vector<uint8_t> Value) {
  return RawSReq(SysCommand::OSAL_NV_WRITE, znp::EncodeT(Id, Offset, Value))
      .then(&ZnpApi::CheckOnlyStatus);
}

stlab::future<void> ZnpApi::SysOsalNvDelete(NvItemId Id, uint16_t ItemLen) {
  return RawSReq(SysCommand::OSAL_NV_DELETE, znp::EncodeT(Id, ItemLen))
      .then(&ZnpApi::CheckOnlyStatus);
}

stlab::future<uint16_t> ZnpApi::SysOsalNvLength(NvItemId Id) {
  return RawSReq(SysCommand::OSAL_NV_LENGTH, znp::EncodeT(Id))
      .then(&znp::Decode<uint16_t>);
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
stlab::future<ShortAddress> ZnpApi::ZdoMgmtLeave(ShortAddress DstAddr,
                                                 IEEEAddress DeviceAddr,
                                                 uint8_t remove_rejoin) {
  return WaitAfter(RawSReq(ZdoCommand::MGMT_LEAVE_REQ,
                           znp::EncodeT(DstAddr, DeviceAddr, remove_rejoin))
                       .then(&ZnpApi::CheckOnlyStatus),
                   ZnpCommandType::AREQ, ZdoCommand::MGMT_LEAVE_RSP)
      .then(&znp::DecodeT<ShortAddress, ZnpStatus>)
      .then([](std::tuple<ShortAddress, ZnpStatus> retval) {
        if (std::get<1>(retval) != ZnpStatus::Success) {
          throw std::runtime_error("MgmtLeave returned non-success status");
        }
        return std::get<0>(retval);
      });
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

stlab::future<ZdoIEEEAddressResponse> ZnpApi::ZdoIEEEAddress(
    ShortAddress address, boost::optional<uint8_t> children_index) {
  return WaitAfter(RawSReq(ZdoCommand::IEEE_ADDR_REQ,
                           znp::EncodeT<ShortAddress, bool, uint8_t>(
                               address, !!children_index,
                               children_index ? *children_index : 0))
                       .then(&ZnpApi::CheckOnlyStatus),
                   ZnpCommandType::AREQ, ZdoCommand::IEEE_ADDR_RSP)
      .then(&ZnpApi::CheckStatus)
      .then(&znp::Decode<ZdoIEEEAddressResponse>);
}

stlab::future<void> ZnpApi::ZdoRemoveLinkKey(IEEEAddress IEEEAddr) {
  return RawSReq(ZdoCommand::REMOVE_LINK_KEY, znp::Encode(IEEEAddr))
      .then(&ZnpApi::CheckOnlyStatus);
}
stlab::future<std::tuple<IEEEAddress, std::array<uint8_t, 16>>>
ZnpApi::ZdoGetLinkKey(IEEEAddress IEEEAddr) {
  return RawSReq(ZdoCommand::GET_LINK_KEY, znp::Encode(IEEEAddr))
      .then(&ZnpApi::CheckStatus)
      .then(&znp::Decode<std::tuple<IEEEAddress, std::array<uint8_t, 16>>>);
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

stlab::future<IEEEAddress> ZnpApi::UtilAddrmgrNwkAddrLookup(
    ShortAddress address) {
  return RawSReq(UtilCommand::ADDRMGR_NWK_ADDR_LOOKUP, znp::Encode(address))
      .then(&znp::Decode<IEEEAddress>);
}

void ZnpApi::OnFrame(ZnpCommandType type, ZnpCommand command,
                     const std::vector<uint8_t>& payload) {
  for (auto it = handlers_.begin(); it != handlers_.end();) {
    auto action = (*it)(type, command, payload);
    if (action.remove_me) {
      it = handlers_.erase(it);
    } else {
      it++;
    }
    if (action.stop_processing) {
      return;
    }
  }
  LOG("ZnpApi", debug) << "Unhandled frame " << type << " " << command;
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
  handlers_.push_back(
      [package, type, command](
          const ZnpCommandType& recvd_type, const ZnpCommand& recvd_command,
          const std::vector<uint8_t>& data) -> FrameHandlerAction {
        if (recvd_type == type && recvd_command == command) {
          package.first(nullptr, data);
          return {true, true};
        }
        return {false, false};
      });
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
  auto package = stlab::package<std::vector<uint8_t>(std::exception_ptr,
                                                     std::vector<uint8_t>)>(
      stlab::immediate_executor,
      [](std::exception_ptr ex, std::vector<uint8_t> data) {
        if (ex != nullptr) {
          std::rethrow_exception(ex);
        }
        return data;
      });
  handlers_.push_back(
      [package, command](
          const ZnpCommandType& type, const ZnpCommand& recvd_command,
          const std::vector<uint8_t>& data) -> FrameHandlerAction {
        // Normal response
        if (type == ZnpCommandType::SRSP && recvd_command == command) {
          package.first(nullptr, data);
          return {true, true};
        }
        // Possible RPC_Error response
        if (type == ZnpCommandType::SRSP &&
            recvd_command == ZnpCommand(ZnpSubsystem::RPC_Error, 0)) {
          try {
            auto info = znp::DecodeT<uint8_t, uint8_t, uint8_t>(data);
            ZnpCommand err_command((ZnpSubsystem)(std::get<1>(info) & 0xF),
                                   std::get<2>(info));
            ZnpCommandType err_type = (ZnpCommandType)(std::get<1>(info) >> 4);
            if (err_type == ZnpCommandType::SREQ && err_command == command) {
              std::stringstream ss;
              ss << "RPC Error: " << (unsigned int)std::get<0>(info);
              package.first(
                  std::make_exception_ptr(std::runtime_error(ss.str())),
                  std::vector<uint8_t>());
              return {true, true};
            }
          } catch (const std::exception& exc) {
            LOG("ZnpApi", debug) << "Unable to parse RPCError";
          }
        }
        return {false, false};
      });
  raw_->SendFrame(ZnpCommandType::SREQ, command, payload);
  return package.second;
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

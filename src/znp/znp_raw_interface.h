#ifndef _ZNP_RAW_INTERFACE_H_
#define _ZNP_RAW_INTERFACE_H_
#include <boost/signals2/signal.hpp>
#include <vector>
#include "znp/znp.h"

namespace znp {
class ZnpRawInterface {
 public:
  ZnpRawInterface() = default;
  virtual ~ZnpRawInterface() = default;

  virtual void SendFrame(ZnpCommandType cmdtype, ZnpCommand command,
                         const std::vector<uint8_t>& payload) = 0;

  boost::signals2::signal<void(ZnpCommandType, ZnpCommand,
                               const std::vector<uint8_t>&)>
      on_frame_;
};
}  // namespace znp
#endif  // _ZNP_RAW_INTERFACE_H_


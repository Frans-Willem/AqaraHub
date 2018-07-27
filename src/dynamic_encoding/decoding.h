#ifndef _DYNAMIC_ENCODING_DECODING_H_
#define _DYNAMIC_ENCODING_DECODING_H_
#include <tao/json.hpp>
#include <vector>
#include "dynamic_encoding/common.h"

namespace dynamic_encoding {
tao::json::value Decode(const Context& ctx, const AnyType& type,
                        std::vector<uint8_t>::const_iterator& begin,
                        const std::vector<uint8_t>::const_iterator& end);
tao::json::value Decode(const Context& ctx, const ObjectType& object,
                        std::vector<uint8_t>::const_iterator& begin,
                        const std::vector<uint8_t>::const_iterator& end);
}  // namespace dynamic_encoding
#endif  // _DYNAMIC_ENCODING_DECODING_H_

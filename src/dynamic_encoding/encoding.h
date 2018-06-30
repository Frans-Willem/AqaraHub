#ifndef _DYNAMIC_ENCODING_ENCODING_H_
#define _DYNAMIC_ENCODING_ENCODING_H_
#include <tao/json.hpp>
#include <vector>
#include "dynamic_encoding/common.h"

namespace dynamic_encoding {
void Encode(const Context& ctx, const AnyType& type,
            const tao::json::value& value, std::vector<uint8_t>& target);
void Encode(const Context& ctx, const ObjectType& object,
            const tao::json::value& value, std::vector<uint8_t>& target);
}  // namespace dynamic_encoding
#endif  // _DYNAMIC_ENCODING_ENCODING_H_

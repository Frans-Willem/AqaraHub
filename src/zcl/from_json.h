#ifndef _ZCL_FROM_JSON_H_
#define _ZCL_FROM_JSON_H_
#include <tao/json.hpp>
#include "zcl/zcl.h"

namespace zcl {
ZclVariant from_json(const tao::json::value& value);
}
#endif  //_ZCL_FROM_JSON_H_

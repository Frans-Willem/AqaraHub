#ifndef _ZCL_TO_JSON_H_
#define _ZCL_TO_JSON_H_
#include <tao/json.hpp>
#include "zcl/zcl.h"

namespace zcl {

template <typename T>
static tao::json::value to_json(const T& value);

template <typename T>
struct to_json_helper {
  static tao::json::value convert(const T& x) { return x; }
};

template <std::size_t N>
struct to_json_helper<std::bitset<N>> {
  static tao::json::value convert(const std::bitset<N>& bitset) {
    tao::json::value array(tao::json::empty_array);
    for (std::size_t i = 0; i < bitset.size(); i++) {
      array.push_back(bitset.test(i));
    }
    return array;
  }
};
template <typename T>
struct to_json_helper<std::vector<T>> {
  static tao::json::value convert(const std::vector<T>& x) {
    tao::json::value array(tao::json::empty_array);
    for (const auto& item : x) {
      array.push_back(to_json_helper<T>::convert(item));
    }
    return array;
  }
};
template <>
struct to_json_helper<ZclVariant> {
  static tao::json::value convert(const ZclVariant& x) {
    return {{"type", to_json(x.type_)}, {"value", to_json(x.data_)}};
  }
};
template <>
struct to_json_helper<DataType> {
  static tao::json::value convert(const DataType& x) { return to_string(x); }
};

template <typename... T>
struct to_json_helper<boost::variant<T...>> {
  struct visitor : public boost::static_visitor<tao::json::value> {
    template <typename T2>
    tao::json::value operator()(const T2& value) const {
      return to_json<T2>(value);
    }
  };
  static tao::json::value convert(const boost::variant<T...>& x) {
    return boost::apply_visitor(visitor(), x);
  }
};

template <typename T>
static tao::json::value to_json(const T& value) {
  return to_json_helper<T>::convert(value);
}
}  // namespace zcl
#endif  // _ZCL_TO_JSON_H_

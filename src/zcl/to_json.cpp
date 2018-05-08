#include "zcl/to_json.h"
#include "string_enum.h"
#include "zcl/zcl_string_enum.h"

namespace zcl {
template <typename T>
static tao::json::value any_to_json(const T& value, bool typed);

template <typename T>
struct to_json_helper {
  static tao::json::value convert(const T& x, bool typed) { return x; }
};

template <std::size_t N>
struct to_json_helper<std::bitset<N>> {
  static tao::json::value convert(const std::bitset<N>& bitset, bool typed) {
    tao::json::value array(tao::json::empty_array);
    for (std::size_t i = 0; i < bitset.size(); i++) {
      array.push_back(bitset.test(i));
    }
    return array;
  }
};
template <typename T>
struct to_json_helper<std::vector<T>> {
  static tao::json::value convert(const std::vector<T>& x, bool typed) {
    tao::json::value array(tao::json::empty_array);
    for (const auto& item : x) {
      array.push_back(to_json_helper<T>::convert(item, typed));
    }
    return array;
  }
};
template <typename T, std::size_t N>
struct to_json_helper<std::array<T, N>> {
  static tao::json::value convert(const std::array<T, N>& x, bool typed) {
    tao::json::value array(tao::json::empty_array);
    for (const auto& item : x) {
      array.push_back(to_json_helper<T>::convert(item, typed));
    }
    return array;
  }
};
template <>
struct to_json_helper<DataType> {
  static tao::json::value convert(const DataType& x, bool typed) {
    return enum_to_string(x);
  }
};

template <typename... T>
struct to_json_helper<boost::variant<T...>> {
  struct visitor : public boost::static_visitor<tao::json::value> {
    bool typed_;
    visitor(bool typed) : typed_(typed) {}
    template <typename T2>
    tao::json::value operator()(const T2& value) const {
      return any_to_json<T2>(value, typed_);
    }
  };
  static tao::json::value convert(const boost::variant<T...>& x, bool typed) {
    return boost::apply_visitor(visitor(typed), x);
  }
};
template <DataType MIN, DataType MAX, typename Enable = void>
struct VariantJsonHelper;

template <DataType DT>
struct VariantJsonHelper<
    DT, DT,
    std::enable_if_t<!std::is_void<typename DataTypeHelper<DT>::Type>::value>> {
  static tao::json::value ConvertValue(DataType type, const ZclVariant& x,
                                       bool typed) {
    auto opt_data = x.Get<DT>();
    if (!opt_data) {
      return tao::json::null;
    }
    return any_to_json(*opt_data, typed);
  }
};

template <DataType DT>
struct VariantJsonHelper<
    DT, DT,
    std::enable_if_t<std::is_void<typename DataTypeHelper<DT>::Type>::value>> {
  static tao::json::value ConvertValue(DataType type, const ZclVariant& x,
                                       bool typed) {
    return tao::json::null;
  }
};

template <DataType MIN, DataType MAX, typename Enable>
struct VariantJsonHelper {
  static tao::json::value ConvertValue(DataType type, const ZclVariant& x,
                                       bool typed) {
    constexpr uint8_t mid = (((uint8_t)MIN) + ((uint8_t)MAX)) / 2;
    if ((uint8_t)type > mid) {
      return VariantJsonHelper<(DataType)(mid + 1), MAX>::ConvertValue(type, x,
                                                                       typed);
    } else {
      return VariantJsonHelper<MIN, (DataType)mid>::ConvertValue(type, x,
                                                                 typed);
    }
  }
};
template <>
struct to_json_helper<ZclVariant> {
  static tao::json::value convert(const ZclVariant& x, bool typed) {
    tao::json::value data_value =
        VariantJsonHelper<DataType::nodata, DataType::unk>::ConvertValue(
            x.GetType(), x, typed);
    if (typed) {
      return {{"type", any_to_json(x.type_, true)}, {"value", data_value}};
    } else {
      return data_value;
    }
  }
};

template <typename T>
static tao::json::value any_to_json(const T& value, bool typed) {
  return to_json_helper<T>::convert(value, typed);
}

tao::json::value to_json(const ZclVariant& variant, bool typed) {
  return any_to_json<ZclVariant>(variant, typed);
}
}  // namespace zcl

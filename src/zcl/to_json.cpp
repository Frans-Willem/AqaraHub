#include "zcl/to_json.h"
#include <template_lookup.h>
#include "string_enum.h"
#include "zcl/zcl_string_enum.h"

namespace zcl {
template <typename T>
struct custom_traits : tao::json::traits<T> {};

template <std::size_t N>
struct custom_traits<std::bitset<N>> {
  template <template <typename...> class Traits, typename Base>
  static void assign(tao::json::basic_value<Traits, Base>& v,
                     const std::bitset<N>& t) noexcept {
    std::vector<tao::json::basic_value<Traits, Base>> retval;
    for (std::size_t i = 0; i < N; i++) {
      retval.push_back(t[i]);
    }
    v = std::move(retval);
  }

  template <template <typename...> class Traits, typename Base>
  static std::bitset<N> as(const tao::json::basic_value<Traits, Base>& v) {
    if (!v.is_array()) {
      throw std::runtime_error("Expected array value for bitset");
    }
    std::bitset<N> t;
    auto arr = v.get_array();
    if (arr.size() != t.size()) {
      throw std::runtime_error("Array is not of the same size as bitset");
    }
    for (std::size_t i = 0; i < t.size(); i++) {
      t[i] = arr[i].template as<bool>();
    }
    return t;
  }
};

template <typename T, std::size_t N>
struct custom_traits<std::array<T, N>> {
  template <template <typename...> class Traits, typename Base>
  static void assign(tao::json::basic_value<Traits, Base>& v,
                     const std::array<T, N>& t) noexcept {
    v = std::vector<tao::json::basic_value<Traits, Base>>(t.begin(), t.end());
  }

  template <template <typename...> class Traits, typename Base>
  static std::array<T, N> as(const tao::json::basic_value<Traits, Base>& v) {
    if (!v.is_array()) {
      throw std::runtime_error("Expected array");
    }
    std::vector<tao::json::basic_value<Traits, Base>> arr = v.get_array();
    if (arr.size() != N) {
      throw std::runtime_error("Array of unexpected size");
    }
    std::array<T, N> ret;
    for (std::size_t i = 0; i < N; i++) {
      ret[i] = Traits<T>::as(arr[i]);
    }
    return ret;
  }
};

template <>
struct custom_traits<zcl::DataType> {
  template <template <typename...> class Traits, typename Base>
  static void assign(tao::json::basic_value<Traits, Base>& v,
                     const zcl::DataType& t) noexcept {
    v = enum_to_string<zcl::DataType>(t);
  }
  template <template <typename...> class Traits, typename Base>
  static zcl::DataType as(const tao::json::basic_value<Traits, Base>& v) {
    if (!v.is_string()) {
      throw std::runtime_error("Expected string to data type");
    }
    boost::optional<zcl::DataType> e =
        string_to_enum<zcl::DataType>(v.get_string());
    if (!e) {
      throw std::runtime_error("Invalid data type");
    }
    return *e;
  }
};

template <template <typename...> class Traits, typename Base>
struct VariantTrait {
  virtual void assign(tao::json::basic_value<Traits, Base>& v,
                      const zcl::ZclVariant& variant) noexcept = 0;
  virtual zcl::ZclVariant as(const tao::json::basic_value<Traits, Base>& v) = 0;
};

template <template <typename...> class Traits, typename Base>
struct VariantTraitImpl {
  template <zcl::DataType DT, typename ValueType>
  struct PerValue : VariantTrait<Traits, Base> {
    void assign(tao::json::basic_value<Traits, Base>& v,
                const zcl::ZclVariant& variant) noexcept override {
      boost::optional<ValueType> value = variant.Get<DT>();
      if (!value) {
        v = tao::json::null;
      } else {
        Traits<ValueType>::assign(v, *value);
      }
    }

    zcl::ZclVariant as(const tao::json::basic_value<Traits, Base>& v) override {
      if (v.is_null()) {
        return zcl::ZclVariant::Create<DT>();
      }
      return zcl::ZclVariant::Create<DT>(v.template as<ValueType>());
    }
  };
  template <zcl::DataType DT>
  struct PerValue<DT, void> : VariantTrait<Traits, Base> {
    void assign(tao::json::basic_value<Traits, Base>& v,
                const zcl::ZclVariant& variant) noexcept override {
      v = tao::json::null;
    }

    zcl::ZclVariant as(const tao::json::basic_value<Traits, Base>& v) override {
      if (!v.is_null()) {
        throw std::runtime_error("Expected null for void datatype");
      }
      return zcl::ZclVariant::Create<DT>();
    }
  };
  template <zcl::DataType DT>
  struct PerDatatype : PerValue<DT, typename zcl::DataTypeHelper<DT>::Type> {};
};

template <>
struct custom_traits<zcl::ZclVariant> {
  template <template <typename...> class Traits, typename Base>
  static std::map<zcl::DataType, std::unique_ptr<VariantTrait<Traits, Base>>>&
  ConversionMap() {
    static std::map<zcl::DataType, std::unique_ptr<VariantTrait<Traits, Base>>>
        map =
            std::move(template_lookup::CreateEnumLookup<
                      zcl::DataType, zcl::DataType::nodata, zcl::DataType::unk,
                      VariantTrait<Traits, Base>,
                      VariantTraitImpl<Traits, Base>::template PerDatatype>());
    return map;
  }

  template <template <typename...> class Traits, typename Base>
  static void assign(tao::json::basic_value<Traits, Base>& v,
                     const zcl::ZclVariant& t) noexcept {
    auto& m = ConversionMap<Traits, Base>();
    auto found = m.find(t.GetType());
    if (found == m.end()) {
      v = tao::json::null;
    }
    tao::json::basic_value<Traits, Base> value;
    found->second->assign(value, t);
    if (value == tao::json::null) {
      v = {{"type", enum_to_string<zcl::DataType>(t.GetType())}};
    } else {
      v = {{"type", enum_to_string<zcl::DataType>(t.GetType())},
           {"value", value}};
    }
  }

  template <template <typename...> class Traits, typename Base>
  static zcl::ZclVariant as(const tao::json::basic_value<Traits, Base>& v) {
    if (!v.is_object()) {
      throw std::runtime_error("Excepted object");
    }
    std::map<std::string, tao::json::basic_value<Traits, Base>> map =
        v.get_object();
    auto type_found = map.find("type");
    auto value_found = map.find("value");
    if (type_found == map.end()) {
      throw std::runtime_error("Expected 'type' member");
    }
    zcl::DataType dt = type_found->second.template as<zcl::DataType>();
    auto& convmap = ConversionMap<Traits, Base>();
    auto conv_found = convmap.find(dt);
    if (conv_found == convmap.end()) {
      throw std::runtime_error("Unsupported variant type");
    }
    return conv_found->second->as(
        (value_found == map.end()) ? tao::json::null : value_found->second);
  }
};

tao::json::value to_json(const ZclVariant& variant) {
  tao::json::basic_value<custom_traits> value(variant);
  return *(tao::json::value*)&value;
}
ZclVariant from_json(const tao::json::value& value) {
  return ((const tao::json::basic_value<custom_traits>*)&value)
      ->as<ZclVariant>();
}
}  // namespace zcl

#ifndef _STRING_ENUM_H_
#define _STRING_ENUM_H_
#include <zcl/zcl.h>
#include <boost/format.hpp>
#include <iostream>
#include <map>

template <typename T>
struct StringEnumHelper;

template <typename T>
std::string enum_to_string(T value) {
  static std::map<T, std::string> table;
  if (table.empty()) {
    table = StringEnumHelper<T>::lookup();
  }
  auto found = table.find(value);
  if (found != table.end()) {
    return found->second;
  }
  typedef typename std::underlying_type<T>::type UT;
  typedef typename std::make_unsigned<UT>::type UUT;
  std::string format_string = boost::str(
      boost::format("%%0%dX") % (std::numeric_limits<UUT>::digits / 4));
  return boost::str(boost::format(format_string) %
                    (unsigned long long)(UUT)value);
}

template <typename T>
boost::optional<T> string_to_enum(const std::string& value) {
  static std::map<std::string, T> table;
  if (table.empty()) {
    for (const auto& item : StringEnumHelper<T>::lookup()) {
      table[item.second] = item.first;
    }
  }
  auto found = table.find(value);
  if (found != table.end()) {
    return found->second;
  }
  typedef typename std::underlying_type<T>::type UT;
  typedef typename std::make_unsigned<UT>::type UUT;

  std::size_t end_pos;
  unsigned long x = std::stoul(value, &end_pos, 16);
  if (end_pos != value.size()) return boost::none;
  if (x > std::numeric_limits<UUT>::max()) {
    return boost::none;
  }
  return (T)(UT)(UUT)x;
}
#endif  // _STRING_ENUM_H_

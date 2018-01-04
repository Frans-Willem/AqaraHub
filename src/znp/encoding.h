#ifndef _ZNP_ENCODING_H_
#define _ZNP_ENCODING_H_
#include <boost/fusion/include/accumulate.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include <iostream>
#include <tuple>
#include <type_traits>
#include "znp/znp.h"

namespace znp {
typedef std::vector<uint8_t> EncodeTarget;
template <typename T, typename Enable = void>
class EncodeHelper;

template <class T>
class EncodeHelper<T, std::enable_if_t<std::is_integral<T>::value &&
                                       std::is_unsigned<T>::value>> {
 public:
  static inline std::size_t GetSize(const T& value) {
    return std::numeric_limits<T>::digits / 8;
  }
  static inline void Encode(const T& value, EncodeTarget::iterator& begin,
                            EncodeTarget::iterator end) {
    std::size_t bytes = GetSize(value);
    for (std::size_t shift = 0; shift < bytes * 8; shift += 8) {
      if (begin == end) {
        throw std::runtime_error("Not enough space to encode integer");
      }
      *(begin++) = (uint8_t)((value >> shift) & 0xFF);
    }
  }
  static inline void Decode(T& value, EncodeTarget::const_iterator& begin,
                            EncodeTarget::const_iterator end) {
    std::size_t bytes = GetSize(0);
    value = 0;
    for (std::size_t shift = 0; shift < bytes * 8; shift += 8) {
      if (begin == end) {
        throw std::runtime_error("Not enough data to decode integer");
      }
      value |= ((T) * (begin++)) << shift;
    }
  }
};
template <class T>
class EncodeHelper<T, std::enable_if_t<std::is_integral<T>::value &&
                                       std::is_signed<T>::value>> {
 private:
  typedef typename std::make_unsigned<T>::type UT;

 public:
  static inline std::size_t GetSize(const T& value) {
    return EncodeHelper<UT>::GetSize((UT)value);
  }
  static inline void Encode(const T& value, EncodeTarget::iterator& begin,
                            EncodeTarget::iterator end) {
    EncodeHelper<UT>::Encode((UT)value, begin, end);
  }
  static inline void Decode(T& value, EncodeTarget::const_iterator& begin,
                            EncodeTarget::const_iterator end) {
    UT unsigned_value = 0;
    EncodeHelper<UT>::Decode(unsigned_value, begin, end);
    value = (T)unsigned_value;
  }
};
template <typename T, size_t length>
class EncodeHelper<std::array<T, length>> {
 public:
  static inline std::size_t GetSize(const std::array<T, length>& value) {
    std::size_t size = 0;
    for (const auto& item : value) {
      size += EncodeHelper<T>::GetSize(item);
    }
    return size;
  }
  static inline void Encode(const std::array<T, length>& data,
                            EncodeTarget::iterator& begin,
                            EncodeTarget::iterator end) {
    for (const auto& item : data) {
      EncodeHelper<T>::Encode(item, begin, end);
    }
  }
  static inline void Decode(std::array<T, length>& data,
                            EncodeTarget::const_iterator& begin,
                            EncodeTarget::const_iterator end) {
    for (auto& item : data) {
      EncodeHelper<T>::Decode(item, begin, end);
    }
  }
};

template <class T, size_t pos>
class EncodeTupleHelper {
 private:
  static constexpr std::size_t index = std::tuple_size<T>::value - 1 - pos;

 public:
  static inline std::size_t GetSize(const T& value) {
    return EncodeHelper<std::tuple_element_t<index, T>>::GetSize(
               std::get<index>(value)) +
           EncodeTupleHelper<T, pos - 1>::GetSize(value);
  }
  static void Encode(const T& value, EncodeTarget::iterator& begin,
                     EncodeTarget::iterator end) {
    EncodeHelper<std::tuple_element_t<index, T>>::Encode(std::get<index>(value),
                                                         begin, end);
    EncodeTupleHelper<T, pos - 1>::Encode(value, begin, end);
  }
  static void Decode(T& value, EncodeTarget::const_iterator& begin,
                     EncodeTarget::const_iterator end) {
    EncodeHelper<std::tuple_element_t<index, T>>::Decode(std::get<index>(value),
                                                         begin, end);
    EncodeTupleHelper<T, pos - 1>::Decode(value, begin, end);
  }
};
template <class T>
class EncodeTupleHelper<T, 0> {
 private:
  static constexpr std::size_t index = std::tuple_size<T>::value - 1;

 public:
  static inline std::size_t GetSize(const T& value) {
    return EncodeHelper<std::tuple_element_t<index, T>>::GetSize(
        std::get<index>(value));
  }
  static void Encode(const T& value, EncodeTarget::iterator& begin,
                     EncodeTarget::iterator end) {
    EncodeHelper<std::tuple_element_t<index, T>>::Encode(std::get<index>(value),
                                                         begin, end);
  }
  static void Decode(T& value, EncodeTarget::const_iterator& begin,
                     EncodeTarget::const_iterator end) {
    EncodeHelper<std::tuple_element_t<index, T>>::Decode(std::get<index>(value),
                                                         begin, end);
  }
};

template <class... T>
class EncodeHelper<std::tuple<T...>> {
 public:
  static inline std::size_t GetSize(const std::tuple<T...>& value) {
    return EncodeTupleHelper<std::tuple<T...>,
                             std::tuple_size<std::tuple<T...>>::value -
                                 1>::GetSize(value);
  }
  static void Encode(const std::tuple<T...>& value,
                     EncodeTarget::iterator& begin,
                     EncodeTarget::iterator end) {
    EncodeTupleHelper<std::tuple<T...>,
                      std::tuple_size<std::tuple<T...>>::value -
                          1>::Encode(value, begin, end);
  };
  static void Decode(std::tuple<T...>& value,
                     EncodeTarget::const_iterator& begin,
                     EncodeTarget::const_iterator end) {
    EncodeTupleHelper<std::tuple<T...>,
                      std::tuple_size<std::tuple<T...>>::value -
                          1>::Decode(value, begin, end);
  };
};

template <typename T>
class EncodeHelper<T, std::enable_if_t<std::is_enum<T>::value>> {
 public:
  static inline std::size_t GetSize(const T& value) {
    return EncodeHelper<std::underlying_type_t<T>>::GetSize(
        (std::underlying_type_t<T>)value);
  }
  static inline void Encode(const T& value, EncodeTarget::iterator& begin,
                            EncodeTarget::iterator end) {
    EncodeHelper<std::underlying_type_t<T>>::Encode(
        (std::underlying_type_t<T>)value, begin, end);
  }
  static inline void Decode(T& value, EncodeTarget::const_iterator& begin,
                            EncodeTarget::const_iterator end) {
    std::underlying_type_t<T> temp;
    EncodeHelper<std::underlying_type_t<T>>::Decode(temp, begin, end);
    value = (T)temp;
  }
};

template <typename T>
class EncodeHelper<std::vector<T>> {
 public:
  static inline std::size_t GetSize(const std::vector<T>& value) {
    std::size_t size = 1;
    for (const auto& item : value) {
      size += EncodeHelper<T>::GetSize(item);
    }
    return size;
  }
  static inline void Encode(const std::vector<T>& value,
                            EncodeTarget::iterator& begin,
                            EncodeTarget::iterator end) {
    if (begin == end) {
      throw std::runtime_error(
          "Not enough space in encoding buffer to encode vector length");
    }
    if (value.size() > 255) {
      throw std::runtime_error("Unable to encode vector of size >255");
    }
    *(begin++) = (uint8_t)value.size();
    for (const auto& item : value) {
      EncodeHelper<T>::Encode(item, begin, end);
    }
  }
  static inline void Decode(std::vector<T>& value,
                            EncodeTarget::const_iterator& begin,
                            EncodeTarget::const_iterator end) {
    if (begin == end) {
      throw std::runtime_error("Expected vector length");
    }
    std::size_t length = (std::size_t) * (begin++);
    value.resize(length);
    for (auto& item : value) {
      EncodeHelper<T>::Decode(item, begin, end);
    }
  }
};

struct FusionGetSizeHelper {
  template <typename T>
  std::size_t operator()(std::size_t current, const T& t) const {
    return current + EncodeHelper<T>(t);
  }
};
struct FusionDecodeHelper {
  EncodeTarget::const_iterator end;
  template <typename T>
  EncodeTarget::const_iterator operator()(EncodeTarget::const_iterator begin,
                                          T& value) {
    EncodeHelper<T>::Decode(value, begin, end);
    return begin;
  }
};
template <typename T>
class EncodeHelper<
    T, std::enable_if_t<boost::fusion::traits::is_sequence<T>::value>> {
 public:
  static inline std::size_t GetSize(const T& value) {
    return boost::fusion::accumulate(value, 0, FusionGetSizeHelper{});
  }
  static inline void Decode(T& value, EncodeTarget::const_iterator& begin,
                            EncodeTarget::const_iterator end) {
    begin = boost::fusion::accumulate(value, begin, FusionDecodeHelper{end});
  }
};

template <>
class EncodeHelper<bool> {
 public:
  static inline std::size_t GetSize(const bool& value) { return 1; }
  static inline void Encode(const bool& value, EncodeTarget::iterator& begin,
                            EncodeTarget::iterator end) {
    EncodeHelper<uint8_t>::Encode(value ? 1 : 0, begin, end);
  }
  static inline void Decode(bool& value, EncodeTarget::const_iterator& begin,
                            EncodeTarget::const_iterator end) {
    uint8_t int_value;
    EncodeHelper<uint8_t>::Decode(int_value, begin, end);
    value = int_value > 0;
  }
};

template <std::size_t N>
class EncodeHelper<
    std::bitset<N>,
    std::enable_if_t<N % 8 == 0 &&
                     std::numeric_limits<unsigned long long>::digits >= N>> {
 public:
  static inline std::size_t GetSize(const std::bitset<N>& value) {
    return N / 8;
  };
  static inline void Encode(const std::bitset<N>& value,
                            EncodeTarget::iterator& begin,
                            EncodeTarget::iterator end) {
    unsigned long long numvalue = value.to_ullong();
    for (std::size_t i = 0; i < N; i += 8) {
      if (begin == end) {
        throw std::runtime_error("Not enough space to encode bitset");
      }
      *(begin++) = (uint8_t)(numvalue & 0xFF);
      numvalue >>= 8;
    }
  }
  static inline void Decode(std::bitset<N>& value,
                            EncodeTarget::const_iterator& begin,
                            EncodeTarget::const_iterator end) {
    unsigned long long numvalue = 0;
    for (std::size_t i = 0; i < N; i += 8) {
      if (begin == end) {
        throw std::runtime_error("Not enough data to decode bitset");
      }
      numvalue |= ((unsigned long long)*(begin++)) << i;
    }
    value = std::bitset<N>(numvalue);
  }
};

template <typename T>
std::vector<uint8_t> Encode(const T& data) {
  std::vector<uint8_t> target(EncodeHelper<T>::GetSize(data));
  std::vector<uint8_t>::iterator current = target.begin();
  EncodeHelper<T>::Encode(data, current, target.end());
  if (current != target.end()) {
    throw std::runtime_error("Encoder failure: Too much data reserved");
  }
  return target;
}

inline std::vector<uint8_t> Encode() { return std::vector<uint8_t>(); }

template <typename T>
T DecodePartial(const std::vector<uint8_t>& data) {
  T retval;
  EncodeTarget::const_iterator current = data.begin();
  EncodeHelper<T>::Decode(retval, current, data.end());
  return std::move(retval);
}
template <typename T>
T Decode(const std::vector<uint8_t>& data) {
  T retval;
  EncodeTarget::const_iterator current = data.begin();
  EncodeHelper<T>::Decode(retval, current, data.end());
  if (current != data.end()) {
    throw std::runtime_error("Decoding failure: Not all bytes parsed");
  }
  return std::move(retval);
}

template <>
inline void DecodePartial<void>(const std::vector<uint8_t>& data) {}
template <>
inline void Decode<void>(const std::vector<uint8_t>& data) {
  if (data.size() != 0) {
    throw std::runtime_error("Decoding failure: Expected empty data");
  }
}

template <typename... T>
std::vector<uint8_t> EncodeT(const T&... args) {
  return Encode<std::tuple<T...>>(std::tuple<T...>(args...));
}
template <typename... T>
std::tuple<T...> DecodeT(const std::vector<uint8_t>& data) {
  return Decode<std::tuple<T...>>(data);
}

template <typename T>
std::size_t EncodedSize(const T& value) {
  return EncodeHelper<T>::GetSize(value);
}
template <typename... T>
std::size_t EncodedSizeT(const T&... args) {
  return EncodedSize<std::tuple<T...>>(std::tuple<T...>(args...));
}
}  // namespace znp
#endif  // _ZNP_ENCODING_H_

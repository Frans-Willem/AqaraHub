#ifndef _ZNP_ENCODING_H_
#define _ZNP_ENCODING_H_
#include <iostream>
#include <tuple>
#include "znp/znp.h"
#include <type_traits>

namespace znp {
typedef std::vector<uint8_t> EncodeTarget;
template <typename T, typename Enable = void>
class EncodeHelper;

template <>
class EncodeHelper<uint8_t> {
 public:
  static inline std::size_t GetSize() { return 1; }
  static inline void Encode(const uint8_t& number,
                            EncodeTarget::iterator& begin,
                            EncodeTarget::iterator end) {
    if (end - begin < 1) {
      throw std::runtime_error("Not enough space reserved for uint8_t");
    }
    *(begin++) = number;
  }
  static inline void Decode(uint8_t& number,
                            EncodeTarget::const_iterator& begin,
                            EncodeTarget::const_iterator end) {
    if (end - begin < 1) {
      throw std::runtime_error("Not enough space to decode an uint8_t");
    }
    number = *(begin++);
  }
};

template <>
class EncodeHelper<uint16_t> {
 public:
  static inline std::size_t GetSize() { return 2; }
  static inline void Encode(const uint16_t& number,
                            EncodeTarget::iterator& begin,
                            EncodeTarget::iterator end) {
    if (end - begin < 2) {
      throw std::runtime_error("Not enough space reserved for uint16_t");
    }
    *(begin++) = (uint8_t)((number >> 0) & 0xFF);
    *(begin++) = (uint8_t)((number >> 8) & 0xFF);
  }
  static inline void Decode(uint16_t& number,
                            EncodeTarget::const_iterator& begin,
                            EncodeTarget::const_iterator end) {
    if (end - begin < 2) {
      throw std::runtime_error("Not enough space to decode an uint16_t");
    }
    number = 0;
    number |= ((uint16_t) * (begin++)) << 0;
    number |= ((uint16_t) * (begin++)) << 8;
  }
};

template <>
class EncodeHelper<uint32_t> {
 public:
  static inline std::size_t GetSize() { return 4; }
  static inline void Encode(const uint32_t& number,
                            EncodeTarget::iterator& begin,
                            EncodeTarget::iterator end) {
    if (end - begin < 4) {
      throw std::runtime_error("Not enough space reserved for uint32_t");
    }
    *(begin++) = (uint8_t)((number >> 0) & 0xFF);
    *(begin++) = (uint8_t)((number >> 8) & 0xFF);
    *(begin++) = (uint8_t)((number >> 16) & 0xFF);
    *(begin++) = (uint8_t)((number >> 24) & 0xFF);
  }
  static inline void Decode(uint32_t& number,
                            EncodeTarget::const_iterator& begin,
                            EncodeTarget::const_iterator end) {
    if (end - begin < 4) {
      throw std::runtime_error("Not enough space to decode an uint32_t");
    }
    number = 0;
    number |= ((uint32_t) * (begin++)) << 0;
    number |= ((uint32_t) * (begin++)) << 8;
    number |= ((uint32_t) * (begin++)) << 16;
    number |= ((uint32_t) * (begin++)) << 24;
  }
};

template <>
class EncodeHelper<uint64_t> {
 public:
  static inline std::size_t GetSize() { return 8; }
  static inline void Encode(const uint64_t& number,
                            EncodeTarget::iterator& begin,
                            EncodeTarget::iterator end) {
    if (end - begin < 8) {
      throw std::runtime_error("Not enough space reserved for uint64_t");
    }
    *(begin++) = (uint8_t)((number >> 0) & 0xFF);
    *(begin++) = (uint8_t)((number >> 8) & 0xFF);
    *(begin++) = (uint8_t)((number >> 16) & 0xFF);
    *(begin++) = (uint8_t)((number >> 24) & 0xFF);
    *(begin++) = (uint8_t)((number >> 32) & 0xFF);
    *(begin++) = (uint8_t)((number >> 40) & 0xFF);
    *(begin++) = (uint8_t)((number >> 48) & 0xFF);
    *(begin++) = (uint8_t)((number >> 56) & 0xFF);
  }
  static inline void Decode(uint64_t& number,
                            EncodeTarget::const_iterator& begin,
                            EncodeTarget::const_iterator end) {
    if (end - begin < 8) {
      throw std::runtime_error("Not enough space to decode an uint64_t");
    }
    number = 0;
    number |= ((uint64_t) * (begin++)) << 0;
    number |= ((uint64_t) * (begin++)) << 8;
    number |= ((uint64_t) * (begin++)) << 16;
    number |= ((uint64_t) * (begin++)) << 24;
    number |= ((uint64_t) * (begin++)) << 32;
    number |= ((uint64_t) * (begin++)) << 40;
    number |= ((uint64_t) * (begin++)) << 48;
    number |= ((uint64_t) * (begin++)) << 56;
  }
};
template <typename T, size_t length>
class EncodeHelper<std::array<T, length>> {
 public:
  static inline std::size_t GetSize() {
    return EncodeHelper<T>::GetSize() * length;
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
  static inline std::size_t GetSize() {
    return EncodeHelper<std::tuple_element_t<index, T>>::GetSize() +
           EncodeTupleHelper<T, pos - 1>::GetSize();
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
  static inline std::size_t GetSize() {
    return EncodeHelper<std::tuple_element_t<index, T>>::GetSize();
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
  static inline std::size_t GetSize() {
    return EncodeTupleHelper<std::tuple<T...>,
                             std::tuple_size<std::tuple<T...>>::value -
                                 1>::GetSize();
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
  static inline std::size_t GetSize() {
    return EncodeHelper<std::underlying_type_t<T>>::GetSize();
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
std::vector<uint8_t> Encode(const T& data) {
  std::vector<uint8_t> target(EncodeHelper<T>::GetSize());
  std::vector<uint8_t>::iterator current = target.begin();
  EncodeHelper<T>::Encode(data, current, target.end());
  if (current != target.end()) {
    throw std::runtime_error("Encoder failure: Too much data reserved");
  }
  return target;
}

inline std::vector<uint8_t> Encode() { return std::vector<uint8_t>(); }

template <typename T>
T Decode(const std::vector<uint8_t>& data) {
  if (data.size() != EncodeHelper<T>::GetSize()) {
    throw std::runtime_error("Decoding failure: Data size did not match");
  }
  T retval;
  EncodeTarget::const_iterator current = data.begin();
  EncodeHelper<T>::Decode(retval, current, data.end());
  if (current != data.end()) {
    throw std::runtime_error("Decoding failure: Not all data parsed");
  }
  return std::move(retval);
}

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
std::size_t EncodedSize() {
  return EncodeHelper<T>::GetSize();
}
template <typename... T>
std::size_t EncodedSizeT() {
  return EncodedSize<std::tuple<T...>>();
}
}  // namespace znp
#endif  // _ZNP_ENCODING_H_

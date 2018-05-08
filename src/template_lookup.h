#ifndef _TEMPLATE_LOOKUP_H_
#define _TEMPLATE_LOOKUP_H_
#include <map>
#include <type_traits>

namespace template_lookup {
namespace detail {
template <typename E, E EMIN, E EMAX>
struct EnumLookupCreater {
  template <typename V, typename C>
  static void Insert(std::map<E, V>& m) {
    typedef typename std::underlying_type<E>::type ET;
    m[EMIN] = std::move(C::template Create<EMIN>());
    EnumLookupCreater<E, (E)(((ET)EMIN) + 1), EMAX>::template Insert<V, C>(m);
  }
};
template <typename E, E EBOTH>
struct EnumLookupCreater<E, EBOTH, EBOTH> {
  template <typename V, typename C>
  static void Insert(std::map<E, V>& m) {
    m[EBOTH] = std::move(C::template Create<EBOTH>());
  }
};
}  // namespace detail
template <typename E, E EMIN, E EMAX, typename V, typename C>
std::map<E, V> CreateEnumLookup() {
  std::map<E, V> m;
  detail::EnumLookupCreater<E, EMIN, EMAX>::template Insert<V, C>(m);
  return std::move(m);
}
}  // namespace template_lookup
#endif  // _TEMPLATE_LOOKUP_H_

#ifndef _TEMPLATE_LOOKUP_H_
#define _TEMPLATE_LOOKUP_H_
#include <map>
#include <memory>
#include <type_traits>

namespace template_lookup {
namespace detail {
template <typename E, E EMIN, E EMAX>
struct EnumLookupCreater {
  template <typename Base, template <E> class Impl>
  static void Insert(std::map<E, std::unique_ptr<Base>>& m) {
    typedef typename std::underlying_type<E>::type ET;
    m[EMIN] = std::make_unique<Impl<EMIN>>();
    EnumLookupCreater<E, (E)(((ET)EMIN) + 1), EMAX>::template Insert<Base,
                                                                     Impl>(m);
  }
};
template <typename E, E EBOTH>
struct EnumLookupCreater<E, EBOTH, EBOTH> {
  template <typename Base, template <E> class Impl>
  static void Insert(std::map<E, std::unique_ptr<Base>>& m) {
    m[EBOTH] = std::make_unique<Impl<EBOTH>>();
  }
};
}  // namespace detail
template <typename E, E EMIN, E EMAX, typename Base, template <E> class Impl>
std::map<E, std::unique_ptr<Base>> CreateEnumLookup() {
  std::map<E, std::unique_ptr<Base>> m;
  detail::EnumLookupCreater<E, EMIN, EMAX>::template Insert<Base, Impl>(m);
  return m;
}
}  // namespace template_lookup
#endif  // _TEMPLATE_LOOKUP_H_

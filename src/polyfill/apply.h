#ifndef _POLYFILL_APPLY_H_
#define _POLYFILL_APPLY_H_
#include <tuple>

namespace polyfill {
namespace detail {
template <class F, class Tuple, std::size_t... I>
constexpr decltype(auto) apply_impl(F&& f, Tuple&& t,
                                    std::index_sequence<I...>) {
  return std::forward<F>(f)(std::get<I>(std::forward<Tuple>(t))...);
}
}  // namespace detail

template <class F, class Tuple>
constexpr decltype(auto) apply(F&& f, Tuple&& t) {
  return detail::apply_impl(
      std::forward<F>(f), std::forward<Tuple>(t),
      std::make_index_sequence<
          std::tuple_size<std::remove_reference_t<Tuple>>::value>{});
}
}  // namespace polyfill
#endif  // _POLYFILL_APPLY_H_

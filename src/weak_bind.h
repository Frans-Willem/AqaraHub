#ifndef _WEAK_BIND_H_
#define _WEAK_BIND_H_
#include <memory>
#include <utility>

namespace detail {
template <typename C, typename F, typename R>
class WeakBound {
 public:
  WeakBound(std::weak_ptr<C> weak_self, F f)
      : weak_self_(std::move(weak_self)), f_(std::move(f)) {}
  template <typename... Args>
  R operator()(Args&&... args) const {
    if (auto self = weak_self_.lock()) {
      return f_(std::forward<Args>(args)...);
    } else {
      return R();
    }
  }

 private:
  std::weak_ptr<C> weak_self_;
  F f_;
};

template <typename E, typename F>
class WeakExecutorBound {
 public:
  WeakExecutorBound(std::weak_ptr<E> weak_executor, F f)
      : weak_executor_(std::move(weak_executor)), f_(std::move(f)) {}

  template <typename... Args>
  bool operator()(Args&&... args) const {
    if (auto executor = weak_executor_.lock()) {
      (*executor)(std::bind(f_, std::forward<Args>(args)...));
      return true;
    }
    return false;
  }

 private:
  std::weak_ptr<E> weak_executor_;
  F f_;
};

template <typename F>
struct GetReturnType {};

template <typename R, typename... Args>
struct GetReturnType<R (*)(Args...)> {
  typedef R type;
};

template <typename R, typename C, typename... Args>
struct GetReturnType<R (C::*)(Args...)> {
  typedef R type;
};
}  // namespace detail

template <typename F, typename C, typename... Args>
auto WeakBind(F&& f, const std::shared_ptr<C>& c, Args&&... args) {
  auto bound =
      std::bind(std::forward<F>(f), c.get(), std::forward<Args>(args)...);
  return detail::WeakBound<C, decltype(bound),
                           typename detail::GetReturnType<F>::type>(
      std::weak_ptr<C>(c), std::move(bound));
}

template <typename E, typename F, typename C, typename... Args>
auto WeakExecutorBind(const std::shared_ptr<E>& executor, F&& f,
                      const std::shared_ptr<C>& c, Args&&... args) {
  auto weak_bound =
      WeakBind(std::forward<F>(f), c, std::forward<Args>(args)...);
  return detail::WeakExecutorBound<E, decltype(weak_bound)>(
      std::weak_ptr<E>(executor), std::move(weak_bound));
}
#endif  //_WEAK_BIND_H_

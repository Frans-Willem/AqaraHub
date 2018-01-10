#ifndef _CORO_H_
#define _CORO_H_
#include <boost/coroutine2/all.hpp>
#include <boost/optional.hpp>
#include <iostream>
#include <stlab/concurrency/future.hpp>
#include <stlab/concurrency/immediate_executor.hpp>
#include "polyfill/apply.h"

namespace coro {
typedef boost::coroutines2::coroutine<stlab::future<void>> coro_t;
// Implementation for "await" functionality
class Await {
 private:
  coro_t::push_type& sink_;

 public:
  Await(coro_t::push_type& sink);
  template <typename T>
  T operator()(stlab::future<T> f) {
    // Explicitly putting this in a shared pointer,
    // if we put it on the stack, and the coroutine is destroyed without being
    // continued, the recover function may have pointers to invalid memory.
    auto context =
        std::make_shared<std::pair<std::exception_ptr, boost::optional<T>>>(
            nullptr, boost::none);
    sink_(std::move(f).recover(stlab::immediate_executor, [context](auto f) {
      try {
        context->second = f.get_try();
      } catch (...) {
        context->first = std::current_exception();
      }
    }));
    if (context->first) {
      std::rethrow_exception(context->first);
    }
    return *std::move(context->second);
  }
  void operator()(stlab::future<void> f) {
    // Explicitly make a copy of this future, otherwise it will be moved out.
    sink_(stlab::future<void>(f));
    f.get_try();
  }
};

template <typename E>
void CoroutineResume(E executor, coro_t::pull_type source) {
  if (!source) {
    // Coroutine has ended
    return;
  }
  stlab::future<void> next = source.get();
  next.recover(executor,
               [ source{std::move(source)},
                 executor{std::move(executor)} ](auto f) mutable {
                 source();
                 CoroutineResume(executor, std::move(source));
               })
      .detach();
}

template <typename T>
struct CoroHelpers {
  template <typename E>
  static auto CreatePackage(E e) {
    return stlab::package<T(std::exception_ptr, boost::optional<T>)>(
        e, &FinalResolve);
  }
  static T FinalResolve(std::exception_ptr exc, boost::optional<T> value) {
    if (exc) {
      std::rethrow_exception(exc);
    }  // namespace coro
    return *std::move(value);
  }
  template <typename F, typename... Args>
  static void RunInCoro(
      std::function<void(std::exception_ptr, boost::optional<T>)> promise, F f,
      std::tuple<Args...> args) {
    try {
      T retval = polyfill::apply(f, std::move(args));
      promise(nullptr, std::move(retval));
    } catch (...) {
      promise(std::current_exception(), boost::none);
    }
  }
};

template <>
struct CoroHelpers<void> {
  template <typename E>
  static auto CreatePackage(E e) {
    return stlab::package<void(std::exception_ptr)>(e, &FinalResolve);
  }
  static void FinalResolve(std::exception_ptr exc) {
    if (exc) {
      std::rethrow_exception(exc);
    }
  }
  template <typename F, typename... Args>
  static void RunInCoro(std::function<void(std::exception_ptr)> promise, F f,
                        std::tuple<Args...> args) {
    try {
      polyfill::apply(f, std::move(args));
    } catch (...) {
      promise(std::current_exception());
      return;
    }
    promise(nullptr);
  }
};

template <typename E, typename F, typename... Args>
auto Run(E executor, F f, Args... args) {
  typedef typename std::result_of<F(Await, Args...)>::type T;
  auto package = CoroHelpers<T>::CreatePackage(executor);
  auto promise = std::move(package.first);
  std::tuple<Args...> args_tuple(std::move(args)...);
  executor([
    executor, promise{std::move(promise)}, f{std::move(f)},
    args_tuple{std::move(args_tuple)}
  ]() mutable {
    coro_t::pull_type source([
      promise{std::move(promise)}, f{std::move(f)},
      args_tuple{std::move(args_tuple)}
    ](coro_t::push_type & sink) mutable {
      auto full_args =
          std::tuple_cat(std::make_tuple(Await(sink)), std::move(args_tuple));
      CoroHelpers<T>::template RunInCoro<F, Await, Args...>(
          promise, f, std::move(full_args));
    });
    CoroutineResume(executor, std::move(source));
  });
  return std::move(package.second);
}
}  // namespace coro
#endif  // _CORO_H_

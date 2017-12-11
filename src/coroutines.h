#ifndef _COROUTINES_H_
#define _COROUTINES_H_
#include <experimental/coroutine>
#include <stlab/concurrency/future.hpp>

template <typename T, typename... Args>
struct std::experimental::coroutine_traits<stlab::future<T>, Args...> {
  struct promise_type {
    stlab::future<T> future;
    stlab::packaged_task<std::exception_ptr, const T&> promise;
    promise_type() {
      std::tie(promise, future) =
          stlab::package<T(std::exception_ptr, const T&)>(
              stlab::immediate_executor,
              [](std::exception_ptr exc, const T& value) {
                if (exc != nullptr) {
                  std::rethrow_exception(exc);
                }
                return value;
              });
    }
    promise_type(const promise_type&) = delete;
    stlab::future<T> get_return_object() { return future; }
    std::experimental::suspend_never initial_suspend() { return {}; }
    std::experimental::suspend_never final_suspend() { return {}; }
    void unhandled_exception() {
      T* invalid = nullptr;
      promise(std::current_exception(), *invalid);
    }
    void return_value(T&& u) { promise(nullptr, u); }
  };
};
template <typename... Args>
struct std::experimental::coroutine_traits<stlab::future<void>, Args...> {
  struct promise_type {
    stlab::future<void> future;
    stlab::packaged_task<std::exception_ptr> promise;
    promise_type() {
      std::tie(promise, future) =
          stlab::package<void(std::exception_ptr)>(
              stlab::immediate_executor,
              [](std::exception_ptr exc) {
                if (exc != nullptr) {
                  std::rethrow_exception(exc);
                }
              });
    }
    promise_type(const promise_type&) = delete;
    stlab::future<void> get_return_object() { return future; }
    std::experimental::suspend_never initial_suspend() { return {}; }
    std::experimental::suspend_never final_suspend() { return {}; }
    void unhandled_exception() {
      promise(std::current_exception());
    }
    void return_void() { promise(nullptr); }
  };
};

template <typename T>
auto operator co_await(stlab::future<T> f) {
  struct Awaiter {
    stlab::future<T>&& input;
    bool await_ready() { return input.is_ready(); }
    T await_resume() { return *input.get_try(); }
    void await_suspend(std::experimental::coroutine_handle<> coro) {
		auto shared_coro = std::make_shared<std::experimental::coroutine_handle<>>(coro);
      input.recover([shared_coro](stlab::future<T> f) { shared_coro->resume(); })
          .detach();
    }
  };
  return Awaiter{std::move(f)};
}

template <>
inline auto operator co_await(stlab::future<void> f) {
  struct Awaiter {
    stlab::future<void>&& input;
    bool await_ready() { return input.is_ready(); }
    void await_resume() { input.get_try(); }
    void await_suspend(std::experimental::coroutine_handle<> coro) {
      input.recover([coro](stlab::future<void> f) mutable { coro.resume(); })
          .detach();
    }
  };
  return Awaiter{std::move(f)};
}
#endif//_COROUTINES_H_

#ifndef _ASIO_EXECUTOR_H_
#define _ASIO_EXECUTOR_H_
#include <boost/asio.hpp>
#include <stlab/concurrency/future.hpp>

class AsioExecutor {
 public:
  AsioExecutor(boost::asio::io_service& io_service);
  ~AsioExecutor() = default;

  template <typename F>
  typename std::enable_if<std::is_copy_constructible<F>::value>::type
  operator()(F&& f) const {
    io_service_.post(std::forward<F>(f));
  }

  template <typename F>
  typename std::enable_if<!std::is_copy_constructible<F>::value>::type
  operator()(F&& f) const {
    // F is not copy constructible, so any lambda containing it isn't either.
    // io_service.post, however, requires a copy constructor.
    // So we're forced to use a shared pointer here :(
    auto copyable_f = std::make_shared<F>(std::forward<F>(f));
    io_service_.post([copyable_f{std::move(copyable_f)}]() {
      (*copyable_f)();
    });
  }

  template <typename T>
  static stlab::future<T> make_exceptional_future(
      boost::asio::io_service& io_service, std::exception_ptr error) {
    auto package = stlab::package<T(std::exception_ptr error)>(
        AsioExecutor(io_service), [](std::exception_ptr error) {
          std::rethrow_exception(error);
          // As we don't have any T, and
          // don't have the guarantee that T
          // has a default contructor, just
          // dereference a null-pointer. This
          // is safe as we're throwing an
          // exception on the previous line
          // ;)
          return *(T*)nullptr;
        });
    package.first(error);
    return std::move(package.second);
  }
  template <typename T>
  static stlab::future<T> make_ready_future(boost::asio::io_service& io_service,
                                            T&& x) {
    auto package = stlab::package<T(T && x)>(AsioExecutor(io_service),
                                             [](T&& x) { return x; });
    package.first(std::move(x));
    return std::move(package.second);
  }

 private:
  boost::asio::io_service& io_service_;
};
#endif  //_ASIO_EXECUTOR_H_

#include "coro.h"
#include <boost/asio.hpp>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <stlab/concurrency/default_executor.hpp>
#include "asio_executor.h"

template <typename T>
stlab::future<T> WrapInFuture(T value) {
  auto package = stlab::package<T(T)>(stlab::default_executor,
                                      [](T value) { return value; });
  package.first(std::move(value));
  return std::move(package.second);
}

stlab::future<void> WrapVoid() {
  auto package = stlab::package<void()>(stlab::default_executor, []() {});
  package.first();
  return std::move(package.second);
}

template <typename T>
T SimpleCoro(coro::Await await, T input) {
  auto void_f = WrapVoid();
  await(void_f);
  auto f = WrapInFuture<T>(std::move(input));
  auto r = await(std::move(f));
  return r;
}

template <typename T>
void VoidCoro(coro::Await await, T input, T& output) {
  auto f = WrapInFuture<T>(std::move(input));
  auto r = await(std::move(f));
  output = std::move(r);
}

BOOST_AUTO_TEST_CASE(Integers) {
  boost::asio::io_service io_service;
  boost::asio::io_service::work work(io_service);

  int result = 0;
  auto f = coro::Run(AsioExecutor(io_service), SimpleCoro<int>, 123);
  f.recover([&io_service, &result](auto f) {
     try {
       result = *f.get_try();
     } catch (const std::exception& e) {
       std::cerr << "Test failed, exception: " << e.what() << std::endl;
     }
     io_service.stop();
   })
      .detach();
  io_service.run();
  BOOST_TEST(result == 123);
}

BOOST_AUTO_TEST_CASE(Movables) {
  boost::asio::io_service io_service;
  boost::asio::io_service::work work(io_service);

  std::unique_ptr<int> result;
  auto f = coro::Run(AsioExecutor(io_service), SimpleCoro<std::unique_ptr<int>>,
                     std::make_unique<int>(123));
  std::move(f)
      .recover([&io_service, &result](auto f) {
        try {
          result = *f.get_try();
        } catch (const std::exception& e) {
          std::cerr << "Test failed, exception: " << e.what() << std::endl;
        }
        io_service.stop();
      })
      .detach();
  io_service.run();
  BOOST_TEST(*result == 123);
}

BOOST_AUTO_TEST_CASE(VoidRetval) {
  boost::asio::io_service io_service;
  boost::asio::io_service::work work(io_service);

  int result;
  auto f =
      coro::Run(AsioExecutor(io_service), VoidCoro<int>, 123, std::ref(result));
  std::move(f)
      .recover([&io_service](auto f) {
        try {
          if (!f.get_try()) {
            throw std::runtime_error("Future was not resolved at all");
          }
        } catch (const std::exception& e) {
          std::cerr << "Test failed, exception: " << e.what() << std::endl;
        }
        io_service.stop();
      })
      .detach();
  io_service.run();
  BOOST_TEST(result == 123);
}

BOOST_AUTO_TEST_CASE(MoveFunction) {
  boost::asio::io_service io_service;
  boost::asio::io_service::work work(io_service);

  int result;
  int counter = 0;
  auto f = coro::Run(AsioExecutor(io_service),
                     [&result, counter](coro::Await await) mutable {
                       counter++;
                       VoidCoro<int>(await, 123, result);
                     });
  std::move(f)
      .recover([&io_service](auto f) {
        try {
          if (!f.get_try()) {
            throw std::runtime_error("Future was not resolved at all");
          }
        } catch (const std::exception& e) {
          std::cerr << "Test failed, exception: " << e.what() << std::endl;
        }
        io_service.stop();
      })
      .detach();
  io_service.run();
  BOOST_TEST(result == 123);
}

class SillyString {
 public:
  SillyString(std::string data) : data_(data) {}
  SillyString(const SillyString&) = delete;
  SillyString(SillyString&& move) : data_(move.data_) {}
  // Ideally, I'd like to remove this one too, but apparently stlab doesn't like
  // that.
  SillyString& operator=(SillyString&& move) {
    data_ = move.data_;
    return *this;
  }
  std::string data_;
};
BOOST_AUTO_TEST_CASE(NonDefaultConstructible) {
  boost::asio::io_service io_service;
  boost::asio::io_service::work work(io_service);

  std::string result = "";
  auto f = coro::Run(AsioExecutor(io_service), SimpleCoro<SillyString>,
                     SillyString("Hello world!"));
  std::move(f)
      .recover([&io_service, &result](auto f) {
        try {
          result = std::move(f).get_try()->data_;
        } catch (const std::exception& e) {
          std::cerr << "Test failed, exception: " << e.what() << std::endl;
        }
        io_service.stop();
      })
      .detach();
  io_service.run();
  BOOST_TEST(result == "Hello world!");
}

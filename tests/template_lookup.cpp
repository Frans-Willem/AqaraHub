#include <template_lookup.h>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <map>
#include <memory>

enum class Numbers { One, Two, Three };

class IHandler {
 public:
  virtual int GetNumber() = 0;
};

template <Numbers N>
class CHandler : public IHandler {
 public:
  virtual int GetNumber() override {
    throw std::runtime_error("Number not implemented");
  }
};

template <>
class CHandler<Numbers::One> : public IHandler {
 public:
  virtual int GetNumber() override { return 1; }
};

template <>
class CHandler<Numbers::Two> : public IHandler {
 public:
  virtual int GetNumber() override { return 2; }
};
template <>
class CHandler<Numbers::Three> : public IHandler {
 public:
  virtual int GetNumber() override { return 3; }
};

struct HandlerFactory {
  template <Numbers N>
  static std::unique_ptr<IHandler> Create() {
    return std::move(std::make_unique<CHandler<N>>());
  }
};

BOOST_AUTO_TEST_CASE(TemplateMagic) {
  std::map<Numbers, std::unique_ptr<IHandler>> handlers =
      template_lookup::CreateEnumLookup<Numbers, Numbers::One, Numbers::Three,
                                        IHandler, CHandler>();
  BOOST_TEST(handlers[Numbers::One]->GetNumber() == 1);
  BOOST_TEST(handlers[Numbers::Two]->GetNumber() == 2);
  BOOST_TEST(handlers[Numbers::Three]->GetNumber() == 3);
}

#include <mqtt_wrapper.h>
#include <boost/optional/optional_io.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(FullExample) {
  std::string uri(
      "mqtts://username:password@example.com:1234/?clientid=foobar");
  auto result = MqttWrapper::ParseUrl(uri);
  MqttWrapper::Parameters expected{true,
                                   false,
                                   "example.com",
                                   std::string("1234"),
                                   std::string("username"),
                                   std::string("password"),
                                   std::string("foobar")};
  BOOST_TEST(result == expected);
}

BOOST_AUTO_TEST_CASE(MinimalExample) {
  std::string uri("ws://example.com");
  auto result = MqttWrapper::ParseUrl(uri);
  MqttWrapper::Parameters expected{
      false,       true,        "example.com", boost::none,
      boost::none, boost::none, boost::none,
  };
  BOOST_TEST(result == expected);
}

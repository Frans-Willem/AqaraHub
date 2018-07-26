#include <boost/test/unit_test.hpp>
#include <uri_parser.h>
#include <boost/optional/optional_io.hpp>

BOOST_AUTO_TEST_CASE(FooExample) {
  std::string uri("foo://example.com:8042/over/there?name=ferret#nose");
  URI expected { "foo", {boost::none, "example.com", std::string("8042")}, "/over/there", std::string("name=ferret"), std::string("nose") };
  auto result = ParseURI(uri);
  BOOST_TEST(expected == result);
}

BOOST_AUTO_TEST_CASE(RegExpExample) {
  std::string uri("http://www.ics.uci.edu/pub/ietf/uri/#Related");
  URI expected { "http", {boost::none, "www.ics.uci.edu", boost::none}, "/pub/ietf/uri/", boost::none, std::string("Related") };
  auto result = ParseURI(uri);
  BOOST_TEST(expected == result);
}

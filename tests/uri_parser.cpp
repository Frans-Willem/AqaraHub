#include <uri_parser.h>
#include <boost/optional/optional_io.hpp>
#include <boost/test/unit_test.hpp>
#include <map>

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

BOOST_AUTO_TEST_CASE(URIUnescapeEntries) {
  std::map<std::string, std::string> examples{
      {"Hello%20World", "Hello World"},
      {"Multiple%20spaces%20in%20one%20string",
       "Multiple spaces in one string"},
  };
  for (const auto& entry : examples) {
    std::string decoded(URIUnescape(entry.first));
    BOOST_TEST(decoded == entry.second);
  }
}

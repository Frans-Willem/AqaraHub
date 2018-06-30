#include <dynamic_encoding/decoding.h>
#include <dynamic_encoding/encoding.h>
#include <boost/format.hpp>
#include <boost/log/utility/manipulators/dump.hpp>
#include <boost/test/unit_test.hpp>

namespace {
std::vector<std::tuple<std::vector<uint8_t>, dynamic_encoding::AnyType,
                       tao::json::value>>
    examples{
        {{0xFF}, zcl::DataType::_bool, tao::json::null},
        {{0x01}, zcl::DataType::_bool, true},
        {{0x00}, zcl::DataType::_bool, false},
        {{0x10, 0x01},
         dynamic_encoding::VariantType{},
         tao::json::value::object_t{{"type", "bool"}, {"value", true}}},
        {{0x23},
         zcl::DataType::map8,
         std::vector<bool>{false, false, true, false, false, false, true,
                           true}},
        {{0x78, 0x56, 0x34, 0x12}, zcl::DataType::int32, 0x12345678},
        {{0x88, 0xA9, 0xCB, 0xED}, zcl::DataType::int32, -0x12345678},
        {{0x78, 0x56, 0x34, 0x12}, zcl::DataType::uint32, 0x12345678},
        {{0x88, 0xA9, 0xCB, 0xED}, zcl::DataType::uint32, 0xEDCBA988L},
        {{0xB6, 0x57}, zcl::DataType::semi, 123.375f},
        {{0xFA, 0x3E, 0xF6, 0x42}, zcl::DataType::single, 123.123f},
        {{0x1D, 0x5A, 0x64, 0x3B, 0xDF, 0xC7, 0x5E, 0x40},
         zcl::DataType::_double,
         123.123},
        {{0x01, 0x02, 0x03},
         zcl::DataType::data24,
         tao::json::value::array_t{1, 2, 3}},
        {{0x00, 0x02}, zcl::DataType::semi, std::pow(2, -15)},
        {{0x00, 0x00, 0x80, 0x7F}, zcl::DataType::single, INFINITY},
        {{0x0B, 0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x77, 0x6F, 0x72, 0x6C,
          0x64},
         zcl::DataType::string,
         "Hello world"},
        {{0x0B, 0x00, 0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x77, 0x6F, 0x72,
          0x6C, 0x64},
         zcl::DataType::string16,
         "Hello world"},
        {{0xFF}, zcl::DataType::string, tao::json::null},
        {{0xFF, 0xFF}, zcl::DataType::string16, tao::json::null},
        {{0x10, 0x03, 0x00, 0xFF, 0x00, 0x01},
         zcl::DataType::array,
         tao::json::value::object_t{
             {"element_type", "bool"},
             {"elements",
              tao::json::value::array_t{tao::json::null, false, true}},
         }},
        {{0x03, 0x00, 0x10, 0xFF, 0x10, 0x00, 0x10, 0x01},
         zcl::DataType::_struct,
         tao::json::value::array_t{
             tao::json::value::object_t{{"type", "bool"},
                                        {"value", tao::json::null}},
             tao::json::value::object_t{{"type", "bool"}, {"value", false}},
             tao::json::value::object_t{{"type", "bool"}, {"value", true}},
         }},
    };
}

BOOST_AUTO_TEST_CASE(DecodeExamples) {
  dynamic_encoding::Context ctx;
  for (const auto& example : examples) {
    const auto& encoded_data = std::get<0>(example);
    const auto& type = std::get<1>(example);
    const auto& json = std::get<2>(example);
    auto parsed_until = encoded_data.cbegin();
    auto rejson =
        dynamic_encoding::Decode(ctx, type, parsed_until, encoded_data.cend());
    BOOST_TEST((parsed_until == encoded_data.cend()) == true);
    BOOST_TEST(rejson == json);
  }
}

BOOST_AUTO_TEST_CASE(EncodeExamples) {
  dynamic_encoding::Context ctx;
  for (const auto& example : examples) {
    const auto& encoded_data = std::get<0>(example);
    const auto& type = std::get<1>(example);
    const auto& json = std::get<2>(example);
    std::vector<uint8_t> reencoded;
    dynamic_encoding::Encode(ctx, type, json, reencoded);
    if (reencoded != encoded_data) {
      std::cout << "Expected:";
      for (const auto& x : encoded_data) {
        std::cout << boost::str(boost::format(" %02X") % (unsigned int)x);
      }
      std::cout << std::endl;
      std::cout << "Encoded :";
      for (const auto& x : reencoded) {
        std::cout << boost::str(boost::format(", 0x%02X") % (unsigned int)x);
      }
      std::cout << std::endl;
    }
    BOOST_TEST(reencoded == encoded_data);
  }
}

BOOST_AUTO_TEST_CASE(DecodeEncodeRoundtrips) {
  dynamic_encoding::Context ctx;
  for (const auto& example : examples) {
    const auto& encoded_data = std::get<0>(example);
    const auto& type = std::get<1>(example);
    auto parsed_until = encoded_data.cbegin();
    auto json =
        dynamic_encoding::Decode(ctx, type, parsed_until, encoded_data.cend());
    BOOST_TEST((parsed_until == encoded_data.cend()) == true);
    std::vector<uint8_t> reencoded;
    dynamic_encoding::Encode(ctx, type, json, reencoded);
    BOOST_TEST(reencoded == encoded_data);
  }
}

BOOST_AUTO_TEST_CASE(EncodeDecodeRoundtrips) {
  dynamic_encoding::Context ctx;
  for (const auto& example : examples) {
    const auto& type = std::get<1>(example);
    const auto& json = std::get<2>(example);
    std::vector<uint8_t> encoded_data;
    dynamic_encoding::Encode(ctx, type, json, encoded_data);
    auto parsed_until = encoded_data.cbegin();
    auto rejson =
        dynamic_encoding::Decode(ctx, type, parsed_until, encoded_data.cend());
    BOOST_TEST((parsed_until == encoded_data.cend()) == true);
    BOOST_TEST(json == rejson);
  }
}

#include <zcl/encoding.h>
#include <zcl/zcl.h>
#include <boost/log/utility/manipulators/dump.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(VariantEncodeIdentity) {
  std::vector<zcl::ZclVariant> variants{
      zcl::ZclVariant::Create<zcl::DataType::nodata>(),
      zcl::ZclVariant::Create<zcl::DataType::_bool>(),
      zcl::ZclVariant::Create<zcl::DataType::_bool>(true),
      zcl::ZclVariant::Create<zcl::DataType::_bool>(false),
      zcl::ZclVariant::Create<zcl::DataType::map8>(std::bitset<8>(0x45)),
      zcl::ZclVariant::Create<zcl::DataType::map32>(
          std::bitset<32>(0x12345678)),
      zcl::ZclVariant::Create<zcl::DataType::string>("Hello world"),
      zcl::ZclVariant::Create<zcl::DataType::semi>(123.375f),
      zcl::ZclVariant::Create<zcl::DataType::single>(123.456f),
      zcl::ZclVariant::Create<zcl::DataType::_double>(123.456),
      zcl::ZclVariant::Create<zcl::DataType::single>(-3.0f),
      zcl::ZclVariant::Create<zcl::DataType::single>(
          INFINITY),  // Special type :)
      zcl::ZclVariant::Create<zcl::DataType::semi>(
          std::pow(2, -15)),  // Denormalized float
      zcl::ZclVariant::Create<zcl::DataType::data24>(
          std::array<std::uint8_t, 3>{{1, 2, 3}})

  };

  for (const auto& variant : variants) {
    auto encoded = znp::Encode(variant);
    auto decoded = znp::Decode<zcl::ZclVariant>(encoded);
    BOOST_TEST(variant == decoded);
  }
}

BOOST_AUTO_TEST_CASE(VariantDecodeExamples) {
  std::map<std::vector<uint8_t>, zcl::ZclVariant> examples{
      {{0x10, 0xFF}, zcl::ZclVariant::Create<zcl::DataType::_bool>()},
      {{0x10, 0x01}, zcl::ZclVariant::Create<zcl::DataType::_bool>(true)},
      {{0x10, 0x00}, zcl::ZclVariant::Create<zcl::DataType::_bool>(false)},
      {{0x18, 0x23},
       zcl::ZclVariant::Create<zcl::DataType::map8>(std::bitset<8>(0x23))},
      {{0x1b, 0x78, 0x56, 0x34, 0x12},
       zcl::ZclVariant::Create<zcl::DataType::map32>(
           std::bitset<32>(0x12345678))},
      {{0x2b, 0x78, 0x56, 0x34, 0x12},
       zcl::ZclVariant::Create<zcl::DataType::int32>(0x12345678)},
      {{0x2b, 0x88, 0xA9, 0xCB, 0xED},
       zcl::ZclVariant::Create<zcl::DataType::int32>(-0x12345678)},
      {{0x23, 0x78, 0x56, 0x34, 0x12},
       zcl::ZclVariant::Create<zcl::DataType::uint32>(0x12345678)},
      {{0x23, 0x88, 0xA9, 0xCB, 0xED},
       zcl::ZclVariant::Create<zcl::DataType::uint32>(0xEDCBA988)}};
  for (const auto& example : examples) {
    auto decoded = znp::Decode<zcl::ZclVariant>(example.first);
    auto recoded = znp::Encode<zcl::ZclVariant>(decoded);
    BOOST_TEST(decoded == example.second,
               "Decoding of [" << boost::log::dump(example.first.data(),
                                                   example.first.size())
                               << "] failed");
    BOOST_TEST(example.first == recoded,
               "Re-encoding of [" << boost::log::dump(example.first.data(),
                                                      example.first.size())
                                  << "] failed");
  }
}

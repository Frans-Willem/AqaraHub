#include <zcl/to_json.h>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(VariantFromToJson) {
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
      // zcl::ZclVariant::Create<zcl::DataType::single>(INFINITY),  // Special
      // type :)
      zcl::ZclVariant::Create<zcl::DataType::semi>(
          std::pow(2, -15)),  // Denormalized float
      zcl::ZclVariant::Create<zcl::DataType::data24>(
          std::array<std::uint8_t, 3>{1, 2, 3})

  };

  for (const auto& variant : variants) {
    auto encoded = zcl::to_json(variant);
    auto decoded = zcl::from_json(encoded);
    BOOST_TEST(variant == decoded);
  }
}

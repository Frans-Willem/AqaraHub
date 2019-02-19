#include <clusterdb/cluster_db.h>
#include <dynamic_encoding/decoding.h>
#include <zcl/encoding.h>
#include <boost/optional/optional_io.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <sstream>
#include <tao/json.hpp>

using namespace clusterdb;

void LoadTestDb(ClusterDb& db) {
  std::string test_db =
      "\
global commands\n\
{\n\
	0x01 \"Read Attributes Response\"\n\
	{\n\
		repeated:object \"records\"\n\
		{\n\
			attribId \"Attribute identifier\"\n\
			error_or:variant \"Attribute value\"\n\
		}\n\
	}\n\
	0x0A \"Report attributes\"\n\
	{\n\
		repeated:object \"report\"\n\
		{\n\
			attribId \"Attribute identifier\"\n\
			variant \"data\"\n\
		}\n\
	}\n\
}\n\
0x0000 \"Basic\"\n\
{\n\
	attributes\n\
	{\n\
		0x0000 \"ZCLVersion\"\n\
		0x0001 \"ApplicationVersion\"\n\
	}\n\
}\n\
0x0001 \"Power Configuration\"\n\
0x0002 \"Device Temperature Configuration\"\n\
0x0006 \"On/Off\"\n\
{\n\
	attributes\n\
	{\n\
		0x0000 \"OnOff\"\n\
		{\n\
			type bool\n\
		}\n\
	}\n\
}\n\
";
  std::stringstream stream(test_db);
  if (!db.ParseFromStream(stream, [](std::string x) { return x; })) {
    BOOST_FAIL("ParseFromStream failed");
  }
}

BOOST_AUTO_TEST_CASE(ClusterByName) {
  ClusterDb db;
  LoadTestDb(db);

  auto found = db.ClusterByName("Power Configuration");
  BOOST_TEST(!!found);
  BOOST_TEST((unsigned int)found->id == 0x0001);
}

BOOST_AUTO_TEST_CASE(ClusterById) {
  ClusterDb db;
  LoadTestDb(db);

  auto found = db.ClusterById((zcl::ZclClusterId)0x0001);
  BOOST_TEST(!!found);
  BOOST_TEST(found->name == "Power Configuration");
}

BOOST_AUTO_TEST_CASE(AttributeById) {
  ClusterDb db;
  LoadTestDb(db);

  auto cluster_found = db.ClusterById((zcl::ZclClusterId)0x0000);
  BOOST_TEST(!!cluster_found);
  auto attribute_found =
      cluster_found->attributes.FindById((zcl::ZclAttributeId)0x0001);
  BOOST_TEST(!!attribute_found);
  BOOST_TEST(attribute_found->name == "ApplicationVersion");
}

BOOST_AUTO_TEST_CASE(ReportAttributesArguments) {
  ClusterDb db;
  LoadTestDb(db);

  auto command_found = db.GlobalCommandById((zcl::ZclCommandId)0x0A);
  BOOST_TEST(!!command_found);
  BOOST_TEST(command_found->data.properties.size() == 1);
  auto repeated_type = boost::strict_get<dynamic_encoding::ArrayType>(
      command_found->data.properties[0].type);
  auto repeated_element_type = boost::strict_get<dynamic_encoding::ObjectType>(
      repeated_type.element_type);
  BOOST_TEST(repeated_type.length_size == 0);
  BOOST_TEST(repeated_element_type.properties.size() == 2);
  BOOST_TEST((repeated_element_type.properties[0].type ==
              dynamic_encoding::AnyType(zcl::DataType::attribId)) == true);
  BOOST_TEST((repeated_element_type.properties[1].type ==
              dynamic_encoding::AnyType(dynamic_encoding::VariantType{})) ==
             true);
}

BOOST_AUTO_TEST_CASE(DecodeOnOffAttribute) {
  ClusterDb db;
  LoadTestDb(db);

  auto onoff_cluster = db.ClusterById((zcl::ZclClusterId)0x0006);  // On/Off
  auto report_command =
      db.GlobalCommandById((zcl::ZclCommandId)0x0A);  // Report attributes

  std::vector<uint8_t> data{0x00, 0x00, 0x10, 0x01, 0x00, 0x00, 0x10, 0x00};

  tao::json::value expected(tao::json::value::object_t{
      {"report",
       tao::json::value::array_t{
           tao::json::value::object_t{
               {"Attribute identifier", "OnOff"},
               {"data",
                tao::json::value::object_t{{"type", "bool"}, {"value", true}}}},
           tao::json::value::object_t{
               {"Attribute identifier", "OnOff"},
               {"data", tao::json::value::object_t{{"type", "bool"},
                                                   {"value", false}}}},
       }}});

  auto parsed_until = data.cbegin();
  dynamic_encoding::Context ctx{*onoff_cluster};
  tao::json::value result = dynamic_encoding::Decode(ctx, report_command->data,
                                                     parsed_until, data.cend());

  BOOST_TEST(!!(parsed_until == data.cend()));
  BOOST_TEST(result == expected);
}

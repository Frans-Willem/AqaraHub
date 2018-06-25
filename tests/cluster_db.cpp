#include <clusterdb/cluster_db.h>
#include <boost/optional/optional_io.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <sstream>

using namespace clusterdb;

void LoadTestDb(ClusterDb& db) {
  std::string test_db =
      "\
global commands\n\
{\n\
	0x0A \"Report attributes\"\n\
	{\n\
		repeated \"attributes\"\n\
		{\n\
			attribute \"Attribute identifier\"\n\
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
  BOOST_TEST(command_found->arguments.size() == 1);
  auto repeated_argument =
      boost::strict_get<RepeatedArgumentType>(command_found->arguments[0].type);
  BOOST_TEST(repeated_argument.contents.size() == 2);
  BOOST_TEST((boost::strict_get<SimpleArgumentType>(
                  repeated_argument.contents[0].type) ==
              SimpleArgumentType::Attribute) == true,
             "First argument to Report Attributes is not attribute.");
  BOOST_TEST((boost::strict_get<SimpleArgumentType>(
                  repeated_argument.contents[1].type) ==
              SimpleArgumentType::Variant) == true,
             "Second argument to Report Attributes is not variant.");
}

#include "xiaomi/ff01_attribute.h"
#include "zcl/encoding.h"
#include "logging.h"
#include "zcl/to_json.h"
#include <boost/log/utility/manipulators/dump.hpp>

namespace xiaomi {
boost::optional<std::map<uint8_t, zcl::ZclVariant>> DecodeFF01Attribute(zcl::ZclClusterId cluster_id, zcl::ZclAttributeId attribute_id, const zcl::ZclVariant& current_value) {
	if (cluster_id != (zcl::ZclClusterId)0x0000 /* Basic */ || attribute_id != (zcl::ZclAttributeId)0xFF01) {
		return boost::none;
	}
	boost::optional<std::string> str_data(current_value.Get<zcl::DataType::string>());
	if (!str_data) {
		return boost::none;
	}
	std::vector<uint8_t> vec_data(str_data->begin(), str_data->end());
	LOG("FF01", info) << "FF01: " << boost::log::dump(vec_data.data(), vec_data.size());
	std::map<uint8_t, zcl::ZclVariant> retval;
	try {
		std::vector<uint8_t>::const_iterator current_data = vec_data.begin();
		while (current_data != vec_data.end()) {
			std::tuple<uint8_t, zcl::ZclVariant> item;
			znp::EncodeHelper<std::tuple<uint8_t, zcl::ZclVariant>>::Decode(item, current_data, vec_data.end());
			retval[std::get<0>(item)] = std::get<1>(item);
		}

	} catch (const std::exception& ex) {
		LOG("FF01", warning) << "Exception while decoding FF01: " << ex.what();
	}

	return retval;
}

tao::json::value FF01AttributeToJson(const std::map<uint8_t, zcl::ZclVariant>& value) {
	tao::json::value retval(tao::json::empty_object);
	for (const auto& item : value) {
		retval += {{boost::str(boost::format("%d") % (unsigned int)item.first), zcl::to_json(item.second, true)}};
	}

	return {{"type", "xiaomi_ff01"}, {"value", retval}};
}

}

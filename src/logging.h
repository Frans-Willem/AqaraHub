#ifndef _LOGGING_H_
#define _LOGGING_H_
#include <boost/log/common.hpp>
#include <boost/log/sources/channel_logger.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/severity_logger.hpp>

enum severity_level { trace, debug, info, warning, error, critical };

std::ostream& operator<<(std::ostream& stream, const severity_level& level);

typedef boost::log::sources::severity_channel_logger_mt<severity_level,
                                                        std::string>
    my_logger_mt;

BOOST_LOG_INLINE_GLOBAL_LOGGER_CTOR_ARGS(
    main_logger, my_logger_mt,
    (boost::log::keywords::severity = debug)(
        boost::log::keywords::channel = "main"))

#define LOG(channel, severity) \
  BOOST_LOG_CHANNEL_SEV(main_logger::get(), channel, severity)
#endif  // _LOGGING_H_

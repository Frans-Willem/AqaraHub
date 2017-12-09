#include "asio_executor.h"

AsioExecutor::AsioExecutor(boost::asio::io_service& io_service) : io_service_(io_service) {}

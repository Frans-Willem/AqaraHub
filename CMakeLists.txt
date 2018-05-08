cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

project(AqaraHub VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

if(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
	include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
	conan_basic_setup(TARGETS)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Werror")
set(CMAKE_CXX_STANDARD 14)

find_package(Threads REQUIRED)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.60.0 COMPONENTS system program_options log coroutine unit_test_framework REQUIRED)

# STLab library
find_path(STLAB_INCLUDE_DIRS NAMES stlab/version.hpp PATHS ${CMAKE_SOURCE_DIR}/submodules/stlab-libraries/)
if(NOT STLAB_INCLUDE_DIRS)
	message(FATAL_ERROR "STLab Libraries not found")
else()
	message(STATUS "STLab Libraries found at: ${STLAB_INCLUDE_DIRS}")
endif()
add_library(stlab INTERFACE)
target_include_directories(stlab INTERFACE ${STLAB_INCLUDE_DIRS})

# MQTT_CPP
find_path(MQTT_CPP_INCLUDE_DIRS NAMES mqtt/client.hpp PATHS ${CMAKE_SOURCE_DIR}/submodules/mqtt_cpp/include)
if(NOT MQTT_CPP_INCLUDE_DIRS)
	message(FATAL_ERROR "MQTT_CPP not found")
else()
	message(STATUS "MQTT_CPP found at: ${MQTT_CPP_INCLUDE_DIRS}")
endif()
add_library(mqtt_cpp INTERFACE)
target_include_directories(mqtt_cpp INTERFACE ${MQTT_CPP_INCLUDE_DIRS})
find_package(OpenSSL REQUIRED)
target_link_libraries(mqtt_cpp INTERFACE OpenSSL::SSL)

find_path(TAO_JSON_INCLUDE_DIRS NAMES tao/json.hpp PATHS ${CMAKE_SOURCE_DIR}/submodules/taocpp-json/include)
if(NOT TAO_JSON_INCLUDE_DIRS)
	message(FATAL_ERROR "taocpp-json not found")
else()
	message(STATUS "taocpp-json found at: ${TAO_JSON_INCLUDE_DIRS}")
endif()
add_library(taocpp_json INTERFACE)
target_include_directories(taocpp_json INTERFACE ${TAO_JSON_INCLUDE_DIRS})
add_library(taocpp::json ALIAS taocpp_json)

# std::auto_ptr doesn't exist in C++17, so disable it just in case.
add_definitions(-DBOOST_NO_AUTO_PTR)

add_library(common
	src/znp/znp.cpp
	src/znp/znp_port.cpp
	src/znp/znp_api.cpp
	src/asio_executor.cpp
	src/logging.cpp
	src/zcl/zcl.cpp
    src/coro.cpp
	src/zcl/name_registry.cpp
	src/zcl/zcl_endpoint.cpp
	src/mqtt_wrapper.cpp
	src/xiaomi/ff01_attribute.cpp
	src/zcl/to_json.cpp
	src/zcl/from_json.cpp
	src/zcl/encoding.cpp
	)
target_include_directories(common PUBLIC "src")
target_link_libraries(common stlab)
target_link_libraries(common mqtt_cpp)
target_link_libraries(common Boost::boost)
target_link_libraries(common Boost::system)
target_link_libraries(common Boost::log)
target_link_libraries(common Boost::program_options)
target_link_libraries(common Boost::coroutine)
target_link_libraries(common Threads::Threads)
target_link_libraries(common taocpp::json)

add_executable(AqaraHub
	src/main.cpp
	)
target_include_directories(AqaraHub PUBLIC "src")
target_link_libraries(AqaraHub common)

install(TARGETS AqaraHub
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES attributes.info DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/AqaraHub/)
install(FILES AqaraHub.service DESTINATION ${CMAKE_INSTALL_LIBDIR}/systemd/system/)

add_executable(tests
	tests/main.cpp
    tests/coro.cpp
	tests/template_lookup.cpp
	tests/variant_encoding.cpp
)
target_link_libraries(tests common)
target_include_directories(tests PUBLIC "src")
target_include_directories(tests PUBLIC "include")
#target_compile_definitions(tests PUBLIC -DBOOST_TEST_DYN_LINK)
target_link_libraries(tests Boost::unit_test_framework)

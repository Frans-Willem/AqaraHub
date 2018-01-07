cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

project(AqaraHub VERSION 0.1.0 LANGUAGES CXX)

if(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
	include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
	conan_basic_setup(TARGETS)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fcoroutines-ts -stdlib=libc++")

find_package(Threads REQUIRED)
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.60.0 COMPONENTS system program_options log REQUIRED)

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

# std::auto_ptr doesn't exist in C++17, so disable it just in case.
add_definitions(-DBOOST_NO_AUTO_PTR)

add_executable(AqaraHub
	src/main.cpp
	src/znp/znp.cpp
	src/znp/znp_port.cpp
	src/znp/znp_api.cpp
	src/asio_executor.cpp
	src/logging.cpp
	src/zcl/zcl.cpp
	)
target_include_directories(AqaraHub PUBLIC "src")
target_link_libraries(AqaraHub stlab)
target_link_libraries(AqaraHub mqtt_cpp)
target_link_libraries(AqaraHub Boost::boost)
target_link_libraries(AqaraHub Boost::system)
target_link_libraries(AqaraHub Boost::log)
target_link_libraries(AqaraHub Boost::program_options)
target_link_libraries(AqaraHub Threads::Threads)

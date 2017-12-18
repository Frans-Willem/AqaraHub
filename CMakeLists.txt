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

find_path(STLAB_INCLUDE_DIRS NAMES version.hpp PATH_SUFFIXES stlab)
if(NOT STLAB_INCLUDE_DIRS)
	message(FATAL_ERROR "STLab not found")
endif()
add_library(stlab INTERFACE)
target_include_directories(stlab INTERFACE ${STLAB_INCLUDE_DIRS})

# std::auto_ptr doesn't exist in C++17, so disable it just in case.
add_definitions(-DBOOST_NO_AUTO_PTR)

add_executable(AqaraHub
	src/main.cpp
	src/znp/znp.cpp
	src/znp/znp_port.cpp
	src/znp/znp_api.cpp
	src/znp/znp_sreq_handler.cpp
	src/znp/af/af_handler.cpp
	src/asio_executor.cpp
	src/logging.cpp
	)
target_include_directories(AqaraHub PUBLIC "src")
target_link_libraries(AqaraHub stlab)
target_link_libraries(AqaraHub Boost::boost)
target_link_libraries(AqaraHub Boost::system)
target_link_libraries(AqaraHub Boost::log)
target_link_libraries(AqaraHub Threads::Threads)

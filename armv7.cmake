set (CMAKE_SYSTEM_NAME Linux)
set (CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set (CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
set (ENV{PKG_CONFIG_PATH} /opt/arm/lib/pkgconfig)

set (CMAKE_FIND_ROOT_PATH /opt/arm/lib /opt/arm/include)
set (THREADS_PREFER_PTHREAD_FLAG On) #https://gitlab.kitware.com/cmake/cmake/issues/16540
set (BOOST_ROOT /usr/src/boost_1_69_0/)
set (BOOST_INCLUDEDIR /opt/arm/include/boost)
set (OPENSSL_INCLUDE_DIR /opt/arm/include)
#set (OPENSSL_CRYPTO_LIBRARY /opt/arm/lib)

FROM debian:buster-slim
RUN apt update && apt install -y --no-install-recommends curl tar xz-utils cmake make git perl bzip2 pkg-config build-essential
RUN curl -kL http://releases.linaro.org/components/toolchain/binaries/6.5-2018.12/arm-linux-gnueabihf/gcc-linaro-6.5.0-2018.12-x86_64_arm-linux-gnueabihf.tar.xz | tar -Jxf -

ENV PATH="/gcc-linaro-6.5.0-2018.12-x86_64_arm-linux-gnueabihf/bin/:${PATH}"
RUN mkdir -p /usr/src/
WORKDIR /usr/src

RUN curl -kL https://www.openssl.org/source/openssl-1.1.1a.tar.gz | tar -zxf -
RUN curl -kL https://dl.bintray.com/boostorg/release/1.69.0/source/boost_1_69_0.tar.bz2 | tar -jxf -
RUN GIT_SSL_NO_VERIFY=1 git clone --recursive https://github.com/Frans-Willem/AqaraHub

WORKDIR /usr/src/openssl-1.1.1a
RUN CROSS_COMPILE=arm-linux-gnueabihf- ./Configure linux-armv4 --prefix=/opt/arm/
RUN make -j`nproc`
RUN make install_sw


WORKDIR /usr/src/
WORKDIR /usr/src/boost_1_69_0
RUN ./bootstrap.sh
RUN sed -i 's/using gcc/using gcc : arm : arm-linux-gnueabihf-gcc/g' project-config.jam
RUN ./b2 toolset=gcc-arm -j`nproc` abi=aapcs cxxstd=14 --with-regex --with-system --with-program_options --with-log --with-coroutine --with-test --prefix=/opt/arm/
RUN ./b2 toolset=gcc-arm -j`nproc` abi=aapcs cxxstd=14 --with-regex --with-system --with-program_options --with-log --with-coroutine --with-test --prefix=/opt/arm/ install

WORKDIR /usr/src/
WORKDIR /usr/src/AqaraHub
RUN mkdir build
WORKDIR /usr/src/AqaraHub/build
ADD armv7.cmake /
RUN cmake -DCMAKE_TOOLCHAIN_FILE=/armv7.cmake ..
RUN make -j`nproc`

RUN mkdir /output
RUN cp -v AqaraHub /output/
RUN cp -v ../clusters.info /output/
ENTRYPOINT tar --create /output

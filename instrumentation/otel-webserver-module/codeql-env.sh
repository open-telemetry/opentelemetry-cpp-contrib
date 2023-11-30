#!/bin/bash

apt-get -y update && apt-get -y upgrade && apt-get -y dist-upgrade
apt-get install -qq -y --ignore-missing \
  build-essential                       \
  curl                                  \
  git                                   \
  make                                  \
  pkg-config                            \
  protobuf-compiler                     \
  libprotobuf-dev                       \
  python                                \
  sudo                                  \
  tar                                   \
  zip                                   \
  unzip                                 \
  wget                                  \
  cmake

apt-get install -y  \
  lcov              \
  m4                \
  autoconf          \
  automake          \
  libtool           \
  default-jre

GRPC_VERSION="1.36.4"
OPENTELEMETRY_VERSION="1.2.0"
BOOST_VERSION="1.75.0"
BOOST_FILENAME="boost_1_75_0"
APR_VERSION="1.7.0"
EXPAT_VERSION="2.3.0"
EXPAT_RVERSION="R_2_3_0"
APRUTIL_VERSION="1.6.1"
LOG4CXX_VERSION="0.11.0"
GTEST_VERSION="1.10.0"
PCRE_VERSION="8.44"
NGINX_VERSION="1.18.0"

# Install GRPC
git clone --shallow-submodules --depth 1 --recurse-submodules -b v${GRPC_VERSION} \
  https://github.com/grpc/grpc \
  && cd grpc \
  && mkdir -p cmake/build \
  && cd cmake/build \
  && cmake \
    -DgRPC_INSTALL=ON \
    -DgRPC_BUILD_TESTS=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DgRPC_BUILD_GRPC_NODE_PLUGIN=OFF \
    -DgRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN=OFF \
    -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF \
    -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF \
    -DgRPC_BUILD_GRPC_PYTHON_PLUGIN=OFF \
    -DgRPC_BUILD_GRPC_RUBY_PLUGIN=OFF \
    ../.. \
  && make -j2 \
  && make install \
  && cd ../../.. && rm -rf grpc

# install opentelemetry
mkdir -p /dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/lib \
    && mkdir -p dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/include \
    && git clone https://github.com/open-telemetry/opentelemetry-cpp \
    && cd opentelemetry-cpp/ \
    && git checkout tags/v${OPENTELEMETRY_VERSION} -b v${OPENTELEMETRY_VERSION} \
    && git submodule update --init --recursive \
    && mkdir build \
    && cd build \
    && cmake .. -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=ON -DWITH_OTLP=ON -DCMAKE_INSTALL_PREFIX=/dependencies/opentelemetry/${OPENTELEMETRY_VERSION} \
    && cmake --build . --target all \
    && cd .. \
    && find . -name "*.so" -type f -exec cp {} /dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/lib/ \; \
    && cp build/libopentelemetry_proto.a /dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/lib \
    && cp -r api/include/ /dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/ \
    && for dir in exporters/*; do if [ -d "$dir" ]; then cp -rf "$dir/include" /dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/; fi; done \
    && cp -r sdk/include/ /dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/ \
    && cp -r build/generated/third_party/opentelemetry-proto/opentelemetry/proto/ /dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/include/opentelemetry/ \
    && cd .. && rm -rf opentelemetry-cpp

#install Apr
mkdir -p /dependencies/apr/${APR_VERSION} \
    && wget https://archive.apache.org/dist/apr/apr-${APR_VERSION}.tar.gz --no-check-certificate \
    && tar -xf apr-${APR_VERSION}.tar.gz \
    && cd apr-${APR_VERSION} \
    && ./configure --prefix=/dependencies/apr/${APR_VERSION} --enable-static=yes --enable-shared=no --with-pic && echo $? \
    && make -j 6 \
    && make install \
    && cd ../ && rm -rf apr-${APR_VERSION} && rm -rf apr-${APR_VERSION}.tar.gz

# install libexpat
mkdir -p /dependencies/expat/${EXPAT_VERSION} \
    && wget https://github.com/libexpat/libexpat/releases/download/${EXPAT_RVERSION}/expat-${EXPAT_VERSION}.tar.gz --no-check-certificate \
    && tar -xf expat-${EXPAT_VERSION}.tar.gz \
    && cd expat-${EXPAT_VERSION} \
    && ./configure --prefix=/dependencies/expat/${EXPAT_VERSION} --enable-static=yes --enable-shared=no --with-pic && echo $? \
    && make -j 6 \
    && make install \
    && cd ../ && rm -rf expat-${EXPAT_VERSION} && rm -rf expat-${EXPAT_VERSION}.tar.gz

# install Apr-util
mkdir -p /dependencies/apr-util/${APRUTIL_VERSION} \
    && wget https://archive.apache.org/dist/apr/apr-util-${APRUTIL_VERSION}.tar.gz --no-check-certificate \
    && tar -xf apr-util-${APRUTIL_VERSION}.tar.gz \
    && cd apr-util-${APRUTIL_VERSION} \
    && ./configure --prefix=/dependencies/apr-util/${APRUTIL_VERSION} --enable-static=yes --enable-shared=no --with-pic --with-apr=/dependencies/apr/1.7.0 --with-expat=/dependencies/expat/2.3.0 && echo $? \
    && make -j 6 \
    && make install \
    && cd ../ && rm -rf apr-util-${APRUTIL_VERSION} && rm -rf apr-util-${APRUTIL_VERSION}.tar.gz


#install log4cxx
mkdir -p /dependencies/apache-log4cxx/${LOG4CXX_VERSION} \
    && wget https://archive.apache.org/dist/logging/log4cxx/${LOG4CXX_VERSION}/apache-log4cxx-${LOG4CXX_VERSION}.tar.gz --no-check-certificate \
    && tar -xf apache-log4cxx-${LOG4CXX_VERSION}.tar.gz \
    && cd apache-log4cxx-${LOG4CXX_VERSION} \
    && ./configure --prefix=/dependencies/apache-log4cxx/${LOG4CXX_VERSION}/ --enable-static=yes --enable-shared=no --with-pic --with-apr=/dependencies/apr/1.7.0/ --with-apr-util=/dependencies/apr-util/1.6.1/ && echo $? \
    && make -j 6 ; echo 0 \
    && automake --add-missing \
    && make install \
    && cd .. && rm -rf apache-log4cxx-${LOG4CXX_VERSION}.tar.gz && rm -rf apache-log4cxx-${LOG4CXX_VERSION}

# install googletest
mkdir -p /dependencies/googletest/${GTEST_VERSION}/ \
    && wget https://github.com/google/googletest/archive/refs/tags/release-${GTEST_VERSION}.tar.gz --no-check-certificate \
    && tar -xf release-${GTEST_VERSION}.tar.gz \
    && cd googletest-release-${GTEST_VERSION}/  \
    && mkdir build && cd build \
    && cmake .. -DCMAKE_INSTALL_PREFIX=/dependencies/googletest/${GTEST_VERSION}/ \
    && make \
    && make install \
    && cd ../.. && rm -rf release-${GTEST_VERSION}.tar.gz && rm -rf googletest-release-${GTEST_VERSION}/

#Installing Apache and apr source code
mkdir -p /build-dependencies \
    && wget --no-check-certificate https://archive.apache.org/dist/apr/apr-${APR_VERSION}.tar.gz \
    && tar -xf apr-${APR_VERSION}.tar.gz \
    && mv -f apr-${APR_VERSION} /build-dependencies \
    && wget --no-check-certificate https://archive.apache.org/dist/apr/apr-util-${APRUTIL_VERSION}.tar.gz \
    && tar -xf apr-util-${APRUTIL_VERSION}.tar.gz \
    && mv -f apr-util-${APRUTIL_VERSION} /build-dependencies \
    && wget --no-check-certificate http://archive.apache.org/dist/httpd/httpd-2.2.31.tar.gz \
    && tar -xf httpd-2.2.31.tar.gz \
    && mv -f httpd-2.2.31 /build-dependencies \
    && wget --no-check-certificate http://archive.apache.org/dist/httpd/httpd-2.4.23.tar.gz \
    && tar -xf httpd-2.4.23.tar.gz \
    && mv -f httpd-2.4.23 /build-dependencies

rm -rf apr-util-${APRUTIL_VERSION} && rm -rf apr-util-${APRUTIL_VERSION}.tar.gz \
    && rm -rf httpd-2.4.23.tar.gz && rm -rf httpd-2.2.31.tar.gz \
    && rm -rf grpc \
    && rm -rf apr-${APR_VERSION} && rm -rf apr-${APR_VERSION}.tar.gz

apt-get install libpcre3 libpcre3-dev -y
apt-get install apache2 -y && a2enmod proxy && a2enmod proxy_http \
    && a2enmod proxy_balancer && a2enmod dav

#Build and install boost
wget https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/${BOOST_FILENAME}.tar.gz \
    && tar -xvf ${BOOST_FILENAME}.tar.gz \
    && cd ${BOOST_FILENAME} \
    && ./bootstrap.sh --with-libraries=filesystem,system --prefix=/dependencies/boost/${BOOST_VERSION}/ \
    && ./b2 install define=BOOST_ERROR_CODE_HEADER_ONLY link=static threading=multi cxxflags="-fvisibility=hidden -fPIC" cflags="-fvisibility=hidden -fPIC" \
    && cd .. && rm -rf ${BOOST_FILENAME} && rm ${BOOST_FILENAME}.tar.gz

# install pcre
mkdir -p /dependencies/pcre/${PCRE_VERSION}/ \
    && wget https://ftp.exim.org/pub/pcre/pcre-${PCRE_VERSION}.tar.gz --no-check-certificate \
    && tar -xvf pcre-${PCRE_VERSION}.tar.gz \
    && cd pcre-${PCRE_VERSION} \
    && ./configure --prefix=/dependencies/pcre/${PCRE_VERSION} --enable-jit \
    && make && make install \
    && cd .. && rm -rf ${PCRE_VERSION}.tar.gz && rm -rf pcre-${PCRE_VERSION}

# install nginx
wget http://nginx.org/download/nginx-${NGINX_VERSION}.tar.gz \
    && tar -xvf nginx-${NGINX_VERSION}.tar.gz -C /build-dependencies \
    && rm -rf nginx-${NGINX_VERSION}.tar.gz

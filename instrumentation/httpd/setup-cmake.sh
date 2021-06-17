#!/bin/bash

set -euxo pipefail

export DEBIAN_FRONTEND=noninteractive

apt-get update

apt-get install --no-install-recommends --no-install-suggests -y \
   build-essential autoconf libtool pkg-config ca-certificates gcc g++ git libcurl4-openssl-dev libpcre3-dev gnupg2 lsb-release curl apt-transport-https software-properties-common zlib1g-dev
curl -o /etc/apt/trusted.gpg.d/kitware.asc https://apt.kitware.com/keys/kitware-archive-latest.asc \
    && apt-add-repository "deb https://apt.kitware.com/ubuntu/ `lsb_release -cs` main"

apt-get install --no-install-recommends --no-install-suggests -y \
cmake libboost-all-dev

git clone --shallow-submodules --depth 1 --recurse-submodules -b v1.36.4 \
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
  && make install

wget https://github.com/libevent/libevent/releases/download/release-2.1.12-stable/libevent-2.1.12-stable.tar.gz \
   && tar zxf libevent-2.1.12-stable.tar.gz \
   && cd libevent-2.1.12-stable \
   && mkdir -p build \
   && cd build \
   && cmake .. \
   && make -j2 \
   && make install

git clone --shallow-submodules --depth 1 --recurse-submodules -b v0.14.0 \
  https://github.com/apache/thrift.git \
   && cd thrift \
   && mkdir -p cmake-build \
   && cd cmake-build \
   && cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTING=OFF \
      -DBUILD_COMPILER=OFF \
      -DBUILD_C_GLIB=OFF \
      -DBUILD_JAVA=OFF \
      -DBUILD_JAVASCRIPT=OFF \
      -DBUILD_NODEJS=OFF \
      -DBUILD_PYTHON=OFF \
      .. \
   && make -j2 \
   && make install

git clone --shallow-submodules --depth 1 --recurse-submodules -b "v1.0.0-rc1" \
   https://github.com/open-telemetry/opentelemetry-cpp.git \
   && cd opentelemetry-cpp \
   && mkdir build \
   && cd build \
   && cmake -DCMAKE_BUILD_TYPE=Release \
     -DWITH_OTLP=ON \
     -DWITH_JAEGER=ON \
     -DBUILD_TESTING=OFF \
     -DWITH_EXAMPLES=OFF \
     -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
     .. \
   && make -j2 \
   && make install

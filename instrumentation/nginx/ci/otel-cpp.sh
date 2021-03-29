#!/bin/bash

set -e

cd opentelemetry-cpp
mkdir build
cd build
cmake -DWITH_OTLP=ON \
  -DCMAKE_PREFIX_PATH=${GITHUB_WORKSPACE}/install \
  -DCMAKE_INSTALL_PREFIX=${GITHUB_WORKSPACE}/install \
  -DBUILD_TESTING=OFF \
  -DWITH_EXAMPLES=OFF \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON ..
make -j2
sudo make install

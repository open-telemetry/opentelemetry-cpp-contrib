#!/bin/bash

set -e

cd opentelemetry-cpp
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH="${GITHUB_WORKSPACE}/install" \
  -DWITH_OTLP=ON \
  -DBUILD_TESTING=OFF \
  -DWITH_EXAMPLES=OFF \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON ..
make -j2
sudo make install

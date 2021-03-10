#!/bin/bash

set -e

cd opentelemetry-cpp
mkdir build
cd build
cmake -DWITH_OTLP=ON \
  -DBUILD_TESTING=OFF \
  -DWITH_EXAMPLES=OFF \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON ..
make -j2
sudo make install

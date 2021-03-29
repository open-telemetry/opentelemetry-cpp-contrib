#!/bin/bash

set -e

mkdir -p build
cd build
cmake -DCMAKE_PREFIX_PATH=${GITHUB_WORKSPACE}/install ..
make -j2

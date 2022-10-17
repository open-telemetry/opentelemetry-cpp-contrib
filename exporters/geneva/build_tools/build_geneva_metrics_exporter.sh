#!/bin/bash
echo Building OpenTelemetry SDK...
pushd "$(dirname "$0")"

#
# Build OpenTelemetry C++ SDK with Geneva Metrics Exporter
# Copy this file to the tools directory of the project repo
# repo, and build below dir structure
#
# <project-repo-root>
#    |
#     -  tools
#         |
#           - build_geneva_metrics_exporter.sh
#           - 001-geneva-metrics-exporter.patch
#     -  deps
#         |
#          - open-telemetry
#              |
#               -   opentelemetry-cpp
#               -   opentelemetry-cpp-contrib/exporters/geneva
#

export OTEL_SDK=`realpath $(pwd)/../deps/open-telemetry/opentelemetry-cpp`
export OTEL_GENEVA_METRICS=`realpath $(pwd)/../deps/open-telemetry/opentelemetry-cpp-contrib/exporters/geneva/`
export PATCH_FILE="001-geneva-exporter.patch"

[ ! -d $OTEL_SDK ] && echo "Error: $OTEL_SDK doesn't exist" && exit 1
[ ! -d $OTEL_GENEVA_METRICS ] && echo "Error: $OTEL_GENEVA_METRICS doesn't exist" && exit 1
[ ! -f $PATCH_FILE ] && echo "Error: Geneva metrics exporter patch doesn't exist" && exit 1

#
# Initialize submodules if necessary
#
if [ ! -f $OTEL_SDK/third_party/nlohmann-json/CMakeLists.txt ]; then
  pushd $OTEL_SDK
  git submodule update --init --recursive
  popd
fi

#
# Create symlink to Geneva exporter
#
if [ ! -L $OTEL_SDK/exporters/geneva/ ]; then
  ln -sf $OTEL_GENEVA_METRICS/ $OTEL_SDK/exporters/geneva
fi

#
# Build and install nlohmann-json dependency
#
pushd $OTEL_SDK/third_party/nlohmann-json
[[ "clean" == "$1" ]] && rm -rf ./build
mkdir -p build
cd build
cmake -G "Ninja" -DCMAKE_INSTALL_PREFIX:PATH=/usr -DJSON_BuildTests=OFF -DBUILD_TESTING=OFF ..
ninja
ninja install
popd

#
# Build with Geneva exporter
#
if [ ! -f ${OTEL_SDK}/${PATCH_FILE} ]; then
  # Apply patch to include geneva metrics exporter as part of the build
  cp -f ${PATCH_FILE} $OTEL_SDK
  pushd $OTEL_SDK
  git apply ${PATCH_FILE}
  popd
fi

pushd $OTEL_SDK
[[ "clean" == "$1" ]] && rm -rf ./build
mkdir -p build
cd build
cmake -G "Ninja" \
        -DCMAKE_INSTALL_PREFIX:PATH=/usr \
        -DWITH_OTLP=OFF \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE \
        -DBUILD_TESTING=OFF  \
        -DBUILD_PACKAGE=ON \
        -DWITH_LOGS_PREVIEW=ON \
        ..
ninja
ninja install
popd
popd

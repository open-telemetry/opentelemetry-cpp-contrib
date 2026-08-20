# Copyright The OpenTelemetry Authors
# SPDX-License-Identifier: Apache-2.0

FROM ubuntu:26.04
ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /work

RUN apt update && apt install -y \
    build-essential \
    cmake \
    git \
    ninja-build \
    protobuf-compiler \
    libcurl4-openssl-dev \
    libgtest-dev

RUN cd /work && \
    git clone --recursive --depth=1 https://github.com/nlohmann/json.git && \
    cd json && mkdir build && cd build && \
    cmake -G Ninja .. && \
    ninja && \
    ninja install

RUN cd /work && \
    git clone --recursive --depth=1 https://github.com/open-telemetry/opentelemetry-cpp.git && \
    cd opentelemetry-cpp && \
    mkdir build && cd build && \
    cmake -G Ninja -D CMAKE_BUILD_TYPE=Debug -D BUILD_TESTING=0 -D WITH_OTLP_HTTP=1 -D WITH_OTLP_GRPC=0 -D WITH_STL=CXX17 .. && \
    ninja && \
    ninja install

RUN cd /work && \
    git clone --recursive --depth=1 https://github.com/open-telemetry/opentelemetry-cpp-contrib

RUN cd /work && \
    cd opentelemetry-cpp-contrib/exporters/user_events && \
    mkdir build && cd build && \
    cmake -G Ninja -D CMAKE_BUILD_TYPE=Debug .. && \
    ninja && \
    ninja install

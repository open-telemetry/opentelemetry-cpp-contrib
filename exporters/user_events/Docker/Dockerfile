# Copyright The OpenTelemetry Authors
# SPDX-License-Identifier: Apache-2.0

FROM ubuntu:latest
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

RUN cd /work && git clone --recursive --depth=1 https://github.com/google/googletest.git && \
    cd googletest && mkdir build && cd build && \
    cmake -G Ninja -D CMAKE_BUILD_TYPE=Debug .. && \
    ninja && ninja install

RUN cd /work && \
    git clone --recursive --depth=1 https://github.com/google/benchmark.git && \
    cd benchmark && mkdir out && cd out && \
    cmake -G Ninja -D CMAKE_BUILD_TYPE=Debug -D BENCHMARK_DOWNLOAD_DEPENDENCIES=on .. && \
    ninja && \
    ninja install

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
    cmake -G Ninja -D CMAKE_BUILD_TYPE=Debug -D BUILD_TESTING=0 -D WITH_OTLP_HTTP=1 -D WITH_OTLP_GRPC=0 -D WITH_LOGS_PREVIEW=1 .. && \
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

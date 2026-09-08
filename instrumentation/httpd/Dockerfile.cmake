FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

#########################################
# copy setup stuff from opentelemetry-cpp
#########################################

WORKDIR /setup-ci

COPY apt-packages.txt /setup-ci/apt-packages.txt

RUN apt-get update -y \
  && xargs -a apt-packages.txt apt-get install -y --no-install-recommends --no-install-suggests \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /root

COPY CMakeLists.txt /root
COPY src /root/src

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
  && cmake --build build --parallel "$(nproc)"

COPY create-otel-load.sh /root
COPY opentelemetry.conf /root
COPY httpd_install_otel.sh /root

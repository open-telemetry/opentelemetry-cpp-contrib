FROM ubuntu:24.04@sha256:534baea6a22c03a63003dbc8dbe78fe34bc0d7e595d9a9dc9834884ff530eb55

ENV DEBIAN_FRONTEND=noninteractive

#########################################
# copy setup stuff from opentelemetry-cpp
#########################################

WORKDIR /setup-ci

COPY apt-packages.txt /setup-ci/apt-packages.txt
COPY ../../apt-packages.txt /setup-ci/otel-contrib-apt-packages.txt

RUN apt-get update -y \
  && xargs -a apt-packages.txt apt-get install -y --no-install-recommends --no-install-suggests \
  && xargs -a otel-contrib-apt-packages.txt apt-get install -y --no-install-recommends --no-install-suggests \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /root

COPY CMakeLists.txt /root
COPY src /root/src

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
  && cmake --build build --parallel "$(nproc)"

COPY create-otel-load.sh /root
COPY opentelemetry.conf /root
COPY httpd_install_otel.sh /root

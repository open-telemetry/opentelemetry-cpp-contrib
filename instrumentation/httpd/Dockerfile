FROM ubuntu:24.04@sha256:73b999393d07f2bf65a40373778a44dc9a6d54ad7718d29247d8ff9413c4d6a8

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

COPY setup-buildtools.sh /setup-ci/setup-buildtools.sh

RUN /setup-ci/setup-buildtools.sh

#########################
# now build plugin itself
#########################

COPY src /root/src

COPY .bazelversion /root
COPY .bazelrc /root
COPY BUILD /root
COPY MODULE.bazel /root

WORKDIR /root
COPY create-otel-load.sh /root
COPY build.sh /root
COPY opentelemetry.conf /root
COPY httpd_install_otel.sh /root
RUN /root/build.sh

# TODO: check apache configuration (apachectl configtest)

# TODO: run tests?

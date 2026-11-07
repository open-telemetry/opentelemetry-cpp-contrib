FROM ubuntu:24.04@sha256:fe81560990fae86dc4743202b2cb4d3df98d0bcf949173cecef15ce1127ef852

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

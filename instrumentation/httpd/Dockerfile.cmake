FROM ubuntu:24.04

#########################################
# copy setup stuff from opentelemetry-cpp
#########################################

WORKDIR /setup-ci

ADD setup-buildtools.sh /setup-ci/setup-buildtools.sh

RUN /setup-ci/setup-buildtools.sh

ADD setup-environment.sh /setup/setup-environment.sh

RUN /setup/setup-environment.sh

WORKDIR /root

COPY CMakeLists.txt /root
COPY src /root/src

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
  && cmake --build build --parallel "$(nproc)"

COPY create-otel-load.sh /root
COPY opentelemetry.conf /root
COPY httpd_install_otel.sh /root

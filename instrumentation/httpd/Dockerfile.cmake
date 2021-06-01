FROM ubuntu:18.04

#########################################
# copy setup stuff from opentelemetry-cpp
#########################################

WORKDIR /setup-ci

ADD setup-buildtools.sh /setup-ci/setup-buildtools.sh

RUN /setup-ci/setup-buildtools.sh

ADD setup-environment.sh /setup/setup-environment.sh

RUN /setup/setup-environment.sh

COPY .clang-format /root

WORKDIR /root

# build with CMake
COPY setup-cmake.sh .
# RUN ls
RUN /root/setup-cmake.sh

COPY CMakeLists.txt /root
COPY src /root/src

RUN mkdir -p build \
  && cd build \
  && cmake .. \
  && make -j2

COPY tools /root/tools
COPY create-otel-load.sh /root
COPY opentelemetry.conf /root
COPY httpd_install_otel.sh /root

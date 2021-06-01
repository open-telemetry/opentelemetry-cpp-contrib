FROM ubuntu:18.04

#########################################
# copy setup stuff from opentelemetry-cpp
#########################################

WORKDIR /setup-ci

ADD setup-buildtools.sh /setup-ci/setup-buildtools.sh

RUN /setup-ci/setup-buildtools.sh

#########################################
# compile Apache thrift
#########################################

# WORKDIR /root
# COPY install-thrift.sh .
# RUN ./install-thrift.sh

#########################
# now build plugin itself
#########################

ADD setup-environment.sh /setup/setup-environment.sh

RUN /setup/setup-environment.sh

COPY src /root/src
COPY .clang-format /root
COPY tools /root/tools

WORKDIR /root
COPY create-otel-load.sh /root
# COPY build.sh /root
COPY opentelemetry.conf /root
COPY httpd_install_otel.sh /root
# RUN /root/build.sh

# TODO: check apache configuration (apachectl configtest)

# TODO: run tests?


RUN apt-get update \
&& DEBIAN_FRONTEND=noninteractive TZ="Europe/London" \
   apt-get install --no-install-recommends --no-install-suggests -y \
   build-essential autoconf libtool pkg-config ca-certificates gcc g++ git libcurl4-openssl-dev libpcre3-dev gnupg2 lsb-release curl apt-transport-https software-properties-common zlib1g-dev
RUN curl -o /etc/apt/trusted.gpg.d/kitware.asc https://apt.kitware.com/keys/kitware-archive-latest.asc \
    && apt-add-repository "deb https://apt.kitware.com/ubuntu/ `lsb_release -cs` main"
RUN curl -o /etc/apt/trusted.gpg.d/nginx_signing.asc https://nginx.org/keys/nginx_signing.key \
    && apt-add-repository "deb http://nginx.org/packages/mainline/ubuntu `lsb_release -cs` nginx" \
    && /bin/bash -c 'echo -e "Package: *\nPin: origin nginx.org\nPin: release o=nginx\nPin-Priority: 900"' | tee /etc/apt/preferences.d/99nginx
RUN apt-get update \
&& DEBIAN_FRONTEND=noninteractive TZ="Europe/London" \
   apt-get install --no-install-recommends --no-install-suggests -y \
   cmake libboost-all-dev
RUN git clone --shallow-submodules --depth 1 --recurse-submodules -b v1.36.4 \
  https://github.com/grpc/grpc \
  && cd grpc \
  && mkdir -p cmake/build \
  && cd cmake/build \
  && cmake \
    -DgRPC_INSTALL=ON \
    -DgRPC_BUILD_TESTS=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DgRPC_BUILD_GRPC_NODE_PLUGIN=OFF \
    -DgRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN=OFF \
    -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF \
    -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF \
    -DgRPC_BUILD_GRPC_PYTHON_PLUGIN=OFF \
    -DgRPC_BUILD_GRPC_RUBY_PLUGIN=OFF \
    ../.. \
  && make -j2 \
  && make install

RUN wget https://github.com/libevent/libevent/releases/download/release-2.1.12-stable/libevent-2.1.12-stable.tar.gz \
   && tar zxf libevent-2.1.12-stable.tar.gz \
   && cd libevent-2.1.12-stable \
   && mkdir -p build \
   && cd build \
   && cmake .. \
   && make -j2 \
   && make install

RUN git clone --shallow-submodules --depth 1 --recurse-submodules -b v0.14.0 \
  https://github.com/apache/thrift.git \
   && cd thrift \
   && mkdir -p cmake-build \
   && cd cmake-build \
   && cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTING=OFF \
      -DBUILD_COMPILER=OFF \
      -DBUILD_C_GLIB=OFF \
      -DBUILD_JAVA=OFF \
      -DBUILD_JAVASCRIPT=OFF \
      -DBUILD_NODEJS=OFF \
      -DBUILD_PYTHON=OFF \
      .. \
   && make -j2 \
   && make install

# RUN git clone --shallow-submodules --depth 1 --recurse-submodules -b v0.7.0
 RUN git clone --shallow-submodules --depth 1 --recurse-submodules \
   https://github.com/open-telemetry/opentelemetry-cpp.git \
   && cd opentelemetry-cpp \
   && mkdir build \
   && cd build \
   && cmake -DCMAKE_BUILD_TYPE=Release \
     -DWITH_OTLP=ON \
     -DWITH_JAEGER=ON \
     -DBUILD_TESTING=OFF \
     -DWITH_EXAMPLES=OFF \
     -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
     .. \
   && make -j2 \
   && make install

# now just build module itself
COPY CMakeLists.txt .
RUN mkdir -p build \
  && cd build \
  && cmake .. \
  && make -j2

ARG image=ubuntu:20.04
FROM $image AS build

RUN apt-get update \
  && DEBIAN_FRONTEND=noninteractive \
     TZ="Europe/London" \
      apt-get install --no-install-recommends --no-install-suggests -y \
        build-essential autoconf libtool pkg-config ca-certificates \
        cmake gcc g++ python3 git nginx libpcre3-dev

RUN git clone --shallow-submodules --depth 1 --recurse-submodules -b v1.35.0 https://github.com/grpc/grpc \
  && cd grpc \
  && mkdir -p cmake/build \
  && cd cmake/build \
  && cmake -DCMAKE_BUILD_TYPE=Release \
    -DgRPC_INSTALL=ON \
    -DgRPC_BUILD_TESTS=OFF \
    -DgRPC_BUILD_CSHARP_EXTENSIONS=OFF \
    -DgRPC_BUILD_GRPC_NODE_PLUGIN=OFF \
    -DgRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN=OFF \
    -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF \
    -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF \
    -DgRPC_BUILD_GRPC_PYTHON_PLUGIN=OFF \
    -DgRPC_BUILD_GRPC_RUBY_PLUGIN=OFF \
    ../.. \
  && make -j2 \
  && make install

RUN git clone --shallow-submodules --depth 1 --recurse-submodules -b v0.3.0 \
  https://github.com/open-telemetry/opentelemetry-cpp.git \
  && cd opentelemetry-cpp \
  && mkdir build \
  && cd build \
  && cmake -DCMAKE_BUILD_TYPE=Release \
    -DWITH_OTLP=ON \
    -DBUILD_TESTING=OFF \
    -DWITH_EXAMPLES=OFF \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    .. \
  && make -j2 \
  && make install

RUN mkdir -p otel-nginx/build && mkdir -p otel-nginx/src
COPY src otel-nginx/src/
COPY CMakeLists.txt nginx.cmake otel-nginx/
RUN cd otel-nginx/build \
  && cmake -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/share/nginx/modules \
    .. \
  && make -j2 \
  && make install

FROM scratch AS export
COPY --from=build /otel-nginx/build/otel_ngx_module.so .

FROM build AS run
CMD ["/usr/sbin/nginx", "-g", "daemon off;"]

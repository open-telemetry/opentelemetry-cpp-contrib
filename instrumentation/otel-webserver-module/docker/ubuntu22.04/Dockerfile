FROM ubuntu:22.04@sha256:b8b6ee6aa931ecd9d0d952abc34dc0e5f7c6a30c6bb71b079fe399fde0329c02
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get -y update && apt-get -y upgrade && apt-get -y dist-upgrade
RUN apt-get install -qq -y --ignore-missing \
	build-essential		                    \
	curl			                        \
	git			                            \
	make			                        \
	pkg-config		                        \
	python2.7		                        \
	sudo			                        \
	tar			                            \
	zip			                            \
	unzip			                        \
	wget			                        \
    xz-utils                                \
    zlib1g-dev                               \
    libssl-dev                               \
    libffi-dev

RUN apt-get install -y \
    lcov    \
    m4                                      \
    autoconf                                \
    automake                                \
    libtool                                 \
    default-jre

# The following arguments would be passed from docker-compose.yml
ARG CMAKE_VERSION="3.28"
ARG CMAKE_0VERSION="3.28.3"
ARG GRPC_VERSION="1.84.0"
# gRPC 1.84.0's own vendored c-ares pin (module provider) resolves to ~1.34.5,
# which is affected by CVE-2025-62408 (DoS in read_answer()/process_answer()),
# fixed upstream in 1.34.6. Build a standalone, newer c-ares and have gRPC use
# it via the "package" provider instead of vendoring its own outdated copy.
ARG CARES_VERSION="1.34.8"
ARG OPENTELEMETRY_VERSION="1.29.0"
ARG BOOST_VERSION="1.92.0"
ARG BOOST_FILENAME="boost_1_92_0"
ARG APR_VERSION="1.7.6"
ARG EXPAT_VERSION="2.8.5"
ARG EXPAT_RVERSION="R_2_8_5"
ARG APRUTIL_VERSION="1.6.5"
ARG LOG4CXX_VERSION="1.8.0"
ARG GTEST_VERSION="1.12.1"
ARG PCRE_VERSION="8.45"
ARG NGINX_VERSION="1.26.0"

# apt's cmake (3.22.x on 22.04) is right at gRPC 1.84's minimum requirement -
# install a known-good newer version explicitly, matching docker/almalinux8's
# approach, rather than depend on exactly which point release apt resolves.
RUN wget https://cmake.org/files/v${CMAKE_VERSION}/cmake-${CMAKE_0VERSION}-linux-x86_64.tar.gz \
    && tar -xvf cmake-${CMAKE_0VERSION}-linux-x86_64.tar.gz \
    && cd cmake-${CMAKE_0VERSION}-linux-x86_64 \
    && cp -rf bin/* /usr/local/bin/ \
    && cp -rf share/* /usr/local/share/ \
    && cd .. && rm -rf cmake-${CMAKE_0VERSION}-linux-x86_64.tar.gz cmake-${CMAKE_0VERSION}-linux-x86_64

# standalone c-ares, newer than gRPC's own vendored pin - see CARES_VERSION comment above.
# Installed to the same prefix gRPC itself installs to, so gRPC's find_package(c-ares
# CONFIG) below picks it up with no extra CMAKE_PREFIX_PATH plumbing needed.
RUN git clone https://github.com/c-ares/c-ares \
  && cd c-ares \
  && git checkout tags/v${CARES_VERSION} -b v${CARES_VERSION} \
  && mkdir build && cd build \
  && cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCARES_STATIC=ON -DCARES_SHARED=OFF -DCARES_BUILD_TESTS=OFF -DCARES_BUILD_TOOLS=OFF -DCMAKE_BUILD_TYPE=Release \
  && cmake --build . -j2 \
  && cmake --install . \
  && cd ../.. && rm -rf c-ares

# Install GRPC. Not apt's protobuf-compiler/libprotobuf-dev - gRPC vendors its own
# protobuf (module provider below), and a second apt-installed copy on the include/
# link path would risk a version mismatch against it.
RUN git clone --shallow-submodules --depth 1 --recurse-submodules -b v${GRPC_VERSION} \
  https://github.com/grpc/grpc \
  && cd grpc \
  && sed -i "s/target_link_libraries(bssl ssl crypto)/target_link_libraries(bssl ssl crypto rt)/g" third_party/boringssl-with-bazel/CMakeLists.txt \
  && mkdir -p cmake/build \
  && cd cmake/build \
  && cmake \
    -DgRPC_INSTALL=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF \
    -DgRPC_BUILD_TESTS=OFF \
    -DCMAKE_INSTALL_LIBDIR=lib \
    -DgRPC_ABSL_PROVIDER=module \
    -DgRPC_CARES_PROVIDER=package \
    -DgRPC_PROTOBUF_PROVIDER=module \
    -DgRPC_RE2_PROVIDER=module \
    -DgRPC_SSL_PROVIDER=module \
    -DgRPC_ZLIB_PROVIDER=module \
    -DCMAKE_C_FLAGS=-Wno-error=attributes \
    -DCMAKE_CXX_FLAGS=-Wno-error=attributes \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -Dprotobuf_INSTALL=ON \
    -DABSL_ENABLE_INSTALL=ON \
    -Dutf8_range_ENABLE_INSTALL=ON \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    ../.. \
  && make -j2 \
  && make install \
  && cd ../../..

# install opentelemetry
RUN printf 'find_package(absl CONFIG REQUIRED)\nfind_package(utf8_range CONFIG REQUIRED)\nfind_package(protobuf CONFIG REQUIRED)\n' > /usr/local/lib/cmake/grpc-preload.cmake \
    && mkdir -p dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/lib \
    && mkdir -p dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/include \
    && git clone https://github.com/open-telemetry/opentelemetry-cpp \
    && cd opentelemetry-cpp/ \
    && git checkout tags/v${OPENTELEMETRY_VERSION} -b v${OPENTELEMETRY_VERSION} \
    && git submodule update --init --recursive \
    && mkdir build \
    && cd build \
    && cmake .. -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=ON -DOTELCPP_WITH_OTLP_GRPC=ON -DCMAKE_CXX_STANDARD=17 -DCMAKE_PROJECT_INCLUDE=/usr/local/lib/cmake/grpc-preload.cmake -DCMAKE_INSTALL_PREFIX=/dependencies/opentelemetry/${OPENTELEMETRY_VERSION} \
    && cmake --build . --target all \
    && cd .. \
    && find . -name "*.so" -type f -exec cp {} /dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/lib/ \; \
    && (find build -iname "libopentelemetry_proto*" -exec cp {} /dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/lib/ \; ; find . -iname "libopentelemetry_proto*" -exec cp {} /dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/lib/ \;) \
    && find build/exporters/otlp -maxdepth 1 -name "*.a" -type f -exec cp {} /dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/lib/ \; \
    && cp -r api/include/ /dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/ \
    && for dir in exporters/*; do if [ -d "$dir" ]; then cp -rf "$dir/include" /dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/; fi; done \
    && cp -r sdk/include/ /dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/ \
    && PROTO_DIR=$(find . -type d -iname proto -path '*opentelemetry*' -exec sh -c 'find "$1" -name "*.pb.h" | head -1 | grep -q .' _ {} \; -print | head -1) \
    && echo "PROTO_DIR=$PROTO_DIR" \
    && cp -r "$PROTO_DIR" /dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/include/opentelemetry/ \
    && cd .. && rm -rf opentelemetry-cpp

#install Apr
RUN mkdir -p dependencies/apr/${APR_VERSION} \
    && wget https://archive.apache.org/dist/apr/apr-${APR_VERSION}.tar.gz --no-check-certificate \
    && tar -xf apr-${APR_VERSION}.tar.gz \
    && cd apr-${APR_VERSION} \
    && ./configure --prefix=/dependencies/apr/${APR_VERSION} --enable-static=yes --enable-shared=no --with-pic && echo $? \
    && make -j 6 \
    && make install \
    && cd ../ && rm -rf apr-${APR_VERSION} && rm -rf apr-${APR_VERSION}.tar.gz

# install libexpat
RUN mkdir -p dependencies/expat/${EXPAT_VERSION} \
    && wget https://github.com/libexpat/libexpat/releases/download/${EXPAT_RVERSION}/expat-${EXPAT_VERSION}.tar.gz --no-check-certificate \
    && tar -xf expat-${EXPAT_VERSION}.tar.gz \
    && cd expat-${EXPAT_VERSION} \
    && ./configure --prefix=/dependencies/expat/${EXPAT_VERSION} --enable-static=yes --enable-shared=no --with-pic && echo $? \
    && make -j 6 \
    && make install \
    && cd ../ && rm -rf expat-${EXPAT_VERSION} && rm -rf expat-${EXPAT_VERSION}.tar.gz

# install Apr-util
RUN mkdir -p dependencies/apr-util/${APRUTIL_VERSION} \
    && wget https://archive.apache.org/dist/apr/apr-util-${APRUTIL_VERSION}.tar.gz --no-check-certificate \
    && tar -xf apr-util-${APRUTIL_VERSION}.tar.gz \
    && cd apr-util-${APRUTIL_VERSION} \
    && ./configure --prefix=/dependencies/apr-util/${APRUTIL_VERSION} --enable-static=yes --enable-shared=no --with-pic --with-apr=/dependencies/apr/${APR_VERSION} --with-expat=/dependencies/expat/${EXPAT_VERSION} && echo $? \
    && make -j 6 \
    && make install \
    && cd ../ && rm -rf apr-util-${APRUTIL_VERSION} && rm -rf apr-util-${APRUTIL_VERSION}.tar.gz


#install log4cxx - 1.8.0 dropped autotools in favor of a CMake-only build.
RUN mkdir -p dependencies/apache-log4cxx/${LOG4CXX_VERSION} \
    && wget https://archive.apache.org/dist/logging/log4cxx/${LOG4CXX_VERSION}/apache-log4cxx-${LOG4CXX_VERSION}.tar.gz --no-check-certificate \
    && tar -xf apache-log4cxx-${LOG4CXX_VERSION}.tar.gz \
    && cd apache-log4cxx-${LOG4CXX_VERSION} \
    && mkdir -p cbuild && cd cbuild \
    && cmake .. -DCMAKE_INSTALL_PREFIX=/dependencies/apache-log4cxx/${LOG4CXX_VERSION}/ -DCMAKE_INSTALL_LIBDIR=lib \
        -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DAPR_STATIC=ON -DAPU_STATIC=ON \
        -DAPR_INCLUDE_DIR=/dependencies/apr/${APR_VERSION}/include/apr-1 \
        -DAPR_LIBRARIES=/dependencies/apr/${APR_VERSION}/lib/libapr-1.a \
        -DAPR_UTIL_INCLUDE_DIR=/dependencies/apr-util/${APRUTIL_VERSION}/include/apr-1 \
        -DAPR_UTIL_LIBRARIES=/dependencies/apr-util/${APRUTIL_VERSION}/lib/libaprutil-1.a \
        -DEXPAT_INCLUDE_DIR=/dependencies/expat/${EXPAT_VERSION}/include \
        -DEXPAT_LIBRARY=/dependencies/expat/${EXPAT_VERSION}/lib/libexpat.a \
    && cmake --build . -j 6 \
    && cmake --install . \
    && cd ../.. && rm -rf apache-log4cxx-${LOG4CXX_VERSION}.tar.gz && rm -rf apache-log4cxx-${LOG4CXX_VERSION}

# install googletest
RUN mkdir -p dependencies/googletest/${GTEST_VERSION}/ \
    && wget https://github.com/google/googletest/archive/refs/tags/release-${GTEST_VERSION}.tar.gz --no-check-certificate \
    && tar -xf release-${GTEST_VERSION}.tar.gz \
    && cd googletest-release-${GTEST_VERSION}/  \
    && mkdir build && cd build \
    && cmake .. -DCMAKE_INSTALL_PREFIX=/dependencies/googletest/${GTEST_VERSION}/ \
    && make \
    && make install \
    && cd ../.. && rm -rf release-${GTEST_VERSION}.tar.gz && rm -rf googletest-release-${GTEST_VERSION}/

#Installing Apache and apr source code
RUN mkdir build-dependencies \
    && wget --no-check-certificate https://archive.apache.org/dist/apr/apr-${APR_VERSION}.tar.gz \
    && tar -xf apr-${APR_VERSION}.tar.gz \
    && mv -f apr-${APR_VERSION} build-dependencies \
    && wget --no-check-certificate https://archive.apache.org/dist/apr/apr-util-${APRUTIL_VERSION}.tar.gz \
    && tar -xf apr-util-${APRUTIL_VERSION}.tar.gz \
    && mv -f apr-util-${APRUTIL_VERSION} build-dependencies \
    && wget --no-check-certificate http://archive.apache.org/dist/httpd/httpd-2.2.31.tar.gz \
    && tar -xf httpd-2.2.31.tar.gz \
    && mv -f httpd-2.2.31 build-dependencies \
    && wget --no-check-certificate http://archive.apache.org/dist/httpd/httpd-2.4.23.tar.gz \
    && tar -xf httpd-2.4.23.tar.gz \
    && mv -f httpd-2.4.23 build-dependencies

RUN rm -rf apr-util-${APRUTIL_VERSION} && rm -rf apr-util-${APRUTIL_VERSION}.tar.gz \
    && rm -rf httpd-2.4.23.tar.gz && rm -rf httpd-2.2.31.tar.gz \
    && rm -rf grpc \
    && rm -rf apr-${APR_VERSION} && rm -rf apr-${APR_VERSION}.tar.gz

RUN apt-get install libpcre3 libpcre3-dev -y
RUN apt-get install apache2 -y && a2enmod proxy && a2enmod proxy_http \
    && a2enmod proxy_balancer && a2enmod dav

#Build and install boost
RUN wget https://archives.boost.io/release/${BOOST_VERSION}/source/${BOOST_FILENAME}_rc1.tar.gz \
    && tar -xvf ${BOOST_FILENAME}_rc1.tar.gz \
    && cd ${BOOST_FILENAME} \
    && ./bootstrap.sh --with-libraries=filesystem,system --prefix=/dependencies/boost/${BOOST_VERSION}/ \
    && ./b2 install define=BOOST_ERROR_CODE_HEADER_ONLY link=static threading=multi cxxflags="-fvisibility=hidden -fPIC" cflags="-fvisibility=hidden -fPIC" \
    && ./b2 headers \
    && cp -rL boost /dependencies/boost/${BOOST_VERSION}/include/ \
    && cd .. && rm -rf ${BOOST_FILENAME} && rm ${BOOST_FILENAME}_rc1.tar.gz

# install pcre
RUN mkdir -p dependencies/pcre/${PCRE_VERSION}/ \
    && wget https://ftp.exim.org/pub/pcre/pcre-${PCRE_VERSION}.tar.gz --no-check-certificate \
    && tar -xvf pcre-${PCRE_VERSION}.tar.gz \
    && cd pcre-${PCRE_VERSION} \
    && ./configure --prefix=/dependencies/pcre/${PCRE_VERSION} --enable-jit \
    && make && make install \
    && cd .. && rm -rf ${PCRE_VERSION}.tar.gz && rm -rf pcre-${PCRE_VERSION}

# install nginx
RUN wget http://nginx.org/download/nginx-${NGINX_VERSION}.tar.gz \
    && tar -xvf nginx-${NGINX_VERSION}.tar.gz -C /build-dependencies \
    && rm -rf nginx-${NGINX_VERSION}.tar.gz

# Install Nginx stable version
RUN apt install curl gnupg2 ca-certificates lsb-release ubuntu-keyring -y \
    && curl https://nginx.org/keys/nginx_signing.key | gpg --dearmor \
    | tee /usr/share/keyrings/nginx-archive-keyring.gpg >/dev/null \
    && gpg --dry-run --quiet --import --import-options import-show /usr/share/keyrings/nginx-archive-keyring.gpg; exit 0
RUN echo "deb [signed-by=/usr/share/keyrings/nginx-archive-keyring.gpg] \
    http://nginx.org/packages/ubuntu `lsb_release -cs` nginx" \
    | tee /etc/apt/sources.list.d/nginx.list \
    && echo "Package: *\nPin: origin nginx.org\nPin: release o=nginx\nPin-Priority: 900\n" \
    | tee /etc/apt/preferences.d/99nginx \
    && apt update -y && apt install "nginx=${NGINX_VERSION}-1~$(lsb_release -cs)" -y

# Build Webserver Module
COPY . /otel-webserver-module

RUN cp -r /dependencies /otel-webserver-module/ \
    && cp -r /build-dependencies /otel-webserver-module/ \
    && cd otel-webserver-module \
    && ./gradlew assembleWebServerModule -DtargetSystem=ubuntu

RUN cd /otel-webserver-module/build \
    && tar -xf opentelemetry-webserver-sdk-x64-linux.tgz \
    && mv -f opentelemetry-webserver-sdk /opt/ \
    && cd ../ \
    && cp opentelemetry_module.conf /etc/apache2/opentelemetry_module.conf \
    && sed -i "s/libmod_apache_otel22.so/libmod_apache_otel.so/g" /etc/apache2/opentelemetry_module.conf \
    && echo 'Include opentelemetry_module.conf' >> /etc/apache2/apache2.conf \
    && cd /opt/opentelemetry-webserver-sdk \
    && ./install.sh \
    && cd /

RUN cd /otel-webserver-module/build \
    && cp ../conf/nginx/opentelemetry_module.conf /opt/ \
    && sed -i '5i load_module /opt/opentelemetry-webserver-sdk/WebServerModule/Nginx/1.26.0/ngx_http_opentelemetry_module.so;' /etc/nginx/nginx.conf \
    && sed -i '33i include /opt/opentelemetry_module.conf;' /etc/nginx/nginx.conf \
    && cd /opt/opentelemetry-webserver-sdk \
    && ./install.sh \
    && cd /

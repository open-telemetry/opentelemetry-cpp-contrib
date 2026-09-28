FROM almalinux:8.10@sha256:9f355ae942d6a6c0561f0771dc053a2cfae9580fc45fa4252756db7c7e80c09f

ARG BUILD_NUMBER

# Populated automatically by buildx/BuildKit from --platform (amd64 or arm64);
# everything architecture-specific below derives from this single value, so
# the same Dockerfile builds correctly for either target.
ARG TARGETARCH

LABEL NAME=apm/build-image-webserver-agent-almalinux8-$TARGETARCH VERSION=$BUILD_NUMBER

ARG PYTHON_VERSION="2.7.18"
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
ARG AUTOCONF_VERSION="2.68"
ARG LIBTOOL_VERSION="2.4.6"
ARG LOG4CXX_VERSION="1.8.0"
ARG GTEST_VERSION="1.12.1"
ARG AUTOMAKE_VERSION="1.16.3"
ARG PERL_VERSION="5.20.2"
ARG PERL_CPANVERSION="5.0"
ARG PCRE_VERSION="8.45"
ARG NGINX_VERSION="1.26.0"

# create default non-root user
RUN groupadd -r swuser && useradd -u 1000 -g swuser -m -s /sbin/nologin -c "default non-root user" swuser

RUN chmod -R 777 /opt

# install required packages


RUN dnf install wget -y \
    && dnf install java-1.8.0-openjdk -y \
    && dnf install java-1.8.0-openjdk-devel -y \
    && dnf install pcre-devel -y \
    && dnf install httpd -y \
    && dnf install git -y

# Several dependency tarballs below (autoconf 2.68, automake, libtool, apr,
# apr-util, expat, log4cxx, pcre) vendor their own GNU config.guess/config.sub
# scripts, most from well before aarch64 existed as an architecture - their
# ./configure fails with "cannot guess build type" on this platform otherwise.
# Refreshed once here and copied into each extracted source tree below,
# rather than patched one at a time as each failure surfaces.
RUN curl -sSL -o /tmp/config.guess https://raw.githubusercontent.com/gcc-mirror/gcc/master/config.guess \
    && curl -sSL -o /tmp/config.sub https://raw.githubusercontent.com/gcc-mirror/gcc/master/config.sub

RUN dnf groupinstall "Development Tools" -y \
    && dnf install wget bzip2 xz-devel openssl-devel libffi-devel zlib-devel -y


# install python 2.7.18
RUN wget http://www.python.org/ftp/python/${PYTHON_VERSION}/Python-${PYTHON_VERSION}.tar.xz \
    && unxz Python-${PYTHON_VERSION}.tar.xz \
    && tar -xf Python-${PYTHON_VERSION}.tar \
    && cd Python-${PYTHON_VERSION} && ./configure --prefix=/usr/local && make && make install \
    && cd .. \
    && rm -rf Python-${PYTHON_VERSION}.tar.xz && rm -rf Python-${PYTHON_VERSION}.tar


ENV GOSU_VERSION=1.10

RUN case "$TARGETARCH" in \
      amd64) GOSU_ARCH=amd64 ;; \
      arm64) GOSU_ARCH=arm64 ;; \
      *) echo "unsupported TARGETARCH: $TARGETARCH" >&2; exit 1 ;; \
    esac \
    && curl -o /usr/bin/gosu -SL "https://github.com/tianon/gosu/releases/download/${GOSU_VERSION}/gosu-${GOSU_ARCH}" \
    && chmod +x /usr/bin/gosu \
    # Verify that the binary works
    && gosu nobody true

# install cmake 3.20
RUN case "$TARGETARCH" in \
      amd64) CMAKE_ARCH=x86_64 ;; \
      arm64) CMAKE_ARCH=aarch64 ;; \
      *) echo "unsupported TARGETARCH: $TARGETARCH" >&2; exit 1 ;; \
    esac \
    && wget https://cmake.org/files/v${CMAKE_VERSION}/cmake-${CMAKE_0VERSION}-linux-${CMAKE_ARCH}.tar.gz \
    && tar -xvf cmake-${CMAKE_0VERSION}-linux-${CMAKE_ARCH}.tar.gz \
    && cd cmake-${CMAKE_0VERSION}-linux-${CMAKE_ARCH} \
    && cp -rf * /usr/local/ \
    && cd .. && rm -rf cmake-${CMAKE_0VERSION}-linux-${CMAKE_ARCH}.tar.gz


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

# install grpc. If planning to upgrade, make sure sed command works
RUN git clone https://github.com/grpc/grpc \
  && cd grpc \
  && git checkout tags/v${GRPC_VERSION} -b v${GRPC_VERSION} \
  && git submodule update --init \
  && sed -i "s/target_link_libraries(bssl ssl crypto)/target_link_libraries(bssl ssl crypto rt)/g" third_party/boringssl-with-bazel/CMakeLists.txt \
  && mkdir -p cmake/build \
  && cd cmake/build \
  && cmake ../.. -DgRPC_INSTALL=ON -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DgRPC_BUILD_TESTS=OFF -DgRPC_ABSL_PROVIDER=module -DgRPC_CARES_PROVIDER=package -DgRPC_PROTOBUF_PROVIDER=module -DgRPC_RE2_PROVIDER=module -DgRPC_SSL_PROVIDER=module -DgRPC_ZLIB_PROVIDER=module -DCMAKE_C_FLAGS=-Wno-error=attributes -DCMAKE_CXX_FLAGS=-Wno-error=attributes -DCMAKE_CXX_STANDARD=17 -DCMAKE_INSTALL_PREFIX=/usr/local -Dprotobuf_INSTALL=ON -DABSL_ENABLE_INSTALL=ON -Dutf8_range_ENABLE_INSTALL=ON -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  && make -j2 \
  && make install \
  && cd ../../..


# Create dependencies folder to store all dependencies.
RUN mkdir -p dependencies

# install boost version 1.75.0
RUN wget https://archives.boost.io/release/${BOOST_VERSION}/source/${BOOST_FILENAME}_rc1.tar.gz \
    && tar -xvf ${BOOST_FILENAME}_rc1.tar.gz \
    && cd ${BOOST_FILENAME} \
    && ./bootstrap.sh --with-libraries=filesystem,system --prefix=/dependencies/boost/${BOOST_VERSION}/ \
    && ./b2 install define=BOOST_ERROR_CODE_HEADER_ONLY link=static threading=multi cxxflags="-fvisibility=hidden -fPIC" cflags="-fvisibility=hidden -fPIC" \
    && ./b2 headers \
    && cp -rL boost /dependencies/boost/${BOOST_VERSION}/include/ \
    && cd .. && rm -rf ${BOOST_FILENAME} && rm ${BOOST_FILENAME}_rc1.tar.gz

#install Apr
RUN mkdir -p dependencies/apr/${APR_VERSION} \
    && wget https://archive.apache.org/dist/apr/apr-${APR_VERSION}.tar.gz --no-check-certificate \
    && tar -xf apr-${APR_VERSION}.tar.gz \
    && cd apr-${APR_VERSION} \
    && find . -name config.guess -exec cp /tmp/config.guess {} \; \
    && find . -name config.sub -exec cp /tmp/config.sub {} \; \
    && ./configure --prefix=/dependencies/apr/${APR_VERSION} --enable-static=yes --enable-shared=no --with-pic && echo $? \
    && make -j 6 \
    && make install \
    && cd ../ && rm -rf apr-${APR_VERSION} && rm -rf apr-${APR_VERSION}.tar.gz

# install libexpat
RUN mkdir -p dependencies/expat/${EXPAT_VERSION} \
    && wget https://github.com/libexpat/libexpat/releases/download/${EXPAT_RVERSION}/expat-${EXPAT_VERSION}.tar.gz --no-check-certificate \
    && tar -xf expat-${EXPAT_VERSION}.tar.gz \
    && cd expat-${EXPAT_VERSION} \
    && find . -name config.guess -exec cp /tmp/config.guess {} \; \
    && find . -name config.sub -exec cp /tmp/config.sub {} \; \
    && ./configure --prefix=/dependencies/expat/${EXPAT_VERSION} --enable-static=yes --enable-shared=no --with-pic && echo $? \
    && make -j 6 \
    && make install \
    && cd ../ && rm -rf expat-${EXPAT_VERSION} && rm -rf expat-${EXPAT_VERSION}.tar.gz

# install Apr-util
RUN mkdir -p dependencies/apr-util/${APRUTIL_VERSION} \
    && wget https://archive.apache.org/dist/apr/apr-util-${APRUTIL_VERSION}.tar.gz --no-check-certificate \
    && tar -xf apr-util-${APRUTIL_VERSION}.tar.gz \
    && cd apr-util-${APRUTIL_VERSION} \
    && find . -name config.guess -exec cp /tmp/config.guess {} \; \
    && find . -name config.sub -exec cp /tmp/config.sub {} \; \
    && ./configure --prefix=/dependencies/apr-util/${APRUTIL_VERSION} --enable-static=yes --enable-shared=no --with-pic --with-apr=/dependencies/apr/${APR_VERSION} --with-expat=/dependencies/expat/${EXPAT_VERSION} && echo $? \
    && make -j 6 \
    && make install \
    && cd ../ && rm -rf apr-util-${APRUTIL_VERSION} && rm -rf apr-util-${APRUTIL_VERSION}.tar.gz


# install autoconf
RUN wget --no-check-certificate https://ftp.gnu.org/gnu/autoconf/autoconf-${AUTOCONF_VERSION}.tar.gz \
    && tar xzf autoconf-${AUTOCONF_VERSION}.tar.gz  \
    && cd autoconf-${AUTOCONF_VERSION} \
    && find . -name config.guess -exec cp /tmp/config.guess {} \; \
    && find . -name config.sub -exec cp /tmp/config.sub {} \; \
    && ./configure --prefix=/usr/ && make -j && make install && autoconf -V \
    && cd .. && rm -rf autoconf-${AUTOCONF_VERSION}.tar.gz

# install automake
RUN wget --no-check-certificate https://ftp.gnu.org/gnu/automake/automake-${AUTOMAKE_VERSION}.tar.gz \
    && tar xzf automake-${AUTOMAKE_VERSION}.tar.gz \
    && cd automake-${AUTOMAKE_VERSION} \
    && find . -name config.guess -exec cp /tmp/config.guess {} \; \
    && find . -name config.sub -exec cp /tmp/config.sub {} \; \
    && ./configure --prefix=/usr --libdir=/usr/lib64 \
    && make -j && make install \
    && automake --version \
    && cd .. && rm -rf automake-${AUTOMAKE_VERSION}.tar.gz

# install libtool
RUN wget --no-check-certificate https://ftp.gnu.org/gnu/libtool/libtool-${LIBTOOL_VERSION}.tar.gz \
    && tar xzf libtool-${LIBTOOL_VERSION}.tar.gz \
    && cd libtool-${LIBTOOL_VERSION} \
    && find . -name config.guess -exec cp /tmp/config.guess {} \; \
    && find . -name config.sub -exec cp /tmp/config.sub {} \; \
    && ./configure --prefix=/usr \
    && make -j 6 \
    && make install \
    && libtool --version \
    && cd .. && rm -rf libtool-${LIBTOOL_VERSION}.tar.gz

#install log4cxx
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

# install pcre
RUN mkdir -p dependencies/pcre/${PCRE_VERSION}/ \
    && wget https://ftp.exim.org/pub/pcre/pcre-${PCRE_VERSION}.tar.gz --no-check-certificate \
    && tar -xvf pcre-${PCRE_VERSION}.tar.gz \
    && cd pcre-${PCRE_VERSION} \
    && find . -name config.guess -exec cp /tmp/config.guess {} \; \
    && find . -name config.sub -exec cp /tmp/config.sub {} \; \
    && ./configure --prefix=/dependencies/pcre/${PCRE_VERSION} --enable-jit \
    && make && make install \
    && cd .. && rm -rf ${PCRE_VERSION}.tar.gz && rm -rf pcre-${PCRE_VERSION}

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

# install nginx
RUN wget http://nginx.org/download/nginx-${NGINX_VERSION}.tar.gz \
    && tar -xvf nginx-${NGINX_VERSION}.tar.gz -C /build-dependencies \
    && rm -rf nginx-${NGINX_VERSION}.tar.gz

# Build Webserver Module
COPY . /otel-webserver-module

RUN cp -r /dependencies /otel-webserver-module/ \
    && cp -r /build-dependencies /otel-webserver-module/ \
    && cd otel-webserver-module \
    && ./gradlew assembleWebServerModule

RUN echo '[nginx-stable]' >> /etc/yum.repos.d/nginx.repo \
    && echo 'name=nginx stable repo' >> /etc/yum.repos.d/nginx.repo \
    && echo 'baseurl=http://nginx.org/packages/centos/8/$basearch/' >> /etc/yum.repos.d/nginx.repo \
    && echo 'gpgcheck=1' >> /etc/yum.repos.d/nginx.repo \
    && echo 'enabled=1' >> /etc/yum.repos.d/nginx.repo \
    && echo 'gpgkey=https://nginx.org/keys/nginx_signing.key' >> /etc/yum.repos.d/nginx.repo \
    && echo 'module_hotfixes=true' >> /etc/yum.repos.d/nginx.repo \
    && dnf install nginx-${NGINX_VERSION} -y

# osArch resolves to the JVM's os.arch value at build time (matching whichever
# platform this image is actually being built for) - common.gradle remaps
# i386/i586/i686->x86 and x86_64/amd64->x64, leaving aarch64 untouched, so the
# packaged tarball is x64-linux on amd64 but aarch64-linux on arm64.
RUN case "$TARGETARCH" in \
      amd64) SDK_ARCH=x64 ;; \
      arm64) SDK_ARCH=aarch64 ;; \
      *) echo "unsupported TARGETARCH: $TARGETARCH" >&2; exit 1 ;; \
    esac \
    && cd /otel-webserver-module/build \
    && tar -xf opentelemetry-webserver-sdk-${SDK_ARCH}-linux.tgz \
    && mv -f opentelemetry-webserver-sdk /opt/ \
    && cd ../ \
    && cp opentelemetry_module.conf /etc/httpd/conf/ \
    && sed -i "s/libmod_apache_otel22.so/libmod_apache_otel.so/g" /etc/httpd/conf/opentelemetry_module.conf \
    && echo 'Include conf/opentelemetry_module.conf' >> /etc/httpd/conf/httpd.conf \
    && cd /opt/opentelemetry-webserver-sdk \
    && ./install.sh \
    && cd /

RUN cp /otel-webserver-module/conf/nginx/opentelemetry_module.conf /opt/ \
    && sed -i "8i load_module /opt/opentelemetry-webserver-sdk/WebServerModule/Nginx/${NGINX_VERSION}/ngx_http_opentelemetry_module.so;" /etc/nginx/nginx.conf \
    && sed -i '33i include /opt/opentelemetry_module.conf;' /etc/nginx/nginx.conf \
    && cd /

COPY entrypoint.sh /usr/local/bin/
ENTRYPOINT ["entrypoint.sh"]

FROM centos:7

ARG BUILD_NUMBER

LABEL NAME=apm/build-image-webserver-agent-centos6-x64 VERSION=$BUILD_NUMBER

ENV GOSU_ARCH amd64
ENV JDK_ARCH x64

ARG PYTHON_VERSION="2.7.8"
ARG CMAKE_VERSION="3.20"
ARG CMAKE_0VERSION="3.20.0"
ARG GRPC_VERSION="1.36.4"
ARG OPENTELEMETRY_VERSION="1.2.0"
ARG BOOST_VERSION="1.75.0"
ARG BOOST_FILENAME="boost_1_75_0"
ARG APR_VERSION="1.7.0"
ARG EXPAT_VERSION="2.3.0"
ARG EXPAT_RVERSION="R_2_3_0"
ARG APRUTIL_VERSION="1.6.1"
ARG AUTOCONF_VERSION="2.68"
ARG LIBTOOL_VERSION="2.4.6"
ARG LOG4CXX_VERSION="0.11.0"
ARG GTEST_VERSION="1.10.0"
ARG AUTOMAKE_VERSION="1.16.3"
ARG PERL_VERSION="5.20.2"
ARG PERL_CPANVERSION="5.0"
ARG PCRE_VERSION="8.44"
ARG NGINX_VERSION="1.22.0"

# create default non-root user
RUN groupadd -r swuser && useradd -u 1000 -g swuser -m -s /sbin/nologin -c "default non-root user" swuser

RUN chmod -R 777 /opt

# RUN curl https://www.getpagespeed.com/files/centos6-eol.repo --output /etc/yum.repos.d/CentOS-Base.repo

# install required packages
RUN yum update -y \
    && yum install -y \
    yum install wget -y \
    yum install -y zlib-devel \
    yum install -y openssl-devel \
    yum install java-1.8.0-openjdk -y \
    yum install pcre-devel -y \
    yum install xz -y \
    yum install httpd -y \
    && yum clean all

# install lcov package
RUN wget https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm \
    && yum install ./epel-release-latest-*.noarch.rpm -y \
    && yum install lcov -y \
    && yum clean all

# install devtoolset toolchain
# RUN curl https://www.getpagespeed.com/files/centos6-eol.repo --output /etc/yum.repos.d/CentOS-Base.repo \
#  && curl https://www.getpagespeed.com/files/centos6-epel-eol.repo --output /etc/yum.repos.d/epel.repo \
  RUN yum -y install centos-release-scl \
#  && curl https://www.getpagespeed.com/files/centos6-scl-eol.repo --output /etc/yum.repos.d/CentOS-SCLo-scl.repo \
#  && curl https://www.getpagespeed.com/files/centos6-scl-rh-eol.repo --output /etc/yum.repos.d/CentOS-SCLo-scl-rh.repo \
  && yum install yum-utils -y \
  && yum-config-manager --enable rhel-server-rhscl-7-rpms -y \
  && yum install devtoolset-7 -y \
  && yum clean all
ENV PATH="/opt/rh/devtoolset-7/root/usr/bin:${PATH}"

# install python 2.7.8
RUN wget http://www.python.org/ftp/python/${PYTHON_VERSION}/Python-${PYTHON_VERSION}.tar.xz \
    && unxz Python-${PYTHON_VERSION}.tar.xz \
    && tar -xf Python-${PYTHON_VERSION}.tar \
    && cd Python-${PYTHON_VERSION} && ./configure --prefix=/usr/local && make && make install \
    && cd .. \
    && rm -rf Python-${PYTHON_VERSION}.tar.xz && rm -rf Python-${PYTHON_VERSION}.tar

# install gosu for easy step-down from root (from https://github.com/tianon/gosu/blob/master/INSTALL.md#from-centos)
ENV GOSU_VERSION=1.10
#RUN gpg --keyserver pgp.mit.edu --recv-keys B42F6819007F00F88E364FD4036A9C25BF357DD4 \
#    && curl -o /usr/bin/gosu -SL "https://github.com/tianon/gosu/releases/download/${GOSU_VERSION}/gosu-amd64" \
#    && curl -o /usr/bin/gosu.asc -SL "https://github.com/tianon/gosu/releases/download/${GOSU_VERSION}/gosu-amd64.asc" \
#    && gpg --verify /usr/bin/gosu.asc \
#    && rm /usr/bin/gosu.asc \
#    && rm -r /root/.gnupg/ \
RUN curl -o /usr/bin/gosu -SL "https://github.com/tianon/gosu/releases/download/${GOSU_VERSION}/gosu-amd64" \
    && chmod +x /usr/bin/gosu \
    # Verify that the binary works
    && gosu nobody true

# install git required for grpc
RUN yum install git -y \
  && yum clean all

# install cmake 3.20
RUN wget https://cmake.org/files/v${CMAKE_VERSION}/cmake-${CMAKE_0VERSION}-linux-x86_64.tar.gz \
  && tar -xvf cmake-${CMAKE_0VERSION}-linux-x86_64.tar.gz \
  && cd cmake-${CMAKE_0VERSION}-linux-x86_64 \
  && cp -rf * /usr/local/ \
  && cd .. && rm -rf cmake-${CMAKE_0VERSION}-linux-x86_64.tar.gz

# install grpc. If planning to upgrade, make sure sed command works
RUN git clone https://github.com/grpc/grpc \
  && cd grpc \
  && git checkout tags/v${GRPC_VERSION} -b v${GRPC_VERSION} \
  && git submodule update --init \
  && sed -i "s/target_link_libraries(bssl ssl crypto)/target_link_libraries(bssl ssl crypto rt)/g" third_party/boringssl-with-bazel/CMakeLists.txt \
  && mkdir -p cmake/build \
  && cd cmake/build \
  && cmake ../.. -DgRPC_INSTALL=ON -DCMAKE_BUILD_TYPE=Release -DgRPC_ABSL_PROVIDER=module -DgRPC_CARES_PROVIDER=module -DgRPC_PROTOBUF_PROVIDER=module -DgRPC_RE2_PROVIDER=module -DgRPC_SSL_PROVIDER=module -DgRPC_ZLIB_PROVIDER=module \
  && make \
  && make install \
  && cd ../../..

# Create dependencies folder to store all dependencies.
RUN mkdir -p dependencies

# install boost version 1.75.0
RUN wget https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/${BOOST_FILENAME}_rc1.tar.gz \
    && tar -xvf ${BOOST_FILENAME}_rc1.tar.gz \
    && cd ${BOOST_FILENAME} \
    && ./bootstrap.sh --with-libraries=filesystem,system --prefix=/dependencies/boost/${BOOST_VERSION}/ \
    && ./b2 install define=BOOST_ERROR_CODE_HEADER_ONLY link=static threading=multi cxxflags="-fvisibility=hidden -fPIC" cflags="-fvisibility=hidden -fPIC" \
    && cd .. && rm -rf ${BOOST_FILENAME} && rm ${BOOST_FILENAME}_rc1.tar.gz

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

# install m4
RUN yum install m4 -y \
    && yum install perl-Data-Dumper -y

# Update perl version
RUN yum install perl-Thread-Queue.noarch -y \
    && wget http://www.cpan.org/src/${PERL_CPANVERSION}/perl-${PERL_VERSION}.tar.gz \
    && tar -xzf perl-${PERL_VERSION}.tar.gz \
    && cd perl-${PERL_VERSION} \
    && ./Configure -des -Dprefix=$HOME/localperl \
    && make \
    && make test; echo 0 \
    && make install 
ENV PATH=$HOME/localperl/bin:$PATH

# install autoconf
RUN wget --no-check-certificate https://ftp.gnu.org/gnu/autoconf/autoconf-${AUTOCONF_VERSION}.tar.gz \
    && tar xzf autoconf-${AUTOCONF_VERSION}.tar.gz  \
    && cd autoconf-${AUTOCONF_VERSION} \
    && ./configure --prefix=/usr/ && make -j && make install && autoconf -V \
    && cd .. && rm -rf autoconf-${AUTOCONF_VERSION}.tar.gz

# install automake
RUN wget --no-check-certificate https://ftp.gnu.org/gnu/automake/automake-${AUTOMAKE_VERSION}.tar.gz \
    && tar xzf automake-${AUTOMAKE_VERSION}.tar.gz \
    && cd automake-${AUTOMAKE_VERSION} \
    && ./configure --prefix=/usr --libdir=/usr/lib64 \
    && make -j && make install \
    && automake --version \
    && cd .. && rm -rf automake-${AUTOMAKE_VERSION}.tar.gz

# install libtool
RUN wget --no-check-certificate https://ftpmirror.gnu.org/libtool/libtool-${LIBTOOL_VERSION}.tar.gz \
    && tar xzf libtool-${LIBTOOL_VERSION}.tar.gz \
    && cd libtool-${LIBTOOL_VERSION} \
    && ./configure --prefix=/usr \
    && make -j 6 \
    && make install \
    && libtool --version \
    && cd .. && rm -rf libtool--${LIBTOOL_VERSION}.tar.gz

#install log4cxx
RUN mkdir -p dependencies/apache-log4cxx/${LOG4CXX_VERSION} \
    && wget https://archive.apache.org/dist/logging/log4cxx/${LOG4CXX_VERSION}/apache-log4cxx-${LOG4CXX_VERSION}.tar.gz --no-check-certificate \
    && tar -xf apache-log4cxx-${LOG4CXX_VERSION}.tar.gz \
    && cd apache-log4cxx-${LOG4CXX_VERSION} \
    && ./configure --prefix=/dependencies/apache-log4cxx/${LOG4CXX_VERSION}/ --enable-static=yes --enable-shared=no --with-pic --with-apr=/dependencies/apr/${APR_VERSION}/ --with-apr-util=/dependencies/apr-util/${APRUTIL_VERSION}/ && echo $? \
    && make -j 6 ; echo 0 \
    && automake --add-missing \
    && make install \
    && cd .. && rm -rf apache-log4cxx-${LOG4CXX_VERSION}.tar.gz && rm -rf apache-log4cxx-${LOG4CXX_VERSION}

# install opentelemetry
RUN mkdir -p dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/lib \
    && mkdir -p dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/include \
    && git clone https://github.com/open-telemetry/opentelemetry-cpp \
    && cd opentelemetry-cpp/ \
    && git checkout tags/v${OPENTELEMETRY_VERSION} -b v${OPENTELEMETRY_VERSION} \
    && git submodule update --init --recursive \
    && mkdir build \
    && cd build \
    && cmake .. -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=ON -DWITH_OTLP=ON -DCMAKE_INSTALL_PREFIX=/dependencies/opentelemetry/${OPENTELEMETRY_VERSION} \
    && cmake --build . --target all \
    && cd .. \
    && find . -name "*.so" -type f -exec cp {} /dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/lib/ \; \
    && cp build/libopentelemetry_proto.a /dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/lib \
    && cp -r api/include/ /dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/ \
    && for dir in exporters/*; do if [ -d "$dir" ]; then cp -rf "$dir/include" /dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/; fi; done \
    && cp -r sdk/include/ /dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/ \
    && cp -r build/generated/third_party/opentelemetry-proto/opentelemetry/proto/ /dependencies/opentelemetry/${OPENTELEMETRY_VERSION}/include/opentelemetry/ \
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

RUN echo '[nginx]' >> /etc/yum.repos.d/nginx.repo \
    && echo 'name=nginx repo' >> /etc/yum.repos.d/nginx.repo \
    && echo 'baseurl=https://nginx.org/packages/centos/7/x86_64' >> /etc/yum.repos.d/nginx.repo \
    && echo 'gpgcheck=0' >> /etc/yum.repos.d/nginx.repo \ 
    && echo 'enabled=1' >> /etc/yum.repos.d/nginx.repo \
    && yum install nginx-${NGINX_VERSION} -y

RUN cd /otel-webserver-module/build \
    && tar -xf opentelemetry-webserver-sdk-x64-linux.tgz \
    && mv -f opentelemetry-webserver-sdk /opt/ \
    && cd ../ \
    && cp opentelemetry_module.conf /etc/httpd/conf/ \
    && sed -i "s/libmod_apache_otel22.so/libmod_apache_otel.so/g" /etc/httpd/conf/opentelemetry_module.conf \
    && echo 'Include conf/opentelemetry_module.conf' >> /etc/httpd/conf/httpd.conf \
    && cd /opt/opentelemetry-webserver-sdk \
    && ./install.sh \
    && cd /

RUN cp /otel-webserver-module/conf/nginx/opentelemetry_module.conf /opt/ \
    && sed -i '8i load_module /opt/opentelemetry-webserver-sdk/WebServerModule/Nginx/1.22.0/ngx_http_opentelemetry_module.so;' /etc/nginx/nginx.conf \
    && sed -i '33i include /opt/opentelemetry_module.conf;' /etc/nginx/nginx.conf \ 
    && cd /

# Remove unwanted files
RUN rm -rf grpc && rm -rf autoconf-${AUTOCONF_VERSION} && rm -rf automake-${AUTOMAKE_VERSION} && rm -rf cmake-${CMAKE_VERSION}-linux-x86_64 \
    && rm -rf libtool-${LIBTOOL_VERSION} && rm -rf Python-${PYTHON_VERSION} \
    && rm -f apr-${APR_VERSION}.tar.gz && rm -f apr-util-${APRUTIL_VERSION}.tar.gz \
    && rm -f httpd-2.2.31.tar.gz && rm -f httpd-2.4.23.tar.gz

COPY entrypoint.sh /usr/local/bin/
ENTRYPOINT ["entrypoint.sh"]

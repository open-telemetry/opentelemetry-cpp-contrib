FROM centos:6.9

ARG BUILD_NUMBER

LABEL NAME=apm/build-image-webserver-agent-centos6-x64 VERSION=$BUILD_NUMBER

ENV GOSU_ARCH amd64
ENV JDK_ARCH x64

# create default non-root user
RUN groupadd -r swuser && useradd -u 1000 -g swuser -m -s /sbin/nologin -c "default non-root user" swuser

RUN chmod -R 777 /opt

RUN curl https://www.getpagespeed.com/files/centos6-eol.repo --output /etc/yum.repos.d/CentOS-Base.repo

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
RUN curl https://www.getpagespeed.com/files/centos6-eol.repo --output /etc/yum.repos.d/CentOS-Base.repo \
  && curl https://www.getpagespeed.com/files/centos6-epel-eol.repo --output /etc/yum.repos.d/epel.repo \
  && yum -y install centos-release-scl \
  && curl https://www.getpagespeed.com/files/centos6-scl-eol.repo --output /etc/yum.repos.d/CentOS-SCLo-scl.repo \
  && curl https://www.getpagespeed.com/files/centos6-scl-rh-eol.repo --output /etc/yum.repos.d/CentOS-SCLo-scl-rh.repo \
  && yum install yum-utils -y \
  && yum-config-manager --enable rhel-server-rhscl-7-rpms -y \
  && yum install devtoolset-7 -y \
  && yum clean all
ENV PATH="/opt/rh/devtoolset-7/root/usr/bin:${PATH}"

# install python 2.7.8
RUN wget http://www.python.org/ftp/python/2.7.8/Python-2.7.8.tar.xz \
	&& unxz Python-2.7.8.tar.xz \
	&& tar -xf Python-2.7.8.tar \
	&& cd Python-2.7.8 && ./configure --prefix=/usr/local && make && make install \
	&& cd .. \
	&& rm -rf Python-2.7.8.tar.xz && rm -rf Python-2.7.8.tar

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
RUN wget https://cmake.org/files/v3.20/cmake-3.20.0-linux-x86_64.tar.gz \
  && tar -xvf cmake-3.20.0-linux-x86_64.tar.gz \
  && cd cmake-3.20.0-linux-x86_64 \
  && cp -rf * /usr/local/ \
  && cd .. && rm -rf cmake-3.20.0-linux-x86_64.tar.gz

# install grpc. If planning to upgrade, make sure sed command works
RUN git clone https://github.com/grpc/grpc \
  && cd grpc \
  && git checkout tags/v1.36.4 -b v1.36.4 \
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
RUN mkdir -p dependencies/boost/1.75.0/ \
    && git clone https://github.com/boostorg/boost \
    && cd boost/ && git checkout tags/boost-1.75.0 -b boost-1.75.0 \
    && git submodule update --init --recursive \
    && ./bootstrap.sh --with-libraries=filesystem,system --prefix=/dependencies/boost/1.75.0 \
    && ./b2 install define=BOOST_ERROR_CODE_HEADER_ONLY link=static threading=multi cxxflags="-fvisibility=hidden -fPIC -D_GLIBCXX_USE_CXX11_ABI=0" cflags="-fvisibility=hidden -fPIC" \
    && cd .. && rm -rf boost

#install Apr
RUN mkdir -p dependencies/apr/1.7.0 \
    && wget https://dlcdn.apache.org//apr/apr-1.7.0.tar.gz --no-check-certificate \
    && tar -xf apr-1.7.0.tar.gz \
    && cd apr-1.7.0 \
    && ./configure --prefix=/dependencies/apr/1.7.0 --enable-static=yes --enable-shared=no --with-pic && echo $? \
    && make -j 6 \
    && make install \
    && cd ../ && rm -rf apr-1.7.0 && rm -rf apr-1.7.0.tar.gz

# install libexpat
RUN mkdir -p dependencies/expat/2.3.0 \
    && wget https://github.com/libexpat/libexpat/releases/download/R_2_3_0/expat-2.3.0.tar.gz --no-check-certificate \
    && tar -xf expat-2.3.0.tar.gz \
    && cd expat-2.3.0 \
    && ./configure --prefix=/dependencies/expat/2.3.0 --enable-static=yes --enable-shared=no --with-pic && echo $? \
    && make -j 6 \
    && make install \
    && cd ../ && rm -rf expat-2.3.0 && rm -rf expat-2.3.0.tar.gz

# install Apr-util
RUN mkdir -p dependencies/apr-util/1.6.1 \
    && wget https://dlcdn.apache.org//apr/apr-util-1.6.1.tar.gz --no-check-certificate \
    && tar -xf apr-util-1.6.1.tar.gz \
    && cd apr-util-1.6.1 \
    && ./configure --prefix=/dependencies/apr-util/1.6.1 --enable-static=yes --enable-shared=no --with-pic --with-apr=/dependencies/apr/1.7.0 --with-expat=/dependencies/expat/2.3.0 && echo $? \
    && make -j 6 \
    && make install \
    && cd ../ && rm -rf apr-util-1.6.1 && rm -rf apr-util-1.6.1.tar.gz

# install m4
RUN yum install m4 -y

# install autoconf
RUN wget --no-check-certificate https://ftp.gnu.org/gnu/autoconf/autoconf-2.68.tar.gz \
    && tar xzf autoconf-2.68.tar.gz  \
    && cd autoconf-2.68 \
    && ./configure --prefix=/usr/ && make -j && make install && autoconf -V \
    && cd .. && rm -rf autoconf-2.68.tar.gz

# install automake
RUN wget --no-check-certificate https://ftp.gnu.org/gnu/automake/automake-1.16.3.tar.gz \
    && tar xzf automake-1.16.3.tar.gz \
    && cd automake-1.16.3 \
    && ./configure --prefix=/usr --libdir=/usr/lib64 \
    && make -j && make install \
    && automake --version \
    && cd .. && rm -rf automake-1.16.3.tar.gz

# install libtool
RUN wget --no-check-certificate https://ftpmirror.gnu.org/libtool/libtool-2.4.6.tar.gz \
    && tar xzf libtool-2.4.6.tar.gz \
    && cd libtool-2.4.6 \
    && ./configure --prefix=/usr \
    && make -j 6 \
    && make install \
    && libtool --version \
    && cd .. && rm -rf libtool-2.4.6.tar.gz

#install log4cxx
RUN mkdir -p dependencies/apache-log4cxx/0.11.0 \
    && wget https://archive.apache.org/dist/logging/log4cxx/0.11.0/apache-log4cxx-0.11.0.tar.gz --no-check-certificate \
    && tar -xf apache-log4cxx-0.11.0.tar.gz \
    && cd apache-log4cxx-0.11.0 \
    && ./configure --prefix=/dependencies/apache-log4cxx/0.11.0/ --enable-static=yes --enable-shared=no --with-pic --with-apr=/dependencies/apr/1.7.0/ --with-apr-util=/dependencies/apr-util/1.6.1/ && echo $? \
    && make -j 6 ; echo 0 \
    && automake --add-missing \
    && make install \
    && cd .. && rm -rf apache-log4cxx-0.11.0.tar.gz && rm -rf apache-log4cxx-0.11.0

# install opentelemetry
RUN mkdir -p dependencies/opentelemetry/1.2.0/lib \
    && mkdir -p dependencies/opentelemetry/1.2.0/include \
    && git clone https://github.com/open-telemetry/opentelemetry-cpp \
    && cd opentelemetry-cpp/ \
    && git checkout tags/v1.2.0 -b v1.2.0 \
    && git submodule update --init --recursive \
    && mkdir build \
    && cd build \
    && cmake .. -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=ON -DWITH_OTLP=ON -DCMAKE_INSTALL_PREFIX=/dependencies/opentelemetry/1.2.0 \
    && cmake --build . --target all \
    && cd .. \
    && find . -name "*.so" -type f -exec cp {} /dependencies/opentelemetry/1.2.0/lib/ \; \
    && cp build/libopentelemetry_proto.a /dependencies/opentelemetry/1.2.0/lib \
    && cp -r api/include/ /dependencies/opentelemetry/1.2.0/ \
    && for dir in exporters/*; do if [ -d "$dir" ]; then cp -rf "$dir/include" /dependencies/opentelemetry/1.2.0/; fi; done \
    && cp -r sdk/include/ /dependencies/opentelemetry/1.2.0/ \
    && cp -r build/generated/third_party/opentelemetry-proto/opentelemetry/proto/ /dependencies/opentelemetry/1.2.0/include/opentelemetry/ \
    && cd .. && rm -rf opentelemetry-cpp

# install googletest
RUN mkdir -p dependencies/googletest/1.10.0/ \
    && wget https://github.com/google/googletest/archive/refs/tags/release-1.10.0.tar.gz --no-check-certificate \
    && tar -xf release-1.10.0.tar.gz \
    && cd googletest-release-1.10.0/  \
    && mkdir build && cd build \
    && cmake .. -DCMAKE_INSTALL_PREFIX=/dependencies/googletest/1.10.0/ \
    && make \
    && make install \
    && cd ../.. && rm -rf release-1.10.0.tar.gz && rm -rf googletest-release-1.10.0/


#Installing Apache and apr source code
RUN mkdir build-dependencies \
    && wget --no-check-certificate https://archive.apache.org/dist/apr/apr-1.5.2.tar.gz \
    && tar -xf apr-1.5.2.tar.gz \
    && mv -f apr-1.5.2 build-dependencies \
    && wget --no-check-certificate https://archive.apache.org/dist/apr/apr-util-1.5.4.tar.gz \
    && tar -xf apr-util-1.5.4.tar.gz \
    && mv -f apr-util-1.5.4 build-dependencies \
    && wget --no-check-certificate http://archive.apache.org/dist/httpd/httpd-2.2.31.tar.gz \
    && tar -xf httpd-2.2.31.tar.gz \
    && mv -f httpd-2.2.31 build-dependencies \
    && wget --no-check-certificate http://archive.apache.org/dist/httpd/httpd-2.4.23.tar.gz \
    && tar -xf httpd-2.4.23.tar.gz \
    && mv -f httpd-2.4.23 build-dependencies

# Build Webserver Module
RUN git clone https://github.com/cisco-open/otel-webserver-module.git \
    && cp -r /dependencies /otel-webserver-module/ \
    && cp -r /build-dependencies /otel-webserver-module/ \
    && cd otel-webserver-module \
    && ./gradlew assembleApacheModule

# install webserver module
RUN cd /otel-webserver-module/build \
    && tar -xf opentelemetry-webserver-sdk-x64-linux.tgz \
    && mv -f opentelemetry-webserver-sdk /opt/ \
    && cd ../ \
    && cp opentelemetry_module.conf /etc/httpd/conf.d/ \
    && cd /opt/opentelemetry-webserver-sdk \
    && ./install.sh \
    && cd /

# Remove unwanted files
RUN rm -rf grpc && rm -rf autoconf-2.68 && rm -rf automake-1.16.3 && rm -rf cmake-3.20.0-linux-x86_64 \
    && rm -rf libtool-2.4.6 && rm -rf Python-2.7.8 \
    && rm -f apr-1.5.2.tar.gz && rm -f apr-util-1.5.4.tar.gz \
    && rm -f httpd-2.2.31.tar.gz && rm -f httpd-2.4.23.tar.gz

COPY entrypoint.sh /usr/local/bin/
ENTRYPOINT ["entrypoint.sh"]

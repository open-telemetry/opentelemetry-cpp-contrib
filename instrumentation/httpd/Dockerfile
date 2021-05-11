FROM ubuntu:18.04

#########################################
# copy setup stuff from opentelemetry-cpp
#########################################

WORKDIR /setup-ci

ADD setup-buildtools.sh /setup-ci/setup-buildtools.sh

RUN /setup-ci/setup-buildtools.sh

#########################
# now build plugin itself
#########################

ADD setup-environment.sh /setup/setup-environment.sh

RUN /setup/setup-environment.sh

COPY src /root/src
COPY .clang-format /root
COPY tools /root/tools

COPY .bazelversion /root
COPY BUILD /root
COPY WORKSPACE /root

WORKDIR /root
COPY create-otel-load.sh /root
COPY build.sh /root
COPY opentelemetry.conf /root
COPY httpd_install_otel.sh /root
RUN /root/build.sh

# TODO: check apache configuration (apachectl configtest)

# TODO: run tests?

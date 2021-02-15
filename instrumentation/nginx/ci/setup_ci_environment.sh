#!/bin/sh

export DEBIAN_FRONTEND=noninteractive
export TZ="Europe/London"

apt-get update
apt-get install --no-install-recommends --no-install-suggests -y \
  build-essential autoconf libtool pkg-config ca-certificates \
  cmake gcc g++ libpcre3-dev zlib1g-dev libssl-dev libre2-dev \


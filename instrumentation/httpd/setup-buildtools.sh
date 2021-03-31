#!/bin/sh

export DEBIAN_FRONTEND=noninteractive
apt-get update -y
apt-get install -qq automake
apt-get install -qq libtool-bin
apt-get install -qq curl
apt-get install -qq libcurl4-openssl-dev
apt-get install -qq zlib1g-dev
apt-get install -qq git
apt-get install -qq build-essential
apt-get install -qq libssl-dev
apt-get install -qq libsqlite3-dev
# Stock sqlite may be too old
#apt install libsqlite3-dev
apt-get install -qq wget

# bazelisk is not in apt repository
BAZELISK_VERSION=v1.7.4

wget -O /usr/local/bin/bazel https://github.com/bazelbuild/bazelisk/releases/download/$BAZELISK_VERSION/bazelisk-linux-amd64
chmod +x /usr/local/bin/bazel


## Change owner from root to current dir owner
chown -R `stat . -c %u:%g` *

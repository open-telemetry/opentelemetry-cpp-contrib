#!/bin/sh

set -e

# bazelisk is not in apt repository
BAZELISK_VERSION=v1.29.0

wget -O /usr/local/bin/bazel https://github.com/bazelbuild/bazelisk/releases/download/$BAZELISK_VERSION/bazelisk-linux-amd64
chmod +x /usr/local/bin/bazel


## Change owner from root to current dir owner
chown -R `stat . -c %u:%g` *
